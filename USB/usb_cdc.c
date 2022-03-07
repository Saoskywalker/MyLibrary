#include "usb.h"
#include "usb_phy.h"
#include "usb_dev.h"
#include "usb_desc.h"
#include <string.h>

#define	CONFIG_CDC_DESCRIPTOR_LEN	(sizeof(USB_ConfigDescriptor) + \
				 sizeof(USB_InterfaceAssocDescriptor) + \
				 sizeof(USB_InterfaceDescriptor) + \
				 sizeof(USB_CDC_HeaderDescriptor) + \
				 sizeof(USB_CDC_CallMgmtDescriptor) + \
				 sizeof(USB_CDC_AcmDescriptor) + \
				 sizeof(USB_CDC_UnionDescriptor) + \
				 sizeof(USB_EndPointDescriptor) + \
				 sizeof(USB_InterfaceDescriptor) + \
				 sizeof(USB_EndPointDescriptor) * 2)


static struct {
	USB_ConfigDescriptor    				configuration_descriptor;
	USB_InterfaceAssocDescriptor    interface_assoc_descriptor;
	USB_InterfaceDescriptor 				control_interface_descritor;
	USB_CDC_HeaderDescriptor				header_descriptor;
	USB_CDC_CallMgmtDescriptor 			call_mgmt_descriptor;
	USB_CDC_AcmDescriptor 					acm_descriptor;
	USB_CDC_UnionDescriptor 				union_descriptor;
	USB_EndPointDescriptor  				hs_notify_descriptor;
	USB_InterfaceDescriptor 				data_interface_descritor;
	USB_EndPointDescriptor  				endpoint_descriptor[2];
} __attribute__ ((packed)) cdc_confDesc = {
	{
		sizeof(USB_ConfigDescriptor),
		CONFIGURATION_DESCRIPTOR,
		CONFIG_CDC_DESCRIPTOR_LEN,  /*  Total length of the Configuration descriptor */
		0x02,                   /*  NumInterfaces */
		0x02,                   /*  Configuration Value */
		0x04,                   /* Configuration Description String Index */
		0xc0,	// Self Powered, no remote wakeup
		0x32	// Maximum power consumption 500 mA
	},
	{
		sizeof(USB_InterfaceAssocDescriptor),
		INTERFACE_ASSOC_DESCRIPTOR,
		0x00,
		0x02,
		0x02,
		0x02,
		0x01,
		0x00
	},
	{
		sizeof(USB_InterfaceDescriptor),
		INTERFACE_DESCRIPTOR,
		0x00,   /* bInterfaceNumber */
		0x00,   /* bAlternateSetting */
		0x01,	/* ep number */
		0x02,   /* Interface Class */
		0x02,      /* Interface Subclass*/
		0x01,      /* Interface Protocol*/
		0x05    /* Interface Description String Index*/
	},
	{
		sizeof(USB_CDC_HeaderDescriptor),
		USB_DT_CS_INTERFACE,
		0x00,
		0x0110
	},
	{
		sizeof(USB_CDC_CallMgmtDescriptor),
		USB_DT_CS_INTERFACE,
		0x01,
		0x00,
		0x01
	},
	{
		sizeof(USB_CDC_AcmDescriptor),
		USB_DT_CS_INTERFACE,
		0x02,
		0x02
	},
	{
		sizeof(USB_CDC_UnionDescriptor),
		USB_DT_CS_INTERFACE,
		0x06,
		0x00,
		0x01
	},
	{
		sizeof(USB_EndPointDescriptor),
		ENDPOINT_DESCRIPTOR,
		(1 << 7) | 2,	/* endpoint 2 IN */
		3,	/* interrupt */
		10,	/* IN EP FIFO size */
		9	/* Interval */
	},
	{
		sizeof(USB_InterfaceDescriptor),
		INTERFACE_DESCRIPTOR,
		0x01,
		0x00,
		0x02,
		0x0a,
		0x00,
		0x00,
		0x06
	},
	{
		{
			sizeof(USB_EndPointDescriptor),
			ENDPOINT_DESCRIPTOR,
			(1 << 7) | 1,// endpoint 1 IN
			2, /* bulk */
			/* Transfer Type: Bulk;
			 * Synchronization Type: No Synchronization;
			 * Usage Type: Data endpoint
			 */
           	512,
			0
		},
		{
			sizeof(USB_EndPointDescriptor),
			ENDPOINT_DESCRIPTOR,
			(0 << 7) | 1,// endpoint 1 OUT
			2, /* bulk */
			/* Transfer Type: Bulk;
			 * Synchronization Type: No Synchronization;
			 * Usage Type: Data endpoint
			 */
           	512,
			0
		}
	}
};




/*——————————————————————————————————————————————————————————————
返回cdc配置cfg描述符
*/
void usb_cdc_get_cfg_descriptor(USB_DeviceRequest *req_data)
{
	usbprint("usb_hid_get_cfg_descriptor\n");
	switch(req_data->wLength)
	{
		case 9:
			usb_device_write_data_ep_pack(0,  (unsigned char *)&cdc_confDesc, 9);
			break;
		case 8:
			usb_device_write_data_ep_pack(0,  (unsigned char *)&cdc_confDesc, 8);
			break;
		default:
			usb_device_write_data_ep_pack(0,  (unsigned char *)&cdc_confDesc, sizeof(cdc_confDesc));
			break;
	}
}


/*——————————————————————————————————————————————————————————————
usb_cdc_发送数据
*/
int usb_cdc_send_data(unsigned char *buf,int len)
{
    int ret = usb_device_write_data(1, buf, len);
    if (ret != 0) {
        return ret;
    }

    // 长度整除packet_length时多发一个空packet，CDC协议要求的
    if (len % USB_DATA_PACKET_SIZE == 0) {
        ret = usb_device_write_data(1, buf, 0);
    }
    return ret;
}
/*——————————————————————————————————————————————————————————————
usb_cdc_out_ep_callback 可发送大于64长
*/
unsigned char usb_cdc_out_ep_callback(unsigned char *pdat, int *len)
{
	usbprint("com-Len=%d\n", *len);
	usb_cdc_send_data(pdat, *len); //回显
	return 0;
}

unsigned char usb_cdc_in_ep_callback(unsigned char *pdat, int *len)
{
	int i = 0;
	for (; i < usb_rx_buf_len; i++, pdat++)
		*pdat = usb_rx_buf[i];
	*len = usb_rx_buf_len;
	usb_rx_buf_len = 0;
	return 0;
}
