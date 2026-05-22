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

The Audio Recording project turns SensEdu into a continuous PCM audio recorder. Analog microphone data is sampled at 44.1 kHz, buffered locally on the board, and streamed over USB CDC to a MATLAB host that saves it as a WAV file and plots its time-domain waveform and spectrum.

The project is built around a problem: **the USB transfer that is supposed to deliver the recording also injects noise into the analog input**. The architecture is shaped around isolating those two activities in time, so the noise only lands between audio segments, not during them. Along the way it touches DMA double-buffering, external SDRAM, framed serial protocols, and recoverable error handling – all common building blocks of more advanced acquisition systems.

## Background

### PCM Audio

Pulse Code Modulation (PCM) represents an analog audio signal as a stream of equally-spaced samples. Two parameters define it:

* **Sampling rate** ($$F_s$$): how often the ADC samples the input. CD audio uses $$F_s = 44.1\text{ kHz}$$, which covers the full audible range.
* **Bit depth**: the resolution of each sample. The STM32H7 ADC in this project produces 16-bit values.

A 30-second mono recording at these settings is therefore $$44100 \times 30 \times 2 = 2.646\text{ MB}$$ — too large for on-chip SRAM but well within the GIGA's 8 MB of external SDRAM.

### USB-Injected Noise

Whenever a high-speed digital interface like USB Full-Speed (12 Mbps) shares PCB with a sensitive analog circuit, layout becomes the dominant factor in noise performance. The audible symptom is usually a high-pitched whine or buzz that tracks USB activity. This is fundamentally a property of the Arduino GIGA R1 itself, not something the SensEdu shield can fix.

Two coupling mechanisms are at play:

- **Ground-return coupling**: return currents from the USB lines flow through the ground plane shared with the ADC, producing a small voltage drop across the finite resistance and inductance of the plane. The analog input, which references the same ground, sees that drop as common-mode noise.
- **Capacitive coupling**: parasitic capacitance between adjacent USB and analog traces lets some USB signal couple directly into the analog trace, where it appears as a voltage glitch.

A look at the official [GIGA R1 schematics](https://docs.arduino.cc/resources/schematics/ABX00063-schematics.pdf) confirms that analog and digital grounds are not separated, which is what an audio-focused board would do differently. A full investigation of the problem would require an analysis of PCB files [here](https://docs.arduino.cc/static/5927a4ebbe3f363ebd68e7c50de5e0af/ABX00063-cad-files.zip).

Since we can't change the hardware, the firmware works around it. The classical "stream every sample as it arrives" architecture spreads this noise uniformly through the recording. The architecture used here instead **batches transfers**: the firmware records into SDRAM with no concurrent USB activity, then dumps a whole segment over USB at once. The audible result is that the noise is confined to short bursts between segments, leaving the rest of the audio clean.

## Code Layout

The firmware is a single `.ino` file and the host is a single `.m` file. Everything lives under `projects/Audio_Recording/`.

| File | Purpose |
|---|---|
| `Audio_Recording.ino` | Entry point. ADC + DMA + SDRAM setup, command parser, capture pipeline, USB transfer pipeline. |
| `matlab/Audio_Recording.m` | Host script. Opens the serial port, drives the start/stop handshake, reads framed segments, saves the WAV file, plots waveform + FFT. |

## Configuration

All tunable constants live at the top of `Audio_Recording.ino`:

| Constant | Default | Meaning |
|---|---|---|
| `SAMPLING_RATE` | 44100 | ADC sampling rate in Hz. |
| `CHUNK_SIZE` | 256 | DMA half-buffer size in samples. |
| `SEGMENT_SECONDS` | 30 | Length of each SDRAM slot. Each slot consumes `SAMPLING_RATE * SEGMENT_SECONDS * 2` bytes. |
| `SEGMENT_NUM` | 2 | Number of SDRAM slots in the ping-pong. Total SDRAM footprint = `SEGMENT_NUM` × per-slot bytes; must stay under 8 MB. |
| `USB_CHUNK_BYTES` | 4080 | Payload bytes per `Serial.write` call. Deliberately not a multiple of 64; see [USB Short Packet](#usb-short-packet). |

On the MATLAB side:

| Variable | Default | Meaning |
|---|---|---|
| `ARDUINO_PORT` | `'COM16'` | Serial port the GIGA enumerates as. |
| `RECORDING_DURATION_SEC` | 40 | Desired recording length. The host requests `ceil(duration / SEGMENT_SECONDS)` segments and trims the excess. |
| `ENABLE_PLAYBACK` | `false` | Auto-play the recording when the script finishes. |

## Firmware Implementation

This section describes the general idea behind the firmware implementation. Refer to the full source under `projects/Audio_Recording/` for the complete picture.

### State Machine

The firmware lives in one of two states:

* `STATE_IDLE` — ADC is off, no slots are filling, no data is being transmitted. Default after boot.
* `STATE_RECORDING` — ADC is on, DMA is filling the SRAM ping-pong, and the main loop is copying samples to SDRAM and (when a slot fills) transmitting them.

Transitions are driven by host commands, covered in [Session Lifecycle](#session-lifecycle). The main loop itself is non-blocking and stateless beyond these two states:

```c
void loop() {
    process_command();
    process_capture();
    process_usb_transfer();
}
```

{: .NOTE}
Each function does at most one unit of work per iteration. The loop never blocks on long operations.

### SDRAM Slot Ring

The Arduino GIGA's on-chip SRAM is too small for the multi-megabyte recordings this project targets, so the slots live in the GIGA's external 8 MB SDRAM. Each slot has its own buffer plus the metadata the transfer side needs to emit a correct header without consulting any global state:

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

Buffers are allocated once at boot via the bundled `SDRAM` library and never freed for the rest of the program's life — the only dynamic allocation in the whole firmware:

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

The `slots[]` array is then treated as a **ring**, with two state structs tracking the producer and consumer sides independently:

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

* **Capture side** (`CaptureState`) writes into `slots[capture.write_idx]`. When the slot fills it sets `ready = true` and advances to the next index.
* **Transfer side** (`TransferState`) picks whichever slot has `ready == true` and the lowest `sequence_id`, transmits it, then sets `ready = false`.

The `ready` flag is the only synchronization between the two sides — a single boolean is enough since they share a thread.


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

`mark_slot_ready` hands the just-filled slot off from the capture side to the transfer side. Once `ready` flips to `true`, the next call to `process_usb_transfer` is free to pick it up.

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

If the host stalls for long enough that the next SDRAM slot isn't yet free when capture needs it, the firmware drops the incoming DMA samples and sets `FLAG_OVERRUN_DROPPED` so that the next emitted header carries a "samples were lost before this segment" notice. The host warns the user and keeps going — capture is never halted on a transient stall.

```c
if (slots[capture.write_idx].ready) {
    pending_overrun_flag |= FLAG_OVERRUN_DROPPED;
    return;
}
```

### Transfer Pipeline

Once a slot is `ready`, `process_usb_transfer` sends a 20-byte header, then the raw sample bytes in `USB_CHUNK_BYTES` pieces, and finally an 8-byte trailer. The trailer acts as a framing integrity check — see [Why a Segment Tail](#why-a-segment-tail).

The slot to transmit is chosen by lowest `sequence_id`, not by lowest index, because the host validates strict sequence ordering on every header:

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

Header and trailer each have their own struct and magic word, so the host can verify framing and integrity immediately on arrival:

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

For clean restarts from the host, every new session needs a clean firmware slate. That is the job of `reset_pipeline`, which zeroes every ring index, slot flag, and transfer cursor — without touching the SDRAM buffer pointers or the `session_id`:

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

Host commands drive the state transitions. The `'s'` command invokes `cmd_start`, which brings the firmware into a clean recording state regardless of what it was doing before:

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
`session_id` is incremented on every start. The host captures the new value from the start ACK and validates it on every subsequent frame, so any leftover data from a prior session is rejected automatically.

{: .WARNING}
The ACK is emitted **before** the ADC is re-enabled. If the ADC were running while the ACK was being written, a slow `Serial.write` could let the first DMA half-buffer fill — and be overwritten — before the main loop got a chance to drain it.

The `'p'` command invokes `cmd_stop`, which brings the firmware back to idle. It also reports the number of completed segments in the ACK's `info` field for the host's own logging:

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

The `'?'` command is a non-intrusive status query. The ACK carries the current state and the number of samples already captured into the active slot:

```c
static void cmd_status() {
    send_ack('?', capture.captured_samples);
}
```

## MATLAB Host

The host script lives in `projects/Audio_Recording/matlab/Audio_Recording.m`. It opens the serial port, drives the start/stop handshake, reads framed segments into a preallocated buffer, saves the result as a WAV file, and plots the waveform and magnitude spectrum.

### Restart-Safe Handshake

After opening the serial port, the script sends `'p'` and consumes the framed ACK. Because `'p'` is idempotent in the firmware and the ACK is uniquely framed, this works regardless of what the firmware was doing — finishing a transfer from a previous run, mid-recording, or idle. Then `'s'` starts a fresh session and the start ACK carries the new `session_id`, which the host pins for every subsequent frame:

```matlab
write(arduino, uint8('p'), 'uint8');
read_ack(arduino, 'p', FIRST_ACK_WAIT_SEC, RESYNC_MAX_BYTES, ACK_MAGIC, ACK_BYTES);

write(arduino, uint8('s'), 'uint8');
start_ack  = read_ack(arduino, 's', ACK_WAIT_SEC, ACK_BYTES * 2, ACK_MAGIC, ACK_BYTES);
session_id = start_ack.session_id;
```

A MATLAB `onCleanup` handler guarantees that the port is closed and a final `'p'` is sent even if the script errors out mid-recording. Without it, an interrupted run would leave the firmware capturing into a port nobody is reading from, which eventually overruns.

### Framed Reading

Every frame from the firmware is read through a single helper, `read_framed`, that locates the magic word and applies a per-frame validator. Conceptually it does this:

1. Read `frame_bytes` from the port.
2. If the first 4 bytes match the magic *and* the validator accepts the frame, return it.
3. Otherwise, slide forward looking for the magic; repeat. Abort once `max_resync_bytes` have been scanned.

The validator does the structural sanity check that the magic alone cannot — for segment headers it requires `session_id` to match the active session and `sample_count` to be within bounds; for ACKs it requires `cmd` to be the one we just sent. Without this, a random 4-byte slice of audio data that happens to match the magic (probability ~1 in 800 over a 5 MB resync window) would be misinterpreted as a real frame and corrupt the rest of the stream.

The actual implementation reads the stream in 64 KB chunks and locates the magic with a `find_pattern` byte-search helper, rather than sliding one byte at a time — same algorithm, far fewer round trips through `serialport`.

### Segment Receive Loop

With the session live, the host runs a fixed-length loop that reads `SEGMENTS_TO_RECORD` segments. Each iteration reads the three framed pieces in order — header, payload, tail — and copies the samples into a preallocated `data_full` buffer at the next write position:

```matlab
for seg = 1:SEGMENTS_TO_RECORD
    hdr = read_segment_header(arduino, HEADER_WAIT_SEC, RESYNC_MAX_BYTES, ...
                              SEG_MAGIC, SEG_HDR_BYTES, session_id, SEGMENT_SAMPLES);

    if double(hdr.sequence_id) ~= (last_seq_id + 1)
        partial_recording = true;  break;     % sequence gap, keep what we have
    end
    last_seq_id = double(hdr.sequence_id);

    samples = read_samples(arduino, double(hdr.sample_count), PAYLOAD_WAIT_SEC);
    tail_ok = read_segment_tail(arduino, PAYLOAD_WAIT_SEC, SEG_TAIL_MAGIC, ...
                                SEG_TAIL_BYTES, hdr.sequence_id);

    data_full(write_pos + 1 : write_pos + numel(samples)) = samples;
    write_pos = write_pos + numel(samples);

    if ~tail_ok
        partial_recording = true;  break;     % stream drift, keep what we have
    end
end
```

The audio payload itself is read as raw `uint8` and reinterpreted as `uint16` rather than read directly with `'uint16'`. This keeps the byte count explicit and pins the boundary between segments exactly where the firmware put it, regardless of any datatype-aware behaviour in the `serialport` read path:

```matlab
n_bytes = sample_count * 2;
raw_bytes = read(arduino, n_bytes, 'uint8');
data = double(typecast(uint8(raw_bytes), 'uint16'));
```

When the sequence_id jumps or the tail magic doesn't match, the loop breaks and the script saves whatever it captured so far rather than throwing. See [Why a Segment Tail](#why-a-segment-tail) for the rationale behind this graceful-degradation behaviour.

### Saving and Plotting

After the loop, the host sends `'p'` to stop the session and trims `data_full` to either `write_pos` (what was actually captured) or `RECORDING_DURATION_SEC * Fs` (the requested length), whichever is smaller. If any segments carried `FLAG_OVERRUN_DROPPED` or the recording ended early, a warning is printed before saving.

The 16-bit unsigned ADC samples are then normalised to the `[-1, +1]` range that `audiowrite` expects, and the residual DC bias from the microphone preamp is subtracted:

```matlab
y = data_full / 65535;
y = 2 * y - 1;
y = y - mean(y);
audiowrite(file_name, y, Fs);
```

The file is written to `Recordings/recorded_audio_<timestamp>.wav`, and the script finally plots the time-domain waveform and the magnitude spectrum (and optionally plays the result if `ENABLE_PLAYBACK` is set).

## Showcase

A clean voice recording produces the following waveform and magnitude spectrum:

<img src="{{site.baseurl}}/assets/images/audio_recording_waveform.png" />
{: .text-center}

_Time-domain waveform of the recorded signal_
{: .text-center}

<img src="{{site.baseurl}}/assets/images/audio_recording_spectrum.png" />
{: .text-center}

_Magnitude spectrum of the recorded signal_
{: .text-center}

The brief glitches visible on the waveform every `SEGMENT_SECONDS` are the residual USB switching noise. Crucially, they are **confined to the segment boundaries**, not smeared across the whole recording — exactly the property the batched-transfer architecture is designed to enforce. The interior of each segment is dominated by the voice content.

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

The byte-loss events that the tail check catches are **timing-sensitive and dominated by host-side conditions, not firmware behaviour**. Empirically:

- With the PC idle (no browser activity, no builds, no other USB traffic), the project has been observed to run dozens of full 30 s captures back-to-back with zero failures.
- With the PC actively used during recording (browser playing video, IDE indexing, file copy, Teams call, antivirus scan), the failure rate can rise to roughly one in two runs.

The root causes all live on the Windows / USB-host side:

- USB CDC is best-effort bulk traffic; any other device on the same controller competes for bandwidth and IRQ time.
- MATLAB drains the port from user-space, so CPU preemption or paging can stall the read long enough for the firmware-side CDC buffer to lose alignment.
- USB selective suspend / power-management renegotiation occasionally injects latency spikes — a classic source of "one bad run per ~50" when the system is otherwise quiet.
- Segment boundaries are the highest-pressure moment for the host buffer (firmware briefly pauses while swapping slots, then bursts), so failures cluster there.

If you need maximum reliability for long stress runs:

1. Plug the GIGA into a USB port on its own controller (rear desktop ports are usually best) and avoid shared hubs.
2. Disable USB selective suspend for the GIGA in Windows Power Options.
3. Close heavy background applications during the run.

None of this is required for correctness — the tail-magic protocol guarantees that any failure is caught immediately and the partial recording is preserved — but it explains why the same firmware and host script can show very different failure rates on the same machine depending on what else is happening at the time.
