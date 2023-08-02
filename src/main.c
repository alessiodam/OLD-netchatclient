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
#include <sys/timers.h>
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
#include <tice.h>

#include "gfx/gfx.h"

/* DEFINE USER */
bool keyfile_available = false;
char *username;
char *authkey;
uint8_t appvar;

/* READ BUFFERS */
size_t read_flen;
uint8_t *ptr;
char in_buffer[64];

/* DEFINE FUNCTIONS */
void GFXspritesInit();
void GFXsettings();
void NoKeyFileGFX();
void KeyFileAvailableGFX();
void FreeMemory();
void quitProgram();
void login();
void readSRL();
void sendSerialInitData();
void getCurrentTime();
void printServerPing();
void dashboardScreen();
void AccountScreen();
void mailNotVerifiedScreen();
bool startsWith(const char *str, const char *prefix);
void displayIP(const char *ipAddress);
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
bool has_unread_data = false;
srl_device_t srl;
bool has_srl_device = false;
uint8_t srl_buf[512];
bool serial_init_data_sent = false;

bool key_pressed = false;
uint8_t debounce_delay = 10;

/* DEFINE SPRITES */
gfx_sprite_t *login_qrcode_sprite = NULL;
gfx_sprite_t *usb_connected_sprite = NULL;
gfx_sprite_t *usb_disconnected_sprite = NULL;
gfx_sprite_t *connecting_sprite = NULL;
gfx_sprite_t *bridge_connected_sprite = NULL;
gfx_sprite_t *internet_connected_sprite = NULL;

/* CONNECTION FUNCTIONS */
void SendSerial(const char *message) {
    size_t totalBytesWritten = 0;
    size_t messageLength = strlen(message);

    while (totalBytesWritten < messageLength) {
        int bytesWritten = srl_Write(&srl, message + totalBytesWritten, messageLength - totalBytesWritten);
        
        if (bytesWritten < 0) {
            printf("SRL W ERR");
        }

        totalBytesWritten += bytesWritten;
    }
}


/* DEFINE BUTTONS */
typedef struct {
    int x, y;
    int width, height;
    const char *label;
    void (*action)();
} Button;

void accountInfoButtonPressed() {
    AccountScreen();
}

void RTCChatButtonPressed() {
    printf("Chat btn press\n");
    msleep(500);
}

void BucketsButtonPressed() {
    printf("Buckets btn press\n");
    msleep(500);
}

Button dashboardButtons[] = {
    {50, 60, 120, 30, "Account Info", accountInfoButtonPressed},
    {50, 100, 120, 30, "RTC Chat", RTCChatButtonPressed},
    {50, 140, 120, 30, "Buckets", BucketsButtonPressed}
};
int numDashboardButtons = sizeof(dashboardButtons) / sizeof(dashboardButtons[0]);

void drawButtons(Button *buttons, int numButtons, int selectedButton) {
    for (int i = 0; i < numButtons; i++) {
        if (i == selectedButton) {
            gfx_SetColor(255);
        } else {
            gfx_SetColor(224);
        }
        gfx_Rectangle(buttons[i].x, buttons[i].y, buttons[i].width, buttons[i].height);
        gfx_PrintStringXY(buttons[i].label, buttons[i].x + 10, buttons[i].y + 10);
    }
    msleep(500);
}


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

int main(void) {
    // Initialize graphics and settings
    gfx_Begin();
    gfx_SetPalette(global_palette, sizeof_global_palette, 0);
    GFXsettings();

    // Display main menu
    gfx_SetTextScale(2, 2);
    gfx_PrintStringXY("TINET", (GFX_LCD_WIDTH - gfx_GetStringWidth("TINET")) / 2, 5);
    gfx_SetTextFGColor(224);
    gfx_PrintStringXY("Press [clear] to quit.", (GFX_LCD_WIDTH - gfx_GetStringWidth("Press [clear] to quit.")) / 2, 35);
    gfx_SetTextFGColor(255);
    gfx_SetTextScale(1, 1);

    // Open and read appvar data
    appvar = ti_Open("NETKEY", "r");
    if (appvar == 0) {
        keyfile_available = false;
        NoKeyFileGFX();
    } else {
        // Read and process appvar data
        read_flen = ti_GetSize(appvar);
        uint8_t *data_ptr = (uint8_t *)ti_GetDataPtr(appvar);
        ti_Close(appvar);

        char *read_username = (char *)data_ptr;
        username = read_username;

        size_t read_un_len = strlen(read_username) + 1;
        data_ptr += (read_un_len + 1);
        read_flen -= (read_un_len + 1);

        char *read_key = (char *)data_ptr - 1;
        size_t key_len = strlen(read_key);
        authkey = read_key;

        keyfile_available = true;
        KeyFileAvailableGFX();
    }

    // Initialize USB communication
    const usb_standard_descriptors_t *usb_desc = srl_GetCDCStandardDescriptors();
    usb_error_t usb_error = usb_Init(handle_usb_event, NULL, usb_desc, USB_DEFAULT_INIT_FLAGS);
    if (usb_error) {
        return 1;
    }

    // Main loop
    do {
        kb_Scan();

        usb_HandleEvents();
        if (has_srl_device) {
            readSRL();
        }

        if (has_srl_device && bridge_connected && !serial_init_data_sent) {
            sendSerialInitData();
        }

        if (prev_USB_connected != USB_connected) {
            prev_USB_connected = USB_connected;
            // Redraw appropriate USB state sprite
            // gfx_Sprite(USB_connected ? usb_connected_sprite : usb_disconnected_sprite, 25, LCD_HEIGHT - 40);
        }

        if (kb_Data[6] == kb_Enter && !USB_connected && !USB_connecting && bridge_connected) {
            USB_connecting = true;
            // gfx_Sprite(connecting_sprite, (GFX_LCD_WIDTH - connecting_width) / 2, 112);
            login();
        }
    } while (kb_Data[6] != kb_Clear);

    quitProgram();

    return 0;
}

void displayAccountInfo(const char *accountInfo)
{
    gfx_ZeroScreen();
    gfx_SetTextScale(2, 2);
    gfx_PrintStringXY("TINET Account Info", ((GFX_LCD_WIDTH - gfx_GetStringWidth("TINET Account Info")) / 2), 5);
    gfx_SetTextFGColor(224);
    gfx_PrintStringXY("Press [clear] to quit.", ((GFX_LCD_WIDTH - gfx_GetStringWidth("Press [clear] to quit.")) / 2), 35);
    gfx_SetTextFGColor(255);
    gfx_SetTextScale(1, 1);

    char *infoTokens[8];
    char *token = strtok(accountInfo, ";");
    int i = 0;
    while (token != NULL && i < 8)
    {
        infoTokens[i++] = token;
        token = strtok(NULL, ";");
    }

    int y = 65;
    gfx_PrintStringXY("Account Info:", 1, y);
    y += 20;
    gfx_PrintStringXY("MB Used This Month: ", 1, y);
    gfx_PrintString(infoTokens[0]);
    y += 15;
    gfx_PrintStringXY("MB Used Total: ", 1, y);
    gfx_PrintString(infoTokens[1]);
    y += 15;
    gfx_PrintStringXY("Requests This Month: ", 1, y);
    gfx_PrintString(infoTokens[2]);
    y += 15;
    gfx_PrintStringXY("Total Requests: ", 1, y);
    gfx_PrintString(infoTokens[3]);
    y += 15;
    gfx_PrintStringXY("Plan: ", 1, y);
    gfx_PrintString(infoTokens[4]);
    y += 15;
    gfx_PrintStringXY("Time Online (seconds): ", 1, y);
    gfx_PrintString(infoTokens[5]);
    y += 15;
    gfx_PrintStringXY("Last Login (epoch time): ", 1, y);
    gfx_PrintString(infoTokens[6]);
    y += 15;
    gfx_PrintStringXY("Profile Public: ", 1, y);
    gfx_PrintString(infoTokens[7]);
}

void dashboardScreen()
{
    gfx_ZeroScreen();
    gfx_SetTextScale(2, 2);
    gfx_PrintStringXY("TINET Dashboard", ((GFX_LCD_WIDTH - gfx_GetStringWidth("TINET Dashboard")) / 2), 5);
    gfx_SetTextFGColor(224);
    gfx_SetTextScale(1, 1);
    gfx_PrintStringXY("Press [clear] to quit.", ((GFX_LCD_WIDTH - gfx_GetStringWidth("Press [clear] to quit.")) / 2), 35);
    gfx_SetTextFGColor(255);
    
    int centerX = (GFX_LCD_WIDTH - gfx_GetStringWidth("Logged in as ")) / 2;

    gfx_PrintStringXY("Logged in as ", centerX - gfx_GetStringWidth("Logged in as "), 45);

    gfx_PrintStringXY(username, centerX, 45);

    int selectedButton = 0;
    drawButtons(dashboardButtons, numDashboardButtons, selectedButton);

    do
    {
        kb_Scan();

        if (has_srl_device)
        {
            readSRL();
        }

        if (kb_Data[7] == kb_Down) {
            selectedButton = (selectedButton + 1) % numDashboardButtons;
            drawButtons(dashboardButtons, numDashboardButtons, selectedButton);
        }
        else if (kb_Data[7] == kb_Up) {
            selectedButton = (selectedButton - 1 + numDashboardButtons) % numDashboardButtons;
            drawButtons(dashboardButtons, numDashboardButtons, selectedButton);
        }
        else if (kb_Data[6] == kb_Enter) {
            dashboardButtons[selectedButton].action();
        }
    } while (kb_Data[6] != kb_Clear);
}

void AccountScreen()
{
    gfx_ZeroScreen();
    gfx_SetTextScale(2, 2);
    gfx_PrintStringXY("TINET Account", ((GFX_LCD_WIDTH - gfx_GetStringWidth("TINET Account")) / 2), 0);
    gfx_SetTextScale(1, 1);
    char out_buff_msg[64] = "ACCOUNT_INFO";
    SendSerial(out_buff_msg);
    printf("Sent serial");
    do {
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
    // gfx_Sprite(login_qrcode_sprite, (GFX_LCD_WIDTH - login_qr_width) / 2, 112);
}

void login()
{
    char username_msg[64];
    snprintf(username_msg, sizeof(username_msg), "USERNAME:%s", username);
    SendSerial(username_msg);
}

void readSRL()
{
    size_t bytes_read = srl_Read(&srl, in_buffer, sizeof in_buffer);

    
    if (bytes_read < 0)
    {
        // has_srl_device = false;
        printf("SRL 0B");
    }
    else if (bytes_read > 0)
    {
        in_buffer[bytes_read] = '\0';
        has_unread_data = true;

        /* BRIDGE CONNECTED GFX */
        if (strcmp(in_buffer, "bridgeConnected") == 0) {
            bridge_connected = true;
            gfx_SetColor(0x00);
            gfx_FillRectangle(((GFX_LCD_WIDTH - gfx_GetStringWidth("Bridge disconnected!")) / 2), 80, gfx_GetStringWidth("Bridge disconnected!"), 15);
            gfx_SetColor(0x00);
            gfx_PrintStringXY("Bridge connected!", ((GFX_LCD_WIDTH - gfx_GetStringWidth("Bridge connected!")) / 2), 80);
        }
        if (strcmp(in_buffer, "bridgeDisconnected") == 0) {
            bridge_connected = false;
            gfx_SetColor(0x00);
            gfx_FillRectangle(((GFX_LCD_WIDTH - gfx_GetStringWidth("Bridge disconnected!")) / 2), 80, gfx_GetStringWidth("Bridge disconnected!"), 15);
            gfx_SetColor(0x00);
            gfx_PrintStringXY("Bridge disconnected!", ((GFX_LCD_WIDTH - gfx_GetStringWidth("Bridge disconnected!")) / 2), 80);
        }

        /* Internet Connected GFX */
        if (strcmp(in_buffer, "SERIAL_CONNECTED_CONFIRMED_BY_SERVER") == 0) {
            internet_connected = true;
            gfx_SetColor(0x00);
            gfx_FillRectangle(((GFX_LCD_WIDTH - gfx_GetStringWidth("Internet disconnected!")) / 2), 110, gfx_GetStringWidth("Internet disconnected!"), 15);
            gfx_SetColor(0x00);
            gfx_PrintStringXY("Internet connected!", ((GFX_LCD_WIDTH - gfx_GetStringWidth("Internet connected!")) / 2), 110);
        }
        if (strcmp(in_buffer, "internetDisconnected") == 0) {
            internet_connected = false;
            gfx_SetColor(0x00);
            gfx_FillRectangle(((GFX_LCD_WIDTH - gfx_GetStringWidth("Internet disconnected!")) / 2), 110, gfx_GetStringWidth("Internet disconnected!"), 15);
            gfx_SetColor(0x00);
            gfx_PrintStringXY("Internet disconnected!", ((GFX_LCD_WIDTH - gfx_GetStringWidth("Internet disconnected!")) / 2), 110);
        }

        if (strcmp(in_buffer, "SEND_TOKEN") == 0) {
            char token_msg[64];
            snprintf(token_msg, sizeof(token_msg), "TOKEN:%s", authkey);
            SendSerial(token_msg);
        }

        if (strcmp(in_buffer, "LOGIN_SUCCESS") == 0) {
            dashboardScreen();
        }

        if (strcmp(in_buffer, "MAIL_NOT_VERIFIED") == 0) {
            printf("Mail not\nverified!");
        }

        if (startsWith(in_buffer, "ACCOUNT_INFO:")) {
            displayAccountInfo(in_buffer + strlen("ACCOUNT_INFO:"));
        }

        if (startsWith(in_buffer, "YOUR_IP:")) {
            displayIP(in_buffer + strlen("YOUR_IP:"));
        }
        has_unread_data = false;
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
    SendSerial(init_serial_connected_text_buffer);
}

void quitProgram()
{
    gfx_End();
    usb_Cleanup();
    // GFXspritesFree();
    exit(0);
}

bool startsWith(const char *str, const char *prefix)
{
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

void displayIP(const char *ipAddress) {
    printf("Received IP address: %s\n", ipAddress);
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