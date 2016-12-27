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
 *  Main source file for the GenericHID demo. This file contains the main tasks of the demo and
 *  is responsible for the initial application hardware configuration.
 */

#include "USBemani.h"
#include "Rotary.h"
#include "Button.h"
#include "Lights.h"
#include "PS2.h"
#include "Config.h"

Settings_Button_t *Button;
Settings_Lights_t *Lights;
Settings_Device_t *Device;

/** Main program entry point. This routine configures the hardware required by the application, then
 *  enters a loop to run the application tasks in sequence.
 */
int main(void)
{
	MCUSR &= ~(1 << WDRF);
	wdt_disable();
	
	Config_Init();
	Config_AddressButton(&Button);
	Config_AddressLights(&Lights);
	Config_AddressDevice(&Device);

	SetupHardware();
	USB_Init();
	GlobalInterruptEnable();

	for (;;)
	{
		HID_Task();
		USB_USBTask();

		// If our interrupt is holding our PS2 assertion, we'll read in new data.
		PS2_LoadData();
	}
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	/** USBemani hardware (lights, buttons, rotary) */
	/*** Rotary encoders, such as IIDX turntable or SDVX knobs. */
	Rotary_Init(_4kHz);
	Rotary_AttachEncoder(0, ChannelA);
	Rotary_AttachEncoder(1, ChannelC);

	Button_Init();
	Lights_Init();

	PS2_Init();
}

/** Event handler for the USB_Connect event. This indicates that the device is enumerating via the status LEDs and
 *  starts the library USB task to begin the enumeration and USB management process.
 */
void EVENT_USB_Device_Connect(void)
{
	/* Indicate USB enumerating */
}

/** Event handler for the USB_Disconnect event. This indicates that the device is no longer connected to a host via
 *  the status LEDs and stops the USB management task.
 */
void EVENT_USB_Device_Disconnect(void)
{
	/* Indicate USB not ready */
}

/** Event handler for the USB_ConfigurationChanged event. This is fired when the host sets the current configuration
 *  of the USB device after enumeration, and configures the generic HID device endpoints.
 */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	/* Setup HID Report Endpoints */
	ConfigSuccess &= Endpoint_ConfigureEndpoint(GENERIC_IN_EPADDR, EP_TYPE_INTERRUPT, GENERIC_EPSIZE, 1);
	ConfigSuccess &= Endpoint_ConfigureEndpoint(GENERIC_OUT_EPADDR, EP_TYPE_INTERRUPT, GENERIC_EPSIZE, 1);

	/* Indicate endpoint configuration success or failure */
}

/** Event handler for the USB_ControlRequest event. This is used to catch and process control requests sent to
 *  the device from the USB host before passing along unhandled control requests to the library for processing
 *  internally.
 */
void EVENT_USB_Device_ControlRequest(void)
{
	/* Handle HID Class specific requests */
	switch (USB_ControlRequest.bRequest)
	{
		case HID_REQ_GetReport:
			if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE))
			{
				Joystick_t JoystickData;
				CreateGenericHIDReport(&JoystickData);

				Endpoint_ClearSETUP();

				/* Write the report data to the control endpoint */
				Endpoint_Write_Control_Stream_LE(&JoystickData, sizeof(JoystickData));
				Endpoint_ClearOUT();
			}

			break;
		case HID_REQ_SetReport:
			if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE))
			{
				Output_t LightsData;

				Endpoint_ClearSETUP();

				/* Read the report data from the control endpoint */
				Endpoint_Read_Control_Stream_LE(&LightsData, sizeof(LightsData));
				Endpoint_ClearIN();

				ProcessGenericHIDReport(&LightsData);
			}

			break;
	}
}

/** Function to process the last received report from the host.
 *
 *  \param[in] DataArray  Pointer to a buffer where the last received report has been stored
 */
void ProcessGenericHIDReport(Output_t* ReportData)
{
	/*
		This is where you need to process reports sent from the host to the device. This
		function is called each time the host has sent a new report. DataArray is an array
		holding the report sent from the host.
	*/

	// If we receive a reset command, we need to push back to bootloader mode.
	if ((ReportData->Command == 0xF5) && (ReportData->Data == 0x73)) {
		// If USB is used, detach from the bus and reset it
		USB_Disable();

		// Disable all interrupts
		cli();

		// Wait two seconds for the USB detachment to register on the host.
		Delay_MS(2000);

		// Perform a watchdog reset, which will kick us back to the bootloader.
		wdt_enable(WDTO_250MS);
		for (;;);
	}

	else if (ReportData->Command == 0xF1) {
		Config_Identify();
		Config_SaveEEPROM();
		SetupHardware();
	}
	
	else if (ReportData->Command == 0xF0) {
		Config_Identify();
		SetupHardware();
	}

	// Otherwise, if the output report contains, well, anything (not 0x00), we'll parse it.
	else if (ReportData->Command >= 0x40) {
		Config_UpdateSettings(ReportData->Command, ReportData->Data);
	}
	
	// We are only handling USB-driven lighting if the PS2 is inactive.
	if (Device->PS2Assert == 0) {
		// We need to reset our timeout for the lights.
		Lights->LightsAssert = 1000;
		// We also need to forward the lighting data over.
		Lights_StoreState(ReportData->Lights);
	}
}

/** Function to create the next report to send back to the host at the next reporting interval.
 *
 *  \param[out] DataArray  Pointer to a buffer where the next report data should be stored
 */
void CreateGenericHIDReport(Joystick_t* const ReportData)
{
	/*
		This is where you need to create reports to be sent to the host from the device. This
		function is called each time the host is ready to accept a new report. DataArray is
		an array to hold the report to the host.
	*/

	// If either lighting or PS2 is asserted, we'll decrement the variables.
	// Additionally, if lights are currently asserted, we will go ahead and pull the state into our buttons for proper setting.
	if (Device->PS2Assert)    Device->PS2Assert--;

	memset(ReportData, 0, sizeof(Joystick_t));

	ReportData->X      = (Rotary_GetDirection(1) * 100);
	ReportData->Y      = (Rotary_GetDirection(0) * 100);

	ReportData->Slider =  Rotary_GetPosition(1);
	ReportData->Dial   =  Rotary_GetPosition(0);

	if (Lights->LightsAssert) {
		uint16_t LightsData = Lights_RetrieveState();
		Lights->LightsAssert--;
		ReportData->Button =  Button_GetState(1, LightsData);
	}
	else {
		ReportData->Button =  Button_GetState(0, 0);
	}
}

void HID_Task(void)
{
	/* Device must be connected and configured for the task to run */
	if (USB_DeviceState != DEVICE_STATE_Configured)
	  return;

	Endpoint_SelectEndpoint(GENERIC_OUT_EPADDR);

	/* Check to see if a packet has been sent from the host */
	if (Endpoint_IsOUTReceived())
	{
		/* Check to see if the packet contains data */
		if (Endpoint_IsReadWriteAllowed())
		{
			/* Create a temporary buffer to hold the read in report from the host */
			Output_t LightsData;

			/* Read Generic Report Data */
			Endpoint_Read_Stream_LE(&LightsData, sizeof(LightsData), NULL);

			/* Process Generic Report Data */
			ProcessGenericHIDReport(&LightsData);
		}

		/* Finalize the stream transfer to send the last packet */
		Endpoint_ClearOUT();
	}

	Endpoint_SelectEndpoint(GENERIC_IN_EPADDR);

	/* Check to see if the host is ready to accept another packet */
	if (Endpoint_IsINReady())
	{
		/* Create a temporary buffer to hold the report to send to the host */
		Joystick_t JoystickData;

		/* Create Generic Report Data */
		CreateGenericHIDReport(&JoystickData);

		/* Write Generic Report Data */
		Endpoint_Write_Stream_LE(&JoystickData, sizeof(JoystickData), NULL);

		/* Finalize the stream transfer to send the last packet */
		Endpoint_ClearIN();
	}
}
