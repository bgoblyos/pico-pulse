# Interfacing with the pico-pulse device

The pico-pulse turns the Pi Pico (2) into a USB controlled intrument for outputting precisely timed pulses on up to 5 channels.
It is designed to integrate with the VISA framework and uses plaintext serail communication. The following commands are available.

### `IDN*?`

Returns information about he device, including the pico's serial number (if the device is rp2350-based)

### `CLK?`

Returns CPU clock speed in Hz. Useful for manually generating timings.

### `BUSY?`

Returns whether the pico-pulse's is currently busy. If the return value is 0, it is safe to upload or start a new sequence without interrupting the previous one.

### `WAIT`

Blocking command, wait until the current sequnce is done and return a 1. It might be a good idea to increase the device timeout when using this.

### `PULSE n t1,p1,t2,p2,...`

Set up pulse sequence. Stops the DMA, clears the PIO FIFO, generates PIO commands and copies them to the buffer.
If `n` is non-zero, the pulse sequence is started as soon as processing is complete. 

  - `n`: Number of times to repeat the sequence. Use 0 to upload a sequence without starting imediately. INF will repeat the sequence until it is aborted.
         Note that the timing between a sequence finishing and being restarted is not guaranteed to be consistent and there may be a delay of up to 1 us, during which the last pulse of the sequence will continue to be generated.
         It is thus recommended to terminate your sequences with a short 0 "turn everything off" pulse. A long (over 1 us) 0 pulse at the end of the sequence will also give time for the CPU to restart the DMA before the PIO runs dry,
         resulting in consistent, but long donwtimes between sequences. 
  - `ti`: Time of i-th pulse in nanoseconds, whole numbers only. Actual pulse time will be rounded down to the nearest multiple of the system clock period time.
  - `pi`: Output states during the i-th pulse. Accepts whole numbers between 0-31, each bit representing a channel.

### `CPULSE n t1,p1,t2,p2,...`

Same as `PULSE`, but timings are given in clock cycles. Pulses must be at least 4 cycles long (will be rounded up to 4 otherwise).
ives more control over rounding than the nanosecocond method, but requires knowledge of the clock frequency (see `CLK?`).

### `RUN n`

Starts and repeats the preloaded pulse sequence `n` times (`n` = "INF" for infinite).

### `STOP`

Stops output immediately. Stops the DMA, clears the PIO FIFO and sets all output pins to 0.
