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

#include "tinet.h"
#include "tice.h"
#include <stdio.h>
#include <keypadc.h>
#include <string.h>

uint8_t previous_kb_Data[8];
uint8_t debounce_delay = 10;

char in_buffer[1024];

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
    const int tinet_init_success = tinet_init();
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

    // TODO: make this work flawlessly and correctly
    printf("waiting for srl device..\n");
    do {
        kb_Update();
        usb_HandleEvents();
        if (has_srl_device) {
            printf("srl device found\n");
            break;
        }
    } while (kb_Data[6] != kb_Clear);

    do {
        const int bytes_received = tinet_read_srl(in_buffer);
        if (bytes_received > 0) {
            has_srl_device = true;
            const int bytes_written = tinet_write_srl("Hello from the new TINET client currently in rewrite!\n");
            printf("wrote %i bytes\n", bytes_written);
        }
        usb_HandleEvents();
        kb_Update();
    } while (kb_Data[6] != kb_Clear);

    /*
    do {
        if (has_srl_device) {
            printf("writing to serial\n");
            const int written = tinet_write_srl("Hello from TINET calc!\n");
            switch (written) {
                case TINET_SRL_WRITE_FAIL:
                    printf("srl write fail\n");
                break;
                case TINET_SUCCESS:
                    printf("write success!\n");
                break;
                default:
                    printf("write scenario not\nimplemented!");
                break;
            }
        } else {
            printf("No serial device!");
        }

        kb_Update();
        usb_HandleEvents();
        msleep(1000);
    } while (kb_Data[6] != kb_Clear);
    */
    usb_Cleanup();
    return 0;
}