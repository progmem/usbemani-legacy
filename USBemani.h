/*
             LUFA Library
     Copyright (C) Dean Camera, 2014.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2014  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Header file for GenericHID.c.
 */

#ifndef _USBEMANI_H_
#define _USBEMANI_H_

	/* Includes: */
		#include <avr/io.h>
		#include <avr/wdt.h>
		#include <avr/power.h>
		#include <avr/interrupt.h>
		#include <stdbool.h>
		#include <string.h>

		#include "Descriptors.h"
		#include "Config/AppConfig.h"

		#include <LUFA/Drivers/USB/USB.h>
		#include <LUFA/Platform/Platform.h>

 	/* The Joystick struct. This is the report that will go out to the OS. */
 		typedef struct {
 			// Our X and Y axes. These are used to output a digital signal for turntable fallback.
 			uint8_t  X;
 			uint8_t  Y;
 			// Our dial and slider axes. Thse are used to output an absolute-positioned signal, for the 'analog' turntable in Bemanitools.
 			uint8_t  Dial;
 			uint8_t  Slider;
 			// Our buttons. 16 bits for 16 buttons.
 			uint16_t Button;
 		} Joystick_t;

 	/* The Output struct. This is the report that comes into the board. */
 		typedef struct {
 			// Our lights. 16 bits for 16 lights, which gets passed to Lights_PushData().
 			uint16_t Lights;
 			// Our configuration bytes. One byte for commands, one byte for data. We will use this data to adjust our configuration.
 			// Byte 0 will be the command, byte 1 will be the data.
 			uint8_t  Command;
 			uint8_t  Data;
 		} Output_t;

	/* Function Prototypes: */
		void SetupHardware(void);
		void HID_Task(void);

		void EVENT_USB_Device_Connect(void);
		void EVENT_USB_Device_Disconnect(void);
		void EVENT_USB_Device_ConfigurationChanged(void);
		void EVENT_USB_Device_ControlRequest(void);
		void EVENT_USB_Device_StartOfFrame(void);

		void ProcessGenericHIDReport(Output_t* ReportData);
		void CreateGenericHIDReport(Joystick_t* const ReportData);

#endif

