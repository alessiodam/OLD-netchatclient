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
#include <ti/info.h>
#include <stdint.h>

#define MAX_MESSAGES 10
#define MAX_LINE_LENGTH (GFX_LCD_WIDTH - 40)

const system_info_t *systemInfo;

bool inside_RTC_chat = false;

/* DEFINE USER */
bool keyfile_available = false;
char *username;
char *authkey;
uint8_t appvar;

/* READ BUFFERS */
size_t read_flen;
uint8_t *ptr;
char in_buffer[8192];

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
void mailNotVerifiedScreen();
bool startsWith(const char *str, const char *prefix);
void displayIP(const char *ipAddress);
void howToUseScreen();
void alreadyConnectedScreen();
void userNotFoundScreen();
void calcIDneedsUpdateScreen();
void TINETChatScreen();
void accountInfoScreen(const char *accountInfo);
void updateCaseBox(bool isUppercase);
void ESP8266login();

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
usb_error_t usb_error;
const usb_standard_descriptors_t *usb_desc;
bool is_esp8266 = false;

bool key_pressed = false;
uint8_t debounce_delay = 10;

/* CONNECTION FUNCTIONS */
void SendSerial(const char *message)
{
    size_t totalBytesWritten = 0;
    size_t messageLength = strlen(message);

    while (totalBytesWritten < messageLength)
    {
        int bytesWritten = srl_Write(&srl, message + totalBytesWritten, messageLength - totalBytesWritten);

        if (bytesWritten < 0)
        {
            printf("SRL W ERR");
        }

        totalBytesWritten += bytesWritten;
    }
}

/* DEFINE BUTTONS */
typedef struct
{
    int x, y;
    int width, height;
    const char *label;
    void (*action)();
} Button;

void accountInfoButtonPressed()
{
    msleep(200);
    printf("in dev\n");
    /*
    char out_buff_msg[14] = "ACCOUNT_INFO";
    SendSerial(out_buff_msg);
    size_t bytes_read;

    do {
        bytes_read = srl_Read(&srl, in_buffer, sizeof in_buffer);
        if (bytes_read > 0) {
            break;
        }
    } while (1);

    accountInfoScreen(in_buffer);
    */
}

void BucketsButtonPressed()
{
    printf("Buckets btn press\n");
    msleep(500);
}

Button dashboardButtons[] = {
    {50, 60, 120, 30, "Account Info", accountInfoButtonPressed},
    {50, 100, 120, 30, "TINET Chat", TINETChatScreen},
    {50, 140, 120, 30, "Buckets", BucketsButtonPressed}};
int numDashboardButtons = sizeof(dashboardButtons) / sizeof(dashboardButtons[0]);

void loginButtonPressed()
{
    if (!USB_connected && !USB_connecting && bridge_connected)
    {
        USB_connecting = true;
        if (is_esp8266 == true)
        {
            login(); // on purpose, the ESP8266 login system is not done currently
        }
        else
        {
            login();
        }
    }
}

Button mainMenuButtons[] = {
    // Button properties: X (top left corner), Y (top left corner), width, height, label, function
    {20, 160, 120, 30, "Login", loginButtonPressed},
    {20, 200, 120, 30, "How to Use", howToUseScreen}};

int numMainMenuButtons = sizeof(mainMenuButtons) / sizeof(mainMenuButtons[0]);

void drawButtons(Button *buttons, int numButtons, int selectedButton)
{
    for (int i = 0; i < numButtons; i++)
    {
        if (i == selectedButton)
        {
            gfx_SetColor(7);
        }
        else
        {
            gfx_SetColor(224);
        }
        gfx_Rectangle(buttons[i].x, buttons[i].y, buttons[i].width, buttons[i].height);
        gfx_PrintStringXY(buttons[i].label, buttons[i].x + 10, buttons[i].y + 10);
    }
    msleep(250);
}

/* DEFINE CHAT */
void printWrappedText(const char *text, int x, int y);
void displayMessages();
void addMessage(const char *message, int posY);

typedef struct
{
    char message[64];
    int posY;
} ChatMessage;

ChatMessage messageList[MAX_MESSAGES];
int messageCount = 0;

/* DEFINE DATETIME */
typedef struct
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} DateTime;

/* DEFINE UI */
#define TITLE_X_POS 5
#define TITLE_Y_POS 5

usb_error_t handle_usb_event(usb_event_t event, void *event_data, usb_callback_data_t *callback_data)
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
    const system_info_t *systemInfo = os_GetSystemInfo();

    if (systemInfo->hardwareType2 != 9)
    {
        printf("%02X", systemInfo->hardwareType2);
        printf("\n");
        printf("not TI-84+CE");
        msleep(500);
        return 1;
    }

    // Initialize graphics and settings
    gfx_Begin();
    GFXsettings();

    // Display main menu
    gfx_ZeroScreen();
    gfx_SetTextScale(2, 2);
    gfx_PrintStringXY("TINET", (GFX_LCD_WIDTH - gfx_GetStringWidth("TINET")) / 2, 5);
    gfx_SetTextFGColor(224);
    gfx_PrintStringXY("Press [clear] to quit.", (GFX_LCD_WIDTH - gfx_GetStringWidth("Press [clear] to quit.")) / 2, 35);
    gfx_SetTextFGColor(255);
    gfx_SetTextScale(1, 1);

    /* CALC ID DISPLAY BOTTOM RIGHT */
    char calcidStr[sizeof(systemInfo->calcid) * 2 + 1];
    for (unsigned int i = 0; i < sizeof(systemInfo->calcid); i++)
    {
        sprintf(calcidStr + i * 2, "%02X", systemInfo->calcid[i]);
    }

    gfx_PrintStringXY(calcidStr, 320 - gfx_GetStringWidth(calcidStr), 232);

    // Open and read appvar data
    appvar = ti_Open("NETKEY", "r");
    if (appvar == 0)
    {
        keyfile_available = false;
        NoKeyFileGFX();
    }
    else
    {
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
        authkey = read_key;

        keyfile_available = true;
        KeyFileAvailableGFX();
    }

    const usb_standard_descriptors_t *usb_desc = srl_GetCDCStandardDescriptors();
    usb_error_t usb_error = usb_Init(handle_usb_event, NULL, usb_desc, USB_DEFAULT_INIT_FLAGS);
    if (usb_error)
    {
        printf("usb init error\n%u\n", usb_error);
        sleep(2);
        return 1;
    }

    int selectedButton = 0;
    drawButtons(mainMenuButtons, numMainMenuButtons, selectedButton);

    do
    {
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

        if (kb_Data[7] == kb_Down)
        {
            selectedButton = (selectedButton + 1) % numMainMenuButtons;
            drawButtons(mainMenuButtons, numMainMenuButtons, selectedButton);
        }
        else if (kb_Data[7] == kb_Up)
        {
            selectedButton = (selectedButton - 1 + numMainMenuButtons) % numMainMenuButtons;
            drawButtons(mainMenuButtons, numMainMenuButtons, selectedButton);
        }
        else if (kb_Data[6] == kb_Enter)
        {
            mainMenuButtons[selectedButton].action();
        }
    } while (kb_Data[6] != kb_Clear);

    quitProgram();

    return 0;
}

void accountInfoScreen(const char *accountInfo)
{
    gfx_ZeroScreen();
    gfx_SetTextScale(2, 2);
    gfx_PrintStringXY("TINET Account Info", ((GFX_LCD_WIDTH - gfx_GetStringWidth("TINET Account Info")) / 2), 5);
    gfx_SetTextFGColor(224);
    gfx_PrintStringXY("Press [clear] to quit.", ((GFX_LCD_WIDTH - gfx_GetStringWidth("Press [clear] to quit.")) / 2), 35);
    gfx_SetTextFGColor(255);
    gfx_SetTextScale(1, 1);

    char *infoTokens[8];
    char *token = strtok((char *)accountInfo, ";");
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
    do
    {
        kb_Scan();

        usb_HandleEvents();
        if (has_srl_device)
        {
            readSRL();
        }
    } while (kb_Data[6] != kb_Clear);
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
        usb_HandleEvents();
        if (has_srl_device)
        {
            readSRL();
        }

        if (kb_Data[6] == kb_Clear)
        {
            break;
        }

        if (kb_Data[7] == kb_Down)
        {
            selectedButton = (selectedButton + 1) % numDashboardButtons;
            drawButtons(dashboardButtons, numDashboardButtons, selectedButton);
        }
        else if (kb_Data[7] == kb_Up)
        {
            selectedButton = (selectedButton - 1 + numDashboardButtons) % numDashboardButtons;
            drawButtons(dashboardButtons, numDashboardButtons, selectedButton);
        }
        else if (kb_Data[6] == kb_Enter)
        {
            dashboardButtons[selectedButton].action();
        }
    } while (1);
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
}

void login()
{
    char calcidStr[sizeof(systemInfo->calcid) * 2 + 1];
    for (unsigned int i = 0; i < sizeof(systemInfo->calcid); i++)
    {
        sprintf(calcidStr + i * 2, "%02X", systemInfo->calcid[i]);
    }
    char login_msg[93];
    snprintf(login_msg, sizeof(login_msg), "LOGIN:%s:%s:%s", calcidStr, username, authkey);
    SendSerial(login_msg);
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
        if (strcmp(in_buffer, "bridgeConnected") == 0)
        {
            bridge_connected = true;
            gfx_SetColor(0);
            gfx_FillRectangle(((GFX_LCD_WIDTH - gfx_GetStringWidth("Bridge disconnected!")) / 2), 80, gfx_GetStringWidth("Bridge disconnected!"), 15);
            gfx_SetColor(0);
            gfx_PrintStringXY("Bridge connected!", ((GFX_LCD_WIDTH - gfx_GetStringWidth("Bridge connected!")) / 2), 80);
        }
        if (strcmp(in_buffer, "bridgeDisconnected") == 0)
        {
            bridge_connected = false;
            gfx_SetColor(0);
            gfx_FillRectangle(((GFX_LCD_WIDTH - gfx_GetStringWidth("Bridge disconnected!")) / 2), 80, gfx_GetStringWidth("Bridge disconnected!"), 15);
            gfx_SetColor(0);
            gfx_PrintStringXY("Bridge disconnected!", ((GFX_LCD_WIDTH - gfx_GetStringWidth("Bridge disconnected!")) / 2), 80);
        }

        /* Internet Connected GFX */
        if (strcmp(in_buffer, "SERIAL_CONNECTED_CONFIRMED_BY_SERVER") == 0)
        {
            internet_connected = true;
            gfx_SetColor(0);
            gfx_FillRectangle(((GFX_LCD_WIDTH - gfx_GetStringWidth("Internet disconnected!")) / 2), 110, gfx_GetStringWidth("Internet disconnected!"), 15);
            gfx_SetColor(0);
            gfx_PrintStringXY("Internet connected!", ((GFX_LCD_WIDTH - gfx_GetStringWidth("Internet connected!")) / 2), 110);
        }
        if (strcmp(in_buffer, "internetDisconnected") == 0)
        {
            internet_connected = false;
            gfx_SetColor(0);
            gfx_FillRectangle(((GFX_LCD_WIDTH - gfx_GetStringWidth("Internet disconnected!")) / 2), 110, gfx_GetStringWidth("Internet disconnected!"), 15);
            gfx_SetColor(0);
            gfx_PrintStringXY("Internet disconnected!", ((GFX_LCD_WIDTH - gfx_GetStringWidth("Internet disconnected!")) / 2), 110);
        }

        if (strcmp(in_buffer, "LOGIN_SUCCESS") == 0)
        {
            dashboardScreen();
        }

        if (strcmp(in_buffer, "MAIL_NOT_VERIFIED") == 0)
        {
            printf("Mail not\nverified!");
        }

        if (strcmp(in_buffer, "ALREADY_CONNECTED") == 0)
        {
            alreadyConnectedScreen();
        }

        if (strcmp(in_buffer, "USER_NOT_FOUND") == 0)
        {
            userNotFoundScreen();
        }

        if (startsWith(in_buffer, "ACCOUNT_INFO:"))
        {
            printf("got acc inf");
            accountInfoScreen(in_buffer + strlen("ACCOUNT_INFO:"));
        }

        if (startsWith(in_buffer, "YOUR_IP:"))
        {
            displayIP(in_buffer + strlen("YOUR_IP:"));
        }
        if (startsWith(in_buffer, "CALC_ID_UPDATE_NEEDED"))
        {
            calcIDneedsUpdateScreen();
        }

        if (startsWith(in_buffer, "RTC_CHAT:"))
        {
            char *messageContent = strstr(in_buffer, ":");
            if (messageContent)
            {
                messageContent = strstr(messageContent + 1, ":");
                if (messageContent)
                {
                    messageContent++;
                    addMessage(messageContent, 200 + messageCount * 15);
                    displayMessages();
                }
            }
        }

        if (strcmp(in_buffer, "ESP8266") == 0)
        {
            is_esp8266 = true;
            gfx_PrintStringXY("ESP8266 connected", 5, 232);
        }

        has_unread_data = false;
    }
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

void displayIP(const char *ipAddress)
{
    printf("Received IP address: %s\n", ipAddress);
}

void howToUseScreen()
{
    gfx_ZeroScreen();
    gfx_SetTextScale(2, 2);
    gfx_PrintStringXY("How To TINET", ((GFX_LCD_WIDTH - gfx_GetStringWidth("How To TINET")) / 2), 5);
    gfx_SetTextFGColor(224);
    gfx_PrintStringXY("Press [clear] to quit.", (GFX_LCD_WIDTH - gfx_GetStringWidth("Press [clear] to quit.")) / 2, 35);
    gfx_SetTextFGColor(255);
    gfx_SetTextScale(1, 1);

    gfx_PrintStringXY("https://tinet.tkbstudios.com/", (GFX_LCD_WIDTH - gfx_GetStringWidth("https://tinet.tkbstudios.com/")) / 2, GFX_LCD_HEIGHT / 2);

    do
    {
        kb_Scan();

        usb_HandleEvents();
        if (has_srl_device)
        {
            readSRL();
        }

        if (kb_Data[6] == kb_Clear)
        {
            break;
        }
    } while (1);
}

void alreadyConnectedScreen()
{
    gfx_ZeroScreen();
    gfx_SetTextScale(2, 2);
    gfx_PrintStringXY("Already Connected", ((GFX_LCD_WIDTH - gfx_GetStringWidth("Already Connected")) / 2), 5);
    gfx_SetTextFGColor(224);
    gfx_PrintStringXY("you're already connected to TINET", (GFX_LCD_WIDTH - gfx_GetStringWidth("you're already connected to TINET")) / 2, 35);
    gfx_SetTextFGColor(255);
    gfx_PrintStringXY("Is it not you?", (GFX_LCD_WIDTH - gfx_GetStringWidth("Is it not you?")) / 2, 65);
    gfx_SetTextScale(1, 1);
    gfx_PrintStringXY("Reset your calc key", (GFX_LCD_WIDTH - gfx_GetStringWidth("Reset your calc key")) / 2, 65);
    gfx_PrintStringXY("And log out from everywhere", (GFX_LCD_WIDTH - gfx_GetStringWidth("And log out from everywhere")) / 2, 80);
    gfx_PrintStringXY("on https://tinet.tkbstudios.com/dashboard", (GFX_LCD_WIDTH - gfx_GetStringWidth("https://tinet.tkbstudios.com/dashboard")) / 2, GFX_LCD_HEIGHT / 2);
    do
    {
        kb_Scan();
        if (kb_Data[6] == kb_Clear)
        {
            break;
        }
    } while (1);
}

void userNotFoundScreen()
{
    gfx_ZeroScreen();
    gfx_SetTextScale(2, 2);
    gfx_PrintStringXY("TINET USER NOT FOUND", ((GFX_LCD_WIDTH - gfx_GetStringWidth("TINET USER NOT FOUND")) / 2), 5);
    gfx_SetTextFGColor(224);
    gfx_PrintStringXY("Your user doesn't exist", (GFX_LCD_WIDTH - gfx_GetStringWidth("Your user doesn't exist")) / 2, 35);
    do
    {
        kb_Scan();

        usb_HandleEvents();
        if (has_srl_device)
        {
            readSRL();
        }

        if (kb_Data[6] == kb_Clear)
        {
            break;
        }
    } while (1);
}

void calcIDneedsUpdateScreen()
{
    gfx_ZeroScreen();
    gfx_SetTextScale(2, 2);
    gfx_PrintStringXY("ID UPDATE", ((GFX_LCD_WIDTH - gfx_GetStringWidth("ID UPDATE")) / 2), 5);
    gfx_SetTextFGColor(224);
    gfx_PrintStringXY("calc ID update", (GFX_LCD_WIDTH - gfx_GetStringWidth("calc ID update")) / 2, 35);
    gfx_SetTextScale(1, 1);
    gfx_PrintStringXY("update it on https://tinet.tkbstudios.com/dashboard", (GFX_LCD_WIDTH - gfx_GetStringWidth("update it on https://tinet.tkbstudios.com/dashboard")) / 2, 50);

    if (systemInfo != NULL)
    {
        gfx_PrintStringXY("calcid: ", 10, 70);

        char calcidStr[sizeof(systemInfo->calcid) * 2 + 1];
        for (unsigned int i = 0; i < sizeof(systemInfo->calcid); i++)
        {
            sprintf(calcidStr + i * 2, "%02X", systemInfo->calcid[i]);
        }
        gfx_PrintStringXY(calcidStr, 10 + gfx_GetStringWidth("calcid: "), 70);
    }
    else
    {
        gfx_SetTextScale(2, 2);
        gfx_PrintStringXY("Failed to get system info!", (GFX_LCD_WIDTH - gfx_GetStringWidth("Failed to get system info!")) / 2, GFX_LCD_HEIGHT / 2);
    }
    do
    {
        kb_Scan();

        usb_HandleEvents();
        if (has_srl_device)
        {
            readSRL();
        }

        if (kb_Data[6] == kb_Clear)
        {
            break;
        }
    } while (1);
}

void TINETChatScreen()
{
    gfx_ZeroScreen();
    gfx_SetTextScale(1, 1);
    gfx_SetColor(25);
    gfx_FillRectangle(0, 0, GFX_LCD_WIDTH, 15);
    gfx_PrintStringXY("TINET Chat", TITLE_X_POS, TITLE_Y_POS);

    const char *uppercasechars = "\0\0\0\0\0\0\0\0\0\0\"WRMH\0\0?[VQLG\0\0:ZUPKFC\0 YTOJEB\0\0XSNIDA\0\0\0\0\0\0\0\0";
    const char *lowercasechars = "\0\0\0\0\0\0\0\0\0\0\"wrmh\0\0?[vqlg\0\0:zupkfc\0 ytojeb\0\0xsnida\0\0\0\0\0\0\0\0";
    uint8_t key, i = 0;
    key = os_GetCSC();

    int textX = 20;

    inside_RTC_chat = true;
    bool need_to_send = true;
    bool uppercase = false;

    updateCaseBox(uppercase);

    while (key != sk_Clear)
    {
        char buffer[128] = {0};
        i = 0;

        usb_HandleEvents();

        if (has_srl_device)
        {
            readSRL();
        }

        gfx_SetColor(25);
        gfx_FillRectangle(0, 210, 320, 30);

        char output_buffer[41] = "RTC_CHAT:";

        do
        {
            key = os_GetCSC();
            if (uppercase)
            {
                if (uppercasechars[key])
                {
                    char typedChar[2] = {uppercasechars[key], '\0'};
                    gfx_SetTextScale(2, 2);
                    gfx_PrintStringXY(typedChar, textX, 210 + 5);
                    gfx_SetTextScale(1, 1);
                    textX += gfx_GetStringWidth(typedChar) * 2;
                    buffer[i++] = uppercasechars[key];
                }
            }
            else
            {
                if (lowercasechars[key])
                {
                    char typedChar[2] = {lowercasechars[key], '\0'};
                    gfx_SetTextScale(2, 2);
                    gfx_PrintStringXY(typedChar, textX, 210 + 5);
                    gfx_SetTextScale(1, 1);
                    textX += gfx_GetStringWidth(typedChar) * 2;
                    buffer[i++] = lowercasechars[key];
                }
            }

            if (key == sk_Del && i > 0)
            {
                i--;
                char removedChar = buffer[i];
                textX -= gfx_GetStringWidth(&removedChar) * 2;
                buffer[i] = '\0';
                gfx_SetColor(25);
                gfx_FillRectangle(0, 210, 320, 30);
                gfx_SetTextScale(2, 2);
                gfx_PrintStringXY(buffer, textX, 210 + 5);
                gfx_SetTextScale(1, 1);
            }

            usb_HandleEvents();

            if (has_srl_device)
            {
                readSRL();
            }

            if (key == sk_Clear)
            {
                break;
            }

            if (key == sk_Enter)
            {
                need_to_send = true;
                break;
            }

            if (key == sk_Alpha)
            {
                uppercase = !uppercase;
                updateCaseBox(uppercase);
            }
        } while (1);

        if (strcmp(buffer, "") != 0 && need_to_send == true)
        {
            strcat(output_buffer, buffer);

            SendSerial(output_buffer);
            msleep(100);
            gfx_SetColor(25);
            gfx_FillRectangle(0, 210, 320, 30);
            textX = 20;
            need_to_send = false;
        }
    }

    inside_RTC_chat = false;
}

/*
void displayMessages()
{
    gfx_SetTextScale(1, 1);
    int yOffset = 60;
    int maxTextWidth = 290;
    int lineHeight = 10;

    for (int i = 0; i < messageCount; i++)
    {
        const char *message = messageList[i].message;
        int messageLength = strlen(message);
        int xPos = 20;
        int j = 0;

        while (j < messageLength)
        {
            int substringWidth = gfx_GetStringWidth(&message[j]);

            if (xPos + substringWidth <= maxTextWidth)
            {
                gfx_PrintStringXY(&message[j], xPos, yOffset);
                xPos += substringWidth;
                j++;
            }
            else
            {
                yOffset += lineHeight;
                xPos = 20;
            }
        }
        yOffset += lineHeight;
    }
}
*/

void displayMessages()
{
    gfx_SetTextScale(1, 1);
    gfx_SetTextFGColor(255);
    int yOffset = 60;
    for (int i = 0; i < messageCount; i++)
    {
        gfx_PrintStringXY(messageList[i].message, 20, yOffset);
        yOffset += 10;
    }
}

void addMessage(const char *message, int posY)
{
    if (messageCount >= MAX_MESSAGES)
    {
        for (int i = 0; i < messageCount - 1; i++)
        {
            strcpy(messageList[i].message, messageList[i + 1].message);
            messageList[i].posY = messageList[i + 1].posY;
        }
        messageCount--;
    }

    ChatMessage newMessage;
    strcpy(newMessage.message, message);
    newMessage.posY = posY;
    messageList[messageCount] = newMessage;
    messageCount++;
}

void updateCaseBox(bool isUppercase)
{
    char *boxText = isUppercase ? "U" : "L";
    gfx_SetColor(18);
    gfx_SetTextFGColor(255);
    gfx_FillRectangle(GFX_LCD_WIDTH - gfx_GetStringWidth(boxText) - 5, 0, gfx_GetStringWidth(boxText) + 5, 14);
    gfx_PrintStringXY(boxText, GFX_LCD_WIDTH - gfx_GetStringWidth(boxText) - 5, 4);
}