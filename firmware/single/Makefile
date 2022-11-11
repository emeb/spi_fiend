# Makefile for STM32F042 w/ HAL
# 01-23-2019 E. Brombaugh

# sub directories
VPATH = .:../CMSIS:../HAL:../common:../USB

# Object files
OBJECTS =   startup_stm32f042x6.o system_stm32f0xx.o main.o cmd.o printf.o \
			usart.o led.o spi.o tim.o \
            usb.o usb_device.o usbd_desc.o usbd_conf.o usbd_cdc_if.o \
			usbd_cdc.o usbd_core.o usbd_ctlreq.o usbd_ioreq.o \
			stm32f0xx_hal.o stm32f0xx_hal_cortex.o stm32f0xx_hal_rcc.o \
			stm32f0xx_hal_rcc_ex.o stm32f0xx_hal_gpio.o stm32f0xx_hal_usart.o \
			stm32f0xx_hal_spi.o stm32f0xx_hal_pcd.o stm32f0xx_hal_pcd_ex.o 
			
# Linker script
LDSCRIPT = STM32F042F6Px_FLASH.ld

# Compiler Flags
# Note -O3 breaks USB - use 0-2 or g for safety
CFLAGS  = -O2 -g3 -ffunction-sections -std=gnu99 -Wall
CFLAGS += -I. -I../CMSIS -I../HAL -I../common -I../USB
CFLAGS += -DARM_MATH_CM0 -DUSE_HAL_DRIVER
CFLAGS += -DSTM32F042x6
CFLAGS += -mcpu=cortex-m0 
CFLAGS += -mlittle-endian -mthumb
AFLAGS  = -mlittle-endian -mthumb -mcpu=cortex-m0
LFLAGS  = $(CFLAGS) -nostartfiles -T $(LDSCRIPT) -Wl,-Map=main.map
LFLAGS += -Wl,--gc-sections
LFLAGS += --specs=nano.specs
CPFLAGS = --output-target=binary
ODFLAGS	= -x --syms

# Executables
#ARCH = arm-none-eabi
ARCH = /opt/launchpad/gcc-arm-none-eabi-7-2018-q2-update/bin/arm-none-eabi
CC = $(ARCH)-gcc
LD = $(ARCH)-ld -v
AS = $(ARCH)-as
OBJCPY = $(ARCH)-objcopy
OBJDMP = $(ARCH)-objdump
GDB = $(ARCH)-gdb
OPENOCD = openocd
DFU = dfu-util

CPFLAGS = --output-target=binary
ODFLAGS	= -x --syms

# Targets
all: main.bin

clean:
	-rm -f $(OBJECTS) crt.lst *.lst *.elf *.bin *.map *.dmp

#flash: gdb_flash
flash: oocd_flash

oocd_flash: main.elf
	$(OPENOCD) -f openocd_stlinkv2.1.cfg -c "program main.elf verify reset exit"

gdb_flash: main.elf
	$(GDB) -x flash_cmd.gdb -batch
	stty sane

dfu: main.bin
	$(DFU) -a 0 -d 0483:df11 -s 0x8000000 -D main.bin

disassemble: main.elf
	$(OBJDMP) -d main.elf > main.dis
	
dist:
	tar -c *.h *.c *.s Makefile *.cmd *.cfg openocd_doflash | gzip > minimal_hello_world.tar.gz

main.ihex: main.elf
	$(OBJCPY) --output-target=ihex main.elf main.ihex

main.bin: main.elf 
	$(OBJCPY) $(CPFLAGS) main.elf main.bin
	$(OBJDMP) $(ODFLAGS) main.elf > main.dmp
	ls -l main.elf main.bin

main.elf: $(OBJECTS) $(LDSCRIPT)
	$(CC) $(LFLAGS) -o main.elf $(OBJECTS) -lnosys -lm

%.o: %.c %.h
	$(CC) $(CFLAGS) -c -o $@ $<

