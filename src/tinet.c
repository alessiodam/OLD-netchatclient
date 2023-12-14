/*
 *--------------------------------------
 * Program Name: TINET system
 * Author: TKB Studios
 * License: Apache License 2.0
 * Description: Used to communicate with TINET
 *--------------------------------------
*/

#include <tinet.h>
#include <string.h>
#include <fileioc.h>
#include <srldrvce.h>

char *username;
char *authkey;
uint8_t NetKeyAppVar;

srl_device_t srl_device;
uint8_t srl_buf[512];
bool has_srl_device = false;

usb_error_t handle_usb_event(usb_event_t event, void *event_data, usb_callback_data_t *callback_data)
{
 usb_error_t err;
 if ((err = srl_UsbEventCallback(event, event_data, callback_data)) != USB_SUCCESS)
  return err;
 if (event == USB_DEVICE_CONNECTED_EVENT && !(usb_GetRole() & USB_ROLE_DEVICE))
 {
  const usb_device_t device = event_data;
  usb_ResetDevice(device);
 }

 if (event == USB_HOST_CONFIGURE_EVENT || (event == USB_DEVICE_ENABLED_EVENT && !(usb_GetRole() & USB_ROLE_DEVICE)))
 {

  if (has_srl_device) {
   return USB_SUCCESS;
  }

  usb_device_t device;
  if (event == USB_HOST_CONFIGURE_EVENT)
  {
   device = usb_FindDevice(NULL, NULL, USB_SKIP_HUBS);
   if (device == NULL)
    return USB_ERROR_NO_DEVICE;
  }
  else
  {
   has_srl_device = true;
   device = event_data;
  }

  const srl_error_t error = srl_Open(&srl_device, device, srl_buf, sizeof srl_buf, SRL_INTERFACE_ANY, 9600);
  if (error)
  {
   has_srl_device = true;
   return 0;
  }
 }

 if (event == USB_DEVICE_DISCONNECTED_EVENT)
 {
  const usb_device_t device = event_data;
  if (device == srl_device.dev)
  {
   srl_Close(&srl_device);
   has_srl_device = false;
  }
 }
 return USB_SUCCESS;
}

int tinet_init() {
 /* keyfile stuff */
 NetKeyAppVar = ti_Open("NETKEY", "r");
 if (NetKeyAppVar == 0) {
  return TINET_NO_KEYFILE;
 }
 uint8_t *data_ptr = ti_GetDataPtr(NetKeyAppVar);
 ti_Close(NetKeyAppVar);

 char *read_username = (char *)data_ptr;
 username = read_username;

 const size_t read_un_len = strlen(read_username) + 1;
 data_ptr += (read_un_len + 1);

 char *read_key = (char *)data_ptr - 1;
 authkey = read_key;

 /* serial stuff */
 const usb_standard_descriptors_t *usb_desc = srl_GetCDCStandardDescriptors();
 const usb_error_t usb_error = usb_Init(handle_usb_event, NULL, usb_desc, USB_DEFAULT_INIT_FLAGS); // inits USB with default flags
 if (usb_error != USB_SUCCESS)
 {
  return TINET_SRL_INIT_FAIL;
 }
 return TINET_SUCCESS;
}

char* tinet_get_username() {
 return username;
}

int tinet_connect() {
 return TINET_SUCCESS;
}

srl_device_t tinet_get_srl_device() {
 return srl_device;
}

int tinet_write_srl(const char *message) {
 size_t totalBytesWritten = 0;
 const size_t messageLength = strlen(message);

 while (totalBytesWritten < messageLength)
 {
  const int bytesWritten = srl_Write(&srl_device, message + totalBytesWritten, messageLength - totalBytesWritten);

  if (bytesWritten < 0)
  {
   return TINET_SRL_WRITE_FAIL;
  }

  totalBytesWritten += bytesWritten;
 }
 usb_HandleEvents();
 return TINET_SUCCESS;
}
