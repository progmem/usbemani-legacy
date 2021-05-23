#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>
// We need access to our lights, to deactivate the assertion.
#include "Lights.h"
// We need access to our rotary data via pointer.
#include "Rotary.h"
// We also need access to our functions for the buttons.
#include "Button.h"
#include "Config.h"
#include "PS2.h"

/** The default mappings. Each of these will load from program memory on startup of PS2 mode. */
// IIDX mapping. This is the straight-line mapping, not based on any controller order.
const uint8_t INPUT_IIDX[12] PROGMEM = {
    SQUARE,
    L1,
    CROSS,
    R1,
    CIRCLE,
    L2,
    DPAD_LEFT,
    SELECT,
    START,
    NC,
    NC,
    NC,
};
// IIDX USKOC mapping. This is the simple-solder mapping, based on the USKOC's pin order.
const uint8_t INPUT_IIDXUS[12] PROGMEM = {
	L2,
	L1,
	DPAD_LEFT,
	SELECT,
	START,
	SQUARE,
	CROSS,
	CIRCLE,
	R1,
	NC,
	NC,
	NC,
};
// IIDX JKOC mapping. This is the simple-solder mapping, based on the JKOC and KASC's pin order.
const uint8_t INPUT_IIDXJP[12] PROGMEM = {
	L1,        // Yellow
	SELECT,    // Lt. Blue
	START,     // White
	SQUARE,    // Red
	CIRCLE,    // Pink
	R1,        // Purple
	CROSS,     // Dk. Blue
	L2,        // Orange
	DPAD_LEFT, // Brown
	NC,        // Green, does nothing!
	NC,
	NC,
};

// pop'n mappings. This is the straight-line mapping, not based on any controller.
// Todo: Check minicon to see if possible?
// Todo: Grab wire order from Konami's ASC.
const uint8_t INPUT_POPN[12] PROGMEM = {
    TRIANGLE,
    CIRCLE,
    R1,
    CROSS,
    L1,
    SQUARE,
    R2,
    DPAD_UP,
    L2,
    SELECT,
    START,
    NC,
};

const uint8_t INPUT_DDR[12]  PROGMEM = {
    DPAD_UP,
    DPAD_RIGHT,
    DPAD_DOWN,
    DPAD_LEFT,
    CROSS,
    CIRCLE,
    SELECT,
    START,
    NC,
    NC,
    NC,
    NC,
};
	
// The GF ASC needs some special wiring requirements.
// The strummer uses a KOC-style emitter-detector. Wire colors are:
// Black - Ground, solder to yellow (GND)
// Red - Power, solder to Orange or Blue.
// Green - Input. This goes to the button harness.
const uint8_t INPUT_GFDM[12]  PROGMEM = {
    R2,       // Red, red wire.
    CIRCLE,   // Green, green wire.
    TRIANGLE, // Blue, blue wire!
    SELECT,   // Select, solder to PCB.
    START,    // Start, solder to PCB.
    DPAD_UP,  // Strum, see above. Solder the green lead from the sensor to this.
    L2,       // Tilt, solder to PCB.
    NC,
    NC,
    NC,
    NC,
    NC,
};

// Todo: Settle on the best direct mapping.
const uint8_t INPUT_DIRECT[12]  PROGMEM = {
	SELECT,
	START,
    DPAD_UP,
    DPAD_RIGHT,
    DPAD_DOWN,
    DPAD_LEFT,
    L2,
    R2,
    L1,
    R1,
    TRIANGLE,
    CIRCLE,
};

// Our loaded mapping. We'll load our mapping from PROGMEM or EEPROM depending on whatever is specified.
uint8_t  PS2_INPUTMAP[12] = {
	NC,
	NC,
	NC,
	NC,
	NC,
	NC,
	NC,
	NC,
	NC,
	NC,
	NC,
	NC
};

// The current state of the PS2. There are a total of 5 bytes of data that we need to transmit, and we'll keep track of where we are.
uint8_t  PS2_State;
// The final data buffer. This will hold the data that gets pushed out via the interrupt. We're storing this out here so we're never without a valid set of data.
// To start, it'll contain nothing, to prevent the PS2 from flipping out.
uint16_t PS2_Data = 0xFFFF;

// We also need an inversion mask. This is loaded based on the kind of board we're using.
// The home board will not invert, but the arcade board will, as the arcade board uses a FET to pull the line low.
uint8_t  InvertMask = 0x00;

Settings_Device_t *SettingsDevice;
Settings_Lights_t *SettingsLights;
Settings_Button_t *SettingsButton;



void PS2_Init(void) {
	cli();
	// For initialization, we need some information about the device.
	// We also need access to our lighting and button data.
	Config_AddressDevice(&SettingsDevice);
	Config_AddressLights(&SettingsLights);
	Config_AddressButton(&SettingsButton);

	// We need to configure our input mapping. By default, we will use a direct mapping, which is primarily used for debugging.
	switch(SettingsButton->ButtonMap) {
		case B_Direct:
			for (int i = 0; i < 12; i++) {
				PS2_INPUTMAP[i] = pgm_read_byte(&(INPUT_DIRECT[i])); }
			break;
		case B_IIDX:
			for (int i = 0; i < 12; i++) {
				PS2_INPUTMAP[i] = pgm_read_byte(&(INPUT_IIDX[i]));   }
			break;
		case B_IIDXUS:
			for (int i = 0; i < 12; i++) {
				PS2_INPUTMAP[i] = pgm_read_byte(&(INPUT_IIDXUS[i]));   }
			break;
		case B_IIDXJP:
			for (int i = 0; i < 12; i++) {
				PS2_INPUTMAP[i] = pgm_read_byte(&(INPUT_IIDXJP[i]));   }
			break;	
		case B_POPN:
			for (int i = 0; i < 12; i++) {
				PS2_INPUTMAP[i] = pgm_read_byte(&(INPUT_POPN[i]));   }
			break;
		case B_DDR:
			for (int i = 0; i < 12; i++) {
				PS2_INPUTMAP[i] = pgm_read_byte(&(INPUT_DDR[i]));    }
			break;
		case B_GFDM:
			for (int i = 0; i < 12; i++) {
				PS2_INPUTMAP[i] = pgm_read_byte(&(INPUT_GFDM[i]));   }
			break;
		case B_Custom:
			for (int i = 0; i < 12; i++) {
				PS2_INPUTMAP[i] = SettingsButton->CustomMap[i]; }
			break;
	}

	// If the device is configured to use both USB and PS2 (the default behavior), then we'll configure PS2 support.
	// Otherwise, we don't do anything.
	if (SettingsDevice->DeviceComm == C_Default) {
		// The PS2 communicates using a modified SPI protocol.
		// This is the SPI protocol plus an additional 'acknowledge' line. We need to configure these pins appropriately.
		// Before we do so, we'll stop all interrupts.
		DDRB  |=  0x08;

		// We also need to handle the inversion mask.
		if (SettingsDevice->DeviceType == ARCADE) InvertMask = 0xFF;

		// Kudos to Curious Inventor! (http://store.curiousinventor.com/guides/PS2/)
		// Finally, we need to configure the SPI interface. This includes a large number of settings.
		SPCR = (1 << SPE) | (1 << DORD) | (1 << CPOL) | (1 << CPHA) | (1 << SPIE);
	}
	sei();
}

// The interrupt for PS2 communications.
// This interrupt goes above all other currently used interrupts. Any interrupt-based process, like the rotary encoders, will completely halt during this time.
ISR(SPI_STC_vect) {
	SPDR = 0x00;
	// Any time we receive a packet from the PS2, we'll set a variable for PS2 assertion.
	// If the PS2 is active, USB lighting will be inactived, and will not be reactivated unless the assertion is cleared by USB input.
	SettingsDevice->PS2Assert    = 1000;
	SettingsLights->LightsAssert = 0;

	uint8_t tempdata = SPDR;
	// The very first byte to be received will be 0x01. If we received this byte, we need to reset PS2_State and load in our next piece of data.
	// Remember the inversion mask!
	if (tempdata == 0x01) {
		PS2_State = 0;
	}

	switch (PS2_State) {
		case 0:
			SPDR = 0x41 ^ InvertMask;
			PS2_Acknowledge();
			break;
		case 1:
			// If we receive the right byte (0x42), we return a proper response.
			if (tempdata == 0x42) {
				SPDR = 0x5A ^ InvertMask;
				PS2_Acknowledge();
			}
			break;
		case 2:
			// From this point on, the PS2 is expecting the right data to be loaded. We'll start with our top chunk of data.
			SPDR = (PS2_Data) ^ InvertMask;
			PS2_Acknowledge();
			break;
		case 3:
			// We need to load in the last of our data.
			SPDR = (PS2_Data >> 8) ^ InvertMask;
			PS2_Acknowledge();
			break;
		case 4:
			// We need to send the acknowledgement that we received the data. We'll load in dummy data.
			SPDR = 0xFF ^ InvertMask;
			PS2_Acknowledge();
		default:
			// After this point, we have nothing else to load, so we'll load, well, nothing!
			// However, we will also acknowledge nothing.
			SPDR = 0xFF ^ InvertMask;
	}
	// Every time we finish here, we need to increment our state.
	PS2_State++;
	// And finally, we'll send our acknowledgement. We do this last to buy us a bit of time. The PS2 will sit and wait a bit
}

// During our free time between interrupts, we need to read in the data for the PS2 and transform it.
void PS2_LoadData(void) {
	// We only do this while we're in PS2 mode.
	if (SettingsDevice->PS2Assert) {
		DDRE  |=  0x40;
		PORTE |=  0x40;
		// We need a temporary place to read data into.
		uint16_t r_temp = Button_GetState();
		// We also need some temporary space to write data to.
		uint16_t w_temp = 0;

		// We get to have fun with transformation here.
		// Note that when we perform the transform, we are prepping our data for the PS2.
		// The PS2 expects that buttons that aren't pressed are high, and buttons that are pressed are low.
		// Therefore, if a button is not pressed, we add a high bit.
		for (int i = 0; i < 12; i++) {
			if (r_temp & (1 << i))
				 w_temp |= (1 << PS2_INPUTMAP[i]);
		}

		// We also need to deal with our rotary encoders here.
		// We're reading into temporary variables to save a bit of time.
		uint8_t r0_temp = Rotary_GetDirection(0);
		uint8_t r1_temp = Rotary_GetDirection(1);

		if (r0_temp) {
			if (r0_temp == 1)
				 w_temp |= (1 << DPAD_UP);
			else w_temp |= (1 << DPAD_DOWN);
		}

		if (r1_temp) {
			if (r1_temp == 1)
				 w_temp |= (1 << DPAD_RIGHT);
			else w_temp |= (1 << DPAD_LEFT);
		}

		// Additionally, we also need to handle any specialty cases here.
		// Todo: Look up more specialty cases?
		switch (SettingsButton->ButtonMap) {
			case B_POPN:
				// pop'n requires that down, left, AND right be held.
				w_temp |= (1 << DPAD_DOWN);
				w_temp |= (1 << DPAD_LEFT);
				w_temp |= (1 << DPAD_RIGHT);
				break;
			case B_GFDM:
				// I got no clue about GFDM. We MAY need to break this out into GF and DM separately?
				// ehhhhh reports that the GF controller holds left and right.
				w_temp |= (1 << DPAD_LEFT);
				w_temp |= (1 << DPAD_RIGHT);
				break;
			default:
				break;
		}

		// Once we're done with transformation, store this data outside the function so our interrupt can access it.
		// Don't forget to invert!
		PS2_Data = ~w_temp;
	}
}

// An acknowledgement statement.
// Each time we have received a chunk of data from the PS2, we need to acknowledge that we have received said data.
void PS2_Acknowledge(void) {
	// This is very simple. We hold our acknowledge line down...
	DDRF  |=  0x40;
	PORTF &= ~0x40;
	// ...and wait... (adjust this until it recognizes)
	asm volatile("nop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\nnop\n");
	// ...and release.
	DDRF &= ~0x40;
}