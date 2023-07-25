/*
 *--------------------------------------
 * Program Name: TINET Client (Calculator)
 * Author: TKB Studios
 * License: Apache License 2.0
 * Description: Allows the user to communicate with the TINET servers
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
#include <stdbool.h>

#include "gfx/gfx.h"

/* DEFINE SETTINGS APPVAR */
char *settingsappv = "NETSett";
char *TEMP_PROGRAM = "_";
char *MAIN_PROGRAM = "TINET";

/* DEFINE USER */
bool keyfile_available = false;
char *username;
char *authkey;
uint8_t appvar;

/* READ BUFFERS */
size_t read_flen;
uint8_t *ptr;
char in_buffer[64];

/* DEFINE SPRITES */
gfx_sprite_t *login_qrcode_sprite = NULL;
gfx_sprite_t *usb_connected_sprite = NULL;
gfx_sprite_t *usb_disconnected_sprite = NULL;
gfx_sprite_t *connecting_sprite = NULL;
gfx_sprite_t *bridge_connected_sprite = NULL;
gfx_sprite_t *internet_connected_sprite = NULL;

/* DEFINE FUNCTIONS */
void GFXspritesInit();
void GFXsettings();
void writeKeyFile();
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
void dashboardScreen();
void GPTScreen();
void AccountScreen();
bool startsWith(const char *str, const char *prefix);
/*
void LoadDashboardSprites();
void LoadUSBSprites();
void FreeSprites();
*/

/* DEFINE CONNECTION VARS */
bool USB_connected = false;
bool prev_USB_connected = false;
bool USB_connecting = false;
bool bridge_connected = false;
bool internet_connected = false;
bool srl_busy = false;
srl_device_t srl;
bool has_srl_device = false;
uint8_t srl_buf[512];
bool serial_init_data_sent = false;

static usb_error_t handle_usb_event(usb_event_t event, void *event_data, usb_callback_data_t *callback_data __attribute__((unused)))
{
    usb_error_t err;
    if ((err = srl_UsbEventCallback(event, event_data, callback_data)) != USB_SUCCESS)
        return err;
    if (event == USB_DEVICE_CONNECTED_EVENT && !(usb_GetRole() & USB_ROLE_DEVICE))
    {
        usb_device_t device = event_data;
        USB_connected = true;
        usb_ResetDevice(device);
    }

    if (event == USB_HOST_CONFIGURE_EVENT || (event == USB_DEVICE_ENABLED_EVENT && !(usb_GetRole() & USB_ROLE_DEVICE)))
    {

        if (has_srl_device)
            return USB_SUCCESS;

        usb_device_t device;
        if (event == USB_HOST_CONFIGURE_EVENT)
        {
            device = usb_FindDevice(NULL, NULL, USB_SKIP_HUBS);
            if (device == NULL)
                return USB_SUCCESS;
        }
        else
        {
            device = event_data;
        }

        srl_error_t error = srl_Open(&srl, device, srl_buf, sizeof srl_buf, SRL_INTERFACE_ANY, 9600);
        if (error)
        {
            gfx_End();
            os_ClrHome();
            printf("Error %d initting serial\n", error);
            while (os_GetCSC())
                ;
            return 0;
            return USB_SUCCESS;
        }
        has_srl_device = true;
    }

    if (event == USB_DEVICE_DISCONNECTED_EVENT)
    {
        usb_device_t device = event_data;
        if (device == srl.dev)
        {
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
    //GFXspritesInit();

    /* GFX BEGIN */
    gfx_Begin();
    gfx_SetPalette(global_palette, sizeof_global_palette, 0);

    /* GFX SETTINGS */
    GFXsettings();

    /* MAIN MENU */
    gfx_SetTextScale(2, 2);
    gfx_PrintStringXY("TINET", ((GFX_LCD_WIDTH - gfx_GetStringWidth("TINET")) / 2), 5);
    gfx_SetTextFGColor(224);
    gfx_PrintStringXY("Press [clear] to quit.", ((GFX_LCD_WIDTH - gfx_GetStringWidth("Press [clear] to quit.")) / 2), 35);
    gfx_SetTextFGColor(255);
    gfx_SetTextScale(1, 1);

    appvar = ti_Open("NetKey", "r");

    if (appvar == 0)
    {
        keyfile_available = false;
        NoKeyFileGFX();
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
    usb_error_t usb_error = usb_Init(handle_usb_event, NULL, usb_desc, USB_DEFAULT_INIT_FLAGS);
    if (usb_error)
    {
        dbg_printf("usb init error %u\n", usb_error);
        return 1;
    }

    //LoadUSBSprites();
    /* Loop until [clear] is pressed */
    do
    {
        /* Update kb_Data */
        kb_Scan();

        usb_HandleEvents();
        if (has_srl_device)
        {
            readSRL();
        }

        if (has_srl_device && bridge_connected && !serial_init_data_sent)
        {
            sendSerialInitData();
        }

        /* Check if USB state has changed */
        if (prev_USB_connected != USB_connected)
        {
            prev_USB_connected = USB_connected;

            /* Redraw the appropriate sprite based on USB state */
            if (USB_connected)
            {
                //gfx_Sprite(usb_connected_sprite, 25, LCD_HEIGHT - 40);
            }
            else
            {
                //gfx_Sprite(usb_disconnected_sprite, 25, LCD_HEIGHT - 40);
            }
        }

        if (kb_Data[6] == kb_Enter && !USB_connected && !USB_connecting && bridge_connected && !srl_busy)
        {
            USB_connecting = true;
            //gfx_Sprite(connecting_sprite, (GFX_LCD_WIDTH - connecting_width) / 2, 112);
            login();
        }

        if (kb_Data[1] == kb_Mode)
        {
            writeKeyFile();
            quitProgram();
        }

    } while (kb_Data[6] != kb_Clear);

    quitProgram();
}

void displayUserStats(const char *stats)
{
    // Parse the received user stats and display them on the screen
    gfx_ZeroScreen();
    gfx_SetTextScale(2, 2);
    gfx_PrintStringXY("TINET Dashboard", ((GFX_LCD_WIDTH - gfx_GetStringWidth("TINET Dashboard")) / 2), 5);
    gfx_SetTextFGColor(224);
    gfx_PrintStringXY("Press [clear] to quit.", ((GFX_LCD_WIDTH - gfx_GetStringWidth("Press [clear] to quit.")) / 2), 35);
    gfx_SetTextFGColor(255);
    gfx_SetTextScale(1, 1);

    // Split the received stats using the delimiter ";"
    char *statTokens[8];
    char *token = strtok(stats, ";");
    int i = 0;
    while (token != NULL && i < 8)
    {
        statTokens[i++] = token;
        token = strtok(NULL, ";");
    }

    // Display each user stat on the screen
    int y = 65;
    gfx_PrintStringXY("User Stats:", 1, y);
    y += 20;
    gfx_PrintStringXY("MB Used This Month: ", 1, y);
    gfx_PrintString(statTokens[0]);
    y += 15;
    gfx_PrintStringXY("MB Used Total: ", 1, y);
    gfx_PrintString(statTokens[1]);
    y += 15;
    gfx_PrintStringXY("Requests This Month: ", 1, y);
    gfx_PrintString(statTokens[2]);
    y += 15;
    gfx_PrintStringXY("Total Requests: ", 1, y);
    gfx_PrintString(statTokens[3]);
    y += 15;
    gfx_PrintStringXY("Plan: ", 1, y);
    gfx_PrintString(statTokens[4]);
    y += 15;
    gfx_PrintStringXY("Time Online (seconds): ", 1, y);
    gfx_PrintString(statTokens[5]);
    y += 15;
    gfx_PrintStringXY("Last Login (epoch time): ", 1, y);
    gfx_PrintString(statTokens[6]);
    y += 15;
    gfx_PrintStringXY("Profile Public: ", 1, y);
    gfx_PrintString(statTokens[7]);
}

void dashboardScreen()
{
    gfx_ZeroScreen();
    /* DASHBOARD MENU */
    gfx_SetTextScale(2, 2);
    gfx_PrintStringXY("TINET Dashboard", ((GFX_LCD_WIDTH - gfx_GetStringWidth("TINET Dashboard")) / 2), 5);
    gfx_SetTextFGColor(224);
    gfx_SetTextScale(1, 1);
    gfx_PrintStringXY("Press [clear] to quit.", ((GFX_LCD_WIDTH - gfx_GetStringWidth("Press [clear] to quit.")) / 2), 35);
    gfx_SetTextFGColor(255);
    gfx_PrintStringXY("Logged in as ", 1, 45);
    gfx_PrintStringXY(username, gfx_GetStringWidth("Logged in as "), 45);

    /* Load sprites for dashboard screen */
    //LoadDashboardSprites();

    do
    {
        kb_Scan();
        if (kb_Data[5] == kb_Tan)
        {
            GPTScreen();
        }
        if (kb_Data[4] == kb_Prgm)
        {
            printf("Chat");
        }
        if (kb_Data[2] == kb_Recip)
        {
            printf("Drive");
        }
        if (kb_Data[2] == kb_Math)
        {
            ConnectSerial("accountInfo");

            char in_buffer[64];
            size_t bytes_read = srl_Read(&srl, in_buffer, sizeof in_buffer);
            if (bytes_read > 0)
            {
                in_buffer[bytes_read] = '\0';
                if (startsWith(in_buffer, "accountInfo:"))
                {
                    displayUserStats(in_buffer + strlen("accountInfo:"));
                }
            }
        }

    } while (kb_Data[6] != kb_Clear);
}

void GPTScreen()
{
    gfx_ZeroScreen();
    gfx_SetTextScale(2, 2);
    gfx_PrintStringXY("TINET GPT", ((GFX_LCD_WIDTH - gfx_GetStringWidth("TINET GPT")) / 2), 0);
    gfx_SetTextScale(1, 1);

    char output_buffer[64] = {0};
    strncpy(output_buffer, "GPT:", 4);

    const char *chars = "\0\0\0\0\0\0\0\0\0\0\"WRMH\0\0?[VQLG\0\0:ZUPKFC\0 YTOJEB\0\0XSNIDA\0\0\0\0\0\0\0\0";
    uint8_t key, i = 0;
    char buffer[64] = {0};

    do
    {
        key = os_GetCSC();
        if (chars[key])
        {
            buffer[i++] = chars[key];
            printf("%c", chars[key]);
        }
    } while (key != sk_Enter);

    // strncpy(output_buffer + 4, buffer, sizeof(output_buffer) - 4 - 1);
    // output_buffer[sizeof(output_buffer) - 1] = '\0';

    printf("\n%s", buffer);
    printf("\n%i", sizeof(buffer));
    // printf("\n%s", output_buffer);
    ConnectSerial(buffer);
    sleep(1000);

    while (!os_GetCSC())
        ;
}

void AccountScreen()
{
    gfx_ZeroScreen();
    gfx_SetTextScale(2, 2);
    gfx_PrintStringXY("TINET Account", ((GFX_LCD_WIDTH - gfx_GetStringWidth("TINET Account")) / 2), 0);
    gfx_SetTextScale(1, 1);
    do
    {
        kb_Scan();
    } while (kb_Data[6] != kb_Clear);

    return;
}

void GFXspritesInit()
{
    login_qrcode_sprite = gfx_MallocSprite(login_qr_width, login_qr_height);
    usb_connected_sprite = gfx_MallocSprite(usb_connected_width, usb_connected_height);
    usb_disconnected_sprite = gfx_MallocSprite(usb_disconnected_width, usb_disconnected_height);
    connecting_sprite = gfx_MallocSprite(connecting_width, connecting_height);
    bridge_connected_sprite = gfx_MallocSprite(bridge_connected_width, bridge_connected_height);
    internet_connected_sprite = gfx_MallocSprite(internet_connected_width, internet_connected_height);

    zx0_Decompress(login_qrcode_sprite, login_qr_compressed);
    zx0_Decompress(usb_connected_sprite, usb_connected_compressed);
    zx0_Decompress(usb_disconnected_sprite, usb_disconnected_compressed);
    zx0_Decompress(connecting_sprite, connecting_compressed);
    zx0_Decompress(bridge_connected_sprite, bridge_connected_compressed);
    zx0_Decompress(internet_connected_sprite, internet_connected_compressed);
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
    if (appvar)
    {
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
    else
    {
        dbg_printf("File IO error");
    }
}

void KeyFileAvailableGFX()
{
    gfx_PrintStringXY("Keyfile detected!", ((GFX_LCD_WIDTH - gfx_GetStringWidth("Keyfile detected!")) / 2), 95);

    char display_str[64];
    snprintf(display_str, sizeof(display_str), "Username: %s", username);
    gfx_PrintStringXY(display_str, ((GFX_LCD_WIDTH - gfx_GetStringWidth(display_str)) / 2), 135);

    gfx_PrintStringXY("Press [enter] to connect!", ((GFX_LCD_WIDTH - gfx_GetStringWidth("Press [enter] to connect!")) / 2), 65);
}

void NoKeyFileGFX()
{
    gfx_PrintStringXY("Please first add your keyfile!!", ((GFX_LCD_WIDTH - gfx_GetStringWidth("Please first add your keyfile!!")) / 2), 90);
    gfx_PrintStringXY("https://tinet.tkbstudios.com/login", ((GFX_LCD_WIDTH - gfx_GetStringWidth("https://tinet.tkbstudios.com/login")) / 2), 100);
    //gfx_Sprite(login_qrcode_sprite, (GFX_LCD_WIDTH - login_qr_width) / 2, 112);
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
    size_t bytes_read = srl_Read(&srl, in_buffer, sizeof in_buffer);

    if (bytes_read < 0)
    {
        dbg_printf("error %d on srl_Read\n", bytes_read);
        has_srl_device = false;
    }
    else if (bytes_read > 0)
    {
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

        if (strcmp(in_buffer, "SEND_TOKEN") == 0)
        {
            char token_msg[64];
            snprintf(token_msg, sizeof(token_msg), "TOKEN:%s", authkey);
            ConnectSerial(token_msg);
        }
        if (strcmp(in_buffer, "LOGIN_SUCCESS") == 0)
        {
            dbg_printf("Logged in!");
            dashboardScreen();
        }

        if (strcmp(in_buffer, "MAIL_NOT_VERIFIED") == 0)
        {
            dbg_printf("Logged in!");
            printf("Mail not\nverified!");
        }

        if (startsWith(in_buffer, "accountInfo:"))
        {
            printf("accountInfo recieved\n");
            printf("%s", in_buffer);
        }
    }
}

void FreeMemory()
{
    /* FREE MEMORY FROM SPRITES */
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

void quitProgram()
{
    gfx_End();
    usb_Cleanup();
    //GFXspritesFree();
    exit(0);
}

bool startsWith(const char *str, const char *prefix)
{
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

/*
void GFXspritesInit()
{
    login_qrcode_sprite = NULL;
    usb_connected_sprite = NULL;
    usb_disconnected_sprite = NULL;

    LoadLoginQRCodeSprite();
    LoadUSBConnectedSprite();
    LoadUSBDIsconnectedSprite();
}

void GFXspritesFree()
{
    // Free the memory from specific sprites
    if (login_qrcode_sprite != NULL)
    {
        gfx_FreeSprite(login_qrcode_sprite);
        login_qrcode_sprite = NULL;
    }
    if (usb_connected_sprite != NULL)
    {
        gfx_FreeSprite(usb_connected_sprite);
        usb_connected_sprite = NULL;
    }
    if (usb_disconnected_sprite != NULL)
    {
        gfx_FreeSprite(usb_disconnected_sprite);
        usb_disconnected_sprite = NULL;
    }
}

void LoadLoginQRCodeSprite()
{
    if (login_qrcode_sprite == NULL)
    {
        login_qrcode_sprite = gfx_MallocSprite(login_qr_width, login_qr_height);
        zx0_Decompress(login_qrcode_sprite, login_qr_compressed);
    }
}

void LoadUSBSprites()
{
    if (usb_connected_sprite == NULL)
    {
        usb_connected_sprite = gfx_MallocSprite(usb_connected_width, usb_connected_height);
        zx0_Decompress(usb_connected_sprite, usb_connected_compressed);
    }

    if (usb_disconnected_sprite == NULL)
    {
        usb_disconnected_sprite = gfx_MallocSprite(usb_disconnected_width, usb_disconnected_height);
        zx0_Decompress(usb_disconnected_sprite, usb_disconnected_compressed);
    }
}
*/