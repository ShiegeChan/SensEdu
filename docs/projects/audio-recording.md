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

Audio Recording is a project that turns the SensEdu shield into a continuous PCM audio recorder. An analog microphone with its own preamp/ADC stage is sampled by Arduino GIGA R1 at 44.1 kHz, buffered locally on the board, and streamed over USB CDC to a MATLAB host that saves it as a WAV file and plots its time-domain waveform and spectrum.

The project is built around a real embedded-systems problem: **the same USB transfer that is supposed to deliver the recording also injects noise into the analog input**. The architecture is shaped around isolating those two activities in time so the noise only lands between audio segments, not during them. Along the way it touches DMA double-buffering, external SDRAM, framed serial protocols, and recoverable error handling — all common building blocks of more advanced acquisition systems.

## Background

### PCM Audio

Pulse Code Modulation (PCM) represents an analog audio signal as a stream of equally-spaced numeric samples. Two parameters define it:

* **Sampling rate** ($$F_s$$, in Hz) — how often the ADC samples the input. By the [Nyquist-Shannon sampling theorem](https://en.wikipedia.org/wiki/Nyquist%E2%80%93Shannon_sampling_theorem), the maximum representable frequency is $$F_s / 2$$. CD-quality audio uses $$F_s = 44100$$ Hz, covering the audible 20 Hz–20 kHz range with margin.
* **Bit depth** — the resolution of each sample. The STM32H7 ADC in this project produces 16-bit values, giving a theoretical dynamic range of $$20 \log_{10}(2^{16}) \approx 96$$ dB.

A 30-second mono recording at this rate is therefore $$44100 \times 30 \times 2 = 2.646$$ MB — too large for on-chip SRAM but well within the GIGA's 8 MB of external SDRAM.

### Why USB Noise Matters

At 44.1 kHz the ADC produces a new sample every ~22 µs. The signal coming into it is small (millivolt-range from a microphone) and routed across the same PCB as a USB 2.0 Full-Speed transceiver switching at ~12 Mbit/s. Every USB bit is a nanosecond-scale voltage edge, and on a small board there are two ways those edges leak into the analog front-end:

* **Capacitive coupling** — any pair of nearby copper traces forms a tiny (pF-scale) parasitic capacitor. A fast `dV/dt` on the USB side pushes a corresponding current through that stray capacitance into the analog trace, where it appears as a voltage glitch.
* **Ground-return coupling** — the current driven into the USB line has to return through the ground plane. That return current produces a small voltage drop across the finite resistance and inductance of the plane, and the analog input — which references the same ground — sees that drop as common-mode noise.

The microphone front-end is high-impedance and millivolt-scale, so even sub-microvolt disturbances are clearly audible. The result is a wideband noise floor that tracks USB activity.

This is fundamentally a PCB-layout problem on the **Arduino GIGA R1** itself, not something the SensEdu shield can fix. Both the ADC and the USB transceiver sit on the GIGA, they share one ground plane, and the USB traces run close enough to the analog pin headers for both coupling paths above to be active. The GIGA is a general-purpose development board, optimised for mechanical compatibility and broad usability rather than for low-noise analog acquisition — sacrificing some analog cleanliness is the standard trade-off for that form factor. A board built specifically for audio capture would use split AGND/DGND planes joined at a single point near the ADC, a separate analog supply rail, and physical separation between USB and analog routing.

Since we can't change the hardware, the firmware works *around* it. The classical "stream every sample as it arrives" architecture spreads this noise uniformly through the recording. The architecture used here instead **batches transfers**: the firmware records silently into SDRAM for tens of seconds, then dumps a whole segment over USB at once. The audible result is that the noise is confined to short bursts between segments, leaving the segment interior clean. Trimming or fading the seams is a host-side problem and out of scope for the firmware.

### DMA Double-Buffering

Servicing the ADC by interrupt at 44.1 kHz would burn most of the CPU just on context switches. Instead, the ADC is paired with a DMA controller that copies each conversion directly into a circular SRAM buffer. The buffer is split into two halves: when the first half is full, the DMA peripheral raises a **half-transfer (HT)** flag and starts writing into the second half; when the second half is full it raises a **transfer-complete (TC)** flag and wraps back to the first.

The CPU's job becomes "drain the half that just finished while the DMA fills the other one." As long as the drain finishes before the next half completes, no samples are lost. With a 256-sample half-buffer at 44.1 kHz, the deadline is $$256 / 44100 \approx 5.8$$ ms — comfortable for a non-blocking main loop.

<img src="{{site.baseurl}}/assets/images/audio_recording_pipeline.png" />
{: .text-center}

_Three-stage data path: ADC → SRAM (DMA, real-time) → SDRAM (CPU copy, every 5.8 ms) → USB (CPU bulk write, only between segments)_
{: .text-center}

## Hardware Setup

The project uses one analog audio input. By default the firmware reads pin **A3** with **ADC1**, though any analog-capable pin works after editing the `adc_pins` array. The input must be biased to mid-supply (1.65 V on a 3.3 V rail) so the AC audio signal swings around the middle of the ADC's range — a microphone module with built-in preamp does this for you.

| Signal | Arduino GIGA pin |
|---|---|
| Microphone OUT (audio) | A3 |
| Microphone VCC | 3.3 V |
| Microphone GND | GND |
| Error indicator | D86 (onboard LED) |

The GIGA's 8 MB SDRAM is enabled via the bundled `SDRAM` library and provides the long-term storage for the recording slots. No external memory hardware is needed.

## Code Layout

The firmware is a single `.ino` file and the host is a single `.m` file. Everything lives under `projects/Audio_Recording/`.

| File | Purpose |
|---|---|
| `Audio_Recording.ino` | Entry point. ADC + DMA + SDRAM setup, command parser, capture pipeline, USB transfer pipeline. |
| `matlab/Audio_Recording.m` | Host script. Opens the serial port, drives the start/stop handshake, reads framed segments, saves WAV, plots waveform + FFT. |

## Configuration

All knobs live as `static const` at the top of `Audio_Recording.ino`.

| Constant | Default | Meaning |
|---|---|---|
| `SAMPLING_RATE` | 44100 | ADC sampling rate in Hz. |
| `CHUNK_SIZE` | 256 | DMA half-buffer size in samples. Larger gives more headroom against transient stalls at the cost of a longer drain deadline. |
| `SEGMENT_SECONDS` | 30 | Length of each SDRAM slot. Each slot consumes `SAMPLING_RATE * SEGMENT_SECONDS * 2` bytes. |
| `SEGMENT_NUM` | 2 | Number of SDRAM slots in the ping-pong. Total SDRAM footprint = `SEGMENT_NUM` × per-slot bytes; must stay under 8 MB. |
| `USB_CHUNK_BYTES` | 4080 | Payload bytes per `Serial.write` call. Deliberately not a multiple of 64 — see [USB short-packet note](#usb-short-packet-on-windows). |

On the MATLAB side, only the user settings normally need editing:

| Variable | Default | Meaning |
|---|---|---|
| `ARDUINO_PORT` | `'COM16'` | Serial port the GIGA enumerates as. |
| `RECORDING_DURATION_SEC` | 40 | Desired recording length. The host requests `ceil(duration / SEGMENT_SECONDS)` segments and trims the surplus. |
| `ENABLE_PLAYBACK` | `false` | Auto-play the recording when the script finishes. |

## Firmware Implementation

The full source is in `projects/Audio_Recording/Audio_Recording.ino`. This section focuses on the structural decisions; the file itself is heavily commented for the line-level details. ADC and DMA setup (the `SensEdu_ADC_Settings` struct, the `SENSEDU_DMA_BUFFER` macro, and the available modes) is covered in the [ADC library reference]({% link library/adc.md %}) and is not duplicated here.

### State Machine

The firmware lives in one of two states:

* `STATE_IDLE` — ADC is off, no slots are filling, no data is being transmitted. Default after boot.
* `STATE_RECORDING` — ADC is on, DMA is filling the SRAM buffer, the main loop is copying samples to SDRAM and (when full) transmitting them.

Transitions are driven by the **host commands** described in [Host Protocol](#host-protocol). The main loop itself is non-blocking and stateless beyond these two states:

```c
void loop() {
    process_command();        // read 's' / 'p' / '?' from USB, change state, emit ACK
    process_capture();        // drain the DMA flags into SDRAM (no-op if idle)
    process_usb_transfer();   // emit one chunk of one ready slot per call (no-op if nothing ready)
}
```

Each helper does at most one bounded unit of work per iteration. The loop runs thousands of times per second and never blocks on long operations.

### SDRAM Slot Ring

The Arduino GIGA's on-chip SRAM is shared with the heap, stack, and peripheral buffers — far too tight for the multi-megabyte recordings this project targets. The slots therefore live in the GIGA's external 8 MB SDRAM, allocated once at boot via the bundled `SDRAM` library:

```c
typedef struct {
    uint16_t* buffer;        // SDRAM buffer pointer
    uint32_t  sequence_id;   // 0-based monotonic id within the current session
    uint32_t  sample_count;  // valid sample count in this slot
    uint32_t  flags;         // FLAG_* bits to be carried in the segment header
    bool      ready;         // filled and ready to transmit
} Slot;

static Slot slots[SEGMENT_NUM];

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

This is the only dynamic allocation in the whole firmware. After `setup()` the heap is never touched again — the slots are reused for the lifetime of the program, and there is no fragmentation risk during operation.

The `slots[]` array is treated as a **ring** managed by two independent indices:

* **Producer** (capture) writes into `capture.write_idx`. When it fills the slot it sets `ready = true` and advances to the next index.
* **Consumer** (transfer) picks whichever slot has `ready == true` and the lowest `sequence_id`, transmits it, then sets `ready = false`.

The four metadata fields per slot make the ring self-describing: the consumer needs nothing beyond the slot itself to emit a correct header. The boolean `ready` flag is the only synchronization between the two sides.

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
            return;                  // see Recoverable Overruns below
        }

        uint16_t* dst = slots[capture.write_idx].buffer;
        uint32_t remaining_in_slot = SEGMENT_SAMPLES - capture.captured_samples;
        uint32_t to_copy = (src_length - copied) > remaining_in_slot
                         ? remaining_in_slot : (src_length - copied);

        for (uint32_t i = 0; i < to_copy; i++) {
            dst[capture.captured_samples + i] = src[copied + i];
        }
        capture.captured_samples += to_copy;
        copied                   += (uint16_t)to_copy;

        if (capture.captured_samples >= SEGMENT_SAMPLES) {
            mark_slot_ready();       // flips .ready, advances write_idx
        }
    }
}
```

The while loop iterates at most twice per call: once to fill the tail of the current slot, once to start the head of the next one.

`mark_slot_ready` is the hand-off from producer to consumer:

```c
static void mark_slot_ready() {
    uint8_t idx = capture.write_idx;
    slots[idx].sequence_id  = capture.next_sequence_id++;
    slots[idx].sample_count = SEGMENT_SAMPLES;
    slots[idx].flags        = pending_overrun_flag;
    slots[idx].ready        = true;
    pending_overrun_flag    = 0;

    capture.captured_samples = 0;
    capture.write_idx        = (idx + 1) % SEGMENT_NUM;
}
```

Once `ready` flips to `true`, the consumer is free to pick the slot up on its next loop iteration — no explicit notification needed, the boolean *is* the signal.

### Transfer Pipeline

`process_usb_transfer` is the consumer. Once a slot is `ready`, it sends a 20-byte header followed by the raw sample bytes, in `USB_CHUNK_BYTES` pieces. One chunk per loop iteration keeps the main loop responsive to incoming commands and capture flags.

The picker chooses the slot with the lowest `sequence_id`, not the lowest index — the host enforces strict ordering, so we have to emit them in fill order even after a transient overrun:

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

### Recoverable Overruns

If the host stalls — e.g. the operating system delays USB IN tokens, or the user pauses MATLAB — a slot may finish filling before the previous one has finished transferring. The classical reaction is to halt with a fatal error. This firmware instead **drops the affected DMA half-buffer and sets `FLAG_OVERRUN_DROPPED` on the next emitted header**:

```c
if (slots[capture.write_idx].ready) {
    pending_overrun_flag |= FLAG_OVERRUN_DROPPED;
    return;   // current slot is still pending transmission, abandon these samples
}
```

The host sees the flag, prints a warning, and continues. No board reset is required to recover from a transient stall.

### Session Lifecycle

The state transitions promised by the State Machine section happen inside two short helpers that `process_command` dispatches to. They share a single state-clearing routine, `reset_pipeline`, which zeroes every ring index and every slot flag without touching the SDRAM buffer pointers (those are allocated once at boot and never reallocated):

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
    pending_overrun_flag = 0;
}
```

`cmd_start` is the more careful of the two — its job is to bring the firmware into a clean recording state regardless of what it was doing before, so the host can recover from any prior failure mode with a single `'s'`:

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

Two ordering decisions are deliberate:

* **The ADC is disabled first**, even if the previous state was already `STATE_IDLE`. The cost is microseconds; the benefit is a defensive guarantee that no stale DMA flag fires into the freshly-reset state during the next few instructions.
* **The ACK is emitted before the ADC is re-enabled.** `Serial.write` on USB CDC can block if the host hasn't drained the TX FIFO yet. If the ADC were already running during that block, the very first DMA half-buffer would be overwritten by the circular DMA before the main loop got a chance to drain it. Sending the ACK first means the only thing affected by a slow write is the moment recording begins — not the recording itself.

`cmd_stop` is symmetric and idempotent:

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

The ADC is stopped only if it was running. The pipeline reset then wipes every `slots[].ready` flag and the `transfer.slot_idx` cursor, which cancels any in-flight USB transfer cleanly — once `process_usb_transfer` runs again it sees no ready slots and does nothing. The number of completed segments is reported back in the ACK's `info` field for the host's own logging.

## Host Protocol

The wire format is two framed message types, both little-endian, both starting with a distinctive 4-byte magic so the host can resynchronize after any sequence of unexpected bytes.

### ACK Frame (16 bytes)

Sent by the firmware in response to every command. The framing means the host can never confuse an ACK with a stale segment header or with audio data.

| Offset | Field | Bytes | Notes |
|---|---|---|---|
| 0 | `magic` | 4 | `0x41434B21` (wire bytes `'!','K','C','A'`) |
| 4 | `cmd` | 1 | `'s'`, `'p'`, or `'?'` — which command this acks |
| 5 | `state` | 1 | `0 = IDLE`, `1 = RECORDING` |
| 6 | `pad` | 2 | always 0 — used as structural validation |
| 8 | `session_id` | 4 | bumped on every `'s'`; tags every subsequent header |
| 12 | `info` | 4 | command-specific (segments completed, samples captured, …) |

### Segment Header (20 bytes)

Immediately precedes the audio payload for one slot.

| Offset | Field | Bytes | Notes |
|---|---|---|---|
| 0 | `magic` | 4 | `0x5345474D` (wire bytes `'M','G','E','S'`) |
| 4 | `session_id` | 4 | must match the session_id received in the start ACK |
| 8 | `sequence_id` | 4 | 0-based, monotonic within the session |
| 12 | `sample_count` | 4 | number of `uint16` samples that follow |
| 16 | `flags` | 4 | bit 0: `OVERRUN_DROPPED` — samples were lost before this segment |

After the header, the firmware writes exactly `sample_count * 2` bytes of raw audio.

### Commands

A command is a single ASCII byte sent from host to firmware. Each is acknowledged by an ACK frame.

* `'s'` — Start a fresh session. Resets the pipeline, increments `session_id`, enables the ADC. Always brings the firmware to a clean recording state regardless of what it was doing before.
* `'p'` — Stop the session. Cancels any in-flight transfer, drops any partially-filled slot, returns to idle. Idempotent.
* `'?'` — Non-intrusive status query. The ACK carries the current state and number of samples captured into the active slot.

The `session_id` discriminator is what makes restarts work without a board reset. Every header that arrives is validated against the session_id the host captured from the start ACK — anything from a prior session is silently dropped by the host's resync logic.

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

### USB Short Packet on Windows

The Windows USB CDC driver delivers bulk data to user-space only when one of three things happens: the URB fills (typically 4096 B), a **short packet** (< 64 B) arrives, or a driver-level read timeout expires. If every firmware `Serial.write` is an exact multiple of 64 B, no short packet is ever produced and trailing bytes sit in the driver's URB until the read times out — which manifests on the host as long stalls of 64×N bytes.

The fix is to size USB writes so they end with a short packet:

```c
static const uint32_t USB_CHUNK_BYTES = 4080;   // 63 * 64 + 48
```

The trailing 48-byte chunk is the short packet that flushes the URB. `Serial.flush()` does not help — it drains the local TX FIFO but does not emit a zero-length packet.

### Why Drop, Not Halt, on Overrun

If the host stalls for long enough that the next SDRAM slot isn't free when capture needs it, a strict implementation would halt the firmware to guarantee data integrity. But every minor host-side hiccup — Ctrl-C in MATLAB, the OS scheduling away the read thread, a debugger pause — would then leave the board wedged until someone presses the reset button.

This firmware instead drops the affected DMA half-buffer and marks `FLAG_OVERRUN_DROPPED` on the next outgoing header. The host knows exactly which segment was affected and can warn the user, while the firmware stays responsive and can be restarted in place. Data integrity is preserved (no silent corruption) without sacrificing recoverability — the firmware is designed so any transient failure leaves it in a state the host can recover from with a fresh `'s'`.
