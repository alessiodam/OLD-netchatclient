/*
 *--------------------------------------
 * Program Name: TI-84 Plus CE Net Client (Calculator)
 * Author: TKB Studios
 * License:
 * Description: Allows the user to communicate with the TI-84 Plus CE Net servers
 *--------------------------------------
*/

/* When contributing, please add your username in the list here */
/*
 *--------------Contributors--------------
 * TIny_Hacker
 * ACagliano (Anthony Cagliano)
 *--------------------------------------
*/


#include <string.h>
#include <sys/lcd.h>
#include <keypadc.h>
#include <graphx.h>
#include <fileioc.h>
#include <stdio.h>
#include <ti/getcsc.h>
#include <compression.h>
#include <srldrvce.h>
#include <tice.h>
#include <debug.h>

/* Include the converted graphics file */
#include "gfx/gfx.h"

/* DEFINE SETTINGS APPVAR */
char *settingsappv = "TI84Sett";
char *TEMP_PROGRAM = "_";
char *MAIN_PROGRAM = "TINET";

/* DEFINE USER */
bool keyfile_available = false;
char *username;
char *authkey;
uint8_t appvar;

/* READ BUFFERS */
size_t read_flen ;
uint8_t *ptr;
char in_buffer[64];

/* DEFINE SPRITES */
gfx_sprite_t *login_qrcode_sprite;
gfx_sprite_t *usb_connected_sprite;
gfx_sprite_t *usb_disconnected_sprite;
gfx_sprite_t *connecting_sprite;

/* DEFINE FUNCTIONS */
void GFXspritesInit();
void GFXsettings();
void writeKeyFile();
void ConnectingGFX();
void NoKeyFileGFX();
void KeyFileAvailableGFX();
void FreeMemory();
void quitProgram();
void ConnectSerial(char *token_msg);
void login();
void readSRL();
void sendSerialInitData();
void getCurrentTime();
void printServerPing();
void ConnectSerial(char *message);

/* DEFINE CONNECTION VARS */
bool USB_connected = false;
bool USB_connecting = false;
bool bridge_connected = false;
bool internet_connected = false;
bool srl_busy = false;
srl_device_t srl;
bool has_srl_device = false;
uint8_t srl_buf[512];
bool serial_init_data_sent = false;

/* DEFINE GUI ITERATIONS AND UPDATE FUNCTION */
typedef enum {
    STATE_LOGIN_SCREEN,
    STATE_NO_KEY_FILE,
    STATE_KEY_FILE_AVAILABLE,
    STATE_CONNECTING,
    STATE_BRIDGE_CONNECTED,
    STATE_INTERNET_CONNECTED,
    STATE_HAS_SRL_DEVICE,
    STATE_NO_SRL_DEVICE,
    STATE_DASHBOARD,
} program_state_t;
program_state_t current_state, previous_state;
void update_UI(program_state_t current_state, program_state_t previous_state);

/* DEFINE KEY STATES AND FUNCTIONS */
typedef void (*State)(void);
State key_state;
void state_no_srl_device();
void state_has_srl_device();
void state_no_key_file();
void state_key_file_available();
void state_connecting();
void state_dashboard();


static usb_error_t handle_usb_event(usb_event_t event, void *event_data, usb_callback_data_t *callback_data __attribute__((unused)))
{
    usb_error_t err;
    /* Delegate to srl USB callback */
    if ((err = srl_UsbEventCallback(event, event_data, callback_data)) != USB_SUCCESS)
        return err;
    /* Enable newly connected devices */
    if(event == USB_DEVICE_CONNECTED_EVENT && !(usb_GetRole() & USB_ROLE_DEVICE)) {
        usb_device_t device = event_data;
        USB_connected = true;
        usb_ResetDevice(device);
    }

    /* Call srl_Open on newly enabled device, if there is not currently a serial device in use */
    if(event == USB_HOST_CONFIGURE_EVENT || (event == USB_DEVICE_ENABLED_EVENT && !(usb_GetRole() & USB_ROLE_DEVICE))) {

        /* If we already have a serial device, ignore the new one */
        if(has_srl_device) return USB_SUCCESS;

        usb_device_t device;
        if(event == USB_HOST_CONFIGURE_EVENT) {
            /* Use the device representing the USB host. */
            device = usb_FindDevice(NULL, NULL, USB_SKIP_HUBS);
            if(device == NULL) return USB_SUCCESS;
        } else {
            /* Use the newly enabled device */
            device = event_data;
        }

        /* Initialize the serial library with the newly attached device */
        srl_error_t error = srl_Open(&srl, device, srl_buf, sizeof srl_buf, SRL_INTERFACE_ANY, 9600);
        if(error) {
            /* Print the error code to the homescreen */
            gfx_End();
            os_ClrHome();
            printf("Error %d initting serial\n", error);
            while(os_GetCSC());
            return 0;
            return USB_SUCCESS;
        }
        has_srl_device = true;
    }

    if(event == USB_DEVICE_DISCONNECTED_EVENT) {
        usb_device_t device = event_data;
        if(device == srl.dev) {
            USB_connected = false;
            srl_Close(&srl);
            has_srl_device = false;
        }
    }

    return USB_SUCCESS;
}

int main(void)
{
    /* SET GFX SPRITES */
    GFXspritesInit();

    /* GFX BEGIN */
    gfx_Begin();
    gfx_SetPalette(global_palette, sizeof_global_palette, 0);

    /* GFX SETTINGS */
    GFXsettings();

    key_state = state_no_key_file;

    /* MAIN MENU */
    gfx_SetTextScale(2, 2);
    gfx_PrintStringXY("TI-84 Plus CE Net", ((GFX_LCD_WIDTH - gfx_GetStringWidth("TI-84 Plus CE Net")) / 2), 5);
    gfx_SetTextFGColor(224);
    gfx_PrintStringXY("Press [clear] to quit.", ((GFX_LCD_WIDTH - gfx_GetStringWidth("Press [clear] to quit.")) / 2), 35);
    gfx_SetTextFGColor(255);
    gfx_SetTextScale(1, 1);

    appvar = ti_Open("NetKey", "r");

    if (appvar == 0)
    {
        keyfile_available = false;
        current_state = STATE_NO_KEY_FILE;
        key_state = state_no_key_file;
        update_UI(current_state, previous_state);
        previous_state = current_state;
    }
    else
    {
        read_flen = ti_GetSize(appvar);
        uint8_t *data_ptr = (uint8_t *)ti_GetDataPtr(appvar);
        ti_Close(appvar);

        char *read_username = (char *)data_ptr;
        username = read_username;
        dbg_printf("User: %s\n", username);

        size_t read_un_len = strlen(read_username) + 1;
        data_ptr += (read_un_len + 1);
        read_flen -= (read_un_len + 1);

        char *read_key = (char *)data_ptr - 1;
        size_t key_len = strlen(read_key);
        authkey = read_key;
        dbg_printf("Token: %s\n", authkey);

        for (size_t i = 0; i < key_len; i++)
            dbg_printf("%02x\n", data_ptr[i]);

        keyfile_available = true;
        KeyFileAvailableGFX();
    }

    const usb_standard_descriptors_t *usb_desc = srl_GetCDCStandardDescriptors();
    /* Initialize the USB driver with our event handler and the serial device descriptors */
    usb_error_t usb_error = usb_Init(handle_usb_event, NULL, usb_desc, USB_DEFAULT_INIT_FLAGS);
    if(usb_error) {
       dbg_printf("usb init error %u\n", usb_error);
       return 1;
    }

    /* Loop until [2nd] is pressed */
    do
    {
        /* Update kb_Data */
        kb_Scan();

        if (kb_Data[6] == kb_Clear)
        {
            quitProgram();
        }

        /* Handle new USB events */
        usb_HandleEvents();
        if (has_srl_device)
        {
            readSRL();
        }

        if (has_srl_device && bridge_connected && !serial_init_data_sent)
        {
            sendSerialInitData();
        }

        /* Draw the USB sprites */
        if(has_srl_device)
        {
            current_state = STATE_HAS_SRL_DEVICE;
            key_state = state_has_srl_device;
            update_UI(current_state, previous_state);
            previous_state = current_state;
        }
        else
        {
            current_state = STATE_NO_SRL_DEVICE;
            key_state = state_no_srl_device;
            update_UI(current_state, previous_state);
            previous_state = current_state;
        }

        key_state();

        update_UI(current_state, previous_state);
        previous_state = current_state;

    } while (1);

    quitProgram();
}

void update_UI(program_state_t current_state, program_state_t previous_state) {
    if (current_state == previous_state) {
        return; // No need to update the UI if the state hasn't changed
    }

    switch (current_state) {
        case STATE_NO_KEY_FILE:
            NoKeyFileGFX();
            break;
        case STATE_KEY_FILE_AVAILABLE:
            KeyFileAvailableGFX();
            break;
        case STATE_CONNECTING:
            ConnectingGFX();
            break;
        case STATE_HAS_SRL_DEVICE:
            gfx_Sprite(usb_connected_sprite, 25, LCD_HEIGHT - 40);
            break;
        case STATE_NO_SRL_DEVICE:
            gfx_Sprite(usb_disconnected_sprite, 25, LCD_HEIGHT - 40);
            break;
        case STATE_DASHBOARD:
            gfx_ZeroScreen();
            FreeMemory();
            /* DASHBOARD MENU */
            gfx_SetTextScale(2, 2);
            gfx_PrintStringXY("TINET Dashboard", ((GFX_LCD_WIDTH - gfx_GetStringWidth("TINET Dashboard")) / 2), 5);
            gfx_SetTextFGColor(224);
            gfx_SetTextScale(1, 1);
            gfx_PrintStringXY("Press [clear] to quit.", ((GFX_LCD_WIDTH - gfx_GetStringWidth("Press [clear] to quit.")) / 2), 35);
            gfx_SetTextFGColor(255);
            gfx_PrintStringXY("Logged in as ", 1, 45);
            gfx_PrintStringXY(username, gfx_GetStringWidth("Logged in as "), 45);
    }
}

void GFXspritesInit()
{
    /* Allocate space for the decompressed sprites */
    /* Same as gfx_AllocSprite() */
    login_qrcode_sprite = gfx_MallocSprite(login_qr_width, login_qr_height);
    usb_connected_sprite = gfx_MallocSprite(usb_connected_width, usb_connected_height);
    usb_disconnected_sprite = gfx_MallocSprite(usb_disconnected_width, usb_disconnected_height);
    connecting_sprite = gfx_MallocSprite(connecting_width, connecting_height);

    /* Decompress  sprite */
    zx0_Decompress(login_qrcode_sprite, login_qr_compressed);
    zx0_Decompress(usb_connected_sprite, usb_connected_compressed);
    zx0_Decompress(usb_disconnected_sprite, usb_disconnected_compressed);
    zx0_Decompress(connecting_sprite, connecting_compressed);
}

void GFXsettings()
{
    /* GFX SETTINGS */
    gfx_FillScreen(0x00);
    gfx_SetTextFGColor(255);
    gfx_SetTextBGColor(0);
    gfx_SetTextTransparentColor(0);
}

void writeKeyFile()
{
    uint8_t appvar;
    char username[] = "sampleuser/0";
    char key[] = "samplekey/0";
    appvar = ti_Open("NetKey", "w");
    if(appvar){
        int bytes_written;
        int file_position;

        file_position = ti_Tell(appvar);
        dbg_printf("Before writing username: file_position=%d\n", file_position);

        bytes_written = ti_Write(username, strlen(username) + 1, 1, appvar);
        dbg_printf("Bytes written for username: %d\n", bytes_written);

        file_position = ti_Tell(appvar);
        dbg_printf("After writing username: file_position=%d\n", file_position);

        bytes_written = ti_Write(key, strlen(key) + 1, 1, appvar);
        dbg_printf("Bytes written for key: %d\n", bytes_written);

        file_position = ti_Tell(appvar);
        dbg_printf("After writing key: file_position=%d\n", file_position);

        ti_Close(appvar);
    }
    else { dbg_printf("File IO error"); }
}


void KeyFileAvailableGFX()
{
    gfx_PrintStringXY("Keyfile detected!", ((GFX_LCD_WIDTH - gfx_GetStringWidth("Keyfile detected!")) / 2), 95);

    char display_str[64]; // Adjust the buffer size as needed
    snprintf(display_str, sizeof(display_str), "Username: %s", username);
    gfx_PrintStringXY(display_str, ((GFX_LCD_WIDTH - gfx_GetStringWidth(display_str)) / 2), 135);

    gfx_PrintStringXY("Press [enter] to connect!", ((GFX_LCD_WIDTH - gfx_GetStringWidth("Press [enter] to connect!")) / 2), 65);
}

void NoKeyFileGFX()
{
    gfx_PrintStringXY("Please first add your keyfile!!", ((GFX_LCD_WIDTH - gfx_GetStringWidth("Please first add your keyfile!!")) / 2), 90);
    gfx_PrintStringXY("https://ti84pluscenet.tkbstudios.tk/login", ((GFX_LCD_WIDTH - gfx_GetStringWidth("https://ti84pluscenet.tkbstudios.tk/login")) / 2), 100);
    gfx_ScaledSprite_NoClip(login_qrcode_sprite, (GFX_LCD_WIDTH - login_qr_width * 4) / 2, 112, 4, 4);
}

void ConnectingGFX()
{
    gfx_Sprite(connecting_sprite, (GFX_LCD_WIDTH - connecting_width) / 2, 112);
}


void ConnectSerial(char *message)
{
    srl_busy = true;
    srl_Write(&srl, message, strlen(message));
    srl_busy = false;
}


void login()
{
    char username_msg[64];
    snprintf(username_msg, sizeof(username_msg), "USERNAME:%s", username);
    ConnectSerial(username_msg);
}



void readSRL()
{    
    /* Read up to 64 bytes from the serial buffer */
    size_t bytes_read = srl_Read(&srl, in_buffer, sizeof in_buffer);

    /* Check for an error (e.g. device disconneced) */
    if(bytes_read < 0) {
        dbg_printf("error %d on srl_Read\n", bytes_read);
        has_srl_device = false;
    } else if(bytes_read > 0) {
        /* Add a null terminator to make in_buffer a valid C-style string */
        in_buffer[bytes_read] = '\0';

        /* BRIDGE CONNECTED GFX */
        if (strcmp(in_buffer, "bridgeConnected") == 0)
        {
            bridge_connected = true;
            gfx_SetColor(0x00);
            gfx_FillRectangle(((GFX_LCD_WIDTH - gfx_GetStringWidth("Bridge disconnected!")) / 2), 80, gfx_GetStringWidth("Bridge disconnected!"), 15);
            gfx_SetColor(0x00);
            gfx_PrintStringXY("Bridge connected!", ((GFX_LCD_WIDTH - gfx_GetStringWidth("Bridge connected!")) / 2), 80);
        }
        if (strcmp(in_buffer, "bridgeDisconnected") == 0)
        {
            bridge_connected = false;
            gfx_SetColor(0x00);
            gfx_FillRectangle(((GFX_LCD_WIDTH - gfx_GetStringWidth("Bridge disconnected!")) / 2), 80, gfx_GetStringWidth("Bridge disconnected!"), 15);
            gfx_SetColor(0x00);
            gfx_PrintStringXY("Bridge disconnected!", ((GFX_LCD_WIDTH - gfx_GetStringWidth("Bridge disconnected!")) / 2), 80);
        }

        /* Internet Connected GFX */
        if (strcmp(in_buffer, "SERIAL_CONNECTED_CONFIRMED_BY_SERVER") == 0)
        {
            internet_connected = true;
            gfx_SetColor(0x00);
            gfx_FillRectangle(((GFX_LCD_WIDTH - gfx_GetStringWidth("Internet disconnected!")) / 2), 110, gfx_GetStringWidth("Internet disconnected!"), 15);
            gfx_SetColor(0x00);
            gfx_PrintStringXY("Internet connected!", ((GFX_LCD_WIDTH - gfx_GetStringWidth("Internet connected!")) / 2), 110);
        }
        if (strcmp(in_buffer, "internetDisconnected") == 0)
        {
            internet_connected = false;
            gfx_SetColor(0x00);
            gfx_FillRectangle(((GFX_LCD_WIDTH - gfx_GetStringWidth("Internet disconnected!")) / 2), 110, gfx_GetStringWidth("Internet disconnected!"), 15);
            gfx_SetColor(0x00);
            gfx_PrintStringXY("Internet disconnected!", ((GFX_LCD_WIDTH - gfx_GetStringWidth("Internet disconnected!")) / 2), 110);
        }

        if (strcmp(in_buffer, "Now send the token.") == 0)
        {
            char token_msg[64];
            snprintf(token_msg, sizeof(token_msg), "TOKEN:%s", authkey);
            ConnectSerial(token_msg);
        }
        if (strcmp(in_buffer, "Logged in!") == 0)
        {
            dbg_printf("Logged in!");
            current_state = STATE_DASHBOARD;
            key_state = state_dashboard;
            update_UI(current_state, previous_state);
            previous_state = current_state;
        }
    }
}


void FreeMemory()
{
    /* FREE MEMORY FROM SPRITES */
    // file deepcode ignore DoubleFree: no.
    free(login_qrcode_sprite);
    free(usb_connected_sprite);
    free(usb_disconnected_sprite);
    free(connecting_sprite);
}

void sendSerialInitData()
{
    serial_init_data_sent = true;
    char init_serial_connected_text_buffer[17] = "SERIAL_CONNECTED";

    srl_Write(&srl, init_serial_connected_text_buffer, strlen(init_serial_connected_text_buffer));
}

void common_state_function(void) {
    if (kb_Data[6] == kb_Enter && !USB_connected && !USB_connecting && bridge_connected && !srl_busy)
    {
        /* login */
        USB_connecting = true;
        current_state = STATE_CONNECTING;
        update_UI(current_state, previous_state);
        previous_state = current_state;
        login();
    }

    if (kb_Data[1] == kb_Mode)
    {
        writeKeyFile();
        quitProgram();
    }
}

void state_dashboard(void) {
    if (kb_Data[1] == kb_Window)
    {
        quitProgram();
    }
}

void state_no_key_file(void) {
    common_state_function();
}

void state_key_file_available(void) {
    common_state_function();
}

void state_connecting(void) {
    common_state_function();
}

void state_has_srl_device(void) {
    common_state_function();
}

void state_no_srl_device(void) {
    common_state_function();
}

void quitProgram() {
    gfx_End();
    usb_Cleanup();
    FreeMemory();
    exit(0);
}