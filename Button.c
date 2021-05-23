#include <avr/io.h>
#include <avr/interrupt.h>
#include "Config.h"
#include "Button.h"
 
#define B07_DDR  DDRD
#define B07_PORT PORTD
#define B07_PIN  PIND

#define B8F_DDR  DDRB
#define B8F_PORT PORTB
#define B8F_PIN  PINB

Settings_Lights_t *SettingsLights;
Settings_Device_t *SettingsDevice;

// Function for initializing buttons.
void Button_Init(void) {
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
  // Since we're setting up buttons, we'll setup our initial I/O state.
	B07_DDR  &= ~0xFF;
	B8F_DDR  &= ~0xF0;
	B07_PORT |=  0xFF;
	B8F_PORT |=  0xF0;

	// Since setup is done, we can re-enable interrupts.
	sei();
}

// Function for retrieving button data.
uint16_t Button_GetState(void) {
  // The reference implementation reads from the 12 configured pins.
  // Each pin should have a 10k resistor between the button and the pin.

	// We need some temporary variables to calculate our buttons out.
	uint16_t buf   = 0;
	uint8_t  buf07 = 0;
	uint8_t  buf8F = 0;

  // Capture the existing state of things first, to handle multiplexed lighting.
  uint8_t port07 = B07_PORT;
  uint8_t port8F = B8F_PORT;

  // We'll make sure all of our pins are input w/ pullups.
	B07_DDR  &= ~0xFF;
	B8F_DDR  &= ~0xF0;
	B07_PORT |=  0xFF;
	B8F_PORT |=  0xF0;

  // Wait for things to settle before reading.
  asm volatile("nop\nnop\nnop\nnop\n");
	buf07 	  = ~B07_PIN;
	buf8F     = ~B8F_PIN;

  // Switch back to output for the lighting.
	// B07_PORT  =  port07;
	// B8F_PORT  =  port8F;
	// B07_DDR  |=  0xFF;
	// B8F_DDR  |=  0xF0;

  // Our read is copied to a return buffer.
  // Since we skip PB0-PB3, we need to shift this data around.
	buf |=   buf07;
	buf |= ((buf8F & 0xF0) << 4);
  // Finally, we return the buffer.
	return (buf & 0x0FFF);
}
