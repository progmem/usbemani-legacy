#define _WIN32_WINNT 0x0501
#define _WIN32_IE    0x0600

#include <stdio.h>
#include <stdint.h>
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <commctrl.h>

#include "AVRStruct.h"

#include "Bootloader.h"

Settings_t Settings = {
    {
        /** Rotary settings. **/
        //// Invert encoder.
        R_InvertA,
        //// Encoder-as-button timeout.
        1000,
        //// Encoder tooth count. This is the exact count of teeth, not "teeth - 1" 
        256,
    },
    {
        /** Lights settings. **/
        //// Invert lights for turntable.
        L_InvertTT,
        //// Light driving method.
        L_Direct, 
        //// Assertion timer for USB control. Should be 0.
        0x00,
    },
    {
        /** Device settings. **/
        //// Device type. Should be 0, we will pull this on initialization.
        0x00, 
        //// Communication method. Either USB-only or USB/PS2 support.
        C_Default, 
        //// Assertion timer for PS2 control. Should be 0.
        0x00, 
        //// Name to report back via USB. Default returns the type of board.
        N_Default   , 
        //// 24-character custom name, wide string.
        "USBemani v2 (change me!)",
    },
    {
        /** Button settings. **/
        //// The current button mapping to use. Used to report the buttons to the PS2 properly.
        B_IIDX,
        //// 12-button custom mapping. In use when B_Custom is used.
        {NC,NC,NC,NC,NC,NC,NC,NC,NC,NC,NC,NC,},
    },
};

const char *filename = NULL;

#define ERROR_0001 "An error occured in passing the filename over to the firmware updater. "
#define ERROR_0002 "An error occured while attempting to read the firmware."
#define ERROR_0003 "Unable to locate a USBemani connected to the machine. Please verify that the USBemani is connected and operational."
#define ERROR_0004 "Unable to locate a USBemani in Update mode.\nIf this is the first time updating the USBemani, Windows may be in the process of installing the Update Mode driver. Please wait a moment before trying again."
#define ERROR_0005 "An error occured while attempting to read the firmware. It may have been corrupted or relocated."
#define ERROR_0006 "An error occured during the update procedure. Please try again."

char DeviceType = 0;
BOOL PS2Enabled;
BOOL CustomMappingSelected;



void UpdateFirmware(void);
int  LoadSettings(void);
int  SaveSettings(void);
void BuildConfig(HWND hDlg);
void UpdateSettings(void);
void InitComboBox(HWND hDlg, int combobox, int first_elem, int count, int pos, int en);

BOOL CALLBACK AboutDlgProc(HWND hWndDlg, UINT Message, WPARAM wParam, LPARAM lParam)
{    
    switch(Message)
    {
        case WM_INITDIALOG:
        {   
            // This temp value will be used with switch/case to determine what value to set items to.
            uint16_t TempData;

            // Rotary
            //// Initialize the Invert Channel checkboxes.
            CheckDlgButton(
                hWndDlg,
                ROTARY_INVERTA,
                (Settings.Rotary.RotaryInvert & 0x03 ? BST_CHECKED : BST_UNCHECKED)
            );

            CheckDlgButton(
                hWndDlg,
                ROTARY_INVERTB,
                (Settings.Rotary.RotaryInvert & 0x30 ? BST_CHECKED : BST_UNCHECKED)
            );
            //// Initialize the Turntable Button Hold combobox.
            switch(Settings.Rotary.RotaryHold) {
                case 1:
                    TempData = 0;
                    break;
                case 400:
                    TempData = 1;
                    break;
                case 800:
                    TempData = 2;
                    break;
                case 1600:
                    TempData = 3;
                    break;
                default:
                    TempData = 2;
            }
            InitComboBox(
                hWndDlg,
                ROTARY_TTHOLDCOUNT,
                ROTARY_HOLD1,
                4,
                TempData,
                1
            );
            //// Initialize the Turntable Scaling combobox.
            switch(Settings.Rotary.RotaryPPR) {
                case 256:
                {
                    TempData = 0;
                    break;
                }
                case 100:
                {
                    TempData = 1;
                    break;
                }
                case 200:
                {
                    TempData = 2;
                    break;
                }
                case 144:
                {
                    TempData = 3;
                    break;
                }
                default:
                {
                    TempData = 0;
                }
            }
            InitComboBox(
                hWndDlg,
                ROTARY_PULSECOUNT,
                ROTARY_NOSCALE,
                4,
                TempData,
                1
            );

            // Lighting
            //// Initializes the Turntable Lighting checkbox.
            if (!(DeviceType == USBEMANI_PID_KOC))
            {
                CheckDlgButton(
                    hWndDlg,
                    LIGHTS_INVERTTT,
                    ((Settings.Lights.LightsInvert) ? BST_CHECKED : BST_UNCHECKED)
                );
            }
            else
            {
                EnableWindow(
                    GetDlgItem(hWndDlg, LIGHTS_INVERTTT),
                    0
                );
            }
            //// Initialize the Lighting Method combobox.
            InitComboBox(
                hWndDlg,
                LIGHTS_COMM, 
                LIGHTS_DIRECT, 
                3,
                (DeviceType == USBEMANI_PID_KOC ? 0 : Settings.Lights.LightsComm),
                (DeviceType == USBEMANI_PID_KOC ? 0 : 1)
            );

            // Device
            //// Initialize the Device Naming Scheme combobox.
            InitComboBox(
                hWndDlg, 
                DEVICE_DEVICENAME,
                DEVICE_NAMEDEF,
                4,
                (Settings.Device.DeviceName == 0xFF ? 3 : Settings.Device.DeviceName),
                1
            );

            // Button
            //// Initialize the PS2 Support Mode checkbox
            if (!Settings.Device.DeviceComm) {
                CheckDlgButton(
                    hWndDlg,
                    BUTTON_PS2ENABLE,
                    BST_CHECKED
                );
            }

            //// Initialize the PS2 Support Mode combobox
            switch(Settings.Button.ButtonMap) {
                case 0x00:
                    TempData = 0;
                    break;
                case 0xFF:
                    TempData = 6;
                    break;
                default:
                    TempData = Settings.Button.ButtonMap - 1;
            }
            InitComboBox(
                hWndDlg,
                BUTTON_MAPTYPE,
                BUTTON_MAPIIDX,
                7,
                TempData,
                (!Settings.Device.DeviceComm ? 0x00 : 0x01)
            );

            //// Initialize 12 comboboxes for Custom Mapping.
            int i;
            for (i = 0; i < 12; i++) {
                InitComboBox(
                    hWndDlg,
                    BUTTON_CUSTOM1 + i,
                    BUTTON_SELECT,
                    17,
                    Settings.Button.CustomMap[i],
                    0
                );
            }

            //// Initialize the Custom Name edit field.
            EnableWindow(
                GetDlgItem(hWndDlg, DEVICE_CUSTOMNAME),
                (Settings.Device.DeviceName == 0xFF ? 1 : 0)
            );
            Edit_SetText(
                GetDlgItem(hWndDlg, DEVICE_CUSTOMNAME),
                Settings.Device.CustomName
            );

            return TRUE;
        }

        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                //// Buttons
                case BUTTON_FIRMWARE:
                {
                    UpdateFirmware();
                    break;
                }

                case BUTTON_APPLYSET:
                {
                    BuildConfig(hWndDlg);
                    UpdateSettings();
                    if(!Device_SendCommand(0xF0, 0x00)) MessageBox(NULL, ERROR_0003, "Error Locating USBemani",         MB_OK | MB_ICONEXCLAMATION);
                    break;
                }

                case BUTTON_STORESET:
                {
                    BuildConfig(hWndDlg);
                    UpdateSettings();
                    if(!Device_SendCommand(0xF1, 0x00)) MessageBox(NULL, ERROR_0003, "Error Locating USBemani",         MB_OK | MB_ICONEXCLAMATION);
                    break;
                }

                //// Close button
                case WM_DESTROY:
                    BuildConfig(hWndDlg);
                    EndDialog(hWndDlg, 0);
            	break;
            }
            BuildConfig(hWndDlg);

            if (Settings.Button.ButtonMap == B_Custom)
                 CustomMappingSelected = TRUE;
            else CustomMappingSelected = FALSE;

            if (Settings.Device.DeviceName == N_Custom)
                 EnableWindow(GetDlgItem(hWndDlg,DEVICE_CUSTOMNAME), 1);
            else EnableWindow(GetDlgItem(hWndDlg,DEVICE_CUSTOMNAME), 0);

            int i = 0;
            if (Settings.Device.DeviceComm == C_Default) {
    			EnableWindow(GetDlgItem(hWndDlg,BUTTON_MAPTYPE), 1);
                for (i = 0; i < 12; i++) 
                EnableWindow(GetDlgItem(hWndDlg,BUTTON_CUSTOM1 + i), (CustomMappingSelected ? 1 : 0)); 

                EnableWindow(GetDlgItem(hWndDlg,LIGHTS_COMM), 0);
                SendDlgItemMessage(hWndDlg, LIGHTS_COMM, CB_SETCURSEL, 0, 0);
            }
            else {
                EnableWindow(GetDlgItem(hWndDlg,BUTTON_MAPTYPE), 0);

                for (i = 0; i < 12; i++) 
                EnableWindow(GetDlgItem(hWndDlg,BUTTON_CUSTOM1 + i), 0); 

                EnableWindow(GetDlgItem(hWndDlg,LIGHTS_COMM), (DeviceType & USBEMANI_PID_KOC ? 0 : 1));
            }

            break;  
        }
        default:
            return FALSE;
    }

    return TRUE;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE h0, LPSTR lpCmdLine, int nCmdShow)
{	
	//// Initialise common controls.
	INITCOMMONCONTROLSEX icc;
	icc.dwSize = sizeof(icc);
	icc.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&icc);

    DeviceType = Device_DetectBoard();

    if (DeviceType) {
        if (LoadSettings())
            DialogBox(hInst, MAKEINTRESOURCE(IDD_DLGFIRST), NULL, AboutDlgProc);
    }
    else {
        MessageBox(NULL, ERROR_0003, "Error Locating USBemani", MB_OK | MB_ICONEXCLAMATION);    
    }

    SaveSettings();

    return 0;
}

int LoadSettings() {
    HANDLE hFile;
    DWORD BytesReadOrWritten;

    hFile = CreateFile(
        "USBemani.settings",           // name of the write
        GENERIC_WRITE | GENERIC_READ,  // open for writing
        0,                             // do not share
        NULL,                          // default security
        OPEN_ALWAYS,                   // create new file only
        FILE_ATTRIBUTE_NORMAL,         // normal file
        NULL                           // no attr. template
    );

    printf("%ld\n", GetLastError());

    if (hFile == INVALID_HANDLE_VALUE) {
         MessageBox(NULL, "Unable to create USBemani.settings", "Cannot Read File", MB_OK | MB_ICONEXCLAMATION);
         CloseHandle(hFile);
         return 0;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (!ReadFile(hFile, &Settings, sizeof(Settings_t), &BytesReadOrWritten, NULL)) {
            MessageBox(NULL, "Unable to read USBemani.settings", "Cannot Read File", MB_OK | MB_ICONEXCLAMATION);
            printf("%ld\n", GetLastError());
            CloseHandle(hFile);
            return 0;
        }
        else {
            CloseHandle(hFile);
        }
    }
    else {
        CloseHandle(hFile);
    }
    return 1;
}

int SaveSettings() {
    HANDLE hFile;
    DWORD BytesReadOrWritten;

    hFile = CreateFile(
        "USBemani.settings",           // name of the write
        GENERIC_WRITE | GENERIC_READ,  // open for writing
        0,                             // do not share
        NULL,                          // default security
        OPEN_ALWAYS,                   // create new file only
        FILE_ATTRIBUTE_NORMAL,         // normal file
        NULL                           // no attr. template
    );

    printf("%ld\n", GetLastError());

    if (hFile == INVALID_HANDLE_VALUE) {
         MessageBox(NULL, "Unable to locate USBemani.settings", "Cannot Read File", MB_OK | MB_ICONEXCLAMATION);
         CloseHandle(hFile);
         return 0;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (!WriteFile(hFile, (uint8_t*)&Settings, sizeof(Settings_t), &BytesReadOrWritten, NULL)) {
            MessageBox(NULL, "Unable to write USBemani.settings", "Cannot Write File", MB_OK | MB_ICONEXCLAMATION);
            printf("%ld\n", GetLastError());
            CloseHandle(hFile);
            return 0;
        }
        else {
            CloseHandle(hFile);
        }
    }
    else {
        if (!WriteFile(hFile, (uint8_t*)&Settings, sizeof(Settings_t), &BytesReadOrWritten, NULL)) {
            MessageBox(NULL, "Unable to write USBemani.settings", "Cannot Write File", MB_OK | MB_ICONEXCLAMATION);
            printf("%ld\n", GetLastError());
            CloseHandle(hFile);
            return 0;
        }
        CloseHandle(hFile);
        return 0;
    }
    return 1;
}

void BuildConfig(HWND hDlg) {
    uint8_t i = 0;
    int nSelected = 0;

    //// Begin Building Config
    ////// Rotary Settings
    //////// Rotary Inversion
    if (IsDlgButtonChecked(hDlg, ROTARY_INVERTA))
         Settings.Rotary.RotaryInvert |=  0x03;
    else Settings.Rotary.RotaryInvert &= ~0x03;

    if (IsDlgButtonChecked(hDlg, ROTARY_INVERTB))
         Settings.Rotary.RotaryInvert |=  0x30;
    else Settings.Rotary.RotaryInvert &= ~0x30;
    //////// Rotary Hold
    nSelected = (int)SendDlgItemMessage(hDlg, ROTARY_TTHOLDCOUNT, CB_GETCURSEL, 0, 0);
    switch(nSelected) {
        case 0:
        {
            Settings.Rotary.RotaryHold = 1;
            break;
        }
        case 1:
        {
            Settings.Rotary.RotaryHold = 400;
            break;
        }
        case 2:
        {
            Settings.Rotary.RotaryHold = 800;
            break;
        }
        case 3:
        {
            Settings.Rotary.RotaryHold = 1600;
            break;
        }
    }
    //////// Rotary PPR
    nSelected = (int)SendDlgItemMessage(hDlg, ROTARY_PULSECOUNT, CB_GETCURSEL, 0, 0);
    switch(nSelected) {
        case 0:
        {
            Settings.Rotary.RotaryPPR = 256;
            break;
        }
        case 1:
        {
            Settings.Rotary.RotaryPPR = 100;
            break;
        }
        case 2:
        {
            Settings.Rotary.RotaryPPR = 200;
            break;
        }
        case 3:
        {
            //// Todo: Check this? I need a tooth count on this one.
            Settings.Rotary.RotaryPPR = 144;
            break;
        }
    }

    ////// Lights Settings
    //////// Lighting Invert
    if (IsDlgButtonChecked(hDlg, LIGHTS_INVERTTT))
         Settings.Lights.LightsInvert = L_InvertTT;
    else Settings.Lights.LightsInvert = L_NoInvert;
    //////// Lighting Method
    Settings.Lights.LightsComm = (int)SendDlgItemMessage(hDlg, LIGHTS_COMM, CB_GETCURSEL, 0, 0);

    ////// Device Settings
    //////// Device Support (USB/PS2)
    if (IsDlgButtonChecked(hDlg, BUTTON_PS2ENABLE))
         Settings.Device.DeviceComm = C_Default;
    else Settings.Device.DeviceComm = C_USBOnly; 
    //////// Device Name
    nSelected = (int)SendDlgItemMessage(hDlg, DEVICE_DEVICENAME, CB_GETCURSEL, 0, 0);
    switch(nSelected) {
        case 3:
        {
            Settings.Device.DeviceName = 0xFF;
            if (!GetDlgItemText(hDlg, DEVICE_CUSTOMNAME, Settings.Device.CustomName, 25))
                 sprintf(Settings.Device.CustomName, "USBemani v2 (change me!)");  
            break;
        }
        default:
        {
            Settings.Device.DeviceName = nSelected;
        }
    }   

    ////// Button Settings
    //////// Button Mapping & Custom Mapping
    nSelected = (int)SendDlgItemMessage(hDlg, BUTTON_MAPTYPE, CB_GETCURSEL, 0, 0);
    switch(nSelected) {
        case 6:
        {
            Settings.Button.ButtonMap = B_Custom;
            for (i = 0; i < 12; i++) {
                Settings.Button.CustomMap[i] = (int)SendDlgItemMessage(hDlg, (BUTTON_CUSTOM1 + i), CB_GETCURSEL, 0, 0);
            }
            break;
        }         
        default:
        {
            Settings.Button.ButtonMap = nSelected + 1;
        }               
    }

    //// End Building Config
}

void UpdateSettings() {
    int i = 0;
    uint8_t *ptr = NULL;

    for (i = 0; i < sizeof(Settings_t); i++) {
        ptr = ((uint8_t*)&Settings + i);
        printf("%4d %4X\n", i, (unsigned int)*ptr);
    }

    for (i = 0; i < sizeof(Settings_t); i++) {
        ptr = ((uint8_t*)&Settings + i);
        if(!Device_SendCommand((i + 0x40), *ptr)) {
            i = 200;
        }
    }
}

void UpdateFirmware() {
	OPENFILENAME ofn;
	char szFileName[2048] = "";

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFile = szFileName;
	//// Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
	//// use the contents of szFile to initialize itself.
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = sizeof(szFileName);
	ofn.lpstrFilter = "USBemani.hex\0USBemani.hex\0All firmware (*.hex)\0*.HEX\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn))
	{
		switch(Device_UpdateFirmware(szFileName)) {
			case 1:
				MessageBox(NULL, ERROR_0001, "Filename Error",					MB_OK | MB_ICONEXCLAMATION);
			break;
			case 2:
				MessageBox(NULL, ERROR_0002, "Error Reading Firmware", 			MB_OK | MB_ICONEXCLAMATION);
			break;
			case 3:
				MessageBox(NULL, ERROR_0003, "Error Locating USBemani", 		MB_OK | MB_ICONEXCLAMATION);
			break;
			case 4:
				MessageBox(NULL, ERROR_0004, "Error Resetting USBemani",		MB_OK | MB_ICONEXCLAMATION);
			break;
			case 5:
				MessageBox(NULL, ERROR_0005, "Error Reading Firmware",			MB_OK | MB_ICONEXCLAMATION);
			break;
			case 6:
				MessageBox(NULL, ERROR_0006, "Error During Update",				MB_OK | MB_ICONEXCLAMATION);
			break;
			default:
				MessageBox(NULL, "The USBemani has been updated.", "Completed", MB_OK);
			//break;
		}
	}
}



void InitComboBox(HWND hDlg, int combobox, int first_elem, int count, int pos, int en) {
    CHAR achTemp[256];
    DWORD dwIndex;

    HWND hwndComboBox = GetDlgItem(hDlg, combobox);

    int i;
    for (i = first_elem; i < first_elem + count; i++) {
        LoadString(NULL, i, achTemp, sizeof(achTemp)/sizeof(TCHAR));
        dwIndex = SendMessage(hwndComboBox, CB_ADDSTRING, 0, (LPARAM)achTemp);
        SendMessage(hwndComboBox, CB_SETITEMDATA, dwIndex, 0);
    }

    SendDlgItemMessage(hDlg, combobox, CB_SETCURSEL, (pos >= count ? count - 1: pos), 0);

    EnableWindow(GetDlgItem(hDlg, combobox), (en ? 1 : 0));
}