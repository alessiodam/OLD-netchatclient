/* CODE IN REWRITE */
/*
*--------------------------------------
 * Program Name: TINET Chat
 * Author: TKB Studios
 * License: Apache License 2.0
 * Description: Allows the user to chat on TINET!
 *--------------------------------------
*/

/* When contributing, please add your username in the list here with what you made/edited */
/*
 * Powerbyte7 - kb_Update
*/

/*
 * Dear programmer,
 * when I wrote this code, only God and I
 * knew how it worked.
 * Now, only God knows it!
 *
 * Therefore, if you are trying to optimize
 * this code and it fails (most surely),
 * please increase this counter as
 * a warning for the next person:
 *
 * Total hours wasted here = 5 hours.
*/

#include <stdio.h>
#include <keypadc.h>
#include <stdlib.h>
#include <string.h>
#include <graphx.h>

#include "tice.h"
#include "tinet-lib/tinet.h"
#include "utils/textutils/textutils.h"
#include "asm/scroll.h"

#define MAX_CHAT_MESSAGES 50
#define MESSAGE_Y_POS 180

// tinet needed vars
bool init_success = false;

// key things
uint8_t previous_kb_Data[8];
uint8_t debounce_delay = 10;

// serial things
char in_buffer[512];

// chat variables
typedef struct {
    char recipient[19];
    char timestamp[20];
    char username[19];
    char message[201];
} ChatMessage;

ChatMessage messageList[MAX_CHAT_MESSAGES];
int messageCount = 0;

char recipient[19];
char message[201];

/* UI vars */
typedef struct
{
    const char *string;
} StringList;

StringList recipients[] = {
    {"global"},
    {"tkbstudios"},
    {"Log4Jake"},
};

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

void processNewChatMessage() {
    char *pieces[4];
    // Ignore "RTC_CHAT:"
    strtok(in_buffer, ":");

    // Split the string into 4 pieces
    for (int i = 0; i < 4; i++) {
        char* token = strtok(NULL, ":");
        if (token != NULL) {
            pieces[i] = token;
        } else {
            printf("Invalid message string!.\n");
        }
    }

    ChatMessage new_message;

    strncpy(new_message.recipient, pieces[0], sizeof(new_message.recipient) - 1);
    strncpy(new_message.timestamp, pieces[1], sizeof(new_message.timestamp) - 1);
    strncpy(new_message.username, pieces[2], sizeof(new_message.username) - 1);
    strncpy(new_message.message, pieces[3], sizeof(new_message.message) - 1);

    messageList[messageCount] = new_message;
    messageCount++;

    // 15 is topbar, 35 is textbox (30) + safety value to prevent scrolling up the textbox
    scrollUp(0, 15, GFX_LCD_WIDTH, GFX_LCD_HEIGHT - 15 - 35, 10);
    gfx_SetTextFGColor(25);
    gfx_PrintStringXY(new_message.username, 10, MESSAGE_Y_POS);
    gfx_SetTextFGColor(0);
    gfx_PrintStringXY(": ", 10 + gfx_GetStringWidth(new_message.username), MESSAGE_Y_POS);
    gfx_PrintStringXY(
        new_message.message,
        10 + gfx_GetStringWidth(new_message.username) + gfx_GetStringWidth(": "),
        MESSAGE_Y_POS
    );
}

void updateCaseText(bool isUppercase)
{
    char *boxText = isUppercase ? "UC" : "lc";
    gfx_SetColor(255);
    gfx_SetTextFGColor(0);
    gfx_FillRectangle(GFX_LCD_WIDTH - gfx_GetStringWidth("UC") - 5, 15, gfx_GetStringWidth("UC") + 5, 14);
    gfx_PrintStringXY(boxText, GFX_LCD_WIDTH - gfx_GetStringWidth("UC") - 5, 19);
}

void drawHeader() {
    gfx_PrintStringXY("NETCHAT", 2, 2);
    gfx_PrintStringXY("Made with TINET", GFX_LCD_WIDTH - gfx_GetStringWidth("Made with TINET") - 2, 2);
    gfx_PrintStringXY(recipient, ((GFX_LCD_WIDTH / 2) - gfx_GetStringWidth(recipient)), 2);
    gfx_SetColor(0);
    gfx_HorizLine(0, 15, GFX_LCD_WIDTH);
    gfx_SetColor(255);
}

void chatScreen() {
    gfx_FillScreen(255);
    drawHeader();
    bool isUppercase = false;
    updateCaseText(isUppercase);
    do {
        kb_Update();
        if (kb_Data[6] == kb_Clear) {break;}
        if (kb_Data[6] == kb_Enter) {
            msleep(100);
            // TODO: prompt for a recipient and message
            tinet_send_rtc_message(recipient, message);
            message[0] = '\0';
            msleep(100);
        }
        if (kb_Data[2] == kb_Alpha) {
            isUppercase = !isUppercase;
            updateCaseText(isUppercase);
            msleep(100);
        }
        const int read_return = tinet_read_srl(in_buffer);
        if (read_return > 0) {
            if (StartsWith(in_buffer, "RTC_CHAT:")) {
                processNewChatMessage();
            }
        } else if (read_return < 0) {
            printf("read err\n");
        }
    } while (has_srl_device && bridge_connected && tcp_connected);
}

int main() {
    os_ClrHome();

    /* Setting up screen */
    gfx_Begin();
    gfx_SetColor(255);
    gfx_SetTextFGColor(0);
    gfx_FillScreen(255);

    drawHeader();

    int setup_log_y_pos = 20;

    const TINET_ReturnCode tinet_init_success = tinet_init();
    switch (tinet_init_success) {
        case TINET_SUCCESS:
            gfx_PrintStringXY("Init success", 10, setup_log_y_pos);
            setup_log_y_pos += 10;
            const char* username = tinet_get_username();
            gfx_PrintStringXY("Username: ", 10, setup_log_y_pos);
            gfx_PrintStringXY(username, 10 + gfx_GetStringWidth("Username: "), setup_log_y_pos);
            setup_log_y_pos += 10;
            init_success = true;
            break;
        case TINET_NO_KEYFILE:
            gfx_PrintStringXY("No keyfile!", 10, setup_log_y_pos);
            setup_log_y_pos += 10;
            break;
        case TINET_INVALID_KEYFILE:
            gfx_PrintStringXY("Invalid keyfile!", 10, setup_log_y_pos);
            setup_log_y_pos += 10;
            break;
        case TINET_SRL_INIT_FAIL:
            gfx_PrintStringXY("SRL init failed!", 10, setup_log_y_pos);
            setup_log_y_pos += 10;
        default:
            gfx_PrintStringXY("init case not implemented!", 10, setup_log_y_pos);
            setup_log_y_pos += 10;
        break;
    }
    if (init_success) {
            gfx_PrintStringXY("waiting for serial device..", 10, setup_log_y_pos);
            setup_log_y_pos += 10;
        do {
            kb_Update();
            usb_HandleEvents();
            if (has_srl_device) {
                gfx_PrintStringXY("serial device found!", 10, setup_log_y_pos);
                setup_log_y_pos += 10;
                break;
            }
        } while (kb_Data[6] != kb_Clear);

        gfx_PrintStringXY("Connecting to TINET..", 10, setup_log_y_pos);
        setup_log_y_pos += 10;

        const TINET_ReturnCode connect_success = tinet_connect(10);
        switch (connect_success) {
            case TINET_SUCCESS:
                gfx_PrintStringXY("connect success!", 10, setup_log_y_pos);
                setup_log_y_pos += 10;
                break;
            case TINET_TIMEOUT_EXCEEDED:
                gfx_PrintStringXY("connect timeout exceeded!", 10, setup_log_y_pos);
                setup_log_y_pos += 10;
                break;
            case TINET_TCP_INIT_FAILED:
                gfx_PrintStringXY("TCP init failed!", 10, setup_log_y_pos);
                setup_log_y_pos += 10;
                break;
            default:
                gfx_PrintStringXY("Unhandled connect response!", 10, setup_log_y_pos);
                setup_log_y_pos += 10;
                break;
        }

        if (has_srl_device && bridge_connected && tcp_connected) {
            gfx_PrintStringXY("Logging in...", 10, setup_log_y_pos);
            setup_log_y_pos += 10;
            TINET_ReturnCode login_success = tinet_login(10);
            switch (login_success) {
            case TINET_SUCCESS:
                gfx_PrintStringXY("Logged in!", 10, setup_log_y_pos);
                setup_log_y_pos += 10;
                break;
            case TINET_LOGIN_ALREADY_CONNECTED:
                gfx_PrintStringXY("Already connected!", 10, setup_log_y_pos);
                setup_log_y_pos += 10;
                break;
            default:
                gfx_PrintStringXY("Unhandled login case.", 10, setup_log_y_pos);
                setup_log_y_pos += 10;
                break;
            }
            msleep(500);
            if (login_success != TINET_SUCCESS) {return 1;}

            gfx_FillScreen(255);
            drawHeader();

            do {
                recipient[0] = '\0';
                gfx_FillScreen(255);
                drawHeader();
                gfx_PrintStringXY("select recipient", ((GFX_LCD_WIDTH / 2) - gfx_GetStringWidth("select recipient")), 2);
                int y_position = 30;
                int selection = 0;
                gfx_SetColor(10);
                for (int i = 0; i < (int)(sizeof recipients); i++) {
                    gfx_FillRectangle(50, y_position, gfx_GetStringWidth(recipients[i].string) + 2, 12);
                    gfx_PrintStringXY(recipients[i].string, 52, y_position + 1);
                    y_position += 15;
                }
                gfx_SetColor(255);
                do {
                    kb_Update();
                    if (kb_Data[6] == kb_Enter) {
                        strncpy(recipient, "global", sizeof(recipient));
                        recipient[sizeof(recipient) - 1] = '\0';
                        chatScreen();
                    }
                } while (kb_Data[6] != kb_Clear);
                break;
            } while (has_srl_device && bridge_connected && tcp_connected);
        } else if (!has_srl_device) {
            gfx_PrintStringXY("No srl device", 10, setup_log_y_pos);
            setup_log_y_pos += 10;
        } else if (!bridge_connected) {
            gfx_PrintStringXY("No bridge", 10, setup_log_y_pos);
            setup_log_y_pos += 10;
        } else if (!tcp_connected) {
            gfx_PrintStringXY("No TCP", 10, setup_log_y_pos);
            setup_log_y_pos += 10;
        }
    }

    gfx_ZeroScreen();
    gfx_SetTextScale(2, 2);
    gfx_PrintStringXY("EXITING..", GFX_LCD_WIDTH / 2 - gfx_GetStringWidth("EXITING.."), GFX_LCD_HEIGHT / 2);

    usb_Cleanup();
    gfx_End();
    return 0;
}
