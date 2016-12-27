#include <windows.h>
#include <setupapi.h>
#include <ddk/hidsdi.h>
#include <ddk/hidclass.h>
#include "Bootloader.h"

#define strcasecmp stricmp
#define MAX_MEMORY_SIZE 0x10000

static unsigned char firmware_image[MAX_MEMORY_SIZE];
static unsigned char firmware_mask[MAX_MEMORY_SIZE];
static int end_record_seen=0;
static int byte_count;
static unsigned int extended_addr = 0;
static int parse_hex_line(char *line);

int wait_for_device_to_appear = 0;
int hard_reboot_device = 1;
int reboot_after_programming = 1;
int verbose = 0;
//int code_size = 32 * 1024;
int code_size = 16 * 1024;
int block_size = 128;

static HANDLE win32_teensy_handle = NULL;

int Device_UpdateFirmware(const char *path_to_file)
{
	printf("Initializing Variables\n");
	unsigned char buf[260];
	int num, addr, r, first_block=1, waited=0, hard_reboot_device=1;

	printf("Checking path...");
	if (!path_to_file) {
		//fprintf(stderr, "Filename must be specified\n\n");
		printf("path not OK, stopping...\n");
		return 1;
	}
	printf("path OK.\n");

	// read the intel hex file
	// this is done first so any error is reported before using USB
	printf("Reading file...");
	num = read_intel_hex(path_to_file);
	if (num < 0) {
		//die("error reading intel hex file \"%s\"", filename);
		printf("error reading file, stopping...\n");
		return 2;
	}
	printf("file OK.\n");
	printf_verbose("Read \"%s\": %d bytes, %.1f%% usage\n",
		path_to_file, num, (double)num / (double)code_size * 100.0);

	// open the USB device
	printf("Opening device...");
	while (1) {
		if (Device_Open()) {
			printf("device open.\n");
			break;
		}
		if (hard_reboot_device) {
			printf("device reboot required.\nRebooting...");
			if (!Device_Reboot()) {
				printf("unable to reboot, stopping...\n");
				//die("Unable to find rebootor\n");
				return 3;
			}
			printf("reboot OK.\n");
			hard_reboot_device = 0; // only hard reboot W
			wait_for_device_to_appear = 1;
		}
		if (!wait_for_device_to_appear) {
			printf("unable to locate device, stopping...\n"); 
			//die("Unable to open device\n");
			return 4;
		}
		if (!waited) {
			printf_verbose("Waiting for Teensy device...\n");
			printf_verbose(" (hint: press the reset button)\n");
			waited = 1;
		}
		delay(0.25);
	}
	printf("Bootloader located.\n");
	// if we waited for the device, read the hex file again
	// perhaps it changed while we were waiting?
	if (waited) {
		printf("Verifying file...");
		num = read_intel_hex(path_to_file);
		if (num < 0) {
			printf("file error, stopping...\n");
			//die("error reading intel hex file \"%s\"", filename);
			return 5;
		}
		printf("file OK.\n");
		printf_verbose("Read \"%s\": %d bytes, %.1f%% usage\n",
		 	path_to_file, num, (double)num / (double)code_size * 100.0);
	}

	// program the data
	printf("Programming, please wait...\n");

	printf_verbose("Programming");
	fflush(stdout);
	for (addr = 0; addr < code_size; addr += block_size) {
		if (addr > 0 && !ihex_bytes_within_range(addr, addr + block_size - 1)) {
			// don't waste time on blocks that are unused,
			// but always do the first one to erase the chip
			continue;
		}
		printf_verbose(".");
		if (code_size < 0x10000) {
			buf[0] = addr & 255;
			buf[1] = (addr >> 8) & 255;
		} else {
			buf[0] = (addr >> 8) & 255;
			buf[1] = (addr >> 16) & 255;
		}
		ihex_get_data(addr, block_size, buf + 2);
		r = Device_Write(buf, block_size + 2, first_block ? 3.0 : 0.25);
		if (!r) 
			//die("error writing to Teensy\n");
			return 6;
		first_block = 0;
	}
	printf_verbose("\n");

	// reboot to the user's new code
	if (reboot_after_programming) {
		printf_verbose("Booting\n");
		buf[0] = 0xFF;
		buf[1] = 0xFF;
		memset(buf + 2, 0, sizeof(buf) - 2);
		Device_Write(buf, block_size + 2, 0.25);
	}
	Device_Close();
	CloseHandle(win32_teensy_handle);

	return 0;
}

HANDLE open_usb_device(int vid, int pid)
{
	GUID guid;
	HDEVINFO info;
	DWORD index, required_size;
	SP_DEVICE_INTERFACE_DATA iface;
	SP_DEVICE_INTERFACE_DETAIL_DATA *details;
	HIDD_ATTRIBUTES attrib;
	HANDLE h;
	BOOL ret;

	HidD_GetHidGuid(&guid);
	info = SetupDiGetClassDevs(&guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (info == INVALID_HANDLE_VALUE) return NULL;
	for (index=0; 1 ;index++) {
		iface.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
		ret = SetupDiEnumDeviceInterfaces(info, NULL, &guid, index, &iface);
		if (!ret) {
			SetupDiDestroyDeviceInfoList(info);
			break;
		}
		SetupDiGetDeviceInterfaceDetail(info, &iface, NULL, 0, &required_size, NULL);
		details = (SP_DEVICE_INTERFACE_DETAIL_DATA *)malloc(required_size);
		if (details == NULL) continue;
		memset(details, 0, required_size);
		details->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
		ret = SetupDiGetDeviceInterfaceDetail(info, &iface, details,
			required_size, NULL, NULL);
		if (!ret) {
			free(details);
			continue;
		}
		h = CreateFile(details->DevicePath, GENERIC_READ|GENERIC_WRITE,
			FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
			FILE_FLAG_OVERLAPPED, NULL);
		free(details);
		if (h == INVALID_HANDLE_VALUE) continue;
		attrib.Size = sizeof(HIDD_ATTRIBUTES);
		ret = HidD_GetAttributes(h, &attrib);
		if (!ret) {
			CloseHandle(h);
			continue;
		}
		if (attrib.VendorID != vid || attrib.ProductID != pid) {
			CloseHandle(h);
			continue;
		}
		SetupDiDestroyDeviceInfoList(info);
		return h;
	}
	return NULL;
}

int write_usb_device(HANDLE h, void *buf, int len, int timeout)
{
	static HANDLE event = NULL;
	unsigned char tmpbuf[1040];
	OVERLAPPED ov;
	DWORD n, r;

	if (len > sizeof(tmpbuf) - 1) return 0;
	if (event == NULL) {
		event = CreateEvent(NULL, TRUE, TRUE, NULL);
		if (!event) return 0;
	}
	ResetEvent(&event);
	memset(&ov, 0, sizeof(ov));
	ov.hEvent = event;
	tmpbuf[0] = 0;
	memcpy(tmpbuf + 1, buf, len);
	if (!WriteFile(h, tmpbuf, len + 1, NULL, &ov)) {
		if (GetLastError() != ERROR_IO_PENDING) return 0;
		r = WaitForSingleObject(event, timeout);
		if (r == WAIT_TIMEOUT) {
			CancelIo(h);
			return 0;
		}
		if (r != WAIT_OBJECT_0) return 0;
	}
	if (!GetOverlappedResult(h, &ov, &n, FALSE)) return 0;
	return 1;
}

int Device_Open(void)
{
	Device_Close();
	win32_teensy_handle = open_usb_device(0x16C0, 0x0478);

	if (!win32_teensy_handle)
		win32_teensy_handle = open_usb_device(0x03eb, 0x2067);

	if (!win32_teensy_handle) return 0;
	return 1;
}

int Device_Write(void *buf, int len, double timeout)
{
	int r;
	if (!win32_teensy_handle) return 0;
	r = write_usb_device(win32_teensy_handle, buf, len, (int)(timeout * 1000.0));
	return r;
}

void Device_Close(void)
{
	if (!win32_teensy_handle) return;
	CloseHandle(win32_teensy_handle);
	win32_teensy_handle = NULL;
}

int Device_Reboot(void)
{
	HANDLE rebootor;
	int r;

	rebootor = open_usb_device(USBEMANI_VID, USBEMANI_PID_KOC);
	if (!rebootor)
		rebootor = open_usb_device(USBEMANI_VID, USBEMANI_PID_DAO);
	if (!rebootor) 
		return 0;
	
	uint8_t buf[4] = {0xFF, 0xFF, 0xF5, 0x73};
	r = write_usb_device(rebootor, &buf, 4, 100);

	CloseHandle(rebootor);
	return r;
}

int Device_SendCommand(const uint8_t command, const uint8_t data)
{
	HANDLE device;
	int r;

	device = open_usb_device(USBEMANI_VID, USBEMANI_PID_KOC);
	if (!device)
		 device = open_usb_device(USBEMANI_VID, USBEMANI_PID_DAO);
	if (!device) return 0;

	uint8_t buf[4] = {0x00, 0x00, command, data};
	r = write_usb_device(device, &buf, 4, 100);
	CloseHandle(device);

	Sleep(10.0);
	return r;
}

int Device_DetectBoard(void)
{
	HANDLE device;

	device = open_usb_device(USBEMANI_VID, USBEMANI_PID_KOC);

	if (device) return USBEMANI_PID_KOC;
	else device = open_usb_device(USBEMANI_VID, USBEMANI_PID_DAO);

	if (device) return USBEMANI_PID_DAO;
	else		return 0;
}

/* from ihex.c, at http://www.pjrc.com/tech/8051/pm2_docs/intel-hex.html */

/* parses a line of intel hex code, stores the data in bytes[] */
/* and the beginning address in addr, and returns a 1 if the */
/* line was valid, or a 0 if an error occurred.  The variable */
/* num gets the number of bytes that were stored into bytes[] */

int read_intel_hex(const char *filename)
{
	FILE *fp;
	int i, lineno=0;
	char buf[1024];

	byte_count = 0;
	end_record_seen = 0;
	for (i=0; i<MAX_MEMORY_SIZE; i++) {
		firmware_image[i] = 0xFF;
		firmware_mask[i] = 0;
	}
	extended_addr = 0;

	fp = fopen(filename, "r");
	if (fp == NULL) {
		//printf("Unable to read file %s\n", filename);
		return -1;
	}
	while (!feof(fp)) {
		*buf = '\0';
		if (!fgets(buf, sizeof(buf), fp)) break;
		lineno++;
		if (*buf) {
			if (parse_hex_line(buf) == 0) {
				//printf("Warning, parse error line %d\n", lineno);
				fclose(fp);
				return -2;
			}
		}
		if (end_record_seen) break;
		if (feof(stdin)) break;
	}
	fclose(fp);
	return byte_count;
}

int parse_hex_line(char *line)
{
	int addr, code, num;
        int sum, len, cksum, i;
        char *ptr;

        num = 0;
        if (line[0] != ':') return 0;
        if (strlen(line) < 11) return 0;
        ptr = line+1;
        if (!sscanf(ptr, "%02x", &len)) return 0;
        ptr += 2;
        if ((int)strlen(line) < (11 + (len * 2)) ) return 0;
        if (!sscanf(ptr, "%04x", &addr)) return 0;
        ptr += 4;
          /* printf("Line: length=%d Addr=%d\n", len, addr); */
        if (!sscanf(ptr, "%02x", &code)) return 0;
	if (addr + extended_addr + len >= MAX_MEMORY_SIZE) return 0;
        ptr += 2;
        sum = (len & 255) + ((addr >> 8) & 255) + (addr & 255) + (code & 255);
	if (code != 0) {
		if (code == 1) {
			end_record_seen = 1;
			return 1;
		}
		if (code == 2 && len == 2) {
			if (!sscanf(ptr, "%04x", &i)) return 1;
			ptr += 4;
			sum += ((i >> 8) & 255) + (i & 255);
        		if (!sscanf(ptr, "%02x", &cksum)) return 1;
			if (((sum & 255) + (cksum & 255)) & 255) return 1;
			extended_addr = i << 4;
			//printf("ext addr = %05X\n", extended_addr);
		}
		if (code == 4 && len == 2) {
			if (!sscanf(ptr, "%04x", &i)) return 1;
			ptr += 4;
			sum += ((i >> 8) & 255) + (i & 255);
        		if (!sscanf(ptr, "%02x", &cksum)) return 1;
			if (((sum & 255) + (cksum & 255)) & 255) return 1;
			extended_addr = i << 16;
			//printf("ext addr = %08X\n", extended_addr);
		}
		return 1;	// non-data line
	}
	byte_count += len;
        while (num != len) {
                if (sscanf(ptr, "%02x", &i) != 1) return 0;
		i &= 255;
		firmware_image[addr + extended_addr + num] = i;
		firmware_mask[addr + extended_addr + num] = 1;
                ptr += 2;
                sum += i;
                (num)++;
                if (num >= 256) return 0;
        }
        if (!sscanf(ptr, "%02x", &cksum)) return 0;
        if (((sum & 255) + (cksum & 255)) & 255) return 0; /* checksum error */
        return 1;
}

int ihex_bytes_within_range(int begin, int end)
{
	int i;

	if (begin < 0 || begin >= MAX_MEMORY_SIZE ||
	   end < 0 || end >= MAX_MEMORY_SIZE) {
		return 0;
	}
	for (i=begin; i<=end; i++) {
		if (firmware_mask[i]) return 1;
	}
	return 0;
}

void ihex_get_data(int addr, int len, unsigned char *bytes)
{
	int i;

	if (addr < 0 || len < 0 || addr + len >= MAX_MEMORY_SIZE) {
		for (i=0; i<len; i++) {
			bytes[i] = 255;
		}
		return;
	}
	for (i=0; i<len; i++) {
		if (firmware_mask[addr]) {
			bytes[i] = firmware_image[addr];
		} else {
			bytes[i] = 255;
		}
		addr++;
	}
}

int printf_verbose(const char *format, ...)
{
	va_list ap;
	int r;

	va_start(ap, format);
	if (verbose) {
		r = vprintf(format, ap);
		fflush(stdout);
		return r;
	}
	return 0;
}

void delay(double seconds)
{
	Sleep(seconds * 1000.0);
}
