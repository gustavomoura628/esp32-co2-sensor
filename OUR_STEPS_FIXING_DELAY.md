# Debugging UI Delay

## Problem

Web UI actions (relay toggle, LED toggle, strip controls) sometimes take >1 second
to respond, even on a single click after idle. Rapid clicking makes it much worse
(3-5 second delays).

## Session 1 — 2026-02-04

### Instrumentation added

- NTP time sync on boot (`configTime` with `pool.ntp.org`)
- `epochMs()` helper returning millisecond wall-clock timestamps
- `dbg(tag, msg)` prints `[epoch_ms] TAG: message` to serial
- `dbgClient(tag)` compares client `Date.now()` (sent as `&t=` param) against
  ESP32 `epochMs()`, printing the delta + RSSI — this is the true network latency
- Client JS logs round-trip time to browser console
- Loop iteration timing: warns on serial if any iteration exceeds 50ms
- Added `server.handleClient()` between HTU21D temp/humidity reads and after
  battery+OLED updates to reduce max blocking time

### Findings

**ESP32 handler processing is instant** — 1-3ms from "request received" to
"response sent" for all handlers (relay, LED, strip, poll). The firmware is not
the bottleneck.

**HTU21D I2C reads block the loop for ~130ms every 5 seconds.** Split with a
`handleClient()` call between temp and humidity reads, reduced to ~65ms max block.
Not the cause of multi-second delays but contributes to queueing.

### TX Power comparison

| TX Power | Connects? | Best delta | Typical | Worst delta |
|----------|-----------|------------|---------|-------------|
| 8.5 dBm | Yes       | 52ms       | 200ms   | 4277ms      |
| 11 dBm  | Yes       | 53ms       | 150ms   | 13266ms*    |
| Default (19.5 dBm) | No (status 6) | — | — | — |

*11 dBm had one good test (max 766ms) and one bad test (max 13s). Inconsistent.

### Ruled out: firmware loop blocking

Disabled all periodic JS polling (only one-shot on page load), disabled OLED
updates entirely, left bare minimum loop. Multi-second delays persisted (3118ms,
4277ms at 8.5 dBm). The loop is not the bottleneck.

### Ruled out: WebServer `handleClient()` timeout

The ESP32 WebServer's `HTTP_MAX_DATA_WAIT` defaults to **5000ms** — when a TCP
connection is accepted but the HTTP request data doesn't arrive (packet loss),
`handleClient()` blocks for up to 5 seconds. This explained the 400-460ms slow
loop iterations seen earlier.

Reduced `HTTP_MAX_DATA_WAIT` and `HTTP_MAX_CLOSE_WAIT` to 500ms via `#define`
before including `<WebServer.h>`. Result:
- Slow loop iterations dropped from 400-460ms to max 119ms (just HTU21D reads)
- **Multi-second request delays unchanged** (still 3118ms, 4277ms)

This confirms the delay is not in the ESP32's request handling. The packets
aren't reaching the ESP32's TCP stack at all — they're stuck in WiFi-layer
retransmissions on the **client side** (phone/browser retransmitting).

### Root cause

**WiFi packet loss due to antenna defect.** The ESP32-C3 SuperMini has a
defective antenna (tuned for 2.7 GHz, not 2.4 GHz). TX power is capped at
8.5 dBm to maintain connectivity. Packets get lost and the client's TCP stack
retransmits with exponential backoff (200ms → 400ms → 800ms → 1.6s → 3.2s).

RSSI (receive signal) is fine at -57 to -66 dBm. The problem is the ESP32's
weak *transmit* — the router doesn't reliably receive the ESP32's packets.

### Mitigations applied

- Reduced `HTTP_MAX_DATA_WAIT` from 5000ms to 500ms (eliminates handleClient blocking)
- Split HTU21D reads with `handleClient()` to reduce max loop block
- Added `handleClient()` after battery/OLED updates
- Optimistic UI updates (buttons update instantly on click)
- Explicit on/off requests (not toggle) prevent state mismatch from duplicate delivery

### What won't help (tested)

- Disabling periodic polling — delays persist on bare-minimum firmware
- Disabling OLED — no effect on delays
- Changing TX power — 11 dBm is inconsistent, default can't connect

### Real fix

Solder a 31mm quarter-wave wire antenna onto the chip antenna pads (6-10 dB gain,
allows full TX power). Documented in COMPONENTS.md.

### Future options

- Hardware antenna mod → full TX power → sub-100ms latency
- Client-side fetch timeout + retry (abort after 1-2s and resend)
- Client-side request deduplication (don't send while previous same-endpoint in flight)
