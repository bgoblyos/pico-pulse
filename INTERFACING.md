# Interfacing with the pico-pulse device

## Introduction
The pico-pulse turns a Pi Pico series microcontroller into a USB controlled intrument for outputting precisely timed pulses on up to 5 channels.
It is designed to integrate with the VISA framework and uses plaintext serial communication.

## Important quirks

The following quirks of the system should be taken into account when sending commands to the pico-pulse:

  - Multiple writes in quick succession (within ~20 ms) may cause data to be lost. To avoid this,
    use queries for every command. Commands that do not have a return value will instead send an "ACK".
  - The timing between a sequence finishing and being restarted is not guaranteed to be consistent and there may be a delay,
    during which the last pulse of the sequence will continue to be generated. During testing, this delay was measured to be 200 ns (30 CPU clock cycles),
    but don't rely on this timing. It is recommended to terminate your sequences with a short 0 "turn everything off" pulse. A long (over 1 us) 0 pulse at
    the end of the sequence will also give time for the CPU to restart the DMA before the PIO runs dry, resulting in consistent,
    but long donwtimes between sequences. It is also possible to copy your sequence into the sequence buffer multiple times to turn it into a longer,
    repeating sequence and thus eliminate all downtime.
  - The device can generate pulses with a temporal resolution of 1 CPU cycle, but each pulse must be at least 4 cycles long. Timings will be rounded up to 4 cycles if they are too short,
    otherwise they will be rounded down to an integer amount of cycles.

## Available commands

### `*IDN?` or `IDN?`

Returns information about he device, including the pico's serial number (if the device is rp2350-based)

### `CLK?`

Returns CPU clock speed in Hz. Useful for manually generating timings.

### `BUFFER?`

Returns the size of the sequence buffer.

### `MAXT?`

Return the maximum time that a single pulse can take. Note that the firmware will automatically split large pulses into smaller identical ones,
so this command is indeded to help with predicting the actual on-device length of the sequence.

### `BUSY?`

Returns whether the pico-pulse's is currently busy. If the return value is 0, it is safe to upload or start a new sequence without interrupting the previous one.

### `WAIT`

Blocking command, wait until the current sequence is done and return a 1. It might be a good idea to increase the device timeout when using this.
If the repetition number is INF, this command will instead stop the loop and return once the current sequence is done.

### `PULSE m n t1,p1,t2,p2,...`

Set up pulse sequence. Stops the DMA, clears the PIO FIFO, generates PIO commands and copies them to the buffer.
If `n` is non-zero, the pulse sequence is started as soon as processing is complete.
Returns the number of times the sequence has been copied into the buffer.

  - `m`: Number of times to copy the sequence into the sequnce buffer. Sequences repeated this way will have no delay between them.
         If `m` is greater than the amount of times the sequencee fits into the buffer (see `MAXT?` and `BUFFER?` for calculating expected size),
         the buffer will be filled completely. Setting this parameter to 0 will also activate this filling behaviour.
  - `n`: Number of times to go over the buffer. Use 0 to upload a sequence without starting imediately.
         Setting this parameter to 2^32-1 will result in the sequence being repeated indefinitely until it is aborted.
         As the CPU needs to restart the DMA each time, a small delay mas be introduced repeats.
  - `ti`: Time of i-th pulse in nanoseconds, whole numbers only. Actual pulse time will be rounded down to the nearest multiple of the system clock period time.
  - `pi`: Output states during the i-th pulse. Accepts whole numbers between 0-31, each bit representing a channel.

### `CPULSE m n t1,p1,t2,p2,...`

Same as `PULSE`, but timings are given in clock cycles. Pulses must be at least 4 cycles long (will be rounded up to 4 otherwise).
ives more control over rounding than the nanosecocond method, but requires knowledge of the clock frequency (see `CLK?`).

### `RUN n`

Starts and repeats the buffered pulse sequence `n` times (`n` = 2^32-1 for infinite).

### `STOP`

Stops output immediately. Stops the DMA, clears the PIO FIFO and sets all output pins to 0.
