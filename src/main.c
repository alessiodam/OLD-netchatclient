/*
 *--------------------------------------
 * Program Name: TI-84 Plus CE Net Client (Calculator)
 * Author: TKB Studios
 * License:
 * Description: Allows the user to communicate with the TI-84 Plus CE Net servers
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


/* Include the converted graphics file */
#include "gfx/gfx.h"

/* DEFINE SETTINGS APPVAR */
char *settingsappv = "TI84Sett";
char *TEMP_PROGRAM = "_";
char *MAIN_PROGRAM = "TINET";

/* DEFINE USER */
uint8_t keyfile;
uint8_t keyfile_buffer[256];
bool keyfile_available = false;
void *username;
void *authkey;

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
void EndProgram();
void Connect();
void PrintError(const char *str);

/* DEFINE CONNECTION VARS */
bool USB_connected = false;
bool internet_connected = false;
srl_device_t srl;
bool has_srl_device = false;
uint8_t srl_buf[512];

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

    /* LOADING SCREEN */
    /*
    for (int i = 0; i < 5; i++)
    {
        gfx_PrintStringXY("Loading program   ", ((GFX_LCD_WIDTH - gfx_GetStringWidth("Loading program   ")) / 2), (GFX_LCD_HEIGHT / 2));
        usleep(200);
        gfx_PrintStringXY("Loading program.  ", ((GFX_LCD_WIDTH - gfx_GetStringWidth("Loading program.  ")) / 2), (GFX_LCD_HEIGHT / 2));
        usleep(200);
        gfx_PrintStringXY("Loading program.. ", ((GFX_LCD_WIDTH - gfx_GetStringWidth("Loading program.. ")) / 2), (GFX_LCD_HEIGHT / 2));
        usleep(200);
        gfx_PrintStringXY("Loading program...", ((GFX_LCD_WIDTH - gfx_GetStringWidth("Loading program...")) / 2), (GFX_LCD_HEIGHT / 2));
        usleep(200);
    }
    */

    /* MAIN MENU */
    gfx_SetTextScale(2, 2);
    gfx_PrintStringXY("TI-84 Plus CE Net", ((GFX_LCD_WIDTH - gfx_GetStringWidth("TI-84 Plus CE Net")) / 2), 5);
    gfx_SetTextFGColor(224);
    gfx_PrintStringXY("Press [2nd] to quit.", ((GFX_LCD_WIDTH - gfx_GetStringWidth("Press [2nd] to quit.")) / 2), 35);
    gfx_SetTextFGColor(255);
    gfx_SetTextScale(1, 1);

    /* KEYFILE CHECK */
    /*
    bool *new_var;
    new_var = ti_Open("NetKey", "w+");
    if (ti_Write('USERNAME:"tkbstudios"', sizeof('USERNAME:"tkbstudios"'), 1, new_var) != 1)
    {
        return 1;
    }
    ti_CloseAll();
    */

    keyfile = ti_Open("NetKey", "r+");

    if (keyfile == 0)
    {
        keyfile_available = false;
        ti_Close(keyfile);
        NoKeyFileGFX();
    }
    else
    {
        keyfile_available = true;
        ti_Close(keyfile);
        KeyFileAvailableGFX();
    }

    const usb_standard_descriptors_t *usb_desc = srl_GetCDCStandardDescriptors();
    /* Initialize the USB driver with our event handler and the serial device descriptors */
    usb_error_t usb_error = usb_Init(handle_usb_event, NULL, usb_desc, USB_DEFAULT_INIT_FLAGS);
    if(usb_error) {
       PrintError(("usb init error %u\n", usb_error));
       return 1;
    }

    /* Loop until [2nd] is pressed */
    do
    {
        /* Update kb_Data */
        kb_Scan();

        /* Handle new USB events */
        usb_HandleEvents();

        /* Draw the USB sprites */
        if(has_srl_device)
        {
            gfx_Sprite(usb_connected_sprite, 25, LCD_HEIGHT - 40);
        }
        else
        {
            gfx_Sprite(usb_disconnected_sprite, 25, LCD_HEIGHT - 40);
        }

        if (kb_Data[6] == kb_Enter)
        {
            /* Connect Login and go dashboard script */
            ConnectingGFX();
        }

        if (kb_Data[1] == kb_Mode)
        {
            writeKeyFile();
            return 1;
        }

    } while (kb_Data[1] != kb_2nd);

    EndProgram();
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
    uint8_t keyfile;
    char *username = "sampleuser";
    keyfile = ti_Open("NetKey", "w+");
    if(keyfile){
        if(ti_Write(username, strlen(username), 1, keyfile) == 1)
        {
            PrintError("Write Success");
        }
        else
        {
            PrintError("Write failed");
        }
        ti_Close(keyfile);
    }
    else { PrintError("FileIO error"); }
}



void KeyFileAvailableGFX()
{
    gfx_PrintStringXY("Keyfile detected!", ((GFX_LCD_WIDTH - gfx_GetStringWidth("Keyfile detected!")) / 2), 95);
    ti_Read(keyfile_buffer, 18, 1, keyfile);
//    gfx_PrintStringXY((strcat("Username: ", username)), ((GFX_LCD_WIDTH - gfx_GetStringWidth("Keyfile detected!")) / 2), 95);
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

void EndProgram()
{
    /* FREE MEMORY FROM SPRITES */
    free(login_qrcode_sprite);
    free(usb_connected_sprite);
    free(usb_disconnected_sprite);
    free(connecting_sprite);

    usb_Cleanup();
    gfx_End();
    
    /* QUIT PROGRAM */
}

void PrintError(const char *str)
{
    gfx_End();
    os_ClrHome();
    printf(str);
    delay(5000);
    EndProgram();
}