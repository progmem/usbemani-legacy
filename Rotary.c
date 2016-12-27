#include <avr/io.h>
#include <avr/interrupt.h>
#include "Rotary.h"
#include "Config.h"

#define R_DDR  DDRF
#define R_PORT PORTF
#define R_PIN  PINF

#define MAX_NUMBER_OF_ENCODERS 2
#define HALF_STEP

// Todo: Per Tau, allow the passing of a custom value to influence the encoder's position.
// Collect a few values from hardware, store those in the board as some defaults while allowing for a custom value.
// We will need to add the necessaary 'known values' in here, so people have some defaults they can use.

/* Rotary lookup. Uncomment out the one needed, as for some reason I can't use the damn #define/#ifdef setup. */
/** Half-step state table (emits a code at 00 and 11) */
#ifdef HALF_STEP
const unsigned char rotary_lookup[6][4] = {
  {0x3 , 0x2, 0x1,  0x0}, {0x23, 0x0, 0x1,  0x0},
  {0x13, 0x2, 0x0,  0x0}, {0x3 , 0x5, 0x4,  0x0},
  {0x3 , 0x3, 0x4, 0x10}, {0x3 , 0x5, 0x3, 0x20},
};
#endif

/** Full-step state table (emits a code at 00 only) */
#ifdef FULL_STEP
const unsigned char rotary_lookup[7][4] = {
{0x0, 0x2, 0x4, 0x0}, {0x3, 0x0, 0x1, 0x10},
{0x3, 0x2, 0x0, 0x0}, {0x3, 0x2, 0x1, 0x0 },
{0x6, 0x0, 0x4, 0x0}, {0x6, 0x5, 0x0, 0x20},
{0x6, 0x5, 0x4, 0x0},
};
#endif

/* Our encoders. */
Rotary_t Rotary[MAX_NUMBER_OF_ENCODERS];

/* Our pointer to the settings in Config. */
Settings_Rotary_t *SettingsRotary;
uint16_t HoldTime = 4000;

/* Internal rotary processing command. This will grab the current state and determine if a change occured. */
uint8_t RotaryProcess(uint8_t encoder) {
	unsigned char pinState = ((R_PIN >> Rotary[encoder].pin) & 0x03);
	Rotary[encoder].state = rotary_lookup[Rotary[encoder].state & 0xf][pinState];
	
	return (Rotary[encoder].state & 0x30);
}

/* Initialize the rotary encoders. */
void Rotary_Init(ROTARY_FREQ rate) {
	// Start by disabling interrupts as a whole. We re-enable them at the end.
	cli();

	// Before we configure our interrupt, we need to load in our settings.
	Config_AddressRotary(&SettingsRotary);
	HoldTime = SettingsRotary->RotaryHold;

	// Clear both registers.
	TCCR0A  = 0;
	TCCR0B  = 0;
	// We'll reset the counter, just in case.
	TCNT0   = 0;
	// We then set our matching rate:
	// At 1kHz, we need to match every 250 counts. (249)
	// At 2kHz, that decreases down to 125 counts. (124)
	// At 4Khz, we have to round a bit, 62.5.      (62, slightly slower) 
	OCR0A   = rate;
	// We need to be in CTC (Clear Timer on Compare) mode.
	TCCR0A |= (1 << WGM01);
	// We also need a 64 prescaler for polling.
	TCCR0B |= (1 << CS01) | (1 << CS00);   
	// After that, we enable this interrupt.
	TIMSK0 |= (1 << OCIE0A);
	sei();
}

/* Attach encoders for use. */
void Rotary_AttachEncoder(uint8_t encoder, ROTARY_CONNECTION pin) {
	// We need to store the pin connection into this specific encoder.
	Rotary[encoder].pin = pin;
	R_DDR  &= ~(0x03 << pin);
	R_PORT |=  (0x03 << pin);


	switch(encoder) {
		case 0:
			if (SettingsRotary->RotaryInvert & R_InvertA) 
				 Rotary[encoder].isInverted = 1;
			else Rotary[encoder].isInverted = 0;
			break;
		case 1:
			if (SettingsRotary->RotaryInvert & R_InvertC) 
				 Rotary[encoder].isInverted = 1;
			else Rotary[encoder].isInverted = 0;
			break;
	}
}

/* Grab the current direction of motion. */
uint8_t Rotary_GetDirection(uint8_t encoder) {
	// As long as we have an output, check if we need to invert it.
	if (Rotary[encoder].direction) {
		if (Rotary[encoder].isInverted) return Rotary[encoder].direction  * -1;
		else                            return Rotary[encoder].direction;
	}
	else return 0;
}

/* Grab the current position of the encoder. */
uint8_t Rotary_GetPosition(uint8_t encoder) {
	if (Rotary[encoder].isInverted) return Rotary[encoder].position ^ 0xFF;
	else                            return Rotary[encoder].position;
}

/* The interrupt that is executed, based on the defined rate. */
ISR(TIMER0_COMPA_vect) {
	// We need to iterate through each encoder.
	for (int i = 0; i < MAX_NUMBER_OF_ENCODERS; i++) {
		// For each encoder, we'll update the current position and direction, along with the hold time for legacy use.
		uint8_t result = RotaryProcess(i);
		if (result) {
	     	// We pass the tooth count into our position function. We start with it as a base, add the current position, plus or minus direction, and mod the whole thing against the tooth count.
    		Rotary[i].position  = 
    			(
    				SettingsRotary->RotaryPPR + (
    					result == CounterClockwise ?
    						Rotary[i].position - 1 : 
    						Rotary[i].position + 1
    				)
    			) % SettingsRotary->RotaryPPR;
    			
		    Rotary[i].direction = (result == CounterClockwise ? -1 : 1);
    		Rotary[i].hold      = HoldTime;
		}
		// If there hasn't been a change in state, we start counting down the hold time.
		else {
			Rotary[i].hold      = (Rotary[i].hold ? Rotary[i].hold - 1 : 0);
		}
		if (Rotary[i].hold == 0)   Rotary[i].direction = 0;
	}
}