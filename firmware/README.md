# spi_fiend
This is the embedded firmware for the SPI Fiend. There are two different builds
here:
* single - this is the original firmware based on the ST USB stack. It provides
one CDC device which is able to program FPGAs or SPI flash chips.
* dual - this is the preferred new firmware based on the TinyUSB stack. It
provides two CDC devices, one for programming FPGA and SOI Flash and a second one
for serial UART I/O.

## Prerequisites
The preferred dual firmware requires the TinyUSB repo. Please be sure to get
that by executing the command
``git submodule update --init

## Building
Go into the desired directory `dual` or `single` and run `make` to build the
binary file.

## Installing
Connect the SPI Fiend hardware to your host USB and press both the `BOOT` and
`RST` buttons to put it into DFU bootloader mode. Then run
``make dfu
to load the new firmware binary into the hardware.
