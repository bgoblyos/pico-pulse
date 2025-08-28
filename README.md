# pico-pulse

This firmware turns the Raspberry Pi Pico 2 into a signal generator capable of producing precisely times pulses on up to 5 channels.
It is designed primarily for use in optically detected magnetic resonance (ODMR) experiments.

## Installation

Ensure that the Pi Pico C SDK is installed and the `PICO_SDK_PATH` env variable points to it.

After cloning the repo, build the binary:
```
mkdir build
cd build
cmake -DPICO_PLATFORM=rp2350 -DPICO_BOARD=pico2 ..
make
```

Upload the binary to the Pico 2 either by copying the UF2 file to it in bootsel mode or by uploading the ELF file via a debug probe
(refer to official documentation on exact instructions).

## Usage

The pico-pulse is designed to be controlled by a PC over a USB link. It accepts commands in plaintext.
See [INTERFACING](INTERFACING.md) for a list of commands.

## Credits
The PIO code is based on the pio_blink example from the [pico-examples](https://github.com/raspberrypi/pico-examples) repo.
[Greg Chadwick's blog](https://gregchadwick.co.uk/blog/playing-with-the-pico-pt4/) has also been a helpful resource in setting up DMA.
