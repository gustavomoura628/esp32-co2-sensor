#!/bin/bash
#
# flash.sh — Trigger a flash and stream serial output in real time
#
# Touches reflash.signal (picked up by watch_flash.sh), then polls
# serial.log for new content. Streams new lines as they appear and
# exits once 3 seconds pass with no new content, or after a hard 20s
# timeout (from when output starts stabilizing, not from flash start).
#
# Usage:
#   ./flash.sh

SIGNAL="./reflash.signal"
LOG="./serial.log"
IDLE_THRESHOLD=3
HARD_TIMEOUT=20

# Record current size so we can detect when the log starts updating
if [ -f "$LOG" ]; then
    old_size=$(stat -c %s "$LOG")
else
    old_size=0
fi

# Trigger the flash
touch "$SIGNAL"

# Wait for serial.log to start updating (flash has begun)
echo "Waiting for flash to start..."
waited=0
while true; do
    if [ -f "$LOG" ]; then
        cur_size=$(stat -c %s "$LOG")
        [ "$cur_size" != "$old_size" ] && break
    fi
    if [ "$waited" -ge 20 ]; then
        echo "Timeout waiting for flash to start."
        exit 1
    fi
    sleep 1
    waited=$((waited + 1))
done

# Stream new content and wait for idle
# tee truncates serial.log on each run, so start printing from 0
printed=0
last_size=$(stat -c %s "$LOG")
last_change=$(date +%s)
start=$(date +%s)

while true; do
    sleep 0.5
    cur_size=$(stat -c %s "$LOG")
    now=$(date +%s)

    # Print any new content
    if [ "$cur_size" -gt "$printed" ]; then
        dd if="$LOG" bs=1 skip="$printed" count=$((cur_size - printed)) 2>/dev/null
        printed="$cur_size"
    fi

    # Track last change
    if [ "$cur_size" != "$last_size" ]; then
        last_size="$cur_size"
        last_change="$now"
    fi

    idle=$(( now - last_change ))
    elapsed=$(( now - start ))

    if [ "$idle" -ge "$IDLE_THRESHOLD" ]; then
        break
    fi
    if [ "$elapsed" -ge "$HARD_TIMEOUT" ]; then
        echo ""
        echo "Hard timeout reached."
        break
    fi
done
