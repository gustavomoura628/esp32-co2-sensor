# Claude Code — Project Instructions

## Flashing the ESP32

**Do not run `pio run -t upload` or `pio device monitor` directly.** The serial
monitor can freeze Claude Code's shell.

Instead, use the flash script:

```
./flash.sh
```

This triggers `watch_flash.sh` (which must be running in a separate terminal).
Build and upload output is suppressed on success (shown on failure). After a
successful flash, 5 seconds of ESP32 serial output is captured and printed.

### When to flash

- After any edit to files that affect the firmware (source, headers, config)
- Run `./flash.sh` and check the output for errors

## UI delay debugging

We are actively debugging a recurring delay issue with the web UI.
Progress is tracked in `OUR_STEPS_FIXING_DELAY.md`. When working on this
issue, update that doc with mid-level detail about what was tried, what
was found, and what was ruled out at each step of debugging.
