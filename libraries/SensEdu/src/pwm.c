#include "pwm.h"

#define TIM_CLK 240000000

#define PJ8     4U
#define PJ6     37U
#define PK0     48U
#define PI2     71U

/* -------------------------------------------------------------------------- */
/*                                   Structs                                  */
/* -------------------------------------------------------------------------- */
typedef struct {
    uint8_t ch;
    uint8_t duty_cycle;
    GPIO_TypeDef* gpio;
    uint8_t stm_idx;
    uint8_t arduino_idx;
    uint32_t clock_mask;
} pin;

/* -------------------------------------------------------------------------- */
/*                                  Variables                                 */
/* -------------------------------------------------------------------------- */
static PWM_ERROR error = PWM_ERROR_NO_ERRORS;
static uint8_t is_tim_init = 0;

static pin pwm_pin_ch1 = {1U, 50, GPIOJ, 8U, PJ8, RCC_AHB4ENR_GPIOJEN};
static pin pwm_pin_ch2 = {2U, 50, GPIOJ, 6U, PJ6, RCC_AHB4ENR_GPIOJEN};
static pin pwm_pin_ch3 = {3U, 50, GPIOK, 0U, PK0, RCC_AHB4ENR_GPIOKEN};
static pin pwm_pin_ch4 = {4U, 50, GPIOI, 2U, PI2, RCC_AHB4ENR_GPIOIEN};
static pin* pins[4] = {&pwm_pin_ch1, &pwm_pin_ch2, &pwm_pin_ch3, &pwm_pin_ch4};

/* -------------------------------------------------------------------------- */
/*                                Declarations                                */
/* -------------------------------------------------------------------------- */
static void assign_error(PWM_ERROR new_error);
static PWM_ERROR check_freq(uint32_t freq);
static PWM_ERROR check_duty_cycle(uint8_t duty_cycle);
static pin* identify_pin(uint8_t arduino_pin_idx);
static void pin_init(pin* pwm_pin);
static void update_duty_cycles(void);

/* -------------------------------------------------------------------------- */
/*                              Public Functions                              */
/* -------------------------------------------------------------------------- */
void SensEdu_PWM_Init(uint8_t arduino_pin_idx, uint32_t freq, uint8_t duty_cycle) {
    PWM_ERROR freq_check = check_freq(freq);
    PWM_ERROR duty_check = check_duty_cycle(duty_cycle);
    assign_error(freq_check);
    assign_error(duty_check);

    pin* pwm_pin = identify_pin(arduino_pin_idx);

    if (freq_check != PWM_ERROR_NO_ERRORS || duty_check != PWM_ERROR_NO_ERRORS
        || pwm_pin == NULL) {
        return;
    }

    pin_init(pwm_pin);
    if (is_tim_init == 0) {
        TIMER_PWMInit();
        is_tim_init = 1;
    }
    SensEdu_PWM_SetFrequency(freq);
    SensEdu_PWM_SetDutyCycle(arduino_pin_idx, duty_cycle);
}

void SensEdu_PWM_Start(void) {
    TIMER_PWMEnable();
}

void SensEdu_PWM_Stop(void) {
    TIMER_PWMDisable();
}

void SensEdu_PWM_SetFrequency(uint32_t freq) {
    TIMER_PWMSetFreq(freq);
    update_duty_cycles(); // recalculate capture registers for all channels
}

void SensEdu_PWM_SetDutyCycle(uint8_t arduino_pin_idx, uint8_t duty_cycle) {
    pin* pwm_pin = identify_pin(arduino_pin_idx);
    pwm_pin->duty_cycle = duty_cycle;
    TIMER_PWMSetDutyCycle(pwm_pin->ch, duty_cycle);
}

PWM_ERROR PWM_GetError(void) {
    return error;
}

/* -------------------------------------------------------------------------- */
/*                              Private Functions                             */
/* -------------------------------------------------------------------------- */
static void assign_error(PWM_ERROR new_error) {
    if (error == PWM_ERROR_NO_ERRORS) {
        error = new_error;
    }
}

static PWM_ERROR check_freq(uint32_t freq)
{
    if (freq < 1 || freq > TIM_CLK/3) {
        return PWM_ERROR_BAD_FREQUENCY;
    }
    return PWM_ERROR_NO_ERRORS;
}

static PWM_ERROR check_duty_cycle(uint8_t duty_cycle)
{
    if (duty_cycle < 0 || duty_cycle > 100) {
        return PWM_ERROR_BAD_DUTY_CYCLE;
    }
    return PWM_ERROR_NO_ERRORS;
}

static pin* identify_pin(uint8_t arduino_pin_idx) {
    if (arduino_pin_idx == pwm_pin_ch1.arduino_idx) {
        return &pwm_pin_ch1;
    } else if (arduino_pin_idx == pwm_pin_ch2.arduino_idx) {
        return &pwm_pin_ch2;
    } else if (arduino_pin_idx == pwm_pin_ch3.arduino_idx) {
        return &pwm_pin_ch3;
    } else if (arduino_pin_idx == pwm_pin_ch4.arduino_idx) {
        return &pwm_pin_ch4;
    }

    assign_error(PWM_ERROR_WRONG_PIN_SELECTED);
    return NULL;
}

static void pin_init(pin* pwm_pin) {
    // Clock
    SET_BIT(RCC->AHB4ENR, pwm_pin->clock_mask);

    // GPIO Speed (play around with oscilloscope and this setting)
    GPIO_TypeDef* gpio = pwm_pin->gpio;
    uint16_t shift = (pwm_pin->stm_idx)*2;
    MODIFY_REG(gpio->OSPEEDR, 0x3UL << shift, 0b11 << shift);

    // Not pull up, pull-down
    MODIFY_REG(gpio->PUPDR, 0x3UL << shift, 0b00 << shift);

    // Alternate Function Mode
    MODIFY_REG(gpio->MODER, 0x3UL << shift, 0b10 << shift);

    // Alternate Function Configuration (AF3 for TIM8)
    shift = (pwm_pin->stm_idx)*4;
    if (shift < 32U) {
        MODIFY_REG(gpio->AFR[0], 0xFUL << shift, 3U << shift);
    } else {
        shift = shift - 32;
        MODIFY_REG(gpio->AFR[1], 0xFUL << shift, 3U << shift);
    }
}

static void update_duty_cycles(void) {
    for (uint8_t i = 0; i < 4; i++) {
        TIMER_PWMSetDutyCycle(pins[i]->ch, pins[i]->duty_cycle);
    }
}
