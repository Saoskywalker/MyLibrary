#ifndef _USB_DESC_H
#define _USB_DESC_H

#include "usb.h"

int usb_device_init(unsigned char type);

void standard_setup_request(USB_DeviceRequest *req_data);
void class_setup_request(USB_DeviceRequest* req_data);
int usb_device_write_data(int ep,unsigned char * databuf,int len);
void usb_setup_handle(unsigned char *dat,int len);

extern char usb_com_open;
void usb_cdc_get_cfg_descriptor(USB_DeviceRequest *req_data);
unsigned char usb_cdc_out_ep_callback(unsigned char *pdat, int *len);
unsigned char usb_cdc_in_ep_callback(unsigned char *pdat, int *len);

void usb_get_hid_descport(USB_DeviceRequest *req_data);
int usb_hid_in_ep_callback(void);
void usb_hid_out_ep_callback(unsigned char *pdat,int len);
void usb_hid_get_cfg_descriptor(USB_DeviceRequest *req_data);

void usb_uvc_get_cfg_descriptor(USB_DeviceRequest *req_data);
int usb_uvc_in_ep_callback(void);
void usb_uvc_out_ep_callback(unsigned char *pdat,int len);
void usb_get_uvc_descport(USB_DeviceRequest *req_data);

#endif
