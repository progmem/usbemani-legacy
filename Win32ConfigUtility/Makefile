CC = gcc
CFLAGS ?= -O2 -Wall 
all:
	# Don't forget to readd -mwindows when finished!
	windres -i resources.rc -o resources.o
	$(CC) $(CFLAGS) -s -DUSE_WIN32 -o USBemaniConfig.exe USBemaniConfig.c Bootloader.c resources.o -lcomdlg32 -lcomctl32 -lhid -lsetupapi -luxtheme

clean:
	rm -f USBemaniConfig.exe resources.o