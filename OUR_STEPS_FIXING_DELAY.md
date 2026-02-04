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

**Root cause: WiFi TX power and antenna defect.** The ESP32-C3 SuperMini has a
defective antenna (tuned for 2.7 GHz, not 2.4 GHz). TX power is capped at 8.5 dBm
to maintain connectivity. This causes frequent packet loss and TCP retransmissions.

### TX Power comparison

| TX Power | Connects? | Best delta | Typical | Worst delta |
|----------|-----------|------------|---------|-------------|
| 8.5 dBm | Yes       | 52ms       | 200ms   | 3533ms      |
| 11 dBm  | Yes       | 53ms       | 150ms   | 766ms       |
| Default (19.5 dBm) | No (status 6) | — | — | — |

At 8.5 dBm, single clicks after idle regularly hit 1-2.5 seconds. During rapid
clicking, requests pile up in the TCP stack and drain in bursts with 3-5 second
deltas. At 11 dBm, no request exceeded 766ms across the entire test session.

**RSSI (receive signal) is fine at -57 to -64 dBm.** The problem is the ESP32's
weak *transmit* — the router doesn't reliably receive the ESP32's responses,
triggering TCP retransmissions.

### Conclusion

The delay is WiFi-layer, not firmware. Keeping 8.5 dBm for connection stability
per the WIFI_TEST_LOG findings. The real fix is the hardware antenna modification
documented in COMPONENTS.md (solder a 31mm wire antenna for 6-10 dB gain).

### Mitigations applied

- Split HTU21D reads with `handleClient()` to reduce max loop block
- Added `handleClient()` after battery/OLED updates
- Optimistic UI updates already in place (buttons update instantly on click)
- Explicit on/off requests (not toggle) prevent state mismatch from duplicate delivery

### Future options

- Solder wire antenna → full TX power → sub-100ms latency
- Try 11 dBm if connection proves stable at current distance/conditions
- Client-side request deduplication (don't send while previous same-endpoint in flight)
