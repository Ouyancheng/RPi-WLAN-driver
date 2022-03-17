SUPPORT_OBJS := 


# define this if you need to give the device for your pi
TTYUSB = 
BOOTLOADER = my-install

# uncomment if you want it to automatically run.
RUN=1

THIS_MAKEFILE_PATH:=$(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))
THIS_DIR:=$(shell cd $(dir $(THIS_MAKEFILE_PATH));pwd)
THIS_MAKEFILE:=$(notdir $(THIS_MAKEFILE_PATH))

# TEST_SRC := ./spi-test.c 
# TEST_SRC := ./emmc-test.c 
# TEST_SRC := ./dma-test.c 
TEST_SRC := ./wlan-test.c 

# should not have to change below

GREP_STR := 'TRACE:\|THREAD:\|ERROR:\|PANIC:' 

# CAN_EMIT = 1

include $(THIS_DIR)/../../libpi/mk/Makefile.template

clean::
	rm -f wlan-test.o wlan-test.elf 
	rm -f dma-test.o dma-test.elf 
	rm -f emmc-test.o emmc-test.elf 
	rm -f spi-test.o spi-test.elf 
