/*
 * Shared pressure-related types.
 *
 * Lives in a header (rather than a .ino file) so every other file can include it
 * regardless of the Arduino concatenation order.
 */

#ifndef PRESSURE_TYPES_H
#define PRESSURE_TYPES_H

#include <stdint.h>

/* -------------------------------------------------------------------------- */
/*                                   Structs                                  */
/* -------------------------------------------------------------------------- */

typedef struct {
    float pressure_hpa;
    uint32_t timestamp_ms;
} PressureSample;

typedef struct {
    bool trend_available;
    float trend_hpa_per_hour;
    uint8_t samples_captured;
    uint8_t samples_required;
    uint32_t ms_until_next_capture;
} PressureTrendStatus;

#endif // PRESSURE_TYPES_H