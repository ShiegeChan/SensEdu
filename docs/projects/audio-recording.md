---
title: Audio Recording
layout: default
math: mathjax
parent: Projects
nav_order: 6
---

# Audio Recording
{: .no_toc .fs-8 .fw-500}
---

- TOC
{:toc}

## Introduction

Audio Recording project turns SensEdu into a continuous PCM audio recorder. An analog microphone data is sampled at 44.1 kHz, buffered locally on the board, and streamed over USB CDC to a MATLAB host that saves it as a WAV file and plots its time-domain waveform and spectrum.

The project is built around the problem: **USB transfer that is supposed to deliver the recording also injects noise into the analog input**. The architecture is shaped around isolating those two activities in time, so the noise only lands between audio segments, not during them. Along the way it touches DMA double-buffering, external SDRAM, framed serial protocols, and recoverable error handling – all common building blocks of more advanced acquisition systems.

## Background

### PCM Audio

Pulse Code Modulation (PCM) represents an analog audio signal as a stream of equally-spaced samples. Two parameters define it:

* **Sampling rate** ($$F_s$$): how often the ADC samples the input. Typical CD audio frequency is $$F_S = 44.1\text{kHz}$$, covering the whole audible range.
* **Bit depth**: the resolution of each sample. The STM32H7 ADC in this project produces 16-bit values.

A 30-second mono recording at this rate is therefore $$44100 \times 30 \times \frac{16}{8} = 2.646\text{MB}$$.

### USB-Injected Noise

As mentioned before, USB transfers inject noise into the analog input. When high-frequency digital switching from USB FS at 12 Mbps and sensitive audio analog circuit is located at the same PCB, then the layout becomes a critical factor in noise performance. It could easily lead to audible interference like a high-pitched whine or buzzing. This is fundamentally a problem on the Arduino GIGA R1 itself, not something the SensEdu shield can fix. 

Potentially, the typical causes for these issues are:

- **Ground return coupling**: return currents from the USB lines flow through the ground plane shared with the ADC, producing a small voltage drop across the finite resistance and non-zero inductance of the plane.
- **Capacitive coupling**: parasitic capacitance between the USB traces and the analog input traces lets high-frequency noise from the USB signals couple directly into the analog input.

From a quick look at the [schematics](https://docs.arduino.cc/resources/schematics/ABX00063-schematics.pdf), analog and digital grounds are not separated, which is typically required for audio applications. For better analysis the [CAD Files](https://docs.arduino.cc/static/5927a4ebbe3f363ebd68e7c50de5e0af/ABX00063-cad-files.zip) should be investigated.

Since we can't change the hardware, the firmware works around it. The classical "stream every sample as it arrives" architecture used in other projects spreads this noise uniformly through the recording. The architecture used here instead **batches transfers**: the firmware records into SDRAM without any concurrent USB activity, then dumps a whole segment over USB at once. The audible result is that the noise is limited to short bursts between segments, leaving the rest of audio clean.

## Code Layout

The firmware is a single `.ino` file and the host is a single `.m` file. Everything is located under `projects/Audio_Recording/`.

| File | Purpose |
|---|---|
| `Audio_Recording.ino` | Entry point. ADC + DMA + SDRAM setup, command parser, capture pipeline, USB transfer pipeline. |
| `matlab/Audio_Recording.m` | Host script. Opens the serial port, drives the start/stop handshake, reads framed segments, saves WAV, plots waveform + FFT. |

## Configuration

All knobs live at the top of `Audio_Recording.ino`.

| Constant | Default | Meaning |
|---|---|---|
| `SAMPLING_RATE` | 44100 | ADC sampling rate in Hz. |
| `CHUNK_SIZE` | 256 | DMA half-buffer size in samples. |
| `SEGMENT_SECONDS` | 30 | Length of each SDRAM slot. Each slot consumes `SAMPLING_RATE * SEGMENT_SECONDS * 2` bytes. |
| `SEGMENT_NUM` | 2 | Number of SDRAM slots in the ping-pong. Total SDRAM footprint = `SEGMENT_NUM` × per-slot bytes; must stay under 8 MB. |
| `USB_CHUNK_BYTES` | 4080 | Payload bytes per `Serial.write` call. Deliberately not a multiple of 64, see [USB short-packet note](#usb-short-packet). |

On the MATLAB side:

| Variable | Default | Meaning |
|---|---|---|
| `ARDUINO_PORT` | `'COM16'` | Serial port of GIGA. |
| `RECORDING_DURATION_SEC` | 40 | Desired recording length. The host requests `ceil(duration / SEGMENT_SECONDS)` segments and trims the excess. |
| `ENABLE_PLAYBACK` | `false` | Auto-play the recording when the script finishes. |

## Firmware Implementation

This section will describe the general idea behind the firmware implementation. Refer to the full source code under `/projects/Audio_Recording/` for the complete picture.

### State Machine

The firmware lives in one of two states:

* `STATE_IDLE`: ADC is off, no slots are filling, no data is being transmitted. Default after boot.
* `STATE_RECORDING`: ADC is on, DMA is filling the SRAM buffer, the main loop is copying samples to SDRAM and (when full) transmitting them.

Transitions are driven by the **host commands** described in [Host Protocol](#host-protocol). The main loop itself is non-blocking and stateless beyond these two states:

```c
void loop() {
    process_command();
    process_capture();
    process_usb_transfer();
}
```

{: .NOTE}
Each function does one unit of work per iteration. The loop never blocks on long operations.

### SDRAM Slot Ring

The Arduino GIGA's on-chip SRAM is too small for the multi-megabyte recordings this project targets. The slots therefore live in the GIGA's external 8 MB SDRAM, allocated once at boot via the bundled `SDRAM` library:

```c
typedef struct {
    uint16_t* buffer;        // SDRAM buffer pointer (allocated once at boot, never freed)
    uint32_t  sequence_id;   // 0-based id within the current firmware session
    uint32_t  sample_count;  // Valid sample count in this slot
    uint32_t  flags;
    bool      ready;
} Slot;

static Slot slots[SEGMENT_NUM];

```

The `slots[]` array is treated as a **ring** managed by two independent indices living in CaptureState and TransferState:

```c
typedef struct {
    uint8_t  write_idx;         // Currently filling slot
    uint32_t captured_samples;  // Samples written into the current slot so far
    uint32_t next_sequence_id;  // Next id to assign on slot completion
} CaptureState;

typedef struct {
    int8_t   slot_idx;     // Slot being transmitted (NO_SLOT if none)
    uint32_t bytes_sent;   // Payload bytes already sent for the current slot
    bool     header_sent;  // Header already sent for the current slot
    bool     tail_sent;    // Tail magic already sent for the current slot
} TransferState;
```

* **CaptureState.write_idx**: ADC+DMA capture loop index. When it fills the slot it sets `ready = true` and advances to the next index.
* **TransferState.slot_idx**: peaks whenever a slot has `ready == true` and the lowest `sequence_id`, transmits it, then sets `ready = false`.

Dynamic allocation happens only once in the whole firmware. After `setup()`, the slots are reused for the lifetime of the program.

```c

static bool allocate_sdram() {
    for (uint8_t i = 0; i < SEGMENT_NUM; i++) {
        slots[i].buffer = (uint16_t*)SDRAM.malloc(SEGMENT_BYTES);
        if (slots[i].buffer == NULL) {
            return false;
        }
    }
    return true;
}
```


### Capture Pipeline

`process_capture` polls the two DMA flags. Whenever either fires, it copies the corresponding half of the SRAM ping-pong into the current SDRAM slot:

```c
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
```

`save_dma_half` is where the slot ring actually advances. A 256-sample DMA half-buffer almost never divides the slot size evenly, so the same call that finishes one slot also has to begin filling the next one:

```c
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
```

`mark_slot_ready` is the point where slot transfers from capture to USB transfer domain. Once `ready` flips to `true`, the slot is free to be picked up on its next loop iteration.

```c
static void mark_slot_ready() {
    uint8_t idx = capture.write_idx;
    slots[idx].sequence_id  = capture.next_sequence_id++;
    slots[idx].sample_count = SEGMENT_SAMPLES;
    slots[idx].flags        = pending_overrun_flag;
    slots[idx].ready        = true;
    pending_overrun_flag    = 0;

    capture.captured_samples = 0;
    capture.write_idx = (uint8_t)((idx + 1) % SEGMENT_NUM);
}
```

#### Slot Overrun

If the host stalls for long enough that the next SDRAM slot isn't free when capture needs it, the `FLAG_OVERRUN_DROPPED` is set on the next emitted header. This way the host could handle it gracefully.

```c
if (slots[capture.write_idx].ready) {
    pending_overrun_flag |= FLAG_OVERRUN_DROPPED;
    return;
}
```

### Transfer Pipeline

Once a slot is `ready`, `process_usb_transfer` sends a 20-byte header, then the raw sample bytes in `USB_CHUNK_BYTES` pieces, then an 8-byte trailer. The trailer is there as a framing integrity check, see [Why a Segment Tail](#why-a-segment-tail).

The picker chooses the slot with the lowest `sequence_id`, not the lowest index – the host enforces strict ordering:

```c
int8_t best = NO_SLOT;
uint32_t best_seq = 0;
for (uint8_t i = 0; i < SEGMENT_NUM; i++) {
    if (!slots[i].ready) continue;
    if (best == NO_SLOT || slots[i].sequence_id < best_seq) {
        best = (int8_t)i;
        best_seq = slots[i].sequence_id;
    }
}
```

Both header and trailer have their own structures and magic words to let the host verify framing and integrity immediately on arrival:

```c
typedef struct {
    uint32_t magic;
    uint32_t session_id;
    uint32_t sequence_id;
    uint32_t sample_count;
    uint32_t flags;
} SegmentHeader;

typedef struct {
    uint32_t magic;
    uint32_t sequence_id;
} SegmentTail;
```

### Session Lifecycle

For proper restarts from host, firmware requires a clean slate on every new session. It is achieved via `reset_pipeline`:

```c
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
    transfer.tail_sent = false;
    pending_overrun_flag = 0;
}
```

Host commands drive the state transitions. First is the start command `cmd_start`, which initiates a new session. It brings the firmware into a clean state regardless of what it was doing before and starts a new recording. It is initiated by the host sending the `'s'` command.

```c
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
```

{: .NOTE}
The `session_id` is advances at each start command, which makes protocol easier by ensuring that any incoming data is validated against the session_id the host captured from the start ACK.

{: .WARNING}
The ACK is emitted before the ADC is re-enabled. If the ADC were already running during ACK, the very first DMA half-buffer would be most likely missed and overwritten before the main loop got a chance to handle the data.

Stop command `cmd_stop` brings the firmware back to idle. It is initiated by the host sending the `'p'` command. It also reports the number of completed segments in the ACK's `info` field for the host's own logging.

```c
static void cmd_stop() {
    if (fw_state == STATE_RECORDING) {
        SensEdu_ADC_Disable(adc);
        fw_state = STATE_IDLE;
    }

    uint32_t segments_completed = capture.next_sequence_id;
    reset_pipeline();
    send_ack('p', segments_completed);
}
```

Additional logging is available by `'?'` command, which reports the current number of captured samples in the active slot.
```c
static void cmd_status() {
    send_ack('?', capture.captured_samples);
}
```

## MATLAB Host

The full source is in `projects/Audio_Recording/matlab/Audio_Recording.m`.

### Restart-Safe Handshake

The first action is to send `'p'` and consume the framed ACK. Because `'p'` is idempotent in the firmware and the ACK is uniquely framed, this works regardless of what the firmware was doing — finishing a transfer from a previous run, recording, or idle. After the ACK, the host sends `'s'`, reads the start ACK, and captures the new `session_id`:

```matlab
write(arduino, uint8('p'), 'uint8');
read_ack(arduino, 'p', FIRST_ACK_WAIT_SEC, RESYNC_MAX_BYTES, ACK_MAGIC, ACK_BYTES);

write(arduino, uint8('s'), 'uint8');
start_ack  = read_ack(arduino, 's', ACK_WAIT_SEC, ACK_BYTES * 2, ACK_MAGIC, ACK_BYTES);
session_id = start_ack.session_id;
```

A MATLAB `onCleanup` guarantees that the port is closed and a final `'p'` is sent even if the script errors out mid-recording — without it, an interrupted run leaves the firmware capturing into a port nobody is reading from, which eventually causes the next slot to overrun.

### Framed Reading

Every frame is read by a single helper that resynchronizes on the magic word and applies a per-frame validator:

```matlab
function raw = read_framed(arduino, frame_bytes, timeout_sec, max_resync_bytes, magic, validator)
    buf = read_exact(arduino, frame_bytes);
    consumed = 0;
    while true
        w = typecast(uint8(buf(1:4)), 'uint32');
        if w == magic && validator(buf)
            raw = buf;
            return;
        end
        if consumed >= max_resync_bytes
            error('Record_Audio:resyncFailed', ...);
        end
        next_byte = read_exact(arduino, 1);
        buf = [buf(2:end), next_byte];
        consumed = consumed + 1;
    end
end
```

The validator does the structural sanity check that the magic on its own cannot — for segment headers it requires `session_id` to match and `sample_count` to be within bounds, for ACKs it requires `cmd` to be the one we just sent. Without this, a random 4-byte slice of audio data that happens to match the magic word (probability ~1 in 800 over a 5 MB resync window) would be misinterpreted as a real frame and corrupt the whole stream.

### Payload Read

Audio payload is read as raw `uint8` and reinterpreted as `uint16`:

```matlab
n_bytes = sample_count * 2;
raw_bytes = read(arduino, n_bytes, 'uint8');
data = double(typecast(uint8(raw_bytes), 'uint16'));
```

Reading as `'uint8'` with an explicit byte count keeps the boundary between segments pinned exactly where the firmware put it, regardless of any datatype-aware behavior in the serialport read path.

### Output

After all segments are received the host trims to the requested duration, normalizes the unsigned 16-bit samples to the `[-1, +1]` range expected by `audiowrite`, and writes a WAV file under `Recordings/`:

```matlab
y = data_full / 65535;
y = 2 * y - 1;
y = y - mean(y);          % remove DC bias from the microphone preamp
audiowrite(file_name, y, Fs);
```

The script then plots the time-domain waveform and the magnitude spectrum.

## Showcase

A clean recording of a voice clip and its spectrum looks like this:

<img src="{{site.baseurl}}/assets/images/audio_recording_waveform.png" />
{: .text-center}

<img src="{{site.baseurl}}/assets/images/audio_recording_spectrum.png" />
{: .text-center}

The narrow tone visible at the segment boundaries in the spectrogram is the residual USB switching noise — visibly confined to short bursts every `SEGMENT_SECONDS`, not smeared across the whole recording. The interior of each segment is dominated by the voice content.

## Developer Notes

### USB Short Packet

The Windows USB CDC driver delivers bulk data to user-space only when one of three things happens: the URB fills (typically 4096 B), a **short packet** (< 64 B) arrives, or a driver-level read timeout expires. If every firmware `Serial.write` is an exact multiple of 64 B, no short packet is ever produced and trailing bytes sit in the driver's URB until the read times out — which manifests on the host as long stalls of 64×N bytes.

The fix is to size USB writes so they end with a short packet:

```c
static const uint32_t USB_CHUNK_BYTES = 4080;   // 63 * 64 + 48
```

The trailing 48-byte chunk is the short packet that flushes the URB. `Serial.flush()` does not help — it drains the local TX FIFO but does not emit a zero-length packet.

### Why Drop, Not Halt, on Overrun

If the host stalls for long enough that the next SDRAM slot isn't free when capture needs it, a strict implementation would halt the firmware to guarantee data integrity. But every minor host-side hiccup — Ctrl-C in MATLAB, the OS scheduling away the read thread, a debugger pause — would then leave the board wedged until someone presses the reset button.

This firmware instead drops the affected DMA half-buffer and marks `FLAG_OVERRUN_DROPPED` on the next outgoing header. The host knows exactly which segment was affected and can warn the user, while the firmware stays responsive and can be restarted in place. Data integrity is preserved (no silent corruption) without sacrificing recoverability — the firmware is designed so any transient failure leaves it in a state the host can recover from with a fresh `'s'`.

### Why a Segment Tail

The segment header alone is enough to *frame* the payload (the host knows how many bytes to read from `sample_count`), but it cannot *verify* that the firmware and host agree on the byte count. If the firmware ever sends fewer or more bytes than the header advertised — due to a USB stack hiccup, a bug in the transfer accounting, or any other source of drift — the host's blocking read would silently consume the wrong bytes and the misalignment would only surface much later as a corrupted next header or a sequence-id discontinuity.

The 8-byte trailer pins down the *end* of the payload exactly. As soon as the host's payload read finishes, the next 8 bytes on the wire must be `SEG_TAIL_MAGIC` followed by the matching `sequence_id`. Any deviation means the stream is misaligned, and the host aborts immediately with a specific error pointing at the affected segment rather than letting the corruption propagate. This turns silent stream drift into a loud, localized failure.

On mismatch the host does not throw — it emits a warning, stops the loop, trims `data_full` to whatever was successfully captured, and saves the partial WAV. The current segment's samples are kept (they passed the payload read cleanly); only subsequent segments are skipped. The same graceful-degradation path is taken on `sequence_id` discontinuity. A short run is far more useful than no run at all, and the user gets a clear warning explaining which segment failed.

### Host PC Load Affects Reliability

The byte-loss events that the tail check catches are **timing-sensitive and dominated by host-side conditions, not firmware behavior**. Empirically:

- With the PC idle (no browser activity, no builds, no other USB traffic), the project has been observed to run dozens of full 30 s captures back-to-back with zero failures.
- With the PC actively used during recording (browser playing video, IDE indexing, file copy, Teams call, antivirus scan), the failure rate can rise to roughly one in two runs.

Root causes are all on the Windows / USB-host side:

- USB CDC is best-effort bulk traffic; any other device on the same controller competes for bandwidth and IRQ time.
- MATLAB drains the port from user-space; CPU preemption or paging can stall the read long enough for the firmware-side CDC buffer to lose alignment.
- USB selective suspend / power-management renegotiation occasionally injects latency spikes — a classic source of "one bad run per ~50" when the system is otherwise quiet.
- The segment boundary is the highest-pressure moment for the host buffer (firmware briefly pauses while swapping slots, then bursts), so failures cluster there.

If you need maximum reliability for long stress runs:

1. Plug the Giga into a USB port on its own controller (rear desktop ports are usually best) and avoid shared hubs.
2. Disable USB selective suspend for the Giga in Windows Power Options.
3. Close heavy background applications during the run.

None of this is required for correctness — the tail-magic protocol guarantees that any failure is caught immediately and the partial recording is preserved — but it explains why the same firmware and host script can show very different failure rates on the same machine depending on what else is happening at the time.
