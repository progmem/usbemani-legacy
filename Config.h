#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdlib.h>

/* Enumerations for device configuration. */
/** Controller type. Used in PS2 mode to determine how to transform our raw input. Stored in EEPROM and loaded at startup. */
typedef enum {
    B_Direct = 0x00,
    B_IIDX   = 0x01,
    B_IIDXUS = 0x02,
    B_IIDXJP = 0x03,
    B_POPN   = 0x04,
    B_DDR    = 0x05,
    B_GFDM   = 0x06,
    B_Custom = 0xFF
} BUTTON_TRANSFORM;

/** Rotary inversion. Used to invert the output of our encoders. Stored in EEPROM and loaded at startup. */
typedef enum {
    R_InvertA = 0x03,
    R_InvertB = 0x0C,
    R_InvertC = 0x30,
    R_InvertD = 0xC0
} ROTARY_TRANSFORM;

/** Light inversion. Used to invert the output of our lights. Stored in EEPROM and loaded at startup. */
typedef enum {
    L_NoInvert = 0x00,
    L_InvertTT = 0x01,
} LIGHTS_TRANSFORM;
/** Light connection. Used to define other methods of handling LEDs, such as WS2811. */
typedef enum {
    L_Direct = 0x00,
    L_WS28XX = 0x01,
    L_OWLED  = 0x02
} LIGHTS_COMM;

/** Board type. Identifies device as a Home or Arcade board. */
typedef enum {
    V1     = 0xC0,
    HOME   = 0x40,
    ARCADE = 0x80
} DEVICE_TYPE;
/** Connection type. This is written on initialization, after EEPROM load. */
typedef enum {
    C_Default = 0x00, 
    C_USBOnly = 0x01
} DEVICE_COMM;
/** Device name when connected to the PC. Determines if the default name, player sides, or a custom name will be used. */
typedef enum {
    N_Default = 0x00,
    N_P1      = 0x01,
    N_P2      = 0x02,
    N_Custom  = 0xFF
} DEVICE_NAME;

/* Structures for the various board functions. These store the various settings needed by other libraries. */

/** Rotary structure. Holds the rotary inversion status and hold time. */
typedef struct {
    // Todo: Per Tau, allow the passing of a custom value to influence the encoder's position.
    // Collect a few values from hardware, store those in the board as some defaults while allowing for a custom value.
    // We will need to add both an enum and a custom teeth count value in.
    ROTARY_TRANSFORM  RotaryInvert;
    uint16_t          RotaryHold;
    uint16_t          RotaryPPR;        
} Settings_Rotary_t;
/** Lights structure. Holds the turntable inversion, the communication method, and if lighting is being controlled via USB. */
typedef struct {
    LIGHTS_TRANSFORM  LightsInvertTT;
    LIGHTS_COMM       LightsComm;
    volatile uint16_t LightsAssert;
} Settings_Lights_t;
/** Device structure. Holds the board type, the communication method, the PS2 assertioon timer, the name to report back, and a 24-character custom name. */
typedef struct {
    DEVICE_TYPE       DeviceType;
    DEVICE_COMM       DeviceComm;
    volatile uint16_t PS2Assert;
    DEVICE_NAME       DeviceName;
    char              CustomName[25];
} Settings_Device_t;
/** Button structure. Holds the current mapping and the custom mapping. */
typedef struct {
    BUTTON_TRANSFORM  ButtonMap;          
    uint8_t           CustomMap[12];    
} Settings_Button_t;
/** Structure structure. Holds all of the structures in a compact, easy-to-write-to-EEPROM formfactor. */
typedef struct {
    Settings_Rotary_t Rotary;
    Settings_Lights_t Lights;
    Settings_Device_t Device;
    Settings_Button_t Button;
} Settings_t;

// Access functions. Each of these will take in a pointer and point it to the right part of the Settings struct.
void Config_Init(void);
void Config_Identify(void); 

void Config_AddressButton(Settings_Button_t** ptr);
void Config_AddressRotary(Settings_Rotary_t** ptr);
void Config_AddressLights(Settings_Lights_t** ptr);
void Config_AddressDevice(Settings_Device_t** ptr);

uint8_t LoadInEEPROM(void);
void    UpdateEEPROM(void);

void Config_UpdateSettings(uint8_t conf_command, uint8_t conf_data);
void Config_SaveEEPROM(void);

#endif