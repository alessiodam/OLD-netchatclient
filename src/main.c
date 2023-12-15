/* CODE IN REWRITE */
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
 * Roccolox Programs
 *--------------------------------------
*/

#include <stdio.h>
#include <keypadc.h>
#include <stdlib.h>
#include <string.h>

#include "tice.h"
#include "tinet-lib/tinet.h"
#include "utils/textutils/textutils.h"

uint8_t previous_kb_Data[8];
uint8_t debounce_delay = 10;

char in_buffer[4096];

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

// TODO: REWRITE COMPLETE CHAT SYSTEM
void showChatMessage() {
    const char *remaining_str = in_buffer + 10;

    const char* delimiter = strchr(remaining_str, ':');
    if (!delimiter) return;
    int index = delimiter - remaining_str;
    char *recipient = malloc(index + 1);
    strncpy(recipient, remaining_str, index);
    recipient[index] = '\0';
    remaining_str = delimiter + 1;

    delimiter = strchr(remaining_str, ':');
    if (!delimiter) return;
    index = delimiter - remaining_str;
    char *timestamp_str = malloc(index + 1);
    strncpy(timestamp_str, remaining_str, index);
    timestamp_str[index] = '\0';
    remaining_str = delimiter + 1;

    delimiter = strchr(remaining_str, ':');
    if (!delimiter) return;
    index = delimiter - remaining_str;
    char *username_str = malloc(index + 1);
    strncpy(username_str, remaining_str, index);
    username_str[index] = '\0';
    remaining_str = delimiter + 1;

    const char *messageContent = strdup(remaining_str);

    if (messageContent)
    {
        char displayMessage[300];
        snprintf(displayMessage, sizeof(displayMessage), "%s: %s", username_str, messageContent);
        printf("%s\n", displayMessage);
    }
}

int main() {
    os_ClrHome();
    const TINET_ReturnCode tinet_init_success = tinet_init();
    switch (tinet_init_success) {
        case TINET_SUCCESS:
            printf("Init success\n");
            const char* username = tinet_get_username();
            printf("%s\n", username);
            break;
        case TINET_NO_KEYFILE:
            printf("No keyfile!\n");
            break;
        case TINET_INVALID_KEYFILE:
            printf("Invalid keyfile!\n");
            break;
        case TINET_SRL_INIT_FAIL:
            printf("SRL init failed!\n");
        default:
            printf("Init case not\nimplemented!\n");
        break;
    }

    msleep(1000);

    printf("waiting for srl device..\n");
    do {
        kb_Update();
        usb_HandleEvents();
        if (has_srl_device) {
            printf("srl device found\n");
            break;
        }
    } while (kb_Data[6] != kb_Clear);

    /* at least 20 seconds timeout during rewrite */
    const TINET_ReturnCode connect_success = tinet_connect(20);
    switch (connect_success) {
        case TINET_SUCCESS:
            printf("Connect success\n");
            break;
        case TINET_TIMEOUT_EXCEEDED:
            printf("Connect timeout exceeded\n");
            break;
        case TINET_TCP_INIT_FAILED:
            printf("TCP init failed\n");
            break;
        default:
            printf("Unhandled connect response\n");
            break;
    }

    do {
        kb_Update();
        const int read_return = tinet_read_srl(in_buffer);
        if (read_return > 0) {
            if (StartsWith(in_buffer, "RTC_CHAT:")) {
                showChatMessage();
            }
        }
    } while (has_srl_device && bridge_connected && kb_Data[6] != kb_Clear);

    usb_Cleanup();
    return 0;
}