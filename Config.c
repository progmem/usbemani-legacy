#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <string.h>
#include "Config.h"
#include "PS2.h"

// Our configuration variable. These will be fail-safe defaults.
Settings_t Settings = {
    {
        /** Rotary settings. **/
        // Invert encoder.
        R_InvertA,
        // Encoder-as-button timeout.
        1000,
        // Encoder tooth count. This is the exact count of teeth, not "teeth - 1" 
        256,
    },
    {
        /** Lights settings. **/
        // Invert lights for turntable.
        L_InvertTT,
        // Light driving method.
        L_Direct, 
        // Assertion timer for USB control. Should be 0.
        0x00,
    },
    {
        /** Device settings. **/
        // Device type. Should be 0, we will pull this on initialization.
        0x00, 
        // Communication method. Either USB-only or USB/PS2 support.
        C_Default, 
        // Assertion timer for PS2 control. Should be 0.
        0x00, 
        // Name to report back via USB. Default returns the type of board.
        N_Default   , 
        // 24-character custom name, wide string.
        "USBemani v2 (Change me!)",
    },
    {
        /** Button settings. **/
        // The current button mapping to use. Used to report the buttons to the PS2 properly.
        B_IIDX,
        // 12-button custom mapping. In use when B_Custom is used.
        {NC,NC,NC,NC,NC,NC,NC,NC,NC,NC,NC,NC,},
    },
};



// All EEPROM functions and related variables will be listed here.
// Before we go any further, we need to have a good idea on where stuff will be going in EEPROM.
// 0x00-07 will be used to verify if the EEPROM has been initialized. That dataspace will store a brief string, containing the word "USB" and a 4-character 'version' code. Remember that the nul-terminator is a character.
#define    EEPROM_HEADER_ADDR   (uint8_t*)0x00
const char EEPROM_HEADER[8] = "USBM573";
// 0x08 and beyond currently store the Settings struct as a byte-for-byte copy.
#define    EEPROM_SETTINGS_ADDR (uint8_t*)0x08

// This command will load the Settings struct from EEPROM.
// It will return 0 if everything went according to plan, and a value if any issues occured.
uint8_t LoadInEEPROM() {
    cli();
    // Before any data is loaded, we run a check against the header stored in EEPROM and the header stored in memory.
    // For debug purposes and to run a thorough scan, we'll use 8 bits, one bit for each character, to flag which bits, if any, are different.
    uint8_t dirty_eeprom = 0;

    for (int i = 0; i < 8; i++) {
        // If the byte is different, flag our bit.
        if ((eeprom_read_byte((uint8_t*)i)) != EEPROM_HEADER[i])
            dirty_eeprom += (1 << i);
    }

    // If the EEPROM is dirty, abort.
    if (dirty_eeprom)
        return dirty_eeprom;
    // If the EEPROM is clean, we'll load the data in.
    else {
        eeprom_read_block(
            &Settings,
            EEPROM_SETTINGS_ADDR,
            sizeof(Settings_t)
        );
        return 0;
    }
    sei();
}

// This command will write all data in the Settings struct to EEPROM.
void Config_SaveEEPROM() {
    cli();
    // The update process will consist of two parts: The header, and the struct.
    // Both are simple commands in the EEPROM library.
    // The first block is the EEPROM block.
    eeprom_update_block(
        EEPROM_HEADER,
        EEPROM_HEADER_ADDR,
        sizeof(EEPROM_HEADER)
    );
    // After that, the settings.
    eeprom_update_block(
        &Settings,
        EEPROM_SETTINGS_ADDR,
        sizeof(Settings_t)
    );
    sei();
}



void Config_Init() {
    // Enable both identifier pins as input with pullups.
    PORTC |=  0xC0;  
    DDRC  &= ~0xC0;
    // A single no-op to deal with some AVR stupidity.
    asm("nop\n");

    // We need to read in our board settings as well from EEPROM.
    // LoadInEEPROM() will return a value and abort if any issues occur with loading. Otherwise, settings will be loaded fine.
    if (LoadInEEPROM()) {
        // If any issues occur, we'll just ignore everything stored in and write new settings.
        Config_SaveEEPROM();
    }

    Config_Identify();

    // We also need to handle the possibility of conflicting settings here.
    // This is a fail-safe, as the configuration tool SHOULD take care of this for us.

    // Lighting methods other than direct-drive are only available if the device is in USB-only mode.
    if (Settings.Device.DeviceComm == C_Default) Settings.Lights.LightsComm = L_Direct;
}

void Config_AddressButton(Settings_Button_t** ptr) { *ptr = &Settings.Button; }
void Config_AddressRotary(Settings_Rotary_t** ptr) { *ptr = &Settings.Rotary; }
void Config_AddressLights(Settings_Lights_t** ptr) { *ptr = &Settings.Lights; }
void Config_AddressDevice(Settings_Device_t** ptr) { *ptr = &Settings.Device; }

void Config_Identify() {
    // For the release of this code, always assume this is a USBemani v2 Home board.
    Settings.Device.DeviceType = HOME;
}

void Config_UpdateSettings(uint8_t conf_command, uint8_t conf_data) {
    if ((conf_command - 0x40) < (uint8_t)sizeof(Settings_t)) {
        uint8_t *ptr = (uint8_t*)&Settings;
         ptr += (conf_command - 0x40);
        *ptr  =  conf_data;
    }
}