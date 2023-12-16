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
#include "ti/vars.h"

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

int runProgramCallback() {
    printf("program callback\n");
    sleep(2);
    exit(0);
}

void handle_in_buffer() {
    if (StartsWith(in_buffer, "RTC_CHAT:")) {
        char *pieces[4];
        msleep(200);
        tinet_write_srl(in_buffer);

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

        // Print the pieces
        printf("Recipient: %s\n", pieces[0]);
        printf("Timestamp: %s\n", pieces[1]);
        printf("Username: %s\n", pieces[2]);
        printf("Message: %s\n", pieces[3]);
    }
    if (StartsWith(in_buffer, "RUN_PROGRAM:")) {
        // Ignore "RTC_CHAT:"
        strtok(in_buffer, ":");
        const char* program = strtok(NULL, ":");
        if (program != NULL) {
            os_RunPrgm(program, NULL, 0, runProgramCallback);
        } else {
            printf("invalid program!");
        }
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

    printf("waiting for srl device..\n");
    do {
        kb_Update();
        usb_HandleEvents();
        if (has_srl_device) {
            printf("srl device found\n");
            break;
        }
    } while (kb_Data[6] != kb_Clear);

    const TINET_ReturnCode connect_success = tinet_connect(10);
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

    if (has_srl_device && bridge_connected && tcp_connected) {
        printf("Logging in...\n");
        tinet_login(10);
        printf("Start reading..\n");
        do {
            kb_Update();
            const int read_return = tinet_read_srl(in_buffer);
            if (read_return > 0) {
                handle_in_buffer();
            } else if (read_return < 0) {
                printf("read error\n");
            }
        } while (has_srl_device && bridge_connected && kb_Data[6] != kb_Clear);
    } else if (!has_srl_device) {
        printf("No srl available\n");
    } else if (!bridge_connected) {
        printf("No bridge available\n");
    } else if (!tcp_connected) {
        printf("No TCP server\n");
    }

    printf("quitting..\n");
    sleep(2);

    usb_Cleanup();
    return 0;
}