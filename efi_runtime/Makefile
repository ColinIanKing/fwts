obj-m += efi_runtime.o
all:
	make -C /lib/modules/`uname -r`/build M=`pwd` modules

install:
	make -C /lib/modules/`uname -r`/build M=`pwd` modules_install

clean:
	make -C /lib/modules/`uname -r`/build M=`pwd` clean
