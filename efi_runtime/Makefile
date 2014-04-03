KVER ?= `uname -r`
obj-m += efi_runtime.o
all:
	make -C /lib/modules/$(KVER)/build M=`pwd` modules

install:
	make -C /lib/modules/$(KVER)/build M=`pwd` modules_install

clean:
	make -C /lib/modules/$(KVER)/build M=`pwd` clean
