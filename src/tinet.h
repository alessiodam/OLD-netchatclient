/*
 *--------------------------------------
 * Program Name: TINET system
 * Author: TKB Studios
 * License: Apache License 2.0
 * Description: Allows the user to communicate with the TINET servers
 *--------------------------------------
*/

#ifndef TINET_H
#define TINET_H

#include <srldrvce.h>

#ifdef __cplusplus
extern "C" {
#endif

 extern bool has_srl_device;

 typedef enum {
  TINET_SUCCESS,
  TINET_NO_KEYFILE,
  TINET_INVALID_KEYFILE,
  TINET_SRL_INIT_FAIL,
  TINET_SRL_WRITE_FAIL
 } TINET_ReturnCodes;

 int tinet_init();

 char* tinet_get_username();

 int tinet_connect();

 srl_device_t tinet_get_srl_device();

 int tinet_write_srl(const char *message);

#ifdef __cplusplus
}
#endif

#endif