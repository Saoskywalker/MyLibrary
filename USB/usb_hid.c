#include "usb.h"
#include "usb_phy.h"
#include "usb_dev.h"
#include "usb_desc.h"
#include <string.h>

#define CUSTOMHID_SIZ_REPORT_DESC  35//33
		
	
#define	CONFIG_HID_DESCRIPTOR_LEN	(sizeof(USB_ConfigDescriptor) + \
				 sizeof(USB_InterfaceDescriptor) + \
				 sizeof(USB_HID_Descriptor) + \
				 sizeof(USB_EndPointDescriptor) * 2)

struct {
	USB_ConfigDescriptor    configuration_descriptor;
	USB_InterfaceDescriptor hid_control_interface_descritor;
	USB_HID_Descriptor hid_header_descriptor;
	USB_EndPointDescriptor  hid_endpoint_descriptor[2];
} __attribute__ ((packed)) hid_confDesc = {
	{
		sizeof(USB_ConfigDescriptor),
		CONFIGURATION_DESCRIPTOR,
		CONFIG_HID_DESCRIPTOR_LEN,  /*  Total length of the Configuration descriptor */
		0x01,                   /*  NumInterfaces */
		0x01,                   /*  Configuration Value */
		0x00,                   /* Configuration Description String Index */
		0xC0,	// Self Powered, no remote wakeup
		0x32	// Maximum power consumption 500 mA
	},
	{
		sizeof(USB_InterfaceDescriptor),
		INTERFACE_DESCRIPTOR,
		0x00,   /* bInterfaceNumber */
		0x00,   /* bAlternateSetting */
		0x02,	/* ep number */
		0x03,   /* Interface Class */
		0x00,      /* Interface Subclass*/
		0x00,      /* Interface Protocol*/
		0x00    /* Interface Description String Index*/
	},
	{
		sizeof(USB_HID_Descriptor),
		0x21,
		0x0111,
		0x00,
		0x01,
		0x22,
		CUSTOMHID_SIZ_REPORT_DESC,
	},
	{
		{
			sizeof(USB_EndPointDescriptor),
			ENDPOINT_DESCRIPTOR,
			(1 << 7) | 1,// endpoint 3 IN
			3, /* interrupt */
			USB_DATA_PACKET_SIZE, /* IN EP FIFO size */
			1
		},
		{
			sizeof(USB_EndPointDescriptor),
			ENDPOINT_DESCRIPTOR,
			(0 << 7) | 1,// endpoint 3 OUT
			3, /* interrupt */
			USB_DATA_PACKET_SIZE, /* OUT EP FIFO size */
			1
		}
	},
};

const u8 CustomHID_ReportDescriptor[CUSTOMHID_SIZ_REPORT_DESC] =
{
	0x06, 0x06, 0xFF,	/* USAGE_PAGE (Vendor Defined)*/
 	0x09, 0x01, 		/* USAGE (0x01)*/
 	0xA1, 0x01,			/* COLLECTION (Application)*/
 	0x15, 0x00,			/*     LOGICAL_MINIMUM (0)*/
 	0x26, 0xFF, 0x00,	/*     LOGICAL_MAXIMUM (255)*/
 	0x75, 0x08,			/*     REPORT_SIZE (8)*/
 	0x96, 
	USB_DATA_PACKET_SIZE & 0x00FF,           /*     REPORT_COUNT*/
	(USB_DATA_PACKET_SIZE & 0xFF00) >> 8, 	
	0x09, 0x01,
 	0x81, 0x02,			/*     INPUT (Data,Var,Abs)*/
 	0x96,  	
	USB_DATA_PACKET_SIZE & 0x00FF,           /*     REPORT_COUNT*/
	(USB_DATA_PACKET_SIZE & 0xFF00) >> 8, 	 	 	
	0x09, 0x01,
 	0x91, 0x02,			/*   OUTPUT (Data,Var,Abs)*/
 	0x95, 0x08,			/*     REPORT_COUNT (8) */
 	0x09, 0x01,
	0xB1, 0x02,			/*     FEATURE */
 	0xC0				/* END_COLLECTION*/
	
}; /* CustomHID_ReportDescriptor */
/*——————————————————————————————————————————————————————————————
返回HID REPORT_DESCRIPTOR描述符
*/
void usb_get_hid_descport(USB_DeviceRequest *req_data)
{
	usb_device_write_data_ep_pack(0,  (unsigned char *)&CustomHID_ReportDescriptor, CUSTOMHID_SIZ_REPORT_DESC);
}
/*——————————————————————————————————————————————————————————————
返回HID配置cfg描述符
*/
void usb_hid_get_cfg_descriptor(USB_DeviceRequest *req_data)
{
	usbprint("usb_hid_get_cfg_descriptor\n");
	switch(req_data->wLength)
	{
		case 9:
			usb_device_write_data_ep_pack(0,  (unsigned char *)&hid_confDesc, 9);
			break;
		case 8:
			usb_device_write_data_ep_pack(0,  (unsigned char *)&hid_confDesc, 8);
			break;
		default:
			usb_device_write_data_ep_pack(0,  (unsigned char *)&hid_confDesc, sizeof(hid_confDesc));
			break;
	}
}

/*——————————————————————————————————————————————————————————————
usb_hid_in_ep_callback
*/
int usb_hid_in_ep_callback(void)
{
	//usbprint("usb_hid_in_ep_callback\n");
	return 0;
}
/*——————————————————————————————————————————————————————————————
usb_hid_out_ep_callback
*/
void usb_hid_out_ep_callback(unsigned char *pdat,int len)
{
int i=0;
	usbprint("hid-Len=%d\n",len);
	usb_device_write_data(1,pdat,len);//回送	
}


