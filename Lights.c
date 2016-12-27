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

// Code to handle the lights are not available in this release, as they were a USBemani v2 Arcade thing.
// The v2 Arcade only saw the light of day in the form of 3 prototype units.
void Lights_Init(void) {
	Config_AddressLights(&SettingsLights);
	Config_AddressDevice(&SettingsDevice);		
}

void Lights_StoreState(uint16_t OutputData) {	
}

uint16_t Lights_RetrieveState(void) {
	return 0;
}

