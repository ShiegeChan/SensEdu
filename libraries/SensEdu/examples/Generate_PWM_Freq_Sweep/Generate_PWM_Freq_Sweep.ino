/*
 * Generate_PWM_Freq_Sweep
 *
 * Generates a stepped PWM frequency sweep on D71 from parameters typed into
 * the Serial Monitor.
 *
 * Expected input: [fstart, fend, steps, step_dur_ms]
 */

#include "SensEdu.h"

#define MAX_INPUT_LEN 256

typedef enum {
    CMD_STATUS_NO_INPUT = 0x00,
    CMD_STATUS_VALID_INPUT = 0x01,
    CMD_STATUS_INVALID_INPUT = 0x02,
    CMD_STATUS_BUFFER_OVERFLOW = 0x03
} CMD_STATUS;

uint32_t lib_error = 0;
const uint8_t pwm_pin = D71;
const uint8_t param_count = 4;
uint32_t sweep_params[param_count];

/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */
void setup() {
    Serial.begin(115200);
    while(!Serial) {
        delay(1);
    }
    Serial.println("Started Initialization...");
    SensEdu_PWM_Init(pwm_pin, 1000000, 50);
    SensEdu_TIMER_DelayInit();
    check_errors();
    Serial.println("Setup is successful.");
}

/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */
void loop() {
    check_errors();
    CMD_STATUS status = read_cmd(sweep_params, param_count);
    switch(status) {
        case CMD_STATUS_INVALID_INPUT:
            Serial.println("ERROR: Invalid Input");
            Serial.println("Expected input: [fstart, fend, steps, step_dur_ms]");
            break;
        case CMD_STATUS_BUFFER_OVERFLOW:
            Serial.println("ERROR: Buffer Overflow.");
            Serial.print(MAX_INPUT_LEN);
            Serial.println(" characters is maximum.");
            break;
        case CMD_STATUS_VALID_INPUT:
            print_params(sweep_params);
            pwm_sequence(sweep_params);
            break;
        default:
            break;
    }
}

/* -------------------------------------------------------------------------- */
/*                                  Functions                                 */
/* -------------------------------------------------------------------------- */
static void check_errors(void) {
    lib_error = SensEdu_GetError();
    while (lib_error != 0) {
        delay(1000);
        Serial.print("Error: 0x");
        Serial.println(lib_error, HEX);
    }
}

static void pwm_sequence(uint32_t* params) {
    uint32_t fstart = params[0];
    uint32_t fend = params[1];
    uint32_t steps = params[2];
    uint32_t step_dur = params[3];

    if (fend <= fstart) {
        Serial.println("ERROR: 'fend' <= 'fstart'");
        return;
    }

    if (steps < 2) {
        Serial.println("ERROR: 'steps' must be at least '2'");
        return;
    }

    // Display
    uint32_t freq_increment = (fend-fstart)/(steps-1);
    Serial.print("Frequency steps: ");
    for (uint32_t i = 0; i < steps; i++) {
        uint32_t freq = fstart + i * freq_increment;
        if (i == steps - 1) {
            freq = fend;
        }

        Serial.print(freq);
        if (i != steps - 1) {
            Serial.print(", ");
        } else {
            Serial.println();
        }
    }

    // Sequence
    for (uint32_t i = 0; i < steps; i++) {
        uint32_t freq = fstart + i * freq_increment;
        if (i == steps - 1) {
            freq = fend;
        }
        SensEdu_PWM_SetFrequency(freq);
        SensEdu_PWM_Start();
        SensEdu_TIMER_Delay_us(1000 * step_dur);
        SensEdu_PWM_Stop();
    }
}

static CMD_STATUS read_cmd(uint32_t* params, const uint8_t param_count) {
    static char buf[MAX_INPUT_LEN];
    static uint8_t idx = 0;
    while(Serial.available()) {
        char c = (char)Serial.read();
        if (c == '\n' || c == '\r') {
            buf[idx] = '\0';
            idx = 0;
            Serial.print("Parsing ");
            Serial.println(buf);
            return parse_buf(buf, params, param_count);
        } else {
            if (idx >= MAX_INPUT_LEN - 1) {
                idx = 0;
                return CMD_STATUS_BUFFER_OVERFLOW;
            } else {
                buf[idx++] = c;
            }
        }
    }
    return CMD_STATUS_NO_INPUT;
}

static CMD_STATUS parse_buf(char* buf, uint32_t* params, const uint8_t param_count) {
    char* start = strchr(buf, '[');
    char* end = strchr(buf, ']');
    if (!start || !end || end <= start) {
        return CMD_STATUS_INVALID_INPUT;
    }

    // exclude brackets
    *end = '\0';
    start++;

    // split by commas
    uint8_t token_count = 0;
    char* token = strtok(start, ",");
    while (token != NULL) {
        token_count++;
        if (token_count > param_count) {
            return CMD_STATUS_INVALID_INPUT;
        }

        // trim leading spaces
        while (isspace(*token)) {
            token++;
        }

        // convert to number
        char* endptr;
        params[token_count-1] = (uint32_t)strtoul(token, &endptr, 10);

        // throw error at inputs like '123x' or '123ab' while allowing for '123  '
        while (isspace(*endptr)) {
            endptr++;
        }
        if (*endptr != '\0') {
            return CMD_STATUS_INVALID_INPUT;
        }
        
        // next token
        token = strtok(NULL, ",");
    }

    if (token_count != param_count) {
        return CMD_STATUS_INVALID_INPUT;
    }

    return CMD_STATUS_VALID_INPUT;
}

static void print_params(uint32_t* params) {
    char buf[128];
    sprintf(
        buf, "Applied params: fstart = %luHz, fend = %luHz, steps = %lu, step_dur = %lums",
        params[0], params[1], params[2], params[3]
    );
    Serial.println(buf);
}
