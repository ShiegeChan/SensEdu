#include "dac.h"
#include "timer.h"

/* -------------------------------------------------------------------------- */
/*                                   Structs                                  */
/* -------------------------------------------------------------------------- */

typedef struct
{
    volatile uint16_t transfer_cnt;  // current written wave cycle to dac
    volatile uint8_t burst_complete;       
} dac_data;

/* -------------------------------------------------------------------------- */
/*                                  Variables                                 */
/* -------------------------------------------------------------------------- */

static volatile DAC_ERROR error = DAC_ERROR_NO_ERRORS;

static SensEdu_DAC_Settings dac_ch1_settings = {DAC_CH1, 1000000, 0x0000, 0, 
    SENSEDU_DAC_MODE_CONTINUOUS_WAVE, 0};
static SensEdu_DAC_Settings dac_ch2_settings = {DAC_CH2, 1000000, 0x0000, 0, 
    SENSEDU_DAC_MODE_CONTINUOUS_WAVE, 0};

static dac_data dac_ch1_data; 
static dac_data dac_ch2_data; 

/* -------------------------------------------------------------------------- */
/*                                Declarations                                */
/* -------------------------------------------------------------------------- */

void dac_init(DAC_Channel* dac_channel);
SensEdu_DAC_Settings* get_dac_settings(DAC_Channel* dac_channel);
static DAC_ERROR check_settings(SensEdu_DAC_Settings* settings);
dac_data* get_dac_data(DAC_Channel* dac_channel);

/* -------------------------------------------------------------------------- */
/*                              Public Functions                              */
/* -------------------------------------------------------------------------- */

void SensEdu_DAC_Init(SensEdu_DAC_Settings* dac_settings) {
    // Sanity checks
    error = check_settings(dac_settings);

    // Store settings locally
    SensEdu_DAC_Settings* settings = get_dac_settings(dac_settings->dac_channel);
    *settings = *dac_settings;

    // Init flags and storage
    get_dac_data(dac_settings->dac_channel)->transfer_cnt = 0; 
    get_dac_data(dac_settings->dac_channel)->burst_complete = 0; 

    // Timer + DAC + DMA
    // Both DAC channels use the same timer
    TIMER_DAC1Init(dac_settings->sampling_freq);
    
    dac_init(dac_settings->dac_channel);

    DMA_DACInit(dac_settings->dac_channel, dac_settings->mem_address, dac_settings->mem_size, dac_settings->wave_mode);

    // Enable Timer (it always runs even if dac/dma is off)
    TIMER_DAC1Enable();
}

void SensEdu_DAC_Enable(DAC_Channel* dac_channel) {
    DMA_DACEnable(dac_channel);

    uint16_t shift = 0U;
    if (dac_channel == DAC_CH2) {
        shift = 16U;  //all bits in CR register are shifted 16 positions for channel 2
    }
    SET_BIT(DAC1->CR, DAC_CR_EN1 << shift);
    while(!READ_BIT(DAC1->CR, DAC_CR_EN1 << shift));
}

void SensEdu_DAC_Disable(DAC_Channel* dac_channel) {
    uint16_t shift = 0U;
    if (dac_channel == DAC_CH2) {
        shift = 16U;  
    }
    CLEAR_BIT(DAC1->CR, DAC_CR_EN1 << shift);
    while(READ_BIT(DAC1->CR, DAC_CR_EN1 << shift));

    DMA_DACDisable(dac_channel);
}

uint8_t SensEdu_DAC_GetBurstCompleteFlag(DAC_Channel* dac_channel) {
    return (get_dac_data(dac_channel))->burst_complete;
}

void SensEdu_DAC_ClearBurstCompleteFlag(DAC_Channel* dac_channel) {
    get_dac_data(dac_channel)->burst_complete = 0;
}

DAC_ERROR DAC_GetError(void) {
    return error;
}

void DAC_WriteDataManually(DAC_Channel* dac_channel, uint16_t data) {
    if(dac_channel == DAC_CH1) {
        WRITE_REG(DAC1->DHR12R1, data);
    }
    else if(dac_channel == DAC_CH2) {
        WRITE_REG(DAC1->DHR12R2, data);
    }
}

uint16_t DAC_ReadCurrentOutputData(DAC_Channel* dac_channel) {
    if(dac_channel == DAC_CH1) {
        return READ_REG(DAC1->DOR1);
    }
    else if(dac_channel == DAC_CH2) {
        return READ_REG(DAC1->DOR2);
    }
}

/* -------------------------------------------------------------------------- */
/*                              Private Functions                             */
/* -------------------------------------------------------------------------- */

void dac_init(DAC_Channel* dac_channel) {

    uint16_t shift = 0U;
    if (dac_channel == DAC_CH2) {
        // all bits in CR register are shifted 16 positions for channel 2
        shift = 16U;  
    }

    // check for both channels
    if (READ_BIT(DAC1->CR, ((DAC_CR_EN1 << shift) | (DAC_CR_CEN1 << shift)))) { 
        error = DAC_ERROR_ENABLED_BEFORE_INIT;
        return;
    }

    // TODO: check if it is needed in default arduino config and change according to DAC channel
    // GPIO 
    MODIFY_REG(GPIOA->MODER, GPIO_MODER_MODE4, (0b11) << GPIO_MODER_MODE4_Pos);

    // Clock
    SET_BIT(RCC->APB1LENR, RCC_APB1LENR_DAC12EN);

    // DMA
    SET_BIT(DAC1->CR, DAC_CR_DMAUDRIE1 << shift);   // Enable DMA Underrun Interrupt
    SET_BIT(DAC1->CR, DAC_CR_DMAEN1 << shift);      // Enable DMA

    // Trigger
    MODIFY_REG(DAC1->CR, DAC_CR_TSEL1 << shift, (3U) << (DAC_CR_TSEL1_Pos + shift)); // dac_chx_trg3 -> tim4_trgo
    SET_BIT(DAC1->CR, DAC_CR_TEN1 << shift); // Enable Trigger

    // Channel Mode
    MODIFY_REG(DAC1->MCR, DAC_MCR_MODE1 << shift, (0b010) << (DAC_MCR_MODE1_Pos + shift)); // Connected to external pin with buffer disabled
}

SensEdu_DAC_Settings* get_dac_settings(DAC_Channel* dac_channel) {
    if (dac_channel == DAC_CH1) {
        return &dac_ch1_settings;
    } else if (dac_channel == DAC_CH2) {
        return &dac_ch2_settings;
    }
}

dac_data* get_dac_data(DAC_Channel* dac_channel) {
    if (dac_channel == DAC_CH1) {
        return &dac_ch1_data;
    } else if (dac_channel == DAC_CH2) {
        return &dac_ch2_data;
    }
}

static DAC_ERROR check_settings(SensEdu_DAC_Settings* settings) {
    if (settings->dac_channel != DAC_CH1 && settings->dac_channel != DAC_CH2) {
        return DAC_ERROR_INIT_FAILED;
    } 
    
    if (settings->sampling_freq > 15000000) {
        return DAC_ERROR_SAMPLING_FREQ_TOO_HIGH;
    } 

    if (settings->mem_address == 0x0000) {
        return DAC_ERROR_INIT_FAILED;
    } 
    
    if (settings->mem_address == 0) {
        return DAC_ERROR_INIT_FAILED;
    } 

    if (settings->wave_mode == SENSEDU_DAC_MODE_BURST_WAVE && settings->burst_num < 1) {
        settings->burst_num = 1; // be careful not to stuck in interrupt
        return DAC_ERROR_INIT_FAILED;
    }

    return DAC_ERROR_NO_ERRORS;
}

/* -------------------------------------------------------------------------- */
/*                                 Interrupts                                 */
/* -------------------------------------------------------------------------- */

void DAC_IRQHandler(void) {
    if (READ_BIT(DAC1->SR, DAC_SR_DMAUDR1)) {
        SET_BIT(DAC1->SR, DAC_SR_DMAUDR1);
        error = DAC_ERROR_DMA_UNDERRUN;
    }

    if (READ_BIT(DAC1->SR, DAC_SR_DMAUDR2)) {
        SET_BIT(DAC1->SR, DAC_SR_DMAUDR2);
        error = DAC_ERROR_DMA_UNDERRUN;
    }
}

void DAC_TransferCompleteDMAinterrupt(DAC_Channel* dac_channel) {
    if (get_dac_settings(dac_channel)->wave_mode == SENSEDU_DAC_MODE_BURST_WAVE) {
        dac_data* data =get_dac_data(dac_channel);
        (data->transfer_cnt)++;
        if ((data->transfer_cnt) == get_dac_settings(dac_channel)->burst_num) {
            (data->transfer_cnt) = 0;
            (data->burst_complete) = 1;
        } else {
            SensEdu_DAC_Enable(dac_channel);
        }
    }
}
