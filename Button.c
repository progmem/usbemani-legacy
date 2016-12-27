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

void Button_Init(void) {
	// We'll turn off interrupts temporarily during setup procedures.
	cli();

	// Because the button layout is dependent on button and board settings, we address both.
	// We also address the lights, as we need to know if the lighting data is PC-controlled.
	Config_AddressLights(&SettingsLights);
	Config_AddressDevice(&SettingsDevice);
	
	B07_DDR  &= ~0xFF;
	B07_PORT |=  0xFF;
	
	B8F_DDR  &= ~0xF0;
	B8F_PORT |=  0xF0;

	// Since setup is done, we can re-enable interrupts.
	sei();
}

uint16_t Button_GetState(uint8_t useLights, uint16_t LightsData) {
	// We need some temporary variables.
	uint16_t buf = 0;
	uint8_t  buf07 = 0;
	uint8_t  buf8F = 0;	

	// Simply read into the buffers...
	B07_DDR  &= ~0xFF;
	B07_PORT |=  0xFF;
	buf07 	  = ~B07_PIN;

	B07_DDR  &= ~0xF0;
	B07_PORT |=  0xF0;
	buf8F     = ~B8F_PIN;

	// ...and output them to the main buffer.
	// Keep in mind that on the home board, we've already set up our pins with PS2/WS28XX lighting in mind.
	buf  =   buf07;
	buf += ((buf8F & 0xF0) << 4);

	// We can finally return our data.
	return (buf & 0x0FFF);
}
