# Claude Code — Project Instructions

## Flashing the ESP32

**Do not run `pio run -t upload` or `pio device monitor` directly.** The serial
monitor can freeze Claude Code's shell.

Instead, use the flash script:

```
./flash.sh
```

This triggers `watch_flash.sh` (which must be running in a separate terminal),
waits for serial output to stabilize (3s idle), and prints the log. Hard timeout
is 20 seconds in case the ESP32 code loops.

### When to flash

- After any edit to files that affect the firmware (source, headers, config)
- Run `./flash.sh` and check the output for errors
