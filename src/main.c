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
#include "tice.h"
#include "tinet-lib/tinet.h"

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


int main() {
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

    const TINET_ReturnCode connect_success = tinet_connect(10);
    switch (connect_success) {
        case TINET_SUCCESS:
            printf("Connect success\n");
            break;
        case TINET_TIMEOUT_EXCEEDED:
            printf("Connect timeout exceeded\n");
            break;
        default:
            printf("Unhandled connect response\n");
            break;
    }

    do {
        const TINET_ReturnCode read_success = tinet_read_srl(in_buffer);
        if (read_success == TINET_SUCCESS) {
            has_srl_device = true;
            const TINET_ReturnCode write_success = tinet_write_srl(in_buffer);
            switch (write_success) {
                case TINET_SUCCESS:
                    printf("written\n");
                    break;
                case TINET_SRL_WRITE_FAIL:
                    printf("Could not write!\n");
                    break;
                default:
                    printf("Write return not handled\n");
                    break;
            }
        }
        in_buffer[0] = '\0';
        usb_HandleEvents();
        kb_Update();
    } while (kb_Data[6] != kb_Clear);

    usb_Cleanup();
    return 0;
}