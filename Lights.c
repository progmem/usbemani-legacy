#include <avr/io.h>
#include <avr/interrupt.h>
#include "Config.h"
#include "Lights.h"

#define L07_DDR  DDRD
#define L07_PORT PORTD
#define L07_PIN  PIND

#define L8F_DDR  DDRB
#define L8F_PORT PORTB
#define L8F_PIN  PINB

Settings_Lights_t *SettingsLights;
Settings_Device_t *SettingsDevice;

uint16_t LightsData;

// Function for initializing lighting.
void Lights_Init(void) {
	// We'll turn off interrupts temporarily during setup procedures.
	cli();

  // We're not really using any settings here in this implementation.
  // However, we can expose configuration objects if we need them.
  Config_AddressLights(&SettingsLights);
	Config_AddressDevice(&SettingsDevice);

	// The reference implementation for lights supports 12 lights, and uses a multiplexing setup for shared I/O with buttons.
	//
	// This implementation works off of the following parts:
	// * Two CY74FCT573T latches
	// * One 10k resistor per I/O pin
	//
	// Wire the following:
	// * Each button should be wired to an I/O pin through a 10k in-line resistor. BTN-------10k---I/O
	// * Each I/O pin should be wired directly to an input on the CY74FCT573T.     CY74:IN---------I/O
	// * Wire /OE on the CY74FCT573T to ground.                                    CY74:/OE--------GND
	// * Wire  LE on the CY74FCT573T to PF7.                                       CY74:LE---------PF7
  // * Wire  LE to ground through a 22k pull-down resistor.                      CY74:LE---10k---GND
  // * Wire each output on the CY74FCT573T to a light. Avoid directly driving LEDs; consider using a MOSFET or transistor and the necessary support components.
  //                                                                             CY74:OUT--------FET
	// * Wire each unused input on the CY74FCT573T to ground.                      CY74:IN---------GND
  //
  // This specific implementation uses the following pins for I/O:
  // * PD0-PD7
  // * PB4-PB7 (PB0-PB3 are the SPI bus used for PS2)

  // For setup, we only configure the latching pin.
  DDRF |= 0x80;

	// Since setup is done, we can re-enable interrupts.
	sei();
}


void Lights_SetState(uint16_t OutputData) {
  // USBemani boards have an onboard LED at PE6.
  // We'll turn it on any time there's a light turned on.
  if(OutputData) {
    DDRE  |=  0x40;
    PORTE |=  0x40;
  } else {
    DDRE  &= ~0x40;
    PORTE &= ~0x40;
  }

  // For lighting, we'll switch to output mode.
	L07_DDR  |=  0xFF;
	L8F_DDR  |=  0xF0;
  // Clear the pins we'll be setting first, then set them to the desired output.
  L07_PORT  = (L07_PORT & ~0xFF) |  (OutputData &   0xFF);
  L8F_PORT  = (L8F_PORT & ~0xF0) | ((OutputData & 0x0F00) >> 4);
	// L07_PORT &= ~0xFF;
	// L8F_PORT &= ~0xF0;
	// L07_PORT |= (OutputData & 0xFF);
	// L8F_PORT |= (OutputData & 0x0F00) >> 4;

  // Send a pulse to the latch. This only takes a brief moment of time.
	PORTF |=  0x80;
	asm volatile("nop\n");
	PORTF &= ~0x80;
}
