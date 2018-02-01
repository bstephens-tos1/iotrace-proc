CONFIG_MODULE_SIG=n

MY_CFLAGS = $(DBGFLAGS) -Wall -Wextra -Wconversion -Wshadow -Wstrict-prototypes
CFLAGS_io_proc.o += $(MY_CFLAGS)
obj-y += io_proc.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean