#makefile for standby.bin
LINUX_ROOT = ./../../../../..
LINUX_INC = $(LINUX_ROOT)/include
ARCH_SRC = $(LINUX_ROOT)/arch/arm/mach-sun4i
ARCH_INC = $(ARCH_SRC)/include
SRCDIR = .

INCLUDE   = -I$(SRCDIR)\
		-I$(LINUX_INC)\
		-I$(ARCH_INC)

INPUTFILE = common.c standby.c stack.S standby_clock.c dram/dram.c  \
            standby_int.c standby_ir.c standby_key.c standby_power.c    \
            standby_tmr.c standby_twi.c standby_usb.c

OBJ = common.o standby.o stack.o standby_clock.o dram/dram.o    \
      standby_int.o standby_ir.o standby_key.o standby_power.o  \
      standby_tmr.o standby_twi.o standby_usb.o


OUPUTFILE = standby

CROSS_COMPILE = arm-none-linux-gnueabi-
LD_FILE = standby.xn
CFLAGS = -Os   -g  -march=armv7-a -fno-unwind-tables -fno-asynchronous-unwind-tables -mlittle-endian  -c

all:$(INPUTFILE)
	$(CROSS_COMPILE)gcc $(INCLUDE) $(CFLAGS) common.c           -o common.o
	$(CROSS_COMPILE)gcc $(INCLUDE) $(CFLAGS) standby.c          -o standby.o
	$(CROSS_COMPILE)gcc $(INCLUDE) $(CFLAGS) standby_clock.c    -o standby_clock.o
	$(CROSS_COMPILE)gcc $(INCLUDE) $(CFLAGS) standby_int.c      -o standby_int.o
	$(CROSS_COMPILE)gcc $(INCLUDE) $(CFLAGS) standby_ir.c       -o standby_ir.o
	$(CROSS_COMPILE)gcc $(INCLUDE) $(CFLAGS) standby_key.c      -o standby_key.o
	$(CROSS_COMPILE)gcc $(INCLUDE) $(CFLAGS) standby_power.c    -o standby_power.o
	$(CROSS_COMPILE)gcc $(INCLUDE) $(CFLAGS) standby_tmr.c      -o standby_tmr.o
	$(CROSS_COMPILE)gcc $(INCLUDE) $(CFLAGS) standby_twi.c      -o standby_twi.o
	$(CROSS_COMPILE)gcc $(INCLUDE) $(CFLAGS) standby_usb.c      -o standby_usb.o
	$(CROSS_COMPILE)gcc $(INCLUDE) $(CFLAGS) dram/dram.c        -o dram/dram.o
	$(CROSS_COMPILE)gcc $(INCLUDE) $(CFLAGS) stack.S            -o stack.o

	$(CROSS_COMPILE)ld -T $(LD_FILE)  -EL  $(OBJ) -o $(OUPUTFILE).elf  -Map $(OUPUTFILE).map
	$(CROSS_COMPILE)objdump -D $(OUPUTFILE).elf > $(OUPUTFILE).lst
	$(CROSS_COMPILE)objcopy -O binary $(OUPUTFILE).elf $(OUPUTFILE).bin

	-rm *.o
	-rm ./../standby.o
	cp standby.bin standby.code

	@echo ----------------------------------------
	@echo well done!
clean:
	-rm *.o *.map *.lst *.bin *.elf *.exe dram/*.o

