#OS ?= LINUX
#OS ?= WINDOWS
OS ?= MACOSX
#OS ?= BSD

ifeq ($(OS), WINDOWS)
CC = i586-mingw32-gcc
CFLAGS ?= -O2 -Wall
hid_bootloader_cli.exe: hid_bootloader_cli.c
	$(CC) $(CFLAGS) -s -DUSE_WIN32 -o hid_bootloader_cli.exe hid_bootloader_cli.c -lhid -lsetupapi


else ifeq ($(OS), LINUX)  # also works on FreeBSD
CC ?= gcc
CFLAGS ?= -O2 -Wall
hid_bootloader_cli: hid_bootloader_cli.c
	$(CC) $(CFLAGS) -s -DUSE_LIBUSB -o hid_bootloader_cli hid_bootloader_cli.c -lusb

else ifeq ($(OS), MACOSX)
CC ?= gcc
SDK ?= //Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.9.sdk
CFLAGS ?= -O2 -Wall
hid_bootloader_cli: hid_bootloader_cli.c
	$(CC) $(CFLAGS) -DUSE_APPLE_IOKIT -isysroot $(SDK) -o hid_bootloader_cli hid_bootloader_cli.c -Wl,-syslibroot,$(SDK) -framework IOKit -framework CoreFoundation

else ifeq ($(OS), BSD)  # works on NetBSD and OpenBSD
CC ?= gcct
CFLAGS ?= -O2 -Wall
hid_bootloader_cli: hid_bootloader_cli.c
	$(CC) $(CFLAGS) -s -DUSE_UHID -o hid_bootloader_cli hid_bootloader_cli.c
endif


clean:
	rm -f hid_bootloader_cli hid_bootloader_cli.exe

bootload-32u4:
	avrdude -c usbtiny -p m32u4 -B 1 -U flash:w:Bootloader_32u4.hex -U lfuse:w:0xFC:m -U hfuse:w:0xD0:m -U efuse:w:0xF3:m

bootload-16u4:
	avrdude -c usbtiny -p m32u4 -F -B 1 -U flash:w:Bootloader_16u4.hex -U lfuse:w:0xFC:m -U hfuse:w:0xD0:m -U efuse:w:0xF3:m