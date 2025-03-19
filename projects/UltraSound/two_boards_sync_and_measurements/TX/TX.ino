// Works on new versions of libraries:
// 1. Arduino Giga 4.2.1
#include "SensEdu.h"
#include "CMSIS_DSP.h"
#include "SineLUT.h"

uint32_t lib_error = 0;
uint8_t error_led = D86;

uint8_t pin_TXfinished = 7;
uint8_t pin_RXfinished = 8;


/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */

#define BAN_DISTANCE	20		    // min distance [cm] - how many self reflections cancelled
#define ACTUAL_SAMPLING_RATE 244000 // You need to measure this value using a wave generator with a fixed e.g. 1kHz Sine
#define STORE_BUF_SIZE  64 * 32     // 2400 for 1 measurement per second. 
                            	    // only multiples of 32!!!!!! (64 chunk size of bytes, so 32 for 16bit)


/********************* DAC ****************/
DAC_Channel* dac_channel = DAC_CH1;
// lut settings are in SineLUT.h
#define DAC_SINE_FREQ     	32000                           // 32kHz
#define DAC_SAMPLE_RATE     DAC_SINE_FREQ * sine_lut_size   // 64 samples per one sine cycle

SensEdu_DAC_Settings dac_settings = {dac_channel, DAC_SAMPLE_RATE, (uint16_t*)sine_lut, sine_lut_size, 
    SENSEDU_DAC_MODE_BURST_WAVE, dac_cycle_num}; // specifying burst mode 

/* -------------------------------------------------------------------------- */
/*                                  Constants                                 */
/* -------------------------------------------------------------------------- */

const uint16_t air_speed = 343; // m/s


/* -------------------------------------------------------------------------- */
/*                              Global Structure                              */
/* -------------------------------------------------------------------------- */
typedef struct {
	uint8_t ban_flag; // activate self reflections ban
	char serial_read_buf;
    float xcorr_buffer[STORE_BUF_SIZE]; // use same buffer for several functions for memory saving
    
} SenseduBoard;

static SenseduBoard SenseduBoardObj;

/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */

void setup() {
	  SenseduBoard* main_obj_ptr = &SenseduBoardObj;
  	main_obj_init(main_obj_ptr);

    pinMode(pin_TXfinished, OUTPUT);
    pinMode(pin_RXfinished, INPUT);

    Serial.begin(115200); // 14400 bytes/sec -> 7200 samples/sec -> 2400 samples/sec for 1 mic

    /* DAC */
    SensEdu_DAC_Init(&dac_settings);
    
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        handle_error();
    }
}

/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */

void loop() {
	SenseduBoard* main_obj_ptr = &SenseduBoardObj;
    // Measurement is initiated by the signal from computing device (matlab script)
    static char serial_buf = 0;
    
    // Measurement is initiated by signal from computing device
	#ifndef SERIAL_OFF
		while (1) {
			while (Serial.available() == 0);    // Wait for a signal
      digitalWrite(pin_TXfinished, 0);
			serial_buf = Serial.read();
			if (serial_buf == 't') {
                break; 
			}
		}
	#endif

  while(digitalRead(pin_RXfinished)==0) {};
  digitalWrite(pin_TXfinished, 0);
    
    // Start dac->adc sequence
    SensEdu_DAC_Enable(dac_channel);
    while(!SensEdu_DAC_GetBurstCompleteFlag(dac_channel)); // wait for dac to finish sending the burst
    SensEdu_DAC_ClearBurstCompleteFlag(dac_channel); 
    

    // check errors
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        handle_error();
    }
    digitalWrite(pin_TXfinished, 1);
}