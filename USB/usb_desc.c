#include "usb.h"
#include "usb_dev.h"
#include "usb_desc.h"
#include <string.h>
#include "usb_phy.h"
#include "delay.h"
/*
 * HID class requests
 */
#define HID_REQ_GET_REPORT          0x01
#define HID_REQ_GET_IDLE            0x02
#define HID_REQ_GET_PROTOCOL        0x03
#define HID_REQ_SET_REPORT          0x09
#define HID_REQ_SET_IDLE            0x0A
#define HID_REQ_SET_PROTOCOL        0x0B


USB_DeviceRequest usb_setup_request;


static USB_DeviceDescriptor CDCdevDesc =
{
	sizeof(USB_DeviceDescriptor),
	DEVICE_DESCRIPTOR,	//1
	0x0200,     //Version 2.0
	0x02,//有些电脑不能识别改为CDC-0X2 HID-0X3
	0x00,
	0x00,
	64,	/* Ep0 FIFO size */
	0x3333,
	0x4444,
	0x0200,
	0x01, 	//iManufacturer;
	0x02,   //iProduct;
	0x00,
	0x01
};
static USB_DeviceDescriptor HIDdevDesc =
{
	sizeof(USB_DeviceDescriptor),
	DEVICE_DESCRIPTOR,	//1
	0x0200,     //Version 2.0
	0x03,//有些电脑不能识别改为CDC-0X2 HID-0X3
	0x00,
	0x00,
	64,	/* Ep0 FIFO size */
	0x8888,
	0x1111,
	0x0200,
	0x01, 	//iManufacturer;
	0x02,   //iProduct;
	0x00,
	0x01
};
static USB_DeviceDescriptor UVCdevDesc =
{
	sizeof(USB_DeviceDescriptor),
	DEVICE_DESCRIPTOR,	//1
	0x0200,     //Version 2.0
	0xEF,//有些电脑不能识别改为CDC-0X2 HID-0X3
	0x02,
	0x01,
	64,	/* Ep0 FIFO size */
	0x0416,
	0x9393,
	0x0200,
	0x01, 	//iManufacturer;
	0x02,   //iProduct;
	0x00,
	0x01
};


static unsigned short str_ret[] = {
		0x0326,
		'H','2','7','5','0',' ',' ','U','s','b',' ','D','e','v','i','c','e','s'
};

static unsigned short str_lang[] = {
	0x0304,
	0x0409
};

static unsigned short str_isernum[] = {
		0x033e,
		'H','e','r','o','J','e',' ','S','c','a','n','n','e','r','s',' ','D','r','i','v','e','s',' ','-',' ','H','2','7','5','0'
};

static unsigned short str_config[] = {
		0x031e,
		'C','D','C',' ','A','C','M',' ','c','o','n','f','i','g'
};

static unsigned short str_interface1[] = {
		0x0342,
		'C','D','C',' ','A','b','s','t','r','a','c','t',' ','C','o','n','t','r','o','l',' ','M','o','d','e','l',' ','(','A','C','M',')'
};

static unsigned short str_interface2[] = {
		0x031a,
		'C','D','C',' ','A','C','M',' ','D','a','t','a'
};

USB_CDC_LineCoding port_line_coding = {
		.dwDTERate = 115200,
		.bCharFormat = 0,
		.bParityType = 0,
		.bDataBits = 8
};
unsigned int port_line_coding_flag;
void set_line_codingstatic_ext(unsigned char * pdata)
{
			port_line_coding_flag = 0;
			port_line_coding.dwDTERate = *(volatile unsigned int *)(pdata);
			port_line_coding.bCharFormat = *(volatile unsigned char *)(pdata+4);
			port_line_coding.bParityType = *(volatile unsigned char *)(pdata+5);
			port_line_coding.bDataBits = *(volatile unsigned char *)(pdata+6);
}
/*——————————————————————————————————————————————————————————————
返回设备device描述符
*/
static void usb_get_device_descriptor(USB_DeviceRequest *req_data)
{
	unsigned int len = req_data->wLength;
	unsigned char *devDesc;	
	if(current_usb_type==USB_TYPE_USB_UVC)devDesc=(unsigned char *)&UVCdevDesc;
	else if(current_usb_type==USB_TYPE_USB_HID)devDesc=(unsigned char *)&HIDdevDesc;
	else if(current_usb_type==USB_TYPE_USB_COM)devDesc=(unsigned char *)&CDCdevDesc;
	else return;
	
	usbprint("usb_get_device_descriptor\n");
	if (len < sizeof(USB_DeviceDescriptor))
	{
		usb_device_write_data_ep_pack(0,devDesc, len);
	}
	else
	{
		usb_device_write_data_ep_pack(0,devDesc,sizeof(USB_DeviceDescriptor));
	}
}
/*——————————————————————————————————————————————————————————————
返回配置cfg描述符
*/
static void usb_get_cfg_descriptor(USB_DeviceRequest *req_data)
{     
		 if(current_usb_type==USB_TYPE_USB_COM)usb_cdc_get_cfg_descriptor(req_data);
	// else if(current_usb_type==USB_TYPE_USB_HID)usb_hid_get_cfg_descriptor(req_data);
	// else if(current_usb_type==USB_TYPE_USB_UVC)usb_uvc_get_cfg_descriptor(req_data);	
}
/*——————————————————————————————————————————————————————————————
返回设备字符串
*/
static inline void udp_get_dev_descriptor_string(USB_DeviceRequest *dreq)
{
	int len = dreq->wLength;
	int index = dreq->wValue & 0xff;
	usbprint("udp_get_dev_descriptor_string:%d\n",index);
	switch ( index)
	{
		case 0:       //land ids
			if ( len > sizeof(str_lang) )
				usb_device_write_data_ep_pack(0,(unsigned char *)str_lang,sizeof(str_lang));
			else
				usb_device_write_data_ep_pack(0,(unsigned char *)str_lang,len);
			return;
			break;
		case 1:       //iserialnumber
			if (len >= sizeof(str_isernum))
				len = sizeof(str_isernum);
			usb_device_write_data_ep_pack(0,(unsigned char *)str_isernum,len);
			break;

		case 2:       //iproduct
			if (len >= 38)
				len = 38;
			str_ret[0] = (0x0300 | len);
			usb_device_write_data_ep_pack(0,(unsigned char *)str_ret,len);
			break;
		case 3:       //iserialnumber
			if (len >= sizeof(str_isernum))
				len = sizeof(str_isernum);
			usb_device_write_data_ep_pack(0,(unsigned char *)str_isernum,len);
			break;
		case 4:
			if (len >= sizeof(str_config))
				len = sizeof(str_config);
			usb_device_write_data_ep_pack(0,(unsigned char *)str_config,len);
			break;
		case 5:
			if (len >= sizeof(str_interface1))
				len = sizeof(str_interface1);
			usb_device_write_data_ep_pack(0,(unsigned char *)str_interface1,len);
			break;
		case 6:
			if (len >= sizeof(str_interface2))
				len = sizeof(str_interface2);
			usb_device_write_data_ep_pack(0,(unsigned char *)str_interface2,len);
			break;
		case 0xee:    //microsoft OS!
			if (len >= sizeof(str_isernum))
				len = sizeof(str_isernum);
			str_isernum[0] = (0x0300 | len);
			usb_device_write_data_ep_pack(0,(unsigned char *)str_isernum,len);
			break;
		default:
			usb_device_write_data_ep_pack(0,0,0);
			break;
	}
}
/*——————————————————————————————————————————————————————————————
返回描述符
*/
static void usb_descriptor_request(USB_DeviceRequest* req_data)
{
	unsigned char  wValue = req_data->wValue >> 8;
	switch(wValue)
	{
		case DEVICE_DESCRIPTOR ://Device DISCRIPTOR
					usb_get_device_descriptor(req_data);break;
		case CONFIGURATION_DESCRIPTOR ://Config DISCRIPTOR
					usb_get_cfg_descriptor(req_data);break;
		case STRING_DESCRIPTOR ://String DISCRIPTOR
					udp_get_dev_descriptor_string(req_data);break;
//		case HID_DESCRIPTOR_TYPE ://Hid DISCRIPTOR
//					usb_get_hid_descriptor(req_data);break;
		case REPORT_DESCRIPTOR ://Hid DISCRIPTOR
					// usb_get_hid_descport(req_data);	
		break;
		default:
					usbprint("usb get descriptor : 0x%04x not suppost!\n",wValue);
					usb_device_read_data_status_ep0(1);
					break;
	}
}
/*——————————————————————————————————————————————————————————————
设置地址
*/
static void set_address(unsigned char addr)
{
	delay_ms(1);
	usb_device_set_address(addr);
	usb_device_read_data_status_ep0(1);
	
	usbprint("set dev address: %x\n", addr);
	usb_device_set_ep0_state(EP0_IDLE);
}
/*——————————————————————————————————————————————————————————————
设置配置
*/
static void set_configuration(void)
{
	usb_config_ep_out(1,USB_DATA_PACKET_SIZE,USBC_TS_TYPE_INT);
	usb_config_ep_in(1,USB_DATA_PACKET_SIZE,USBC_TS_TYPE_INT);
	
	usb_device_read_data_status_ep0(1);
	
	usb_device_clear_setup_end();
	usb_device_send_nullpack_ep0();
	usb_device_set_ep0_state(EP0_IDLE);
}
/*——————————————————————————————————————————————————————————————
 标准步
*/
void standard_setup_request(USB_DeviceRequest *req_data)
{
		unsigned char bRequest =  req_data->bRequest;
    if(bRequest==USB_REQ_GET_DESCRIPTOR)
    {
    	usbprint("getDescriptor\n");
    	usb_descriptor_request(req_data);
    }
    else if(bRequest==USB_REQ_SET_CONFIGURATION)
    {
    	usbprint("set_configuration\n");
      set_configuration();
    }
    else if(bRequest==USB_REQ_GET_CONFIGURATION)
    {
    	usbprint("get_configuration\n");
			//not support
			usbprint("Error!! unsupprot USB_REQ_GET_CONFIGURATION command");
			usb_device_read_data_status_ep0(1);
    }
    else if(bRequest==USB_REQ_SET_INTERFACE)
    {
    	usbprint("set_interface\n");
			//not support
			usbprint("Error!! unsupprot USB_REQ_SET_INTERFACE command");
			usb_device_read_data_status_ep0(1);
    }
    else if(bRequest==USB_REQ_GET_INTERFACE)
    {
			//not support
			usbprint("Error!! unsupprot USB_REQ_GET_INTERFACE command");
			usb_device_read_data_status_ep0(1);
    }
    else if(bRequest==USB_REQ_SET_ADDRESS)
    {
    	usbprint("set_addr\n");
      set_address(req_data->wValue&0x7f);
    }
    else if(bRequest==USB_REQ_SET_DESCRIPTOR)
    {
    	usbprint("set_Descriptor\n");
			//not support
			usbprint("Error!! unsupprot USB_REQ_SET_DESCRIPTOR command");
			usb_device_read_data_status_ep0(1);
    }
    else if(bRequest==USB_REQ_SYNCH_FRAME)
    {
    	usbprint("sync frame\n");
			//not support
			usbprint("Error!! unsupprot USB_REQ_SYNCH_FRAME command");
			usb_device_read_data_status_ep0(1);
    }
    else if(bRequest == USB_REQ_CLEAR_FEATURE)
    {
    	int epaddr = req_data->wIndex;
    	usbprint("clear feature\n");
    	usb_device_clear_ep_halt(epaddr);
		usb_device_read_data_status_ep0(1);
    }
    else
    {
    	usbprint("other  req \n");
			usbprint("Error!! received controller command:%02X.\n", bRequest);
			usb_device_read_data_status_ep0(1);
			//other command process by controller
    }
}
/*——————————————————————————————————————————————————————————————
 类步
*/
void class_setup_request(USB_DeviceRequest* req_data)
{
	// u8 buffl[8]={0};
	unsigned char bRequest = req_data->bRequest;
   if(bRequest==HID_REQ_SET_REPORT)//HID Set report
   {
	   usbprint("HID_REQ_SET_REPORT, host set device_report success!!!\n");
   }
   else if(bRequest==HID_REQ_SET_IDLE)
   {
	   usbprint("HID_REQ_SET_IDLE, host set device_report success!!!\n");
   }
   else if(bRequest == 0x20)//set_line_coding
   {
	   usbprint("set_line_coding\n");
	   port_line_coding_flag = 1;
	   int len = USBC_ReadLenFromFifo(1);
	   usbprint("set_line_coding:readlen:%d\n",len);
	   if(len == 7)
	   {
		   unsigned char tmp[7];
		   usb_device_read_data_ep_pack(0,tmp,7);
		   set_line_codingstatic_ext(tmp);
		   usb_device_read_data_status_ep0(1);
		   usb_device_set_ep0_state(EP0_IDLE);
		   port_line_coding_flag = 0;
	   }
   }
   else if(bRequest == 0x21)//get_line_coding
	{
		int lent = sizeof(USB_CDC_LineCoding);
		usb_device_write_data_ep_pack(0,( u8 *)(&port_line_coding),lent);
	   usbprint("get_line_coding\n");
	}
   else if(bRequest == 0x22)//set_control_line_state
	{
	   usbprint("set_control_line_state \n");
	   usb_device_clear_setup_end();
	   usb_device_set_ep0_state(EP0_IDLE);
	}
   else
   {
	   usbprint("Warnning!!! received unsupport command:%08x ,do nothing\n", bRequest);
   }
 }
/*——————————————————————————————————————————————————————————————
usb_hid_setup_handle
*/
#include "delay.h"
void usb_setup_handle(unsigned char *dat,int len)
{
//   volatile int Timeout = 10000000;
	if(len == 8)
	{		
		memcpy(&usb_setup_request,dat,len);//usb is big endian
		delay_us(100); //add this, otherwise fail
		usbprint("bRequestType:0x%02x\n",usb_setup_request.bRequestType);
		usbprint("bRequest:0x%02x\n",usb_setup_request.bRequest);
		usbprint("wValue:0x%04x\n",usb_setup_request.wValue);
		usbprint("wIndex:0x%04x\n",usb_setup_request.wIndex);
		usbprint("wLength:0x%04x\n",usb_setup_request.wLength);
			
		if((usb_setup_request.bRequestType & USB_TYPE_MASK) == USB_TYPE_STANDARD)
		{
			standard_setup_request(&usb_setup_request);
		}
		else if((usb_setup_request.bRequestType & USB_TYPE_MASK) == USB_TYPE_CLASS)
		{
			class_setup_request(&usb_setup_request);
		}
		else if((usb_setup_request.bRequestType & USB_TYPE_MASK) == USB_TYPE_VENDOR)
		{

		}
	}
	else
	{
		usbprint("not setup data.\n");
		delay_us(100); //add this, otherwise fail
	}
}
