/*
 * Pressure history & trend.
 *
 * Maintains a circular buffer of sea-level pressure samples captured every
 * PRESSURE_PERIOD_MIN minutes (config in Weather_Station.ino) and computes
 * the pressure trend in hPa/hour over the most recent
 * PRESSURE_TREND_SAMPLES * PRESSURE_PERIOD_MIN minute window.
 *
 * The trend is computed via least-squares linear regression over all samples
 * that fall inside the trend window.
 */

#include <math.h>
#include "pressure_types.h"

/* -------------------------------------------------------------------------- */
/*                                  Constants                                 */
/* -------------------------------------------------------------------------- */

#define PRESSURE_PERIOD_MS          ((uint32_t)PRESSURE_PERIOD_MIN * 60UL * 1000UL)
#define PRESSURE_TREND_WINDOW_MS    ((uint32_t)PRESSURE_TREND_SAMPLES * PRESSURE_PERIOD_MS)

/* -------------------------------------------------------------------------- */
/*                                   Globals                                  */
/* -------------------------------------------------------------------------- */

// Circular history of pressure samples.
static PressureSample pressure_history[PRESSURE_HISTORY_SIZE];

// Acts as the "have we captured anything yet" flag.
static uint8_t pressure_history_count = 0;

// Next slot to overwrite.
static uint8_t pressure_history_head = 0;

// Timestamp of the last captured sample, used to schedule the next capture.
static uint32_t last_pressure_capture_ms = 0;

/* -------------------------------------------------------------------------- */
/*                                Declarations                                */
/* -------------------------------------------------------------------------- */

static bool is_pressure_history_empty(void);
static bool can_compute_pressure_trend(void);
static bool is_ready_next_pressure_meas(void);
static void push_pressure_history(float pressure_hpa);
static uint32_t ms_until_next_pressure_meas(void);

/* -------------------------------------------------------------------------- */
/*                              Public Functions                              */
/* -------------------------------------------------------------------------- */

// Captures the given pressure if the configured period has elapsed.
// Returns true if the sample was stored.
bool try_save_pressure_sample(float pressure_hpa) {
    if (!is_ready_next_pressure_meas()) return false;
    push_pressure_history(pressure_hpa);
    return true;
}

// Computes the pressure trend in hPa/hour using a least-squares linear regression 
// over all samples whose age is within PRESSURE_TREND_WINDOW_MS.
//
// Note #1: slope = (n * Sum(t*p) - Sum(t)*Sum(p)) / (n * Sum(t*t) - Sum(t)^2)
// Note #2: Time axis is in hours, with t = 0 at "now" and older samples negative.
bool try_get_pressure_trend(float* trend_hpa_per_hour) {
    if (!can_compute_pressure_trend()) return false;

    uint32_t now_ms = millis();

    // Accumulate least-squares sums for all in-window samples.
    float sum_t = 0.0f;
    float sum_p = 0.0f;
    float sum_tt = 0.0f;
    float sum_tp = 0.0f;
    uint8_t n = 0;

    for (uint8_t i = 0; i < pressure_history_count; i++) {
        uint8_t idx = (pressure_history_head + PRESSURE_HISTORY_SIZE - 1 - i) % PRESSURE_HISTORY_SIZE;
        uint32_t age_ms = now_ms - pressure_history[idx].timestamp_ms;
        if (age_ms > PRESSURE_TREND_WINDOW_MS) break;

        float t_h = -(float)age_ms / 3600000.0f;
        float p = pressure_history[idx].pressure_hpa;
        sum_t += t_h;
        sum_p += p;
        sum_tt += t_h * t_h;
        sum_tp += t_h * p;
        n++;
    }

    if (n < PRESSURE_TREND_SAMPLES) return false;

    float denom = (float)n * sum_tt - sum_t * sum_t;

    // Check for 0 division (degenerate regression)
    if (fabsf(denom) < 1e-9f) return false;

    *trend_hpa_per_hour = ((float)n * sum_tp - sum_t * sum_p) / denom;
    return true;
}

// Returns the entire trend state (useful for printing in metrics.ino).
void get_pressure_trend_status(PressureTrendStatus* status) {
    status->trend_available = try_get_pressure_trend(&status->trend_hpa_per_hour);
    status->samples_captured = pressure_history_count;
    status->samples_required = PRESSURE_TREND_SAMPLES;
    status->ms_until_next_capture = ms_until_next_pressure_meas();
}

/* -------------------------------------------------------------------------- */
/*                              Private Functions                             */
/* -------------------------------------------------------------------------- */

// True once at least one sample has been captured.
static bool is_pressure_history_empty(void) {
    return pressure_history_count == 0;
}

// True once the buffer holds enough samples to compute a trend.
static bool can_compute_pressure_trend(void) {
    return pressure_history_count >= PRESSURE_TREND_SAMPLES;
}

// True if PRESSURE_PERIOD_MS has elapsed since the last capture.
// (or if no sample has ever been captured).
static bool is_ready_next_pressure_meas(void) {
    if (is_pressure_history_empty()) return true;
    return (millis() - last_pressure_capture_ms) >= PRESSURE_PERIOD_MS;
}

// Appends a sample to the circular buffer.
static void push_pressure_history(float pressure_hpa) {
    uint32_t t = millis();
    pressure_history[pressure_history_head].pressure_hpa = pressure_hpa;
    pressure_history[pressure_history_head].timestamp_ms = t;
    pressure_history_head = (pressure_history_head + 1) % PRESSURE_HISTORY_SIZE;
    if (pressure_history_count < PRESSURE_HISTORY_SIZE) {
        pressure_history_count++;
    }
    last_pressure_capture_ms = t;
}

// Milliseconds remaining until the next capture is due.
static uint32_t ms_until_next_pressure_meas(void) {
    if (is_pressure_history_empty()) return 0;
    uint32_t elapsed = millis() - last_pressure_capture_ms;
    if (elapsed >= PRESSURE_PERIOD_MS) return 0;
    return PRESSURE_PERIOD_MS - elapsed;
}
