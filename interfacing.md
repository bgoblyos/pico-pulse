# Interfacing with the pico-pulse device

## Introduction
The pico-pulse turns a Pi Pico series microcontroller into a USB controlled intrument for outputting precisely timed pulses on up to 5 channels.
It is designed to integrate with the VISA framework and uses plaintext serial communication.

## Important quirks

The following quirks of the system should be taken into account when sending commands to the pico-pulse:

  - Multiple write in quick succession (within ~20 ms) may cause data to be lost. To avoid this,
    use queries for every command. Commands that do not have a return value will instead send and "ACK".
  - The timing between a sequence finishing and being restarted is not guaranteed to be consistent and there may be a delay on the order of a few microseconds, during which the last pulse of the sequence will continue to be generated.
    It is thus recommended to terminate your sequences with a short 0 "turn everything off" pulse. A long (over 1 us) 0 pulse at the end of the sequence will also give time for the CPU to restart the DMA before the PIO runs dry,
    resulting in consistent, but long donwtimes between sequences. 
  - The device can generate pulses with a temporal resolution of 1 CPU cycle, but each pulse must be at least 4 cycles long. Timings will be rounded up to 4 cycles if they are too short,
    otherwise they will be rounded down to an integer amount of cycles.

## Available commands

### `IDN*?`

Returns information about he device, including the pico's serial number (if the device is rp2350-based)

### `CLK?`

Returns CPU clock speed in Hz. Useful for manually generating timings.

### `BUSY?`

Returns whether the pico-pulse's is currently busy. If the return value is 0, it is safe to upload or start a new sequence without interrupting the previous one.

### `WAIT`

Blocking command, wait until the current sequence is done and return a 1. It might be a good idea to increase the device timeout when using this.
If the repetition number is INF, this command will instead stop the loop and return once the current sequence is done.

### `PULSE n t1,p1,t2,p2,...`

Set up pulse sequence. Stops the DMA, clears the PIO FIFO, generates PIO commands and copies them to the buffer.
If `n` is non-zero, the pulse sequence is started as soon as processing is complete. 

  - `n`: Number of times to repeat the sequence. Use 0 to upload a sequence without starting imediately. INF will repeat the sequence until it is aborted.
  - `ti`: Time of i-th pulse in nanoseconds, whole numbers only. Actual pulse time will be rounded down to the nearest multiple of the system clock period time.
  - `pi`: Output states during the i-th pulse. Accepts whole numbers between 0-31, each bit representing a channel.

### `CPULSE n t1,p1,t2,p2,...`

Same as `PULSE`, but timings are given in clock cycles. Pulses must be at least 4 cycles long (will be rounded up to 4 otherwise).
ives more control over rounding than the nanosecocond method, but requires knowledge of the clock frequency (see `CLK?`).

### `RUN n`

Starts and repeats the preloaded pulse sequence `n` times (`n` = "INF" for infinite).

### `STOP`

Stops output immediately. Stops the DMA, clears the PIO FIFO and sets all output pins to 0.
