#include "CMSIS-DSP-main.h"

// Works on new versions of libraries:
// 1. Arduino Giga 4.2.1
// 2. AdvancedAnalog 1.5.0
// 3. STMSpeeduino 0.2.1

//
// TODO:
// 1. filter ! Azra

#include <Arduino_AdvancedAnalog.h>
#include "STMSpeeduino.h"

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */

// (comment this line to start up from matlab)
//#define SERIAL_OFF    // disable serial communication to test board by itself

// make this reconfig too
#define LOCAL_XCORR     true    // doing xcorr on the microcontroller
#define XCORR_DEBUG     false   // sending only distance w/o other data

// Arduino Giga has 3 ADCs -> up to 3 microphones simultaneously
// More - ADC channels 1-16
#define MIC1_PIN    A9 // PC3_C | ADC3_INP1
#define MIC2_PIN    A5 // PC2   | ADC123_INN11, ADC123_INP12
#define MIC3_PIN    A1 // PC5   | ADC12_INN4, ADC12_INP8

#define ADC_RESOLUTION 		16   	// 8, 10, 12, 14, 16
#define ADC_DIFF           	0      	// A10 has to be used as input positive on giga r1, A11 as negative
#define ADC_CLK_FREQ        40     	// Clock speed in mhz, stable up to 40mhz
#define ADC_SAMPLE_TIME 	0       // 0 to 7: 0 is minimal sampling time. If you change these, recalculate sampling rate in matlab
#define ADC_SAMPLE_NUM    	0       // oversampling: 0 - OFF

#define ACTUAL_SAMPLING_RATE	370000	// You need to measure this value using a wave generator with a fixed e.g. 1kHz Sine
#define BAN_DISTANCE			30		// min distance [cm] - how many self reflections cancelled

#define DAC_PIN          	A12
#define DAC_SINE_FREQ     	32000 // Hz

#define DAC_RESOLUTION    	AN_RESOLUTION_12         	// 12bit
#define DAC_SAMPLE_RATE    	DAC_SINE_FREQ * sine_lut_size    // 32kHz sine wave
#define DAC_SAMPLES_PER_CH	64    	// samples in each buffer (one sine wave)
#define DAC_QUEUE_DEPTH 	128     // queue depth. twice the buffer for one wave pending

#define STORE_BUF_SIZE 		2400    // 2400 for 1 measurement per second. 
                            		// only multiples of 32!!!!!! (64 chunk size of bytes, so 32 for 16bit)

#define FILTER_KERNEL_LENGTH    683                     // length of the coefficient buffer AT
#define FILTER_BLOCK_LENGTH     (STORE_BUF_SIZE/4)      // how many samples we want to process every time we call the fir process function AT
#define FILTER_TAP_NUM          405                     // tap number for the bandpass filter


/* -------------------------------------------------------------------------- */
/*                                  Constants                                 */
/* -------------------------------------------------------------------------- */

const uint16_t air_speed = 343; // m/s

// e.g. 25cm ban means 0.25*2/343 time ban, then multiply by sample rate
const uint32_t banned_sample_num = ((BAN_DISTANCE*2*ACTUAL_SAMPLING_RATE)/air_speed)/100; 

const uint16_t dac_cycle_num = 10; // hard coded for 10 (more is allegedly worse).
const uint16_t dac_execution_delay = 277; // calculated for 10 (!) cycle number with an oscilloscope [us]

// Sine Wave look up table
const uint16_t sine_lut[] = {
	0x0800,0x08c8,0x098f,0x0a52,0x0b0f,0x0bc5,0x0c71,0x0d12,0x0da7,0x0e2e,0x0ea6,0x0f0d,0x0f63,0x0fa7,0x0fd8,0x0ff5,
	0x0fff,0x0ff5,0x0fd8,0x0fa7,0x0f63,0x0f0d,0x0ea6,0x0e2e,0x0da7,0x0d12,0x0c71,0x0bc5,0x0b0f,0x0a52,0x098f,0x08c8,
	0x0800,0x0737,0x0670,0x05ad,0x04f0,0x043a,0x038e,0x02ed,0x0258,0x01d1,0x0159,0x00f2,0x009c,0x0058,0x0027,0x000a,
	0x0000,0x000a,0x0027,0x0058,0x009c,0x00f2,0x0159,0x01d1,0x0258,0x02ed,0x038e,0x043a,0x04f0,0x05ad,0x0670,0x0737
};
const size_t sine_lut_size = sizeof(sine_lut) / sizeof(sine_lut[0]);

const uint16_t dac_wave[] = {
    0x8000, 0xC22E, 0xF14C, 0xFFC5, 0xE96F, 0xB4B8, 0x70D2, 0x314A, 0x0870, 0x0208, 0x1FEA, 0x597B, 0x9E25, 0xDA1F, 0xFC23, 0xFA63, 
    0xD560, 0x97C5, 0x5350, 0x1BBB, 0x0109, 0x0AEB, 0x3688, 0x774F, 0xBA97, 0xECFE, 0xFFFE, 0xEE1E, 0xBC84, 0x797B, 0x3853, 0x0BD0, 
    0x00C7, 0x1A65, 0x5149, 0x95A1, 0xD3BF, 0xF9BB, 0xFCA6, 0xDBA7, 0xA040, 0x5B8F, 0x215D, 0x026F, 0x07AE, 0x2F96, 0x6EAA, 0xB2BC, 
    0xE830, 0xFF9F, 0xF24A, 0xC408, 0x822C, 0x3FAF, 0x0FBA, 0x001E, 0x1558, 0x494D, 0x8D04, 0xCCFB, 0xF6C4, 0xFE56, 0xE181, 0xA895, 
    0x63F8, 0x276E, 0x0469, 0x04FE, 0x2903, 0x6618, 0xAAA4, 0xE2E6, 0xFEAA, 0xF5F0, 0xCB3C, 0x8ADA, 0x4758, 0x1429, 0x000B, 0x10CA, 
    0x4193, 0x8458, 0xC5DD, 0xF341, 0xFF70, 0xE6E8, 0xB0BB, 0x6C83, 0x2DE8, 0x06F4, 0x02DF, 0x22D7, 0x5DA6, 0xA259, 0xDD28, 0xFD20, 
    0xF90B, 0xD217, 0x937C, 0x4F44, 0x1917, 0x008F, 0x0CBE, 0x3A22, 0x7BA7, 0xBE6C, 0xEF35, 0xFFF4, 0xEBD6, 0xB8A7, 0x7525, 0x34C3, 
    0x0A0F, 0x0155, 0x1D19, 0x555B, 0x99E7
};
const size_t dac_wave_size = sizeof(dac_wave) / sizeof(dac_wave[0]);

static double filter_taps[FILTER_TAP_NUM] = {
  0.004581170986460159,
  -0.0007363017137110708,
  -0.0005241463922754832,
  -0.0002709576239758131,
  -0.00003205024268935798,
  0.00013365498584004795,
  0.00017505340093010042,
  0.00006418553019759515,
  -0.00018936844716191305,
  -0.0005308629361824034,
  -0.0008657153153013188,
  -0.001080513402056736,
  -0.001076571698633199,
  -0.0008069287973408011,
  -0.00030376862738431675,
  0.00031591309631290976,
  0.0008734320684888978,
  0.001177479505860953,
  0.0010866616939273041,
  0.0005662263029389072,
  -0.00028253134921566434,
  -0.0012343634951117847,
  -0.0019951950842453823,
  -0.002291647922427684,
  -0.0019600126287806444,
  -0.0010201747238555159,
  0.00031134870022891617,
  0.0016651706575246984,
  0.0026194101302388817,
  0.002830944734957594,
  0.0021541024945868127,
  0.0007076457619061069,
  -0.0011358406932620332,
  -0.0028428384658799686,
  -0.003872408720998332,
  -0.003846188775151497,
  -0.0026814737077271505,
  -0.0006416073389655167,
  0.0017193316268242559,
  0.003707080619788171,
  0.004692493427764177,
  0.004311277465154573,
  0.0025934006826001344,
  -0.00002125078166039251,
  -0.0027942737075991307,
  -0.00489938520670127,
  -0.005670401360435051,
  -0.004813903301920263,
  -0.0025147454420339967,
  0.0005993290126314651,
  0.0036310166407076943,
  0.005676405924517979,
  0.006098528424865404,
  0.004728448591751969,
  0.001929254125267857,
  -0.001500919010711076,
  -0.0045603633056421735,
  -0.0063415585042030055,
  -0.006305726615767466,
  -0.004449185803072986,
  -0.0013084446092164687,
  0.0021975886717334144,
  0.005042032480252326,
  0.006398683899829635,
  0.0058867544295914676,
  0.003682357408771957,
  0.00045828572998217096,
  -0.0028247226890800855,
  -0.0052108627856764875,
  -0.00603527320222373,
  -0.005114330475332399,
  -0.0027853100025284844,
  0.0002136160844102452,
  0.002981046698316141,
  0.004730068887213109,
  0.0050192909182827745,
  0.0038670097809627127,
  0.0017177685488888328,
  -0.0007213229014101913,
  -0.002715977781047867,
  -0.0037323776532889727,
  -0.003584309008522602,
  -0.002458805529988743,
  -0.0008219577768321504,
  0.000760181810338075,
  0.001821935151671197,
  0.002132467644195123,
  0.0017528122921740118,
  0.0009704926958221637,
  0.0001664657218084051,
  -0.00034367012167628506,
  -0.0004287132167366305,
  -0.00017804013488850263,
  0.0001556055254313246,
  0.0002816937127830015,
  0.000013700505402439925,
  -0.0006316839115781954,
  -0.0014204670110151295,
  -0.0019826783547400462,
  -0.0019614811017779803,
  -0.0011741817497714694,
  0.00027928130614725076,
  0.002008689026703571,
  0.0034397756809588405,
  0.004005319828939491,
  0.0033573675580360137,
  0.001518306627005081,
  -0.0010561352747423159,
  -0.0036446435713895437,
  -0.005387034138667613,
  -0.005642332935905365,
  -0.004190881433027501,
  -0.0013391946458306925,
  0.0021356156809143293,
  0.005202163830031452,
  0.0068900857285652374,
  0.00660465289705638,
  0.004336022184425852,
  0.0006892139473480471,
  -0.0032822052576029575,
  -0.0063915363476395445,
  -0.007685881675758148,
  -0.006748047293879004,
  -0.0038347738646630843,
  0.0001954096974915973,
  0.004144098199258205,
  0.006843233088622816,
  0.007514965813826183,
  0.006006659709875221,
  0.0028277040150889894,
  -0.0010254523591800423,
  -0.004388996012848174,
  -0.006301526998957509,
  -0.006292104236990457,
  -0.004502980986218334,
  -0.0016105333497707078,
  0.0014242449506748492,
  0.0036836869568180265,
  0.004586575742915871,
  0.004049702864789503,
  0.0024685722662026893,
  0.0005326586954716323,
  -0.0010503616934943972,
  -0.0018165626928998721,
  -0.0016973585708821234,
  -0.0010066894288868326,
  -0.000274425036506455,
  0.000004954808437771928,
  -0.0003924916100776934,
  -0.0013011310251657988,
  -0.002215034806527445,
  -0.0024960810932109236,
  -0.0016649386871770517,
  0.00034137203391968965,
  0.0030537277827226202,
  0.005564611416191374,
  0.00681719263975734,
  0.006001692174852416,
  0.002919569505558762,
  -0.0018290471513793501,
  -0.006928006387777735,
  -0.01070288824068674,
  -0.011659284247262511,
  -0.009030620026009906,
  -0.0031430493750067364,
  0.004550141275633004,
  0.011813692604778764,
  0.01627951315489019,
  0.016209714081868973,
  0.011129632206209984,
  0.002111853229991697,
  -0.008421220229436213,
  -0.017345163942307742,
  -0.021765316225087098,
  -0.019949912020332163,
  -0.01196592192057193,
  0.00020715998147612837,
  0.013158989059523335,
  0.023010878361567372,
  0.026585884175768137,
  0.022431050967295103,
  0.011362820252529946,
  -0.003659862581950051,
  -0.018329965133892977,
  -0.0282405380077922,
  -0.03022130825477253,
  -0.023354483531878988,
  -0.0093429267130264,
  0.007907660580673463,
  0.02339095617328027,
  0.03246795900835663,
  0.03226884065626874,
  0.02261888580790374,
  0.006135831405652525,
  -0.012469940467291768,
  -0.027768992348804377,
  -0.035214632729871916,
  -0.03251141821924333,
  -0.02032904317904696,
  -0.0021382073178670487,
  0.01679170994027232,
  0.0309400114762076,
  0.03616665963474611,
  0.0309400114762076,
  0.01679170994027232,
  -0.0021382073178670487,
  -0.02032904317904696,
  -0.03251141821924333,
  -0.035214632729871916,
  -0.027768992348804377,
  -0.012469940467291768,
  0.006135831405652525,
  0.02261888580790374,
  0.03226884065626874,
  0.03246795900835663,
  0.02339095617328027,
  0.007907660580673463,
  -0.0093429267130264,
  -0.023354483531878988,
  -0.03022130825477253,
  -0.0282405380077922,
  -0.018329965133892977,
  -0.003659862581950051,
  0.011362820252529946,
  0.022431050967295103,
  0.026585884175768137,
  0.023010878361567372,
  0.013158989059523335,
  0.00020715998147612837,
  -0.01196592192057193,
  -0.019949912020332163,
  -0.021765316225087098,
  -0.017345163942307742,
  -0.008421220229436213,
  0.002111853229991697,
  0.011129632206209984,
  0.016209714081868973,
  0.01627951315489019,
  0.011813692604778764,
  0.004550141275633004,
  -0.0031430493750067364,
  -0.009030620026009906,
  -0.011659284247262511,
  -0.01070288824068674,
  -0.006928006387777735,
  -0.0018290471513793501,
  0.002919569505558762,
  0.006001692174852416,
  0.00681719263975734,
  0.005564611416191374,
  0.0030537277827226202,
  0.00034137203391968965,
  -0.0016649386871770517,
  -0.0024960810932109236,
  -0.002215034806527445,
  -0.0013011310251657988,
  -0.0003924916100776934,
  0.000004954808437771928,
  -0.000274425036506455,
  -0.0010066894288868326,
  -0.0016973585708821234,
  -0.0018165626928998721,
  -0.0010503616934943972,
  0.0005326586954716323,
  0.0024685722662026893,
  0.004049702864789503,
  0.004586575742915871,
  0.0036836869568180265,
  0.0014242449506748492,
  -0.0016105333497707078,
  -0.004502980986218334,
  -0.006292104236990457,
  -0.006301526998957509,
  -0.004388996012848174,
  -0.0010254523591800423,
  0.0028277040150889894,
  0.006006659709875221,
  0.007514965813826183,
  0.006843233088622816,
  0.004144098199258205,
  0.0001954096974915973,
  -0.0038347738646630843,
  -0.006748047293879004,
  -0.007685881675758148,
  -0.0063915363476395445,
  -0.0032822052576029575,
  0.0006892139473480471,
  0.004336022184425852,
  0.00660465289705638,
  0.0068900857285652374,
  0.005202163830031452,
  0.0021356156809143293,
  -0.0013391946458306925,
  -0.004190881433027501,
  -0.005642332935905365,
  -0.005387034138667613,
  -0.0036446435713895437,
  -0.0010561352747423159,
  0.001518306627005081,
  0.0033573675580360137,
  0.004005319828939491,
  0.0034397756809588405,
  0.002008689026703571,
  0.00027928130614725076,
  -0.0011741817497714694,
  -0.0019614811017779803,
  -0.0019826783547400462,
  -0.0014204670110151295,
  -0.0006316839115781954,
  0.000013700505402439925,
  0.0002816937127830015,
  0.0001556055254313246,
  -0.00017804013488850263,
  -0.0004287132167366305,
  -0.00034367012167628506,
  0.0001664657218084051,
  0.0009704926958221637,
  0.0017528122921740118,
  0.002132467644195123,
  0.001821935151671197,
  0.000760181810338075,
  -0.0008219577768321504,
  -0.002458805529988743,
  -0.003584309008522602,
  -0.0037323776532889727,
  -0.002715977781047867,
  -0.0007213229014101913,
  0.0017177685488888328,
  0.0038670097809627127,
  0.0050192909182827745,
  0.004730068887213109,
  0.002981046698316141,
  0.0002136160844102452,
  -0.0027853100025284844,
  -0.005114330475332399,
  -0.00603527320222373,
  -0.0052108627856764875,
  -0.0028247226890800855,
  0.00045828572998217096,
  0.003682357408771957,
  0.0058867544295914676,
  0.006398683899829635,
  0.005042032480252326,
  0.0021975886717334144,
  -0.0013084446092164687,
  -0.004449185803072986,
  -0.006305726615767466,
  -0.0063415585042030055,
  -0.0045603633056421735,
  -0.001500919010711076,
  0.001929254125267857,
  0.004728448591751969,
  0.006098528424865404,
  0.005676405924517979,
  0.0036310166407076943,
  0.0005993290126314651,
  -0.0025147454420339967,
  -0.004813903301920263,
  -0.005670401360435051,
  -0.00489938520670127,
  -0.0027942737075991307,
  -0.00002125078166039251,
  0.0025934006826001344,
  0.004311277465154573,
  0.004692493427764177,
  0.003707080619788171,
  0.0017193316268242559,
  -0.0006416073389655167,
  -0.0026814737077271505,
  -0.003846188775151497,
  -0.003872408720998332,
  -0.0028428384658799686,
  -0.0011358406932620332,
  0.0007076457619061069,
  0.0021541024945868127,
  0.002830944734957594,
  0.0026194101302388817,
  0.0016651706575246984,
  0.00031134870022891617,
  -0.0010201747238555159,
  -0.0019600126287806444,
  -0.002291647922427684,
  -0.0019951950842453823,
  -0.0012343634951117847,
  -0.00028253134921566434,
  0.0005662263029389072,
  0.0010866616939273041,
  0.001177479505860953,
  0.0008734320684888978,
  0.00031591309631290976,
  -0.00030376862738431675,
  -0.0008069287973408011,
  -0.001076571698633199,
  -0.001080513402056736,
  -0.0008657153153013188,
  -0.0005308629361824034,
  -0.00018936844716191305,
  0.00006418553019759515,
  0.00017505340093010042,
  0.00013365498584004795,
  -0.00003205024268935798,
  -0.0002709576239758131,
  -0.0005241463922754832,
  -0.0007363017137110708,
  0.004581170986460159
};

float32_t firState[FILTER_BLOCK_LENGTH + FILTER_TAP_NUM - 1];   // current filter state buffer
arm_fir_instance_f32 Fir_filt;  // creating an object instance


/* -------------------------------------------------------------------------- */
/*                              Global Structure                              */
/* -------------------------------------------------------------------------- */
typedef struct {

	uint8_t ban_flag; // activate self reflections ban
	char serial_read_buf;

	AdvancedDAC dac = NULL;

	uint16_t mic1_values[STORE_BUF_SIZE];
	uint16_t mic2_values[STORE_BUF_SIZE];
	uint16_t mic3_values[STORE_BUF_SIZE];

    float xcorr_buffer[STORE_BUF_SIZE]; // use same buffer for several functions for memory saving

} SenseduBoard;

static SenseduBoard SenseduBoardObj;


/* -------------------------------------------------------------------------- */
/*                                  Functions                                 */
/* -------------------------------------------------------------------------- */

// initialize main structure
void main_obj_init(SenseduBoard* obj_ptr) {
	obj_ptr->ban_flag = 1;
	obj_ptr->serial_read_buf = '0';

	AdvancedDAC dac0(DAC_PIN);
	obj_ptr->dac = dac0;

	clear_16bit_buf(obj_ptr->mic1_values, STORE_BUF_SIZE);
    clear_16bit_buf(obj_ptr->mic2_values, STORE_BUF_SIZE);
    clear_16bit_buf(obj_ptr->mic3_values, STORE_BUF_SIZE);
    clear_float_buf(obj_ptr->xcorr_buffer, STORE_BUF_SIZE);
}

// output sine wave from the defined look up table
void dac_output_sinewave(AdvancedDAC& dac_out) {
    static size_t lut_offs = 0;
    for (uint16_t i = 0; i <= dac_cycle_num; i++) {
        if (dac_out.available()) {
            SampleBuffer buf = dac_out.dequeue();
            for (size_t i=0; i<buf.size(); i++, lut_offs++) {
                buf[i] = sine_lut[lut_offs % sine_lut_size];
            }
            
            dac_out.write(buf);
        }
    }
}

// outputs dc at half max value after the sine wave
void dac_output_dc_halfrail(AdvancedDAC& dac_out) {
    for (uint16_t i = 0; i <= dac_cycle_num; i++) {
        if (dac_out.available()) {
            SampleBuffer buf = dac_out.dequeue();
            for (size_t i=0; i<buf.size(); i++) {
                buf[i] = 0x0800;
            }
            
            dac_out.write(buf);
        }
    }
}

// make sure the buffer is initialized because it crashes if you access an array element w/o initialization
void clear_16bit_buf(uint16_t array[], uint32_t size_array){
    for (uint32_t i = 0; i < size_array; i++){
        array[i] = 0x0000;
    }
}

void clear_float_buf(float array[], uint32_t size_array){
    for (uint32_t i = 0; i < size_array; i++){
        array[i] = 0.0f;
    }
}

// sent data size to matlab - how many samples we currently have for 1 measurement
void send_config() {
    // 4bytes for data length
    Serial.write(uint8_t(STORE_BUF_SIZE >> 24));
    Serial.write(uint8_t(STORE_BUF_SIZE >> 16));
    Serial.write(uint8_t(STORE_BUF_SIZE >> 8));
    Serial.write(uint8_t(STORE_BUF_SIZE));
}

// "adc_data_length" should be the size of the wave "rescaled_adc_wave"
void custom_xcorr(float* xcorr_buf, const uint16_t* dac_wave, uint32_t adc_data_length) {

    // delay loop
    for (int32_t m = 0; m < adc_data_length; m++) {
        // sum loop
        float sum = 0;
        for (uint16_t n = 0; n < dac_wave_size; n++) {
            uint32_t idx = n + m;
            if (idx < adc_data_length) {
                sum += dac_wave[n]*xcorr_buf[idx]; 
            }
        }

        // indexes never overlap with previous computation -> safe to reuse for memory management
        xcorr_buf[m] = sum; 
    }
}

// rescale samples to normalized -1:1 values around zero
void rescale_adc_wave(float* rescaled_adc_wave, uint16_t* adc_wave, uint16_t adc_data_length) {
    // 0:65535 -> -1:1
    float sum = 0.0f;
    for (uint32_t i = 0; i < adc_data_length; i++) {
        rescaled_adc_wave[i] = (2.0f * adc_wave[i])/65535.0f - 1.0f;
        sum += rescaled_adc_wave[i];
    }

    filter_32kHz_wave(rescaled_adc_wave, adc_data_length);

    // also center it around zero (remove it after filtering which would remove DC component)
    float mean = sum/adc_data_length;
    for (uint32_t i = 0; i < adc_data_length; i++) {
        rescaled_adc_wave[i] = rescaled_adc_wave[i] - mean;
    }
}

// filter the wave around 32kHz
void filter_32kHz_wave(float* rescaled_adc_wave, uint16_t adc_data_length) {
    float32_t output_signal[adc_data_length];
    // need to take block chunks of the input signal
    for(uint16_t i = 0; i < adc_data_length; i += FILTER_BLOCK_LENGTH) {
        // take care of the last block
        size_t block_size = min(FILTER_BLOCK_LENGTH, adc_data_length - i);
        // perform the filter operation for the current block
        arm_fir_f32(&Fir_filt, &rescaled_adc_wave[i], &output_signal[i], block_size);
    }
    
}


// send serial data in 32 byte chunks
void serial_send_array(const uint8_t* data, size_t size) {
    const size_t chunk_size = 32; // buffer is 32 bytes, but 32 for 2400 data samples
	for (uint16_t i = 0; i < size/chunk_size; i++) {
		Serial.write(data + chunk_size * i, chunk_size);
	}
}

// clean it up in the future, especially with variables
uint32_t xcorr(float* xcorr_buf, size_t xcorr_buf_size, uint16_t mic_array[], size_t mic_array_size, uint8_t ban_flag) {
	rescale_adc_wave(xcorr_buf, mic_array, STORE_BUF_SIZE);
    if (XCORR_DEBUG)
        serial_send_array((const uint8_t*)mic_array, mic_array_size);

	// remove self reflections from a dataset
	if (ban_flag == 1) {
		for (uint32_t i = 0; i < banned_sample_num; i++) {
			xcorr_buf[i] = 0;
			xcorr_buf[i] = 0;
			xcorr_buf[i] = 0;
		}
	}
    if (XCORR_DEBUG)
        serial_send_array((const uint8_t*)xcorr_buf, xcorr_buf_size);

    custom_xcorr(xcorr_buf, dac_wave, STORE_BUF_SIZE);
    if (XCORR_DEBUG)
	    serial_send_array((const uint8_t*)xcorr_buf, xcorr_buf_size);

	uint32_t peak_index = 0;
	float biggest = 0.0f;

	//Serial.println(peak_index);
	for (uint32_t i = 0; i < STORE_BUF_SIZE; i++) {
		if (xcorr_buf[i] > biggest) {
			biggest = xcorr_buf[i];
			peak_index = i;
		}
	}

	uint16_t sr = 370; // kS/sec  sample rate
	uint16_t c = 343; // speed in air
	// (lag_samples * sample_time) * air_speed / 2
	uint32_t distance = ((peak_index * 1000 * c) / sr) >> 1; // in micrometers

    return distance;
}


/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */

void setup() {

	SenseduBoard* main_obj_ptr = &SenseduBoardObj;
	main_obj_init(main_obj_ptr);

    Serial.begin(115200); // 14400 bytes/sec -> 7200 samples/sec -> 2400 samples/sec for 1 mic

    if (!main_obj_ptr->dac.begin(DAC_RESOLUTION, DAC_SAMPLE_RATE, DAC_SAMPLES_PER_CH, DAC_QUEUE_DEPTH)) {
        Serial.write("DAC didn't start. Board has been crashed. Restart.");
        while (1);
    }
    
    // be careful with adc -> mic mapping
    ADCBegin(ADC3, MIC1_PIN, ADC_RESOLUTION, ADC_DIFF, ADC_CLK_FREQ, ADC_SAMPLE_TIME, ADC_SAMPLE_NUM);
    ADCBegin(ADC2, MIC2_PIN, ADC_RESOLUTION, ADC_DIFF, ADC_CLK_FREQ, ADC_SAMPLE_TIME, ADC_SAMPLE_NUM);
    ADCBegin(ADC1, MIC3_PIN, ADC_RESOLUTION, ADC_DIFF, ADC_CLK_FREQ, ADC_SAMPLE_TIME, ADC_SAMPLE_NUM);

    // initializing the filter
    arm_fir_init_f32(&Fir_filt, FILTER_TAP_NUM, (float32_t *)&filter_taps[0], &firState[0], FILTER_BLOCK_LENGTH); 
}


/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */

void loop() {

	SenseduBoard* main_obj_ptr = &SenseduBoardObj;
    
    // Measurement is initiated by signal from computing device
	#ifndef SERIAL_OFF
		while (1) {
			while (Serial.available() == 0);    // Wait for a signal
			main_obj_ptr->serial_read_buf = Serial.read();
			// Send Configuration Data to computing device
			if (main_obj_ptr->serial_read_buf == 'c') {
				send_config();
			} else {
				// if not config command, stop input loop
				break; 
			}
		}
	#endif
    
    // Send sine wave to the speaker - total ~357us execution
    dac_output_sinewave(main_obj_ptr->dac); // ~44us execution
    dac_output_dc_halfrail(main_obj_ptr->dac); // ~38us execution
    delayMicroseconds(dac_execution_delay);

    // Read data from mics 1-3 simultaneously
    // be careful with adc -> mic mapping
    for (uint32_t i = 0; i < STORE_BUF_SIZE; i++) {
        main_obj_ptr->mic1_values[i] = CatchADCValue(ADC3);
        main_obj_ptr->mic2_values[i] = CatchADCValue(ADC2);
        main_obj_ptr->mic3_values[i] = CatchADCValue(ADC1);
    }

	// Send full dataset to computing device (eg. matlab)
    if (!LOCAL_XCORR) {
		serial_send_array((const uint8_t *) & main_obj_ptr->mic1_values, sizeof(main_obj_ptr->mic1_values));
		serial_send_array((const uint8_t *) & main_obj_ptr->mic2_values, sizeof(main_obj_ptr->mic2_values));
		serial_send_array((const uint8_t *) & main_obj_ptr->mic3_values, sizeof(main_obj_ptr->mic3_values));
        return;
    }

    static uint32_t distance[3];
	distance[0] = xcorr(main_obj_ptr->xcorr_buffer, sizeof(main_obj_ptr->xcorr_buffer), main_obj_ptr->mic1_values, sizeof(main_obj_ptr->mic1_values), main_obj_ptr->ban_flag);
    distance[1] = xcorr(main_obj_ptr->xcorr_buffer, sizeof(main_obj_ptr->xcorr_buffer), main_obj_ptr->mic2_values, sizeof(main_obj_ptr->mic2_values), main_obj_ptr->ban_flag);
    distance[2] = xcorr(main_obj_ptr->xcorr_buffer, sizeof(main_obj_ptr->xcorr_buffer), main_obj_ptr->mic3_values, sizeof(main_obj_ptr->mic3_values), main_obj_ptr->ban_flag);

    Serial.write((const uint8_t *) & distance[0], 4);
    Serial.write((const uint8_t *) & distance[1], 4);
    Serial.write((const uint8_t *) & distance[2], 4);
    
}