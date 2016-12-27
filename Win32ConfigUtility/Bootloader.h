#ifndef _BOOTLOADER_H_
#define _BOOTLOADER_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include "resources.h"

#define USBEMANI_VID     0x0573
#define USBEMANI_PID_KOC 0x0001
#define USBEMANI_PID_DAO 0x0002

// USB Access Functions
int  Device_UpdateFirmware(const char *path_to_file);
int  Device_Reboot(void);
int  Device_SendCommand(const uint8_t command, const uint8_t data);
int  Device_DetectBoard(void);

int  Device_Open(void);
void Device_Close(void);
int  Device_Write(void *buf, int len, double timeout);

// Intel Hex File Functions
int  read_intel_hex(const char *filename);
int  ihex_bytes_within_range(int begin, int end);
void ihex_get_data(int addr, int len, unsigned char *bytes);

// Misc stuff
int  printf_verbose(const char *format, ...);
void delay(double seconds);

#endif