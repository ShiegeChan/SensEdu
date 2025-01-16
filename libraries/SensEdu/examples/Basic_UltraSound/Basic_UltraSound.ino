#include "SensEdu.h"
#include <Arduino_AdvancedAnalog.h>
#include "SineLUT.h"

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */

/* DAC */
#define DAC_PIN          	A12
#define DAC_SINE_FREQ     	32000                   // 32kHz
#define DAC_RESOLUTION    	AN_RESOLUTION_12        // 12bit
#define DAC_SAMPLE_RATE    	DAC_SINE_FREQ * 64      // ~2MHz
#define DAC_SAMPLES_PER_CH	sine_lut_size    	    // samples in each buffer (one sine wave)
#define DAC_QUEUE_DEPTH 	64                      // queue depth
AdvancedDAC dac0(DAC_PIN);

/* ADC */
ADC_TypeDef* adc = ADC1;
const uint8_t mic_num = 1;
uint8_t mic_pins[mic_num] = {A1};

const uint16_t mic_data_size = 64*32; // must be multiple of 64 for 16bit
__attribute__((aligned(__SCB_DCACHE_LINE_SIZE))) uint16_t mic_data[mic_data_size]; // cache aligned

/* errors */
uint32_t lib_error = 0;
uint8_t error_led = D86;

/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */
void setup() {

    Serial.begin(115200);

    dac0.begin(DAC_RESOLUTION, DAC_SAMPLE_RATE, DAC_SAMPLES_PER_CH, DAC_QUEUE_DEPTH);

    SensEdu_Init(adc, mic_pins, mic_num, SENSEDU_ADC_MODE_CONT_TIM_TRIGGERED, 250000, SENSEDU_ADC_DMA_CONNECT); // continuos mode for ADC
    SensEdu_ADC_Enable(adc);

    SensEdu_DMA_Init((uint16_t*)mic_data, mic_data_size);

    pinMode(error_led, OUTPUT);
    digitalWrite(error_led, HIGH);

    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        handle_error();
    }
}

/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */
void loop() {

    // Measurement is initiated by signal from computing device
    static char serial_buf = 0;
    
    while (1) {
        while (Serial.available() == 0);    // Wait for a signal
        serial_buf = Serial.read();

        if (serial_buf == 't') {
            // expected 't' symbol (trigger)
            break;
        }
    }

    // start dac->adc sequence
    dac_output_sinewave(dac0); // ~44us execution
    delayMicroseconds(277); // calculated dealy for x10 64sine cycles with an oscilloscope [us] (RECALCULATE!!!!)
    SensEdu_DMA_Enable((uint16_t*)mic_data, mic_data_size);
    SensEdu_ADC_Start(adc);
    
    // wait for the data and send it
    while(!SensEdu_DMA_GetTransferStatus());
    SensEdu_DMA_ClearTransferStatus();
    serial_send_array((const uint8_t *) & mic_data, mic_data_size << 1);

    // check errors
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        handle_error();
    }
}

/* -------------------------------------------------------------------------- */
/*                                  Functions                                 */
/* -------------------------------------------------------------------------- */
void handle_error() {
    // serial is taken by matlab, use LED as indication
    //Serial.print("Error: 0x");
    //Serial.println(lib_error, HEX);

    digitalWrite(error_led, LOW);
}

// send serial data in 32 byte chunks
void serial_send_array(const uint8_t* data, size_t size) {
    const size_t chunk_size = 32;
	for (uint32_t i = 0; i < size/chunk_size; i++) {
		Serial.write(data + chunk_size * i, chunk_size);
	}
}
