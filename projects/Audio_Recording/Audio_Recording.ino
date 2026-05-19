/*
 * Audio Recording
 *
 * Continuous PCM audio capture from an analog microphone on pin A3, buffered
 * in external SDRAM in segments of SEGMENT_SECONDS each, streamed over USB CDC
 * to a MATLAB host as framed segments with session_id + sequence_id tagging.
 */

#include "SensEdu.h"
#include "SDRAM.h"

/* -------------------------------------------------------------------------- */
/*                                  Settings                                  */
/* -------------------------------------------------------------------------- */

// Error indicator LED
static const uint8_t ERROR_LED_PIN = D86;

// ADC sampling rate in Hz
static const uint32_t SAMPLING_RATE = 44100;

// Half-transfer chunk size of the ADC's DMA ping-pong buffer (samples)
static const uint16_t CHUNK_SIZE = 256;

// Length of each SDRAM slot, in seconds.
// Per-slot footprint: SAMPLING_RATE * SEGMENT_SECONDS * 2 bytes.
static const uint32_t SEGMENT_SECONDS = 30;

// Number of SDRAM slots used as a ping-pong buffer
static const uint8_t SEGMENT_NUM = 2;

// USB payload chunk size per loop iteration (bytes).
// MUST NOT be a multiple of 64 (USB-FS bulk packet size); see docs for why.
static const uint32_t USB_CHUNK_BYTES = 4080;

// Frame magic values. Chosen distinct so the host can resync to either type.
static const uint32_t SEG_MAGIC = 0x5345474DUL;  // wire bytes: 'M','G','E','S'
static const uint32_t ACK_MAGIC = 0x41434B21UL;  // wire bytes: '!','K','C','A'

// Sentinel meaning "no slot currently selected for transfer"
static const int8_t NO_SLOT = -1;

// Segment header flag bits
static const uint32_t FLAG_OVERRUN_DROPPED = 0x1UL;

// Derived per-slot sample and byte counts
static const uint32_t SEGMENT_SAMPLES = SAMPLING_RATE * SEGMENT_SECONDS;
static const uint32_t SEGMENT_BYTES   = SEGMENT_SAMPLES * sizeof(uint16_t);

/* -------------------------------------------------------------------------- */
/*                                   Structs                                  */
/* -------------------------------------------------------------------------- */

typedef enum {
    STATE_IDLE      = 0,
    STATE_RECORDING = 1
} FwState;

// One SDRAM segment slot
typedef struct {
    uint16_t* buffer;        // SDRAM buffer pointer
    uint32_t  sequence_id;   // 0-based monotonic id within the current session
    uint32_t  sample_count;  // valid sample count in this slot
    uint32_t  flags;         // FLAG_* bits to be carried in the segment header
    bool      ready;         // filled and ready to transmit
} Slot;

// ADC -> SDRAM capture state
typedef struct {
    uint8_t  write_idx;         // currently filling slot
    uint32_t captured_samples;  // samples written into the current slot so far
    uint32_t next_sequence_id;  // next id to assign on slot completion
} CaptureState;

// SDRAM -> USB transfer state
typedef struct {
    int8_t   slot_idx;     // slot being transmitted, or NO_SLOT
    uint32_t bytes_sent;   // payload bytes already sent for the current slot
    bool     header_sent;  // header already emitted for the current slot
} TransferState;

// 20-byte header that precedes every transmitted segment payload
typedef struct __attribute__((packed)) {
    uint32_t magic;        // SEG_MAGIC
    uint32_t session_id;
    uint32_t sequence_id;
    uint32_t sample_count; // number of uint16 samples that follow
    uint32_t flags;        // FLAG_* bits
} SegmentHeader;

// 16-byte command-ACK frame
typedef struct __attribute__((packed)) {
    uint32_t magic;        // ACK_MAGIC
    uint8_t  cmd;          // 's', 'p', or '?'
    uint8_t  state;        // FwState
    uint16_t pad;          // 0
    uint32_t session_id;
    uint32_t info;         // command-specific (segments_completed, captured_samples, ...)
} AckFrame;

/* -------------------------------------------------------------------------- */
/*                                  Globals                                   */
/* -------------------------------------------------------------------------- */

static FwState  fw_state = STATE_IDLE;
static uint32_t session_id = 0;
static uint32_t pending_overrun_flag = 0;

static Slot          slots[SEGMENT_NUM];
static CaptureState  capture;
static TransferState transfer;

static const uint16_t DMA_BUF_SIZE = CHUNK_SIZE * 2;
volatile SENSEDU_DMA_BUFFER(dma_buf, DMA_BUF_SIZE);

static ADC_TypeDef* adc = ADC1;
static uint8_t adc_pins[1] = {A3};

SensEdu_ADC_Settings adc_settings = {
    .adc = adc,
    .pins = adc_pins,
    .pin_num = 1U,

    .sr_mode = SENSEDU_ADC_SR_MODE_FIXED,
    .sampling_rate_hz = SAMPLING_RATE,

    .adc_mode = SENSEDU_ADC_MODE_DMA_CIRCULAR,
    .mem_address = (uint16_t*)dma_buf,
    .mem_size = DMA_BUF_SIZE
};

/* -------------------------------------------------------------------------- */
/*                                Declarations                                */
/* -------------------------------------------------------------------------- */

static bool allocate_sdram();
static void reset_pipeline();
static void cmd_start();
static void cmd_stop();
static void cmd_status();
static void send_ack(uint8_t cmd, uint32_t info);
static void process_command();
static void process_capture();
static void save_dma_half(volatile uint16_t* src, uint16_t src_length);
static void mark_slot_ready();
static void process_usb_transfer();
static void check_lib_errors();
static void fatal_error();

/* -------------------------------------------------------------------------- */
/*                                    Setup                                   */
/* -------------------------------------------------------------------------- */

void setup() {
    Serial.begin(2000000);  // baud is cosmetic for USB CDC

    pinMode(ERROR_LED_PIN, OUTPUT);
    digitalWrite(ERROR_LED_PIN, HIGH);

    SDRAM.begin();
    if (!allocate_sdram()) {
        fatal_error();
    }

    SensEdu_ADC_Init(&adc_settings);
    check_lib_errors();

    reset_pipeline();
}

/* -------------------------------------------------------------------------- */
/*                                    Loop                                    */
/* -------------------------------------------------------------------------- */

void loop() {
    process_command();
    process_capture();
    process_usb_transfer();
}

/* -------------------------------------------------------------------------- */
/*                              Public Functions                              */
/* -------------------------------------------------------------------------- */

// Allocates one SDRAM buffer per slot. Call once after SDRAM.begin().
static bool allocate_sdram() {
    for (uint8_t i = 0; i < SEGMENT_NUM; i++) {
        slots[i].buffer = (uint16_t*)SDRAM.malloc(SEGMENT_BYTES);
        if (slots[i].buffer == NULL) {
            return false;
        }
    }
    return true;
}

// Clears every ring index, slot flag and pending overrun bit.
// Preserves session_id and the SDRAM buffer pointers.
static void reset_pipeline() {
    for (uint8_t i = 0; i < SEGMENT_NUM; i++) {
        slots[i].ready = false;
        slots[i].sequence_id = 0;
        slots[i].sample_count = 0;
        slots[i].flags = 0;
    }
    capture.write_idx = 0;
    capture.captured_samples = 0;
    capture.next_sequence_id = 0;
    transfer.slot_idx = NO_SLOT;
    transfer.bytes_sent = 0;
    transfer.header_sent = false;
    pending_overrun_flag = 0;
}

// 's': clean restart from any prior state.
// ACK is sent BEFORE ADC start so a slow Serial.write cannot eat the first
// DMA half. See docs.
static void cmd_start() {
    SensEdu_ADC_Disable(adc);
    SensEdu_ADC_ClearDmaTransferComplete(adc);
    SensEdu_ADC_ClearDmaHalfTransferComplete(adc);

    reset_pipeline();
    session_id++;
    fw_state = STATE_RECORDING;

    send_ack('s', 0);

    SensEdu_ADC_Enable(adc);
    SensEdu_ADC_Start(adc);
}

// 'p': stop capture and discard any pending/in-flight transfer. Idempotent.
static void cmd_stop() {
    if (fw_state == STATE_RECORDING) {
        SensEdu_ADC_Disable(adc);
        fw_state = STATE_IDLE;
    }
    uint32_t segments_completed = capture.next_sequence_id;
    reset_pipeline();
    send_ack('p', segments_completed);
}

// '?': non-intrusive status query.
static void cmd_status() {
    send_ack('?', capture.captured_samples);
}

static void send_ack(uint8_t cmd, uint32_t info) {
    AckFrame ack = {
        .magic = ACK_MAGIC,
        .cmd = cmd,
        .state = (uint8_t)fw_state,
        .pad = 0,
        .session_id = session_id,
        .info = info
    };
    Serial.write((const uint8_t*)&ack, sizeof(ack));
}

static void process_command() {
    while (Serial.available() > 0) {
        char cmd = (char)Serial.read();
        switch (cmd) {
            case 's': cmd_start();  break;
            case 'p': cmd_stop();   break;
            case '?': cmd_status(); break;
            default: break;
        }
    }
}

// Polls DMA flags and writes captured samples into the current SDRAM slot.
// Overrun handling is in save_dma_half.
static void process_capture() {
    if (fw_state != STATE_RECORDING) return;

    if (SensEdu_ADC_IsDmaHalfTransferComplete(adc)) {
        SensEdu_ADC_ClearDmaHalfTransferComplete(adc);
        save_dma_half(&dma_buf[0], DMA_BUF_SIZE / 2);
    }

    if (SensEdu_ADC_IsDmaTransferComplete(adc)) {
        SensEdu_ADC_ClearDmaTransferComplete(adc);
        save_dma_half(&dma_buf[DMA_BUF_SIZE / 2], DMA_BUF_SIZE / 2);
    }
}

// Copies one DMA half-buffer into SDRAM, spilling across slot boundaries
// when needed. Drops samples + flags OVERRUN if the next slot isn't free.
static void save_dma_half(volatile uint16_t* src, uint16_t src_length) {
    uint16_t copied = 0;
    while (copied < src_length) {
        if (slots[capture.write_idx].ready) {
            pending_overrun_flag |= FLAG_OVERRUN_DROPPED;
            return;
        }

        uint16_t* dst = slots[capture.write_idx].buffer;
        uint32_t remaining_in_slot = SEGMENT_SAMPLES - capture.captured_samples;
        uint32_t to_copy = (uint32_t)(src_length - copied);
        if (to_copy > remaining_in_slot) {
            to_copy = remaining_in_slot;
        }

        for (uint32_t i = 0; i < to_copy; i++) {
            dst[capture.captured_samples + i] = src[copied + i];
        }
        capture.captured_samples += to_copy;
        copied += (uint16_t)to_copy;

        if (capture.captured_samples >= SEGMENT_SAMPLES) {
            mark_slot_ready();
        }
    }
}

// Promotes the just-filled slot to ready and advances to the next slot.
static void mark_slot_ready() {
    uint8_t idx = capture.write_idx;
    slots[idx].sequence_id  = capture.next_sequence_id++;
    slots[idx].sample_count = SEGMENT_SAMPLES;
    slots[idx].flags        = pending_overrun_flag;
    slots[idx].ready        = true;
    pending_overrun_flag    = 0;

    capture.captured_samples = 0;
    capture.write_idx        = (uint8_t)((idx + 1) % SEGMENT_NUM);
}

// Drives the SDRAM -> USB transfer in non-blocking, per-loop steps:
// pick slot -> emit 20-byte header -> emit USB_CHUNK_BYTES payload pieces -> release.
static void process_usb_transfer() {
    if (transfer.slot_idx == NO_SLOT) {
        // Pick the oldest ready slot (lowest sequence_id), not lowest index.
        // The host enforces strict sequence order.
        int8_t best = NO_SLOT;
        uint32_t best_seq = 0;
        for (uint8_t i = 0; i < SEGMENT_NUM; i++) {
            if (!slots[i].ready) continue;
            if (best == NO_SLOT || slots[i].sequence_id < best_seq) {
                best = (int8_t)i;
                best_seq = slots[i].sequence_id;
            }
        }
        if (best == NO_SLOT) {
            return;
        }
        transfer.slot_idx    = best;
        transfer.bytes_sent  = 0;
        transfer.header_sent = false;
    }

    uint8_t idx = (uint8_t)transfer.slot_idx;

    if (!transfer.header_sent) {
        SegmentHeader hdr = {
            .magic        = SEG_MAGIC,
            .session_id   = session_id,
            .sequence_id  = slots[idx].sequence_id,
            .sample_count = slots[idx].sample_count,
            .flags        = slots[idx].flags
        };
        Serial.write((const uint8_t*)&hdr, sizeof(hdr));
        transfer.header_sent = true;
        return;
    }

    uint32_t total_payload_bytes = slots[idx].sample_count * sizeof(uint16_t);
    if (transfer.bytes_sent < total_payload_bytes) {
        uint32_t remaining = total_payload_bytes - transfer.bytes_sent;
        uint32_t to_send = (remaining < USB_CHUNK_BYTES) ? remaining : USB_CHUNK_BYTES;
        const uint8_t* ptr = ((const uint8_t*)slots[idx].buffer) + transfer.bytes_sent;
        Serial.write(ptr, to_send);
        transfer.bytes_sent += to_send;
        return;
    }

    // No Serial.flush() — the 4080-byte chunk's trailing short packet
    // already flushes the host URB.
    slots[idx].ready  = false;
    transfer.slot_idx = NO_SLOT;
}

// Halts on any reported SensEdu library error during init.
static void check_lib_errors() {
    if (SensEdu_GetError() != 0) {
        fatal_error();
    }
}

// Hard halt: blinks the error LED at ~2.5 Hz forever.
// Only used for unrecoverable init failures, NOT runtime overruns.
static void fatal_error() {
    while (true) {
        digitalWrite(ERROR_LED_PIN, !digitalRead(ERROR_LED_PIN));
        delay(200);
    }
}
