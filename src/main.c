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
 * Powerbyte7
 * Log4Jake (Invalid_Jake)
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
#include <ti/info.h>
#include <stdint.h>

#include "gfx/gfx.h"

#include "ui/shapes.h"

#define MAX_CHAT_MESSAGES 15
#define MAX_CHAT_LINE_LENGTH 260

char client_version[4] = "dev";

const system_info_t *systemInfo;

bool inside_RTC_chat = false;

/* DEFINE USER */
bool keyfile_available = false;
char *username;
char *authkey;
uint8_t NetKeyAppVar;

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
void dashboardScreen();
void mailNotVerifiedScreen();
bool startsWith(const char *str, const char *prefix);
void howToUseScreen();
void alreadyConnectedScreen();
void userNotFoundScreen();
void TINETChatScreen();
void accountInfoScreen(const char *accountInfo);
void updateCaseBox(bool isUppercase);
bool kb_Update();
void updateClient();
void clearBuffer(char *buffer);

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

uint8_t previous_kb_Data[8];
uint8_t debounce_delay = 10;

/* DEFINE SPRITES */
gfx_sprite_t *globe_sprite;
gfx_sprite_t *key_sprite;
gfx_sprite_t *bridge_sprite;
gfx_sprite_t *keyboard_sprite;

/* DEFINE EMOJIS */
gfx_sprite_t *blush_sprite;
gfx_sprite_t *cry_sprite;
gfx_sprite_t *dark_sunglasses_sprite;
gfx_sprite_t *dizzy_face_sprite;
gfx_sprite_t *eyeglasses_sprite;
gfx_sprite_t *eyes_sprite;
gfx_sprite_t *flushed_sprite;
gfx_sprite_t *frowning2_sprite;
gfx_sprite_t *grimacing_sprite;
gfx_sprite_t *grin_sprite;
gfx_sprite_t *grinning_sprite;
gfx_sprite_t *heart_eyes_sprite;
gfx_sprite_t *hushed_sprite;
gfx_sprite_t *innocent_sprite;
gfx_sprite_t *joy_sprite;
gfx_sprite_t *kissing_sprite;
gfx_sprite_t *kissing_heart_sprite;
gfx_sprite_t *no_mouth_sprite;
gfx_sprite_t *open_mouth_sprite;
gfx_sprite_t *pensive_sprite;
gfx_sprite_t *poop_sprite;
gfx_sprite_t *rage_sprite;
gfx_sprite_t *rofl_sprite;
gfx_sprite_t *sleeping_sprite;
gfx_sprite_t *slight_smile_sprite;
gfx_sprite_t *smiley_sprite;
gfx_sprite_t *smirk_sprite;
gfx_sprite_t *sob_sprite;
gfx_sprite_t *stuck_out_tongue_sprite;
gfx_sprite_t *stuck_out_tongue_closed_eyes_sprite;
gfx_sprite_t *stuck_out_tongue_winking_eye_sprite;
gfx_sprite_t *sunglasses_sprite;
gfx_sprite_t *sweat_sprite;
gfx_sprite_t *wink_sprite;
gfx_sprite_t *yum_sprite;
gfx_sprite_t *zipper_mouth_sprite;

void alloc_sprites()
{
    globe_sprite = gfx_MallocSprite(globe_width, globe_height);
    key_sprite = gfx_MallocSprite(key_width, key_height);
    bridge_sprite = gfx_MallocSprite(bridge_width, bridge_height);
    keyboard_sprite = gfx_MallocSprite(keyboard_width, keyboard_height);
    blush_sprite = gfx_MallocSprite(blush_width, blush_height);
    cry_sprite = gfx_MallocSprite(cry_width, cry_height);
    dark_sunglasses_sprite = gfx_MallocSprite(dark_sunglasses_width, dark_sunglasses_height);
    dizzy_face_sprite = gfx_MallocSprite(dizzy_face_width, dizzy_face_height);
    eyeglasses_sprite = gfx_MallocSprite(eyeglasses_width, eyeglasses_height);
    eyes_sprite = gfx_MallocSprite(eyes_width, eyes_height);
    flushed_sprite = gfx_MallocSprite(flushed_width, flushed_height);
    frowning2_sprite = gfx_MallocSprite(frowning2_width, frowning2_height);
    grimacing_sprite = gfx_MallocSprite(grimacing_width, grimacing_height);
    grin_sprite = gfx_MallocSprite(grin_width, grin_height);
    grinning_sprite = gfx_MallocSprite(grinning_width, grinning_height);
    heart_eyes_sprite = gfx_MallocSprite(heart_eyes_width, heart_eyes_height);
    hushed_sprite = gfx_MallocSprite(hushed_width, hushed_height);
    innocent_sprite = gfx_MallocSprite(innocent_width, innocent_height);
    joy_sprite = gfx_MallocSprite(joy_width, joy_height);
    kissing_sprite = gfx_MallocSprite(kissing_width, kissing_height);
    kissing_heart_sprite = gfx_MallocSprite(kissing_heart_width, kissing_heart_height);
    no_mouth_sprite = gfx_MallocSprite(no_mouth_width, no_mouth_height);
    open_mouth_sprite = gfx_MallocSprite(open_mouth_width, open_mouth_height);
    pensive_sprite = gfx_MallocSprite(pensive_width, pensive_height);
    poop_sprite = gfx_MallocSprite(poop_width, poop_height);
    rage_sprite = gfx_MallocSprite(rage_width, rage_height);
    rofl_sprite = gfx_MallocSprite(rofl_width, rofl_height);
    sleeping_sprite = gfx_MallocSprite(sleeping_width, sleeping_height);
    slight_smile_sprite = gfx_MallocSprite(slight_smile_width, slight_smile_height);
    smiley_sprite = gfx_MallocSprite(smiley_width, smiley_height);
    smirk_sprite = gfx_MallocSprite(smirk_width, smirk_height);
    sob_sprite = gfx_MallocSprite(sob_width, sob_height);
    stuck_out_tongue_sprite = gfx_MallocSprite(stuck_out_tongue_width, stuck_out_tongue_height);
    stuck_out_tongue_closed_eyes_sprite = gfx_MallocSprite(stuck_out_tongue_closed_eyes_width, stuck_out_tongue_closed_eyes_height);
    stuck_out_tongue_winking_eye_sprite = gfx_MallocSprite(stuck_out_tongue_winking_eye_width, stuck_out_tongue_winking_eye_height);
    sunglasses_sprite = gfx_MallocSprite(sunglasses_width, sunglasses_height);
    sweat_sprite = gfx_MallocSprite(sweat_width, sweat_height);
    wink_sprite = gfx_MallocSprite(wink_width, wink_height);
    yum_sprite = gfx_MallocSprite(yum_width, yum_height);
    zipper_mouth_sprite = gfx_MallocSprite(zipper_mouth_width, zipper_mouth_height);
}

void decompress_sprites()
{
    zx0_Decompress(globe_sprite, globe_compressed);
    zx0_Decompress(key_sprite, key_compressed);
    zx0_Decompress(bridge_sprite, bridge_compressed);
    zx0_Decompress(keyboard_sprite, keyboard_compressed);
    zx0_Decompress(blush_sprite, blush_compressed);
    zx0_Decompress(cry_sprite, cry_compressed);
    zx0_Decompress(dark_sunglasses_sprite, dark_sunglasses_compressed);
    zx0_Decompress(dizzy_face_sprite, dizzy_face_compressed);
    zx0_Decompress(eyeglasses_sprite, eyeglasses_compressed);
    zx0_Decompress(eyes_sprite, eyes_compressed);
    zx0_Decompress(flushed_sprite, flushed_compressed);
    zx0_Decompress(frowning2_sprite, frowning2_compressed);
    zx0_Decompress(grimacing_sprite, grimacing_compressed);
    zx0_Decompress(grin_sprite, grin_compressed);
    zx0_Decompress(grinning_sprite, grinning_compressed);
    zx0_Decompress(heart_eyes_sprite, heart_eyes_compressed);
    zx0_Decompress(hushed_sprite, hushed_compressed);
    zx0_Decompress(innocent_sprite, innocent_compressed);
    zx0_Decompress(joy_sprite, joy_compressed);
    zx0_Decompress(kissing_sprite, kissing_compressed);
    zx0_Decompress(kissing_heart_sprite, kissing_heart_compressed);
    zx0_Decompress(no_mouth_sprite, no_mouth_compressed);
    zx0_Decompress(open_mouth_sprite, open_mouth_compressed);
    zx0_Decompress(pensive_sprite, pensive_compressed);
    zx0_Decompress(poop_sprite, poop_compressed);
    zx0_Decompress(rage_sprite, rage_compressed);
    zx0_Decompress(rofl_sprite, rofl_compressed);
    zx0_Decompress(sleeping_sprite, sleeping_compressed);
    zx0_Decompress(slight_smile_sprite, slight_smile_compressed);
    zx0_Decompress(smiley_sprite, smiley_compressed);
    zx0_Decompress(smirk_sprite, smirk_compressed);
    zx0_Decompress(sob_sprite, sob_compressed);
    zx0_Decompress(stuck_out_tongue_sprite, stuck_out_tongue_compressed);
    zx0_Decompress(stuck_out_tongue_closed_eyes_sprite, stuck_out_tongue_closed_eyes_compressed);
    zx0_Decompress(stuck_out_tongue_winking_eye_sprite, stuck_out_tongue_winking_eye_compressed);
    zx0_Decompress(sunglasses_sprite, sunglasses_compressed);
    zx0_Decompress(sweat_sprite, sweat_compressed);
    zx0_Decompress(wink_sprite, wink_compressed);
    zx0_Decompress(yum_sprite, yum_compressed);
    zx0_Decompress(zipper_mouth_sprite, zipper_mouth_compressed);
}

void free_sprites()
{
    free(globe_sprite);
    free(key_sprite);
    free(bridge_sprite);
    free(keyboard_sprite);
    free(blush_sprite);
    free(cry_sprite);
    free(dark_sunglasses_sprite);
    free(dizzy_face_sprite);
    free(eyeglasses_sprite);
    free(eyes_sprite);
    free(flushed_sprite);
    free(frowning2_sprite);
    free(grimacing_sprite);
    free(grin_sprite);
    free(grinning_sprite);
    free(heart_eyes_sprite);
    free(hushed_sprite);
    free(innocent_sprite);
    free(joy_sprite);
    free(kissing_sprite);
    free(kissing_heart_sprite);
    free(no_mouth_sprite);
    free(open_mouth_sprite);
    free(pensive_sprite);
    free(poop_sprite);
    free(rage_sprite);
    free(rofl_sprite);
    free(sleeping_sprite);
    free(slight_smile_sprite);
    free(smiley_sprite);
    free(smirk_sprite);
    free(sob_sprite);
    free(stuck_out_tongue_sprite);
    free(stuck_out_tongue_closed_eyes_sprite);
    free(stuck_out_tongue_winking_eye_sprite);
    free(sunglasses_sprite);
    free(sweat_sprite);
    free(wink_sprite);
    free(yum_sprite);
    free(zipper_mouth_sprite);
}

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

/* Updates kb_Data and keeps track of previous keypresses, returns true if changes were detected */
bool kb_Update()
{
    // Update previous input state
    for (uint8_t i = 0; i <= 7; i++)
    {
        previous_kb_Data[i] = kb_Data[i];
    }

    kb_Scan();

    // Determine whether input has changed
    for (uint8_t i = 0; i <= 7; i++)
    {
        if (previous_kb_Data[i] != kb_Data[i])
        {
            return true;
        }
    }

    return false;
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
}

void BucketsButtonPressed()
{
    printf("Buckets btn press\n");
    msleep(500);
}

Button dashboardButtons[] = {
    /*
    button struct: {xpostopleftcorner, ypostopleftcorner, width, height, "text", function}
    Spacing the buttons vertically with 10px is the perfect layout.
    */
    {GFX_LCD_WIDTH / 2 - 120, 60, 120, 30, "TINET Chat", TINETChatScreen},
    {GFX_LCD_WIDTH / 2 - 120, 100, 120, 30, "Update TINET", updateClient}
};

int numDashboardButtons = sizeof(dashboardButtons) / sizeof(dashboardButtons[0]);

void loginButtonPressed()
{
    if (!USB_connected && !USB_connecting && bridge_connected)
    {
        USB_connecting = true;
        login();
    }
}

Button mainMenuButtons[] = {
    // Button properties: X (top left corner), Y (top left corner), width, height, label, function
    {103, 130, 114, 30, "Login", loginButtonPressed},
    {103, 165, 114, 30, "Help", howToUseScreen}};

int numMainMenuButtons = sizeof(mainMenuButtons) / sizeof(mainMenuButtons[0]);

void drawButtons(Button *buttons, int numButtons, int selectedButton)
{
    for (int i = 0; i < numButtons; i++)
    {
        if (i == selectedButton)
        {
            shapes_RoundRectangleFill(255, 10, buttons[i].width, buttons[i].height, buttons[i].x, buttons[i].y);
            shapes_RoundRectangleFill(49, 10, buttons[i].width - 2, buttons[i].height - 2, buttons[i].x + 1, buttons[i].y + 1);
        }
        else
        {
            shapes_RoundRectangleFill(49, 10, buttons[i].width, buttons[i].height, buttons[i].x, buttons[i].y);
        }
        gfx_PrintStringXY(buttons[i].label, buttons[i].x + ((buttons[i].width / 2) - (gfx_GetStringWidth(buttons[i].label)) / 2), buttons[i].y + 10);
    }
    msleep(250);
}

/* DEFINE CHAT */
void printWrappedText(const char *text, int x, int y);
void displayMessages();
void addMessage(const char *message, int posY);

typedef struct
{
    char recipient[19];
    int timestamp;
    char message[200];
    int posY;
} ChatMessage;

ChatMessage messageList[MAX_CHAT_MESSAGES];
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

    // load sprites
    alloc_sprites();
    decompress_sprites();

    // Display main menu
    gfx_ZeroScreen();
    gfx_SetTextScale(1, 2);
    gfx_SetColor(49);
    shapes_RoundRectangleFill(16, 20, 147, 165, 87, 54);
    shapes_RoundRectangleFill(49, 20, 114, 62, 101, 63);
    gfx_SetTextFGColor(255);
    gfx_PrintStringXY("Username:", (GFX_LCD_WIDTH - gfx_GetStringWidth("Username:")) / 2, 68);
    gfx_SetColor(16);
    gfx_FillRectangle(0, 0, GFX_LCD_WIDTH, 23);
    gfx_SetTextScale(2, 2);
    gfx_PrintStringXY("TINET", (GFX_LCD_WIDTH - gfx_GetStringWidth("TINET")) / 2, 5);
    gfx_SetTextScale(1, 1);
    gfx_SetTextFGColor(224);
    //gfx_PrintStringXY("Press [clear] to quit.", (GFX_LCD_WIDTH - gfx_GetStringWidth("Press [clear] to quit.")) / 2, 35);
    gfx_SetTextFGColor(255);

    /* CREATE CALCID */
    char calcidStr[sizeof(systemInfo->calcid) * 2 + 1];
    for (unsigned int i = 0; i < sizeof(systemInfo->calcid); i++)
    {
        sprintf(calcidStr + i * 2, "%02X", systemInfo->calcid[i]);
    }

    /* DISPLAY CLIENT VERSION BOTTOM RIGHT */
    gfx_PrintStringXY(client_version, 320 - gfx_GetStringWidth(client_version), 232);

    /* CALC ID DISPLAY CENTER */
    gfx_PrintStringXY(calcidStr, (GFX_LCD_WIDTH - gfx_GetStringWidth(calcidStr)) / 2, 205);

    // Open and read NetKeyAppVar data
    NetKeyAppVar = ti_Open("NETKEY", "r");
    if (NetKeyAppVar == 0)
    {
        keyfile_available = false;
        NoKeyFileGFX();
    }
    else
    {
        // Read and process NetKeyAppVar data
        read_flen = ti_GetSize(NetKeyAppVar);
        uint8_t *data_ptr = (uint8_t *)ti_GetDataPtr(NetKeyAppVar);
        ti_Close(NetKeyAppVar);

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
        kb_Update();

        usb_HandleEvents();
        if (has_srl_device)
        {
            readSRL();
        }

        if (has_srl_device && bridge_connected && !serial_init_data_sent)
        {
            SendSerial("SERIAL_CONNECTED");
            serial_init_data_sent = true;
        }

        if (kb_Data[7] == kb_Down && previous_kb_Data[7] != kb_Down)
        {
            selectedButton = (selectedButton + 1) % numMainMenuButtons;
            drawButtons(mainMenuButtons, numMainMenuButtons, selectedButton);
        }
        else if (kb_Data[7] == kb_Up && previous_kb_Data[7] != kb_Up)
        {
            selectedButton = (selectedButton - 1 + numMainMenuButtons) % numMainMenuButtons;
            drawButtons(mainMenuButtons, numMainMenuButtons, selectedButton);
        }
        else if (kb_Data[6] == kb_Enter && previous_kb_Data[6] != kb_Enter)
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
    gfx_PrintStringXY("MB Used Total: ", 1, y);
    gfx_PrintString(infoTokens[1]);
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
        kb_Update();

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
        kb_Update();
        usb_HandleEvents();
        if (has_srl_device)
        {
            readSRL();
        }

        if (kb_Data[6] == kb_Clear)
        {
            break;
        }

        if (kb_Data[7] == kb_Down && previous_kb_Data[7] != kb_Down)
        {
            selectedButton = (selectedButton + 1) % numDashboardButtons;
            drawButtons(dashboardButtons, numDashboardButtons, selectedButton);
        }
        else if (kb_Data[7] == kb_Up && previous_kb_Data[7] != kb_Up)
        {
            selectedButton = (selectedButton - 1 + numDashboardButtons) % numDashboardButtons;
            drawButtons(dashboardButtons, numDashboardButtons, selectedButton);
        }
        else if (kb_Data[6] == kb_Enter && previous_kb_Data[6] != kb_Enter)
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

    gfx_PrintStringXY("Waiting for bridge..", ((GFX_LCD_WIDTH - gfx_GetStringWidth("Waiting for bridge..")) / 2), 65);
}

void NoKeyFileGFX()
{
    //gfx_PrintStringXY("Please first add your keyfile!!", ((GFX_LCD_WIDTH - gfx_GetStringWidth("Please first add your keyfile!!")) / 2), 90);
    //gfx_PrintStringXY("https://tinet.tkbstudios.com/login", ((GFX_LCD_WIDTH - gfx_GetStringWidth("https://tinet.tkbstudios.com/login")) / 2), 100);
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
        printf("SRL 0B");
    }
    else if (bytes_read > 0)
    {
        in_buffer[bytes_read] = '\0';
        has_unread_data = true;

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

        if (strcmp(in_buffer, "SERIAL_CONNECTED_CONFIRMED_BY_SERVER") == 0)
        {
            internet_connected = true;
            gfx_SetColor(0);
            gfx_FillRectangle(((GFX_LCD_WIDTH - gfx_GetStringWidth("Internet disconnected!")) / 2), 110, gfx_GetStringWidth("Internet disconnected!"), 15);
            gfx_SetColor(0);
            gfx_PrintStringXY("Internet connected!", ((GFX_LCD_WIDTH - gfx_GetStringWidth("Internet connected!")) / 2), 110);
            gfx_FillRectangle(0, 65, GFX_LCD_WIDTH, 12);
            gfx_PrintStringXY("Press [enter] to connect!", ((GFX_LCD_WIDTH - gfx_GetStringWidth("Press [enter] to connect!")) / 2), 65);
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

        if (strcmp(in_buffer, "INVALID_CALC_KEY") == 0)
        {
            printf("Invalid\ncalc key");
        }

        if (strcmp(in_buffer, "DIFFERENT_CALC_ID") == 0)
        {
            printf("Different\ncalc ID");
        }

        if (strcmp(in_buffer, "CALC_BANNED") == 0)
        {
            printf("Your calc\nis banned.");
        }

        if (startsWith(in_buffer, "ACCOUNT_INFO:"))
        {
            printf("got acc inf");
            accountInfoScreen(in_buffer + strlen("ACCOUNT_INFO:"));
        }

        if (startsWith(in_buffer, "YOUR_IP:"))
        {
            printf("Received IP address: %s\n", in_buffer + strlen("YOUR_IP:"));
        }

        if (startsWith(in_buffer, "RTC_CHAT:"))
        {
            if (inside_RTC_chat)
            {
                char *messageContent = strstr(in_buffer, ":");
                
                if (messageContent)
                {
                    messageContent = strstr(messageContent + 1, ":");
                    if (messageContent)
                    {
                        messageContent++;
                        messageContent++;
                        addMessage(messageContent, 200 + messageCount * 15);
                        displayMessages();
                    }
                }
            }
        }

        clearBuffer(in_buffer);
        has_unread_data = false;
    }
}

void quitProgram()
{
    gfx_End();
    usb_Cleanup();
    exit(0);
}

bool startsWith(const char *str, const char *prefix)
{
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

void howToUseScreen()
{
    free(globe_sprite);
    free(bridge_sprite);
    free(key_sprite);

    gfx_ZeroScreen();
    gfx_FillRectangle(0, 0, GFX_LCD_WIDTH, 23);
    gfx_SetTextScale(2, 2);
    gfx_PrintStringXY("How To TINET", ((GFX_LCD_WIDTH - gfx_GetStringWidth("How To TINET")) / 2), 5);
    gfx_SetTextScale(1, 1);
    shapes_RoundRectangleFill(49, 20, 284, 165, 18, 38);
    gfx_SetTextFGColor(224);
    gfx_PrintStringXY("Press [clear] to quit.", (GFX_LCD_WIDTH - gfx_GetStringWidth("Press [clear] to quit.")) / 2, 45);
    gfx_SetTextFGColor(255);
    gfx_SetTextScale(1, 1);
    gfx_PrintStringXY("Please visit", (GFX_LCD_WIDTH - gfx_GetStringWidth("Please visit")) / 2, GFX_LCD_HEIGHT / 2 - 10);
    gfx_PrintStringXY("https://tinetdocs.tkbstudios.com/", (GFX_LCD_WIDTH - gfx_GetStringWidth("https://tinetdocs.tkbstudios.com/")) / 2, GFX_LCD_HEIGHT / 2);

    do
    {
        kb_Update();

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
        kb_Update();
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
        kb_Update();

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
    const char *uppercasechars = "\0\0\0\0\0\0\0\0\0\0\"WRMH\0\0?[VQLG\0\0:ZUPKFC\0 YTOJEB\0\0XSNIDA\0\0\0\0\0\0\0\0";
    const char *lowercasechars = "\0\0\0\0\0\0\0\0\0\0\"wrmh\0\0?[vqlg\0\0:zupkfc\0 ytojeb\0\0xsnida\0\0\0\0\0\0\0\0";
    uint8_t key, i = 0;
    int textX = 10;
    int textY = 195;
    inside_RTC_chat = true;
    bool need_to_send = true;
    bool uppercase = false;
    bool emojiMethod = false;

    typedef struct
    {
        const char *sequence;
        gfx_sprite_t *sprite;
    } EmojiSpriteEntry;

    EmojiSpriteEntry emojiSpriteTable[] = {
        {":kb:", keyboard_sprite}, // pointer to emoji
    };

    /* DRAW SCREEN */
    gfx_ZeroScreen();
    gfx_SetTextScale(1, 1);
    gfx_SetColor(49);
    gfx_FillRectangle(0, 0, GFX_LCD_WIDTH, 20);
    gfx_PrintStringXY("TINET Chat", TITLE_X_POS, TITLE_Y_POS);
    updateCaseBox(uppercase);

    key = os_GetCSC();
    while (key != sk_Clear)
    {
        char buffer[128] = {0};
        i = 0;

        usb_HandleEvents();

        if (has_srl_device)
        {
            readSRL();
        }

        gfx_SetColor(49);
        gfx_FillRectangle(0, 190, 320, 50);

        char output_buffer[48] = "RTC_CHAT:global:";

        do
        {
            key = os_GetCSC();
            if (!emojiMethod)
            {
                const char *charSet = (uppercase) ? uppercasechars : lowercasechars;
                char typedChar = charSet[key];

                if (typedChar && key != 0)
                {
                    // printf("%s\n\n", &typedChar);
                    // printf("%c\n", typedChar);
                    gfx_SetTextScale(1, 1);
                    if (textX + gfx_GetCharWidth(typedChar) > MAX_CHAT_LINE_LENGTH)
                    {
                        textX = 10;
                        textY = textY + 10;
                    }
                    gfx_SetTextXY(textX, textY);
                    gfx_PrintChar(typedChar);
                    textX += gfx_GetCharWidth(typedChar);
                    buffer[i++] = typedChar;
                }
            }
            else
            {
                for (size_t j = 0; j < sizeof(emojiSpriteTable) / sizeof(emojiSpriteTable[0]); j++)
                {
                    if (strncmp(emojiSpriteTable[j].sequence, buffer + i, strlen(emojiSpriteTable[j].sequence)) == 0)
                    {
                        gfx_Sprite(emojiSpriteTable[j].sprite, textX, 210);
                        textX += emojiSpriteTable[j].sprite->width;
                        i += strlen(emojiSpriteTable[j].sequence);
                        break;
                    }
                }
            }

            if (key == sk_Del && i > 0)
            {
                i--;
                char removedChar = buffer[i];
                textX -= gfx_GetCharWidth(removedChar);
                buffer[i] = '\0';
                gfx_SetColor(45);
                gfx_FillRectangle(0, 190, 320, 50);
                gfx_PrintStringXY(buffer, 10, textY);
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

            if (key == sk_Mode)
            {
                emojiMethod = !emojiMethod;
                gfx_FillRectangle(GFX_LCD_WIDTH - keyboard_width - 5, GFX_LCD_HEIGHT - keyboard_height - 5, keyboard_width, keyboard_height);
                if (emojiMethod == true)
                {
                    gfx_Sprite(keyboard_sprite, GFX_LCD_WIDTH - keyboard_width - 5, GFX_LCD_HEIGHT - keyboard_height - 5);
                }
                else
                {
                    gfx_Sprite(keyboard_sprite, GFX_LCD_WIDTH - keyboard_width - 5, GFX_LCD_HEIGHT - keyboard_height - 5);
                }
            }
        } while (1);

        if (strcmp(buffer, "") != 0 && need_to_send == true)
        {
            strcat(output_buffer, buffer);

            SendSerial(output_buffer);
            msleep(100);
            gfx_SetColor(49);
            gfx_FillRectangle(0, 210, 320, 30);
            textX = 10;
            textY = 195;
            need_to_send = false;
        }
    }

    inside_RTC_chat = false;
    free(keyboard_sprite);
}

void displayMessages()
{
    gfx_SetTextScale(1, 1);
    gfx_SetTextFGColor(255);
    int yOffset = 25;
    gfx_SetColor(0);
    gfx_FillRectangle(0, 20, GFX_LCD_WIDTH, 170);
    for (int i = 0; i < messageCount; i++)
    {
        char* message = messageList[i].message;
        int messageLength = strlen(message);
        char buffer[300];
        buffer[0] = '\0';
        int lineWidth = 0;

        for (int j = 0; j < messageLength; j++)
        {
            char toAdd[2];
            sprintf(toAdd, "%c", message[j]);
            strcat(buffer, toAdd);
            
            int currentLineWidth = gfx_GetStringWidth(buffer);
            
            if (currentLineWidth > MAX_CHAT_LINE_LENGTH)
            {
                gfx_PrintStringXY(buffer, 10 + lineWidth, yOffset);
                yOffset += 10;
                lineWidth = 0;
                buffer[0] = '\0';
            }
        }

        if (buffer[0] != '\0') 
        {
            gfx_PrintStringXY(buffer, 10, yOffset);
            yOffset += 20;
        }
    }
}

void addMessage(const char *message, int posY)
{
    if (messageCount >= MAX_CHAT_MESSAGES)
    {
        for (int i = 0; i < messageCount - 1; i++)
        {
            strncpy(messageList[i].recipient, messageList[i + 1].recipient, sizeof(messageList[i].recipient) - 1);
            strncpy(messageList[i].message, messageList[i + 1].message, sizeof(messageList[i].message) - 1);
            messageList[i].recipient[sizeof(messageList[i].recipient) - 1] = '\0';
            messageList[i].message[sizeof(messageList[i].message) - 1] = '\0';
            messageList[i].posY = messageList[i + 1].posY;
        }
        messageCount--;
    }

    ChatMessage newMessage;
    strncpy(newMessage.message, message, sizeof(newMessage.message) - 1);
    newMessage.recipient[sizeof(newMessage.recipient) - 1] = '\0';
    newMessage.message[sizeof(newMessage.message) - 1] = '\0';
    newMessage.posY = posY;
    messageList[messageCount] = newMessage;
    messageCount++;
}

void updateCaseBox(bool isUppercase)
{
    char *boxText = isUppercase ? "UC" : "lc";
    gfx_SetColor(49);
    gfx_SetTextFGColor(255);
    gfx_FillRectangle(GFX_LCD_WIDTH - gfx_GetStringWidth("UC") - 5, 0, gfx_GetStringWidth("UC") + 5, 14);
    gfx_PrintStringXY(boxText, GFX_LCD_WIDTH - gfx_GetStringWidth("UC") - 5, 4);
}

void updateClient()
{
    ti_var_t update_var;
    bool isText = true;
    char update_in_buffer[512];
    size_t update_in_buffer_size = 0;

    printf("uds1\n");
    SendSerial("UPDATE_CLIENT:dev");

    printf("uds2\n");
    update_var = ti_OpenVar("NETNEW", "w", OS_TYPE_PRGM);
    if (!update_var) {
        printf("Failed to open variable\n");
        return;
    }

    while (true) {
        usb_HandleEvents();
        update_in_buffer_size = srl_Read(&srl, update_in_buffer, sizeof update_in_buffer);

        if (update_in_buffer_size == 0) {
            continue;
        }

        if (strncmp(update_in_buffer, "UPDATE_DONE", 11) == 0) {
            break;
        }

        printf("uds3\n");
        for (size_t i = 0; i < update_in_buffer_size; i++) {
            if (update_in_buffer[i] < 32 || update_in_buffer[i] > 126) {
                isText = false;
                break;
            }
        }

        if (!isText) {
            printf("udsa\n");
            ti_Write(update_in_buffer, update_in_buffer_size, 512, update_var);
            printf("written\n");
            SendSerial("UPDATE_CONTINUE");
        }
    }
    printf("updated");

    ti_SetArchiveStatus(update_var, true);
    ti_Close(update_var);
}

void clearBuffer(char *buffer) {
    for (int i = 0; buffer[i] != '\0'; i++) {
        buffer[i] = 0;
    }
}
