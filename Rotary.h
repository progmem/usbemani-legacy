#ifndef _ROTARY_H_
#define _ROTARY_H_

/* Private Interface - Used only within this library. */
/** Encoder frequency enumeration. Adjust as needed. */
typedef enum {
	_1kHz = 249,
	_2kHz = 124,
	_4kHz = 62
} ROTARY_FREQ;
/** Encoder connection enumeration. Holds masks for each channel. */
typedef enum {
	ChannelA = 0x00,
	ChannelB = 0x02,
	ChannelC = 0x04,
	ChannelD = 0x06
} ROTARY_CONNECTION;
/** Direction enumeration. Used when calculating legacy output. */
typedef enum {
	Clockwise        = 0x10,
	CounterClockwise = 0x20
} ROTARY_DIRECTION;

typedef enum {
	Default = 0x00,
	Invert  = 0x01
} ROTARY_INVERT;

/** Turntable structure. Holds the state of each encoder. */
typedef struct {
	ROTARY_CONNECTION pin;        // The current connection mask.
	uint8_t           position;   // Reported position of each encoder.
	uint8_t           state;      // Internal state. Holds current and previous states.
	ROTARY_DIRECTION  direction;  // Contains the current direction, for legacy use.
	uint16_t          hold;       // Used to provide a sustained output, for legacy use.
	ROTARY_INVERT     isInverted; // Used to determine if our output needs to be flipped.
} Rotary_t;

/* Function prototypes */
/** Initialize the encoders. This sets up the interrupt. */
void Rotary_Init(ROTARY_FREQ rate);
/** Attaches an encoder for use. */
void Rotary_AttachEncoder(uint8_t encoder, ROTARY_CONNECTION pin);
/** Outputs for direction and position. */
uint8_t Rotary_GetDirection(uint8_t encoder);
uint8_t Rotary_GetPosition (uint8_t encoder);

#endif