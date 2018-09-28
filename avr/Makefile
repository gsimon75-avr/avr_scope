PROJECT=scope

CC=avr-gcc
OC=avr-objcopy
AS=avra
AD=avrdude
M4=m4

MCU=328p
CFLAGS=-mmcu=atmega$(MCU) -DF_CPU=16000000UL -O3 -Wall -Wstrict-prototypes -mcall-prologues
LDFLAGS=-mmcu=atmega$(MCU) -O3
ASFLAGS=
M4FLAGS=-DF_CPU=16000000 -DMCU=ATmega328P

ADFLAGS=-c stk500v2 -p m$(MCU) -P /dev/ttyU0
OCFLAGS=-j .text -j .data -O ihex 

DOCPATH=~/Documents/electro/atmega328p

all:		install
#all:		$(PROJECT).hex

.PRECIOUS:	scope.asm


.PHONY:		all help clean install reset rdfuses wrfuses rescue_fuses refman instrset

all:		$(PROJECT).hex

refman:
		xpdf $(DOCPATH)/Atmel-42735-8-bit-AVR-Microcontroller-ATmega328-328P_Datasheet.pdf &

instrset:
		xpdf $(DOCPATH)/Atmel-0856-AVR-Instruction-Set-Manual.pdf &

reset: # read a fuse
		$(AD) $(ADFLAGS) -F -q -q -U efuse:r:-:h >/dev/null

rdfuses:
		@echo -n "Extd Fuse Byte: "; $(AD) $(ADFLAGS) -F -q -q -U efuse:r:-:h
		@echo -n "High Fuse Byte: "; $(AD) $(ADFLAGS) -F -q -q -U hfuse:r:-:h
		@echo -n "Low  Fuse Byte: "; $(AD) $(ADFLAGS) -F -q -q -U lfuse:r:-:h

wrfuses:                
		$(AD) $(ADFLAGS) -F -U lfuse:w:0xd7:m
		$(AD) $(ADFLAGS) -F -U hfuse:w:0xd1:m
		$(AD) $(ADFLAGS) -F -U efuse:w:0xfc:m

rescue_fuses:   # don't forget to attach the external clk cable
		$(AD) -B 6 $(ADFLAGS) -F -U lfuse:w:0x62:m
		$(AD) -B 6 $(ADFLAGS) -F -U hfuse:w:0xd9:m
		$(AD) -B 6 $(ADFLAGS) -F -U efuse:w:0xff:m

clean:
		rm -f *.o *.map *.elf *.hex *.out

install:	$(PROJECT).hex
		$(AD) $(ADFLAGS) -e -U flash:w:$^

iosyms.inc:	hdr_to_inc.sed
		echo "#include <avr/io.h>" | avr-gcc -E -dM $(CFLAGS) - | sed -r -f $^ > $@

%.o:		%.c 
		$(CC) $(CFLAGS) -c $^

%.asm:		%.asm4 iosyms.m4 boilerplate.m4
		$(M4) $(M4FLAGS) $< | grep '.' >$@

%.hex:		%.asm
		$(AS) $(ASFLAGS) $< -l q.lst -o $@

# Atmel AVR ATmega328p, signature = 0x1e 0x95 0x0f
# Fuse byte settings:
#  Factory default is low=0x62, high=0xd9, ext=0xff
#  My setup is        low=0xd7, high=0xd1, ext=0xfc
#  (ext.osc. w/ crystal, bod@4.1/4.5V, no clkdiv8, no bootldr
# Check this with make rdfuses
