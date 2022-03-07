#include "usb.h"
#include "usb_phy.h"
#include "usb_dev.h"
#include "usb_desc.h"
#include <string.h>

#define CUSTOMUVC_SIZ_REPORT_DESC  35//33
		
	
unsigned char UVCHighSpeedConfigDscr[306]=
 {
   
9,               // Descriptor length
2,                 // Descriptor type
sizeof(UVCHighSpeedConfigDscr) % 256, // Total Length (LSB)
sizeof(UVCHighSpeedConfigDscr) /  256, // Total Length (MSB)
2,      // Number of interfaces
1,      // Configuration number
0,      // Configuration string
128,   // Attributes (b7 - buspwr, b6 - selfpwr, b5 - rwu)
50,      // Power requirement (div 2 ma)
		 
//Interface association descriptor 
0x08,    // Descriptor size 
0x0b,    // Interface association descr type  
0x00,   // I/f number of first video control i/f 
0x02,    // Number of video streaming i/f 
0x0E,    // CC_VIDEO : Video i/f class code  
0x03,    // SC_VIDEO_INTERFACE_COLLECTION : subclass code  
0x00,    // Protocol : not used  
0x00,    // String desc index for interface  

//Standard video control interface descriptor  
0x09,              // Descriptor size  
0x04,      // Interface descriptor type  
0x00,             // Interface number 
0x00,              // Alternate setting number 
0x00,              // Number of end points 
0x0E,              // CC_VIDEO : Interface class  
0x01,              // CC_VIDEOCONTROL : Interface sub class  
0x00,              // Interface protocol code  
0x00,              // Interface descriptor string index  

// Class specific VC interface header descriptor  
0x0D,   // Descriptor size  
0x24,   // Class Specific I/f header descriptor type 
0x01,   // Descriptor sub type : VC_HEADER  
0x00,   // Revision of class spec : 1.0 
0x01,
0x50,   // Total size of class specific descriptors (till output terminal) 
0x00,
0x00,   // Clock frequency : 48MHz 
0x6C,
0xDC,       
0x02,         
0x01,  // Number of streaming interfaces 
0x01,  // Video streaming I/f 1 belongs to VC i/f 

//Input (camera) terminal descriptor  
0x12,  // Descriptor size 
0x24,  // Class specific interface desc type 
0x02,  // Input Terminal Descriptor type 
0x01,  // ID of this terminal  
0x01,  // Camera terminal type  
0x02,  
0x00,  // No association terminal 
0x00,  // String desc index : not used  
0x00,  // No optical zoom supported  
0x00,
0x00,  // No optical zoom supported 
0x00,
0x00,  // No optical zoom supported  
0x00,
0x03,  // Size of controls field for this terminal : 3 bytes  
0x00,  // No controls supported  
0x00,
0x00,
 

      
      
      
      
//Processing unit descriptor  
0x0C,  // Descriptor size 
0x24,  // Class specific interface desc type 
0x05,  // Processing unit descriptor type  
0x02,  // ID of this terminal  
0x01,  // Source ID : 1 : conencted to input terminal  
0x00,  // Digital multiplier  
0x40,
0x03,  // Size of controls field for this terminal : 3 bytes  
0x0f,  //  controls supported   //01Ê¹ÄÜÁÁ¶È¿ØÖÆ 
0x00,
0x00,
0x00,  // String desc index : not used  

// Extension unit descriptor 
0x1C,   // Descriptor size 
0x24,  // Class specific interface desc type  
0x06,  // Extension unit descriptor type 
0x03,   // ID of this terminal  
0x0FF,  // 16 byte GUID 
0x0FF,
0x0FF,
0x0FF,      
0x0FF,
0x0FF,
0x0FF,
0x0FF,      
0x0FF,
0x0FF,
0x0FF,
0x0FF,     
0x0FF, 
0x0FF,
0x0FF,
0x0FF,      
0x00,   // Number of controls in this terminal  
0x01,   // Number of input pins in this terminal 
0x02,   // Source ID : 2 : connected to proc unit  
0x03,   // Size of controls field for this terminal : 3 bytes  
0x00,   // No controls supported 
0x00, 
0x00,      
0x00,   // String desc index : not used 

//Output terminal descriptor 
0x09,   // Descriptor size  
0x24,   // Class specific interface desc type  
0x03,   // Output terminal descriptor type  
0x04,   // ID of this terminal 
0x01,   // USB Streaming terminal type  
0x01,
0x00,   // No association terminal 
0x03,   // Source ID : 3 : connected to extn unit  
0x00,   // String desc index : not used 
 



// Standard video streaming interface descriptor (alternate setting 0) 
0x09,                           // Descriptor size  
0x04,                   // Interface descriptor type 
0x01,                           // Interface number  
0x00,                           // Alternate setting number  
0x01,                           // Number of end points : zero bandwidth  
0x0E,                           // Interface class : CC_VIDEO  
0x02,                           // Interface sub class : CC_VIDEOSTREAMING  
0x00,                           // Interface protocol code : undefined  
0x00,                           // Interface descriptor string index 
// Endpoint Descriptor
0x07,      // Descriptor length
0x05,         // Descriptor type
0x81,                  // Endpoint number, and direction
0x02,              // Endpoint type
0x00,                  // Maximun packet size (LSB)
0x02,                  // Max packect size (MSB)
0x00,                  // Polling interval

 
//Class-specific video streaming input header descriptor 
0x0E,                          // Descriptor size  
0x24,                          // Class-specific VS i/f type  
0x01,                           // Descriptotor subtype : input header  
0x01,                          // 2 format desciptor follows 
0x19,                           // Total size of class specific VS descr 
0x00,     
0x81,                         // EP address for BULK video data 
0x00,                         //No dynamic format change supported 
0x04,                         // Output terminal ID : 4  
0x01,                         // Still image capture method 1 supported 
0x01,                         // Hardware trigger supported for still image  
0x00,                         // Hardware to initiate still image capture 
0x01,                         // Size of controls field : 1 byte 
0x00,                         // D2 : Compression quality supported 



//////////////////////////////////////////////////////////


 

//Class specific VS format descriptor 
0x0B,                          // Descriptor size  
0x24,                          // Class-specific VS i/f type  
0x06,                          // Descriptotor subtype : VS_FORMAT_MJPEG  
0x01,                          // Format desciptor index 
0x05,                          // 5 Frame desciptor follows  
0x01,                          // Uses fixed size samples  
0x02,                          // Default frame index is 1  
0x00,                          // Non interlaced stream not reqd. 
0x00,                          // Non interlaced stream not reqd.  
0x00,                          // Non interlaced stream  
0x00,                          // CopyProtect: duplication unrestricted 

//Class specific VS frame descriptor  1280*720 
0x1E,     // Descriptor size 
0x24,     // Class-specific VS I/f Type 
0x07,     // Descriptotor subtype : VS_FRAME_MJPEG  
0x01,     // Frame desciptor index  
0x00,     // Still image capture method not supported  
0x00,     // Width of the frame L  
0x05,     // Width of the frame H
0x0d0,     // Height of the frame L  
0x02,     // Height of the frame H 
0x00,     // Min bit rate bits/s 0x00,0x50,0x97,0x31
0x50,
0x97,
0x31,
0x00,     // Min bit rate bits/s  
0x50,
0x97,
0x31, 
0x00,     //Maximum video or still frame size in bytes  0x00,0x48,0x3f,0x00
0x48,
0x3F,
0x00, 
0x15,     // Default frame interval (15FPS 0x2A,0x2c,0x0a,0x00) (30FPS 0x15,0x16,0x05,0x00) (60FPS 0x0A,0x8b,0x02,0x00,) 
0x16,
0x05,
0x00, 
0x01,     // Frame interval type : No of discrete intervals 
0x15,    // Frame interval 3 
0x16,
0x05,
0x00,

//Class specific VS frame descriptor  1920*1080
0x1E,     // Descriptor size 
0x24,     // Class-specific VS I/f Type 
0x07,     // Descriptotor subtype : VS_FRAME_MJPEG  
0x02,     // Frame desciptor index  
0x00,     // Still image capture method not supported  
0x80,     // Width of the frame L  
0x07,     // Width of the frame H
0x38,     // Height of the frame L  
0x04,     // Height of the frame H 
0x00,     // Min bit rate bits/s 0x00,0x50,0x97,0x31
0x50,
0x97,
0x31,
0x00,     // Min bit rate bits/s  
0x50,
0x97,
0x31, 
0x00,     //Maximum video or still frame size in bytes  0x00,0x48,0x3f,0x00
0x48,
0x3F,
0x00, 
0x15,     // Default frame interval (15FPS 0x2A,0x2c,0x0a,0x00) (30FPS 0x15,0x16,0x05,0x00) (60FPS 0x0A,0x8b,0x02,0x00,) 
0x16,
0x05,
0x00, 
0x01,     // Frame interval type : No of discrete intervals 
0x15,     // Frame interval 3 
0x16,
0x05,
0x00,


//Class specific VS frame descriptor   2592*1944
0x1E,     // Descriptor size 
0x24,     // Class-specific VS I/f Type 
0x07,     // Descriptotor subtype : VS_FRAME_MJPEG  
0x03,     // Frame desciptor index  
0x00,     // Still image capture method not supported  
0x20,     // Width of the frame L  
0x0a,     // Width of the frame H
0x98,     // Height of the frame L  
0x07,     // Height of the frame H 
0x00,     // Min bit rate bits/s 0x00,0x50,0x97,0x31
0x50,
0x97,
0x31,
0x00,     // Min bit rate bits/s  
0x50,
0x97,
0x31, 
0x00,     //Maximum video or still frame size in bytes  0x00,0x48,0x3f,0x00
0x48,
0x3F,
0x00,
0x15,     // Default frame interval (15FPS 0x2A,0x2c,0x0a,0x00) (30FPS 0x15,0x16,0x05,0x00) (60FPS 0x0A,0x8b,0x02,0x00,) 
0x16,
0x05,
0x00, 
0x01,    // Frame interval type : No of discrete intervals 
0x15,     // Frame interval 3 
0x16,
0x05,
0x00,


//Class specific VS frame descriptor   3264*2448 
0x1E,     // Descriptor size 
0x24,     // Class-specific VS I/f Type 
0x07,     // Descriptotor subtype : VS_FRAME_MJPEG  
0x04,     // Frame desciptor index  
0x00,     // Still image capture method not supported  
0x0c0,     // Width of the frame L  
0x0c,     // Width of the frame H
0x90,     // Height of the frame L  
0x09,     // Height of the frame H 
0x00,     // Min bit rate bits/s 0x00,0x50,0x97,0x31
0x50,
0x97,
0x31,
0x00,     // Min bit rate bits/s  
0x50,
0x97,
0x31, 
0x00,     //Maximum video or still frame size in bytes  0x00,0x48,0x3f,0x00
0x48,
0x3F,
0x00, 
0x15,     // Default frame interval (15FPS 0x2A,0x2c,0x0a,0x00) (30FPS 0x15,0x16,0x05,0x00) (60FPS 0x0A,0x8b,0x02,0x00,) 
0x16,
0x05,
0x00, 
0x01,     // Frame interval type : No of discrete intervals 
0x15,     // Frame interval 3 
0x16,
0x05,
0x00,

//Class specific VS frame descriptor   4000*3000
0x1E,     // Descriptor size 
0x24,     // Class-specific VS I/f Type 
0x07,     // Descriptotor subtype : VS_FRAME_MJPEG  
0x05,     // Frame desciptor index  
0x00,     // Still image capture method not supported  
0x0a0,     // Width of the frame L  
0x0f,     // Width of the frame H
0x0b8,     // Height of the frame L  
0x0b,     // Height of the frame H 
0x00,     // Min bit rate bits/s 0x00,0x50,0x97,0x31
0x50,
0x97,
0x31,
0x00,     // Min bit rate bits/s  
0x50,
0x97,
0x31, 
0x00,     //Maximum video or still frame size in bytes  0x00,0x48,0x3f,0x00
0x48,
0x3F,
0x00, 
0x15,     // Default frame interval (15FPS 0x2A,0x2c,0x0a,0x00) (30FPS 0x15,0x16,0x05,0x00) (60FPS 0x0A,0x8b,0x02,0x00,) 
0x16,
0x05,
0x00, 
0x01,     // Frame interval type : No of discrete intervals 
0x15,     // Frame interval 3 
0x16,
0x05,
0x00 
};

/*——————————————————————————————————————————————————————————————
返回HID配置cfg描述符
*/
void usb_uvc_get_cfg_descriptor(USB_DeviceRequest *req_data)
{
	usbprint("usb_uvc_get_cfg_descriptor\n");
	switch(req_data->wLength)
	{
		case 9:
			usb_device_write_data_ep_pack(0,  (unsigned char *)&UVCHighSpeedConfigDscr, 9);
			break;
		case 8:
			usb_device_write_data_ep_pack(0,  (unsigned char *)&UVCHighSpeedConfigDscr, 8);
			break;
		default:
			usb_device_write_data_ep_pack(0,  (unsigned char *)&UVCHighSpeedConfigDscr, req_data->wLength);
			break;
	}
}

/*——————————————————————————————————————————————————————————————
usb_uvc_in_ep_callback
*/
int usb_uvc_in_ep_callback(void)
{
	//usbprint("usb_hid_in_ep_callback\n");
	return 0;
}
/*——————————————————————————————————————————————————————————————
usb_uvc_out_ep_callback
*/
void usb_uvc_out_ep_callback(unsigned char *pdat,int len)
{
//int i=0;
//	usbprint("uvc-Len=%d\n",len);
//	usb_device_write_data(1,pdat,len);//回送	
}


