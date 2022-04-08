#include "piclib.h"
#include "gif.h"	 
#include "delay.h"	 
#include <string.h>   
#include "MTF_io.h"
#include "UI_engine.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK战舰STM32开发板V3
//图片解码 驱动代码-gif解码部分
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2014/5/15
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2014-2024
//All rights reserved
//********************************************************************************
//升级说明 
//无
//////////////////////////////////////////////////////////////////////////////////
					    
const u16 _aMaskTbl[16] =
{
	0x0000, 0x0001, 0x0003, 0x0007,
	0x000f, 0x001f, 0x003f, 0x007f,
	0x00ff, 0x01ff, 0x03ff, 0x07ff,
	0x0fff, 0x1fff, 0x3fff, 0x7fff,
};	  
const u8 _aInterlaceOffset[]={8,8,4,2};
const u8 _aInterlaceYPos  []={0,4,2,1};
 
u8 gifdecoding=0;//标记GIF正在解码.

//////////////////////////////////////////用户配置区//////////////////////////////////
#define GIF_USE_MALLOC		0	//定义是否使用malloc,这里我们选择使用malloc	     
//////////////////////////////////////////////END/////////////////////////////////////

//定义是否使用malloc, 此处只用于gif_decode() gif_decode2(), 其他再函数内部
#if GIF_USE_MALLOC==0 	
gif89a tgif89a;			//gif89a文件
LZW_INFO tlzw;			//lzw
#endif

//检测GIF头
//返回值:0,是GIF89a/87a;非零,非GIF89a/87a
u8 gif_check_head(mFILE *file)
{
	u8 gifversion[6];
	cache_read(gifversion,sizeof(gifversion),file);
	if(MTF_error(file)) return 1;	   
	if((gifversion[0]!='G')||(gifversion[1]!='I')||(gifversion[2]!='F')||
	(gifversion[3]!='8')||((gifversion[4]!='7')&&(gifversion[4]!='9'))||
	(gifversion[5]!='a'))return 2;
	else return 0;	
}
//将RGB888转为RGB565
//ctb:RGB888颜色数组首地址.
//返回值:RGB565颜色.
u16 gif_getrgb565(u8 *ctb) 
{
	u16 r,g,b;
	r=(ctb[0]>>3)&0X1F;
	g=(ctb[1]>>2)&0X3F;
	b=(ctb[2]>>3)&0X1F;
	return b+(g<<5)+(r<<11);
}

//读取颜色表
//file:文件;
//gif:gif信息;
//num:tbl大小.
//返回值:0,OK;其他,失败;
u8 gif_readcolortbl(mFILE *file,gif89a * gif,u16 num)
{
	u8 rgb[3];
	u16 t;
	for(t=0;t<num;t++)
	{
		cache_read(rgb,sizeof(rgb),file);
		if(MTF_error(file)) return 1;//读错误
		//gif->colortbl[t]=gif_getrgb565(rgb); //note: 转为系统显示颜色格式 colortbl需适应
		gif->colortbl[t] = ((u32)rgb[0]<<16)+((u32)rgb[1]<<8)+rgb[2]+0XFF000000; //将RGB转为BGR适应系统
	}
	return 0;
} 
//得到逻辑屏幕描述,图像尺寸等
//file:文件;
//gif:gif信息;
//返回值:0,OK;其他,失败;
u8 gif_getinfo(mFILE *file,gif89a * gif)
{   
	cache_read((u8*)&gif->gifLSD,7,file);
	if(MTF_error(file)) return 1;
	if(gif->gifLSD.flag&0x80)//存在全局颜色表
	{
		gif->numcolors=2<<(gif->gifLSD.flag&0x07);//得到颜色表大小
		if(gif_readcolortbl(file,gif,gif->numcolors))return 1;//读错误	
	}	   
	return 0;
}
//保存全局颜色表	 
//gif:gif信息;
void gif_savegctbl(gif89a* gif)
{
	u16 i=0;
	for(i=0;i<256;i++)gif->bkpcolortbl[i]=gif->colortbl[i];//保存全局颜色.
}
//恢复全局颜色表	 
//gif:gif信息;
void gif_recovergctbl(gif89a* gif)
{
	u16 i=0;
	for(i=0;i<256;i++)gif->colortbl[i]=gif->bkpcolortbl[i];//恢复全局颜色.
}

//初始化LZW相关参数	   
//gif:gif信息;
//codesize:lzw码长度
void gif_initlzw(gif89a* gif,u8 codesize) 
{
 	memset((u8 *)gif->lzw, 0, sizeof(LZW_INFO));
	gif->lzw->SetCodeSize  = codesize;
	gif->lzw->CodeSize     = codesize + 1;
	gif->lzw->ClearCode    = (1 << codesize);
	gif->lzw->EndCode      = (1 << codesize) + 1;
	gif->lzw->MaxCode      = (1 << codesize) + 2;
	gif->lzw->MaxCodeSize  = (1 << codesize) << 1;
	gif->lzw->ReturnClear  = 1;
	gif->lzw->LastByte     = 2;
	gif->lzw->sp           = gif->lzw->aDecompBuffer;
}

//读取一个数据块
//gfile:gif文件;
//buf:数据缓存区
//maxnum:最大读写数据限制
u16 gif_getdatablock(mFILE *gfile,u8 *buf,u16 maxnum) 
{
	u8 cnt;
	u32 fpos;
	cache_read(&cnt,1,gfile);//得到LZW长度			 
	if(cnt) 
	{
		if (buf)//需要读取 
		{
			if(cnt>maxnum)
			{
				fpos=cache_tell(gfile);
				cache_lseek(gfile,fpos+cnt,SEEK_SET);//跳过
				return cnt;//直接不读
			}
			cache_read(buf,cnt,gfile);//得到LZW长度	
		}else 	//直接跳过
		{
			fpos=cache_tell(gfile);
			cache_lseek(gfile,fpos+cnt,SEEK_SET);//跳过
		}
	}
	return cnt;
}
//ReadExtension		 
//Purpose:
//Reads an extension block. One extension block can consist of several data blocks.
//If an unknown extension block occures, the routine failes.
//返回值:0,成功;
// 		 其他,失败
u8 gif_readextension(mFILE *gfile,gif89a* gif, int *pTransIndex,u8 *pDisposal)
{
	u8 temp;
	u8 buf[4];  
	cache_read(&temp,1,gfile);//得到长度		 
	switch(temp)
	{
		case GIF_PLAINTEXT:
		case GIF_APPLICATION:
		case GIF_COMMENT:
			while(gif_getdatablock(gfile,0,256)>0);			//获取数据块
			return 0;
		case GIF_GRAPHICCTL://图形控制扩展块
			if(gif_getdatablock(gfile,buf,4)!=4)return 1;	//图形控制扩展块的长度必须为4 
 		 	gif->delay=(buf[2]<<8)|buf[1];					//得到延时 
			*pDisposal=(buf[0]>>2)&0x7; 	    			//得到处理方法
			if((buf[0]&0x1)!=0)*pTransIndex=buf[3];			//透明色表 
			cache_read(&temp,1,gfile);	 		//得到LZW长度	
 			if(temp!=0)return 1;							//读取数据块结束符错误.
			return 0;
	}
	return 1;//错误的数据
}

//从LZW缓存中得到下一个LZW码,每个码包含12位
//返回值:<0,错误.
//		 其他,正常.
int gif_getnextcode(mFILE *gfile,gif89a* gif) 
{
	int i,j,End;
	long Result;
	if(gif->lzw->ReturnClear)
	{
		//The first code should be a clearcode.
		gif->lzw->ReturnClear=0;
		return gif->lzw->ClearCode;
	}
	End=gif->lzw->CurBit+gif->lzw->CodeSize;
	if(End>=gif->lzw->LastBit)
	{
		int Count;
		if(gif->lzw->GetDone)return-1;//Error 
		gif->lzw->aBuffer[0]=gif->lzw->aBuffer[gif->lzw->LastByte-2];
		gif->lzw->aBuffer[1]=gif->lzw->aBuffer[gif->lzw->LastByte-1];
		if((Count=gif_getdatablock(gfile,&gif->lzw->aBuffer[2],300))==0)gif->lzw->GetDone=1;
		if(Count<0)return -1;//Error 
		gif->lzw->LastByte=2+Count;
		gif->lzw->CurBit=(gif->lzw->CurBit-gif->lzw->LastBit)+16;
		gif->lzw->LastBit=(2+Count)*8;
		End=gif->lzw->CurBit+gif->lzw->CodeSize;
	}
	j=End>>3;
	i=gif->lzw->CurBit>>3;
	if(i==j)Result=(long)gif->lzw->aBuffer[i];
	else if(i+1==j)Result=(long)gif->lzw->aBuffer[i]|((long)gif->lzw->aBuffer[i+1]<<8);
	else Result=(long)gif->lzw->aBuffer[i]|((long)gif->lzw->aBuffer[i+1]<<8)|((long)gif->lzw->aBuffer[i+2]<<16);
	Result=(Result>>(gif->lzw->CurBit&0x7))&_aMaskTbl[gif->lzw->CodeSize];
	gif->lzw->CurBit+=gif->lzw->CodeSize;
	return(int)Result;
}	
//得到LZW的下一个码
//返回值:<0,错误(-1,不成功;-2,读到结束符了)
//		 >=0,OK.(LZW的第一个码)
int gif_getnextbyte(mFILE *gfile,gif89a* gif) 
{
	int i,Code,Incode;
	while((Code=gif_getnextcode(gfile,gif))>=0)
	{
		if(Code==gif->lzw->ClearCode)
		{
			//Corrupt GIFs can make this happen  
			if(gif->lzw->ClearCode>=(1<<MAX_NUM_LWZ_BITS))return -1;//Error 
			//Clear the tables 
			memset((u8*)gif->lzw->aCode,0,sizeof(gif->lzw->aCode));
			for(i=0;i<gif->lzw->ClearCode;++i)gif->lzw->aPrefix[i]=i;
			//Calculate the'special codes' independence of the initial code size
			//and initialize the stack pointer 
			gif->lzw->CodeSize=gif->lzw->SetCodeSize+1;
			gif->lzw->MaxCodeSize=gif->lzw->ClearCode<<1;
			gif->lzw->MaxCode=gif->lzw->ClearCode+2;
			gif->lzw->sp=gif->lzw->aDecompBuffer;
			//Read the first code from the stack after clear ingand initializing*/
			do
			{
				gif->lzw->FirstCode=gif_getnextcode(gfile,gif);
			}while(gif->lzw->FirstCode==gif->lzw->ClearCode);
			gif->lzw->OldCode=gif->lzw->FirstCode;
			return gif->lzw->FirstCode;
		}
		if(Code==gif->lzw->EndCode)return -2;//End code
		Incode=Code;
		if(Code>=gif->lzw->MaxCode)
		{
			*(gif->lzw->sp)++=gif->lzw->FirstCode;
			Code=gif->lzw->OldCode;
		}
		while(Code>=gif->lzw->ClearCode)
		{
			*(gif->lzw->sp)++=gif->lzw->aPrefix[Code];
			if(Code==gif->lzw->aCode[Code])return Code;
			if((gif->lzw->sp-gif->lzw->aDecompBuffer)>=sizeof(gif->lzw->aDecompBuffer))return Code;
			Code=gif->lzw->aCode[Code];
		}
		*(gif->lzw->sp)++=gif->lzw->FirstCode=gif->lzw->aPrefix[Code];
		if((Code=gif->lzw->MaxCode)<(1<<MAX_NUM_LWZ_BITS))
		{
			gif->lzw->aCode[Code]=gif->lzw->OldCode;
			gif->lzw->aPrefix[Code]=gif->lzw->FirstCode;
			++gif->lzw->MaxCode;
			if((gif->lzw->MaxCode>=gif->lzw->MaxCodeSize)&&(gif->lzw->MaxCodeSize<(1<<MAX_NUM_LWZ_BITS)))
			{
				gif->lzw->MaxCodeSize<<=1;
				++gif->lzw->CodeSize;
			}
		}
		gif->lzw->OldCode=Incode;
		if(gif->lzw->sp>gif->lzw->aDecompBuffer)return *--(gif->lzw->sp);
	}
	return Code;
}
//DispGIFImage		 
//Purpose:
//   This routine draws a GIF image from the current pointer which should point to a
//   valid GIF data block. The size of the desired image is given in the image descriptor.
//Return value:
//  0 if succeed
//  1 if not succeed
//Parameters:
//  pDescriptor  - Points to a IMAGE_DESCRIPTOR structure, which contains infos about size, colors and interlacing.
//  x0, y0       - Obvious.
//  Transparency - Color index which should be treated as transparent.
//  Disposal     - Contains the disposal method of the previous image. If Disposal == 2, the transparent pixels
//                 of the image are rendered with the background color.
u8 gif_dispimage(mFILE *gfile,gif89a* gif,u16 x0,u16 y0,int Transparency, u8 Disposal) 
{  
   	u8 lzwlen;
	int Index,OldIndex,XPos,YPos,YCnt,Pass,Interlace,XEnd;
	int Width,Height,Cnt,ColorIndex;
	// u32 bkcolor=gif->colortbl[gif->gifLSD.bkcindex]; //note: 和系统颜色格式对应
	u32 *pTrans;

	Width=gif->gifISD.width;
	Height=gif->gifISD.height;
	XEnd=Width+x0-1;
	pTrans=(u32*)gif->colortbl;
	cache_read(&lzwlen,1,gfile);//得到LZW长度	 
	gif_initlzw(gif,lzwlen);//Initialize the LZW stack with the LZW code size 
	Interlace=gif->gifISD.flag&0x40;//是否交织编码
	for(YCnt=0,YPos=y0,Pass=0;YCnt<Height;YCnt++)
	{
		Cnt=0;
		OldIndex=-1;
		for(XPos=x0;XPos<=XEnd;XPos++)
		{
			if(gif->lzw->sp>gif->lzw->aDecompBuffer)Index=*--(gif->lzw->sp);
		    else Index=gif_getnextbyte(gfile,gif);	   
			if(Index==-2)return 0;//Endcode     
			if((Index<0)||(Index>=gif->numcolors))
			{
				//IfIndex out of legal range stop decompressing
				return 1;//Error
			}
			//If current index equals old index increment counter
			if((Index==OldIndex)&&(XPos<=XEnd))Cnt++;
	 		else
			{
				if(Cnt)
				{
					if(OldIndex!=Transparency)
					{									    
						pic_phy.draw_hline(XPos-Cnt-1,YPos,Cnt+1,*(pTrans+OldIndex));
					}else if(Disposal==2)
					{
						//pic_phy.draw_hline(XPos-Cnt-1,YPos,Cnt+1,bkcolor);
						LCD_FillBackToFront(XPos - Cnt - 1, YPos, XPos - 1, YPos);
					}
					Cnt=0;
				}else
				{
					if (OldIndex >= 0)
					{
						if (OldIndex != Transparency)
							pic_phy.git_draw_point(XPos - 1, YPos, *(pTrans + OldIndex));
						else if (Disposal == 2) //pic_phy.git_draw_point(XPos-1,YPos,bkcolor);
							pic_phy.git_draw_point(XPos - 1, YPos, LCD_ReadBackPoint(XPos - 1, YPos));
					}
				}
			}
			OldIndex=Index;
		}
		if((OldIndex!=Transparency)||(Disposal==2))
		{
			// if(OldIndex!=Transparency)ColorIndex=*(pTrans+OldIndex);
		    // else ColorIndex=bkcolor;
	 		// if(Cnt)
			// {
			// 	pic_phy.draw_hline(XPos-Cnt-1,YPos,Cnt+1,ColorIndex);
			// }else pic_phy.git_draw_point(XEnd,YPos,ColorIndex);
			if (OldIndex != Transparency)
			{
				ColorIndex = *(pTrans + OldIndex);
				if (Cnt)
					pic_phy.draw_hline(XPos - Cnt - 1, YPos, Cnt + 1, ColorIndex);
				else
					pic_phy.git_draw_point(XEnd, YPos, ColorIndex);
			}
			else
			{
				if (Cnt)
				{
					//pic_phy.draw_hline(XPos - Cnt - 1, YPos, Cnt + 1, ColorIndex);
					LCD_FillBackToFront(XPos - Cnt - 1, YPos, XPos - 1, YPos);
				}
				else
					pic_phy.git_draw_point(XEnd, YPos, LCD_ReadBackPoint(XEnd, YPos));
			}
		}
		//Adjust YPos if image is interlaced 
		if(Interlace)//交织编码
		{
			YPos+=_aInterlaceOffset[Pass];
			if((YPos-y0)>=Height)
			{
				++Pass;
				YPos=_aInterlaceYPos[Pass]+y0;
			}
		}else YPos++;	    
	}
	return 0;
}  			   
//恢复成背景色
//x,y:坐标
//gif:gif信息.
//pimge:图像描述块信息
void gif_clear2bkcolor(u16 x,u16 y,gif89a* gif,ImageScreenDescriptor pimge)
{
	u16 x0,y0,x1,y1;
	// u16 color=gif->colortbl[gif->gifLSD.bkcindex];
	if(pimge.width==0||pimge.height==0)return;//直接不用清除了,原来没有图像!!
	if(gif->gifISD.yoff>pimge.yoff)
	{
   		x0=x+pimge.xoff;
		y0=y+pimge.yoff;
		x1=x+pimge.xoff+pimge.width-1;;
		y1=y+gif->gifISD.yoff-1;
		//设定xy,的范围不能太大.
		if(x0<x1&&y0<y1&&x1<320&&y1<320)
		{
			//pic_phy.fill(x0,y0,x1,y1,color);
			LCD_FillBackToFront(x0, y0, x1, y1);
		}
	}
	if(gif->gifISD.xoff>pimge.xoff)
	{
   		x0=x+pimge.xoff;
		y0=y+pimge.yoff;
		x1=x+gif->gifISD.xoff-1;;
		y1=y+pimge.yoff+pimge.height-1;
		if(x0<x1&&y0<y1&&x1<320&&y1<320)
		{
			//pic_phy.fill(x0,y0,x1,y1,color);
			LCD_FillBackToFront(x0, y0, x1, y1);
		}
	}
	if((gif->gifISD.yoff+gif->gifISD.height)<(pimge.yoff+pimge.height))
	{
   		x0=x+pimge.xoff;
		y0=y+gif->gifISD.yoff+gif->gifISD.height-1;
		x1=x+pimge.xoff+pimge.width-1;;
		y1=y+pimge.yoff+pimge.height-1;
		if(x0<x1&&y0<y1&&x1<320&&y1<320)
		{
			//pic_phy.fill(x0,y0,x1,y1,color);
			LCD_FillBackToFront(x0, y0, x1, y1);
		}
	}
 	if((gif->gifISD.xoff+gif->gifISD.width)<(pimge.xoff+pimge.width))
	{
   		x0=x+gif->gifISD.xoff+gif->gifISD.width-1;
		y0=y+pimge.yoff;
		x1=x+pimge.xoff+pimge.width-1;;
		y1=y+pimge.yoff+pimge.height-1;
		if(x0<x1&&y0<y1&&x1<320&&y1<320)
		{
			//pic_phy.fill(x0,y0,x1,y1,color);
			LCD_FillBackToFront(x0, y0, x1, y1);
		}
	}   
}

//画GIF图像的一帧
//gfile:gif文件.
//x0,y0:开始显示的坐标
u8 gif_drawimage(mFILE *gfile,gif89a* gif,u16 x0,u16 y0)
{		  
	size_t readed;
	u8 res,temp;    
	u16 numcolors;
	ImageScreenDescriptor previmg;

	u8 Disposal;
	int TransIndex;
	u8 Introducer;
	TransIndex=-1;				  
	do
	{
		readed=cache_read(&Introducer,1,gfile);//读取一个字节
		if(MTF_error(gfile)) return 1;   
		switch(Introducer)
		{		 
			case GIF_INTRO_IMAGE://图像描述
				previmg.xoff=gif->gifISD.xoff;
 				previmg.yoff=gif->gifISD.yoff;
				previmg.width=gif->gifISD.width;
				previmg.height=gif->gifISD.height;

				readed=cache_read((u8*)&gif->gifISD,9,gfile);//读取一个字节
				if(MTF_error(gfile)) return 1;			 
				if(gif->gifISD.flag&0x80)//存在局部颜色表
				{							  
					gif_savegctbl(gif);//保存全局颜色表
					numcolors=2<<(gif->gifISD.flag&0X07);//得到局部颜色表大小
					if(gif_readcolortbl(gfile,gif,numcolors))return 1;//读错误	
				}
				if(Disposal==2)gif_clear2bkcolor(x0,y0,gif,previmg);
				gif_dispimage(gfile,gif,x0+gif->gifISD.xoff,y0+gif->gifISD.yoff,TransIndex,Disposal);
 				while(1)
				{
					cache_read(&temp,1,gfile);//读取一个字节
					if(temp==0)break;
					readed=cache_tell(gfile);//还存在块.	
					if(cache_lseek(gfile,readed+temp,SEEK_SET))break;//继续向后偏移	 
			    }
				if(temp!=0)return 1;//Error 
				return 0;
			case GIF_INTRO_TERMINATOR://得到结束符了
				return 2;//代表图像解码完成了.
			case GIF_INTRO_EXTENSION:
				//Read image extension*/
				res=gif_readextension(gfile,gif,&TransIndex,&Disposal);//读取图像扩展块消息
				if(res)return 1;
	 			break;
			default:
				return 1;
		}
	}while(Introducer!=GIF_INTRO_TERMINATOR);//读到结束符了
	return 0;
}

//退出当前解码.
void gif_quit(void)
{
	gifdecoding=0;
}

//解码一个gif文件
//本例子不能显示尺寸大与给定尺寸的gif图片!!!
//filename:带路径的gif文件名字
//x,y,width,height:显示坐标及区域大小.
u8 gif_decode2(const u8 *filename,u16 x,u16 y,u16 width,u16 height)
{
	u8 res=0;
	u16 dtime=0;//解码延时
	gif89a *mygif89a;
	mFILE *gfile;
#if GIF_USE_MALLOC==1 	//定义是否使用malloc
	mygif89a=(gif89a*)pic_memalloc(sizeof(gif89a));
	if(mygif89a==NULL)res=PIC_MEM_ERR;//申请内存失败    
	mygif89a->lzw=(LZW_INFO*)pic_memalloc(sizeof(LZW_INFO));
	if(mygif89a->lzw==NULL)res=PIC_MEM_ERR;//申请内存失败 
#else
	mygif89a=&tgif89a;
	mygif89a->lzw=&tlzw;
#endif

	if(res==0)//OK
	{
		gfile=MTF_open((const char *)filename,"rb");
		if(gfile==NULL)
			res = 4;
		else
			res = 0;
		if(res==0)//打开文件ok
		{
			if(gif_check_head(gfile))res=PIC_FORMAT_ERR;
			if(gif_getinfo(gfile,mygif89a))res=PIC_FORMAT_ERR;
			if(mygif89a->gifLSD.width>width||mygif89a->gifLSD.height>height)res=PIC_SIZE_ERR;//尺寸太大.
			else
			{
				x=(width-mygif89a->gifLSD.width)/2+x;
				y=(height-mygif89a->gifLSD.height)/2+y;
			}
			gifdecoding=1;
			while(gifdecoding&&res==0)//解码循环
			{	 
				res=gif_drawimage(gfile,mygif89a,x,y);//显示一张图片
				if(mygif89a->gifISD.flag&0x80)gif_recovergctbl(mygif89a);//恢复全局颜色表
				if(mygif89a->delay)dtime=mygif89a->delay;
				else dtime=10;//默认延时
				while(dtime--&&gifdecoding)delay_ms(10);//延迟
				if(res==2)
				{
					res=0;
					break;
				}
				LCD_Exec(); //更新一帧到屏幕
			}
			gifdecoding = 0;
			MTF_close(gfile);
		}
	}   
#if GIF_USE_MALLOC==1 	//定义是否使用malloc
	pic_memfree(mygif89a->lzw);
	pic_memfree(mygif89a); 
#endif 
	return res;
}

//解码一个gif文件
//filename:带路径的gif文件名字
//x,y显示坐标.
//rect: 返回的矩形信息(长宽)
#include "system_port.h"
u8 gif_decode(const u8 *filename, u16 x, u16 y, RectInfo *rect)
{
	u16 width = 0, height = 0;
	u8 res = 0;
	u16 dtime = 0; //解码延时
	gif89a *mygif89a;
	mFILE *gfile;
#if GIF_USE_MALLOC == 1 //定义是否使用malloc
	mygif89a = (gif89a *)pic_memalloc(sizeof(gif89a));
	if (mygif89a == NULL)
		res = PIC_MEM_ERR; //申请内存失败
	mygif89a->lzw = (LZW_INFO *)pic_memalloc(sizeof(LZW_INFO));
	if (mygif89a->lzw == NULL)
		res = PIC_MEM_ERR; //申请内存失败
#else
	mygif89a = &tgif89a;
	mygif89a->lzw = &tlzw;
#endif

	if (res == 0) //OK
	{
		gfile = cache_open((const char *)filename, "rb");
		if (gfile == NULL)
			res = 4;
		else
			res = 0;
		if (res == 0) //打开文件ok
		{
			if (gif_check_head(gfile))
				res = PIC_FORMAT_ERR;
			if (gif_getinfo(gfile, mygif89a))
				res = PIC_FORMAT_ERR;

			width = mygif89a->gifLSD.width;
			height = mygif89a->gifLSD.height;
			x = (width - mygif89a->gifLSD.width) / 2 + x;
			y = (height - mygif89a->gifLSD.height) / 2 + y;
			if(rect!=NULL)
			{
				rect->width = width;
				rect->height = height;
				rect->totalPixels = width*height;
			}

			gifdecoding = 1;
			while (gifdecoding && res == 0) //解码循环
			{
				if (system_process_weak() == 1) //SDL专用, 要定时获取事件, 不然无响应, 停机
				{
					break;
				}

				res = gif_drawimage(gfile, mygif89a, x, y); //显示一张图片
				if (mygif89a->gifISD.flag & 0x80)
					gif_recovergctbl(mygif89a); //恢复全局颜色表
				if (mygif89a->delay)
					dtime = mygif89a->delay;
				else
					dtime = 10; //默认延时
				while (dtime-- && gifdecoding)
					delay_always(6); //延迟10ms
				if (res == 2)
				{
					res = 0;
					break;
				}
				LCD_Exec(); //更新一帧到屏幕
			}
			gifdecoding = 0;
			cache_close(gfile);
		}
	}
#if GIF_USE_MALLOC == 1 //定义是否使用malloc
	pic_memfree(mygif89a->lzw);
	pic_memfree(mygif89a);
#endif
	return res;
}

//解码一个gif文件
//filename:带路径的gif文件名字 state: 播放控制: 0停止, 1暂停, 2开始
//x,y显示坐标.
/* 
u8 gif_run_loop0(const u8 *filename, u8 state, u8 *loop_cnt, u16 x, u16 y)
{
	u16 width = 0, height = 0;
	u8 res = 0;
	static gif89a *mygif89a;
	static mFILE *gfile;
	static u16 dtime = 0;				 //解码延时
	static char filename_temp[64] = {0}; //存储再运行的文件, 用于自动新旧切换
	u8 stop_release = 0;				 //结束释放
	static u8 file_open_ok = 0;			 //标记成功打开文件
	static u8 gifdecoding_loop = 0;		 //标记解码运行中

#if GIF_USE_MALLOC == 0
	static LZW_INFO LZWstruct;
	static gif89a gif89astruct;
#endif

	if (state == 0) //停止
	{
		if (gifdecoding_loop)
		{
			stop_release = 1;
			res = 0;
		}
		else
		{
			return 0;
		}
	}
	else if (state == 1) //暂停
	{
		return 0;
	}
	else if (gifdecoding_loop) //下面为开始
	{
		if (--dtime)
			return 0;
		if (gifdecoding_loop && res == 0) //解码循环
		{
			res = gif_drawimage(gfile, mygif89a, x, y); //显示一张图片
			if (mygif89a->gifISD.flag & 0x80)
				gif_recovergctbl(mygif89a); //恢复全局颜色表
			if (mygif89a->delay)
				dtime = mygif89a->delay;
			else
				dtime = 10; //默认延时
			// while (dtime-- && gifdecoding_loop)
			// 	delay_ms(10); //延迟
			if (res == 2)
			{
				stop_release = 1;
				res = 0;
				//break;
			}
			LCD_Exec(); //更新一帧到屏幕
		}
	}
	else
	{
		if(*loop_cnt) //有没循环次数
		{
			if (*loop_cnt!=255) //255为无限循环
				*loop_cnt -= 1;
		}
		else
		{
			return 0;
		}
//初始化
#if GIF_USE_MALLOC == 1 //定义是否使用malloc
		mygif89a = (gif89a *)pic_memalloc(sizeof(gif89a));
		if (mygif89a == NULL)
			res = PIC_MEM_ERR; //申请内存失败
		mygif89a->lzw = (LZW_INFO *)pic_memalloc(sizeof(LZW_INFO));
		if (mygif89a->lzw == NULL)
			res = PIC_MEM_ERR; //申请内存失败
#else
		mygif89a = &gif89astruct;
		mygif89a->lzw = &LZWstruct;
#endif

		if (res == 0) //OK
		{
			gfile = MTF_open((const char *)filename, "rb");
			if (gfile == NULL)
				res = 4;
			else
				res = 0;
			if (res == 0) //打开文件ok
			{
				file_open_ok = 1;
				if (gif_check_head(gfile))
					res = PIC_FORMAT_ERR;
				if (gif_getinfo(gfile, mygif89a))
					res = PIC_FORMAT_ERR;

				width = mygif89a->gifLSD.width;
				height = mygif89a->gifLSD.height;
				if (x >= 0 && x <= lcddev.width - 1 && y >= 0 && y <= lcddev.height - 1)
				{
					if (x + width - 1 >= 0 && x + width - 1 <= lcddev.width - 1 &&
						y + height - 1 >= 0 && y + height - 1 <= lcddev.height - 1)
					{
						x = (width - mygif89a->gifLSD.width) / 2 + x;
						y = (height - mygif89a->gifLSD.height) / 2 + y;
					}
					else
					{
						res = PIC_WINDOW_ERR;
					}
				}
				else
				{
					res = PIC_WINDOW_ERR;
				}

				memset(filename_temp, 0, sizeof(filename_temp));
				strcpy(filename_temp, (char *)filename); //标记正在处理文件

				gifdecoding_loop = 1;			  //decode运行中
				if (gifdecoding_loop && res == 0) //解码循环
				{
					res = gif_drawimage(gfile, mygif89a, x, y); //显示一张图片
					if (mygif89a->gifISD.flag & 0x80)
						gif_recovergctbl(mygif89a); //恢复全局颜色表
					if (mygif89a->delay)
						dtime = mygif89a->delay;
					else
						dtime = 10; //默认延时
					// while (dtime-- && gifdecoding_loop)
					// 	delay_ms(10); //延迟
					if (res == 2)
					{
						stop_release = 1;
						res = 0;
						//break;
					}
					LCD_Exec(); //更新一帧到屏幕
				}
				else
				{
					stop_release = 1;
				}
			}
			else
			{
				stop_release = 1;
			}
		}
		else
		{
			stop_release = 1;
		}
	}

	if (gifdecoding_loop)
	{
		if (strcmp(filename_temp, (char *)filename) != 0) //文件变换
		{
			//不同文件, 先停止, 下次再开始
			stop_release = 1;
		}
	}

	if (stop_release) //结束释放
	{
		res = 0X40; //gif停止
		if (file_open_ok)
			MTF_close(gfile);
		file_open_ok = 0;
		gifdecoding_loop = 0;
#if GIF_USE_MALLOC == 1 //定义是否使用malloc
		pic_memfree(mygif89a->lzw);
		mygif89a->lzw = NULL; //free后变量不变, 若再次free就卡死
		pic_memfree(mygif89a);
		mygif89a = NULL;
#endif
	}
	return res;
}
 */

typedef struct
{
	u16 dtime;				 //解码延时
	u8 gifdecoding_loop;		 //标记解码运行中
	LZW_INFO LZWstruct;
	gif89a gif89astruct;
	gif89a *mygif89a;
	mFILE *gfile;
}gifDecodeTemp;

typedef struct 
{
	int id;
	char state;
	unsigned char loop_cnt;
	int x;
	int y;
	gifDecodeTemp *temp;
}gifDecodeDevice;

#define GIF_MAX_CNT 8 //最大同时播放gif数量
static gifDecodeDevice *gif_decode_table[GIF_MAX_CNT] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}; //保存gif播放变量
static gifDecodeDevice *gif_decode_id_create(int id)
{
	gifDecodeDevice *gif_device = (gifDecodeDevice *)pic_memalloc(sizeof(gifDecodeDevice));
	gifDecodeTemp *gif_temp = (gifDecodeTemp *)pic_memalloc(sizeof(gifDecodeTemp));

	if (gif_temp != NULL && gif_device != NULL && id < GIF_MAX_CNT && gif_decode_table[id] == NULL)
	{
		gif_device->temp = gif_temp;
		gif_device->id = id;
		gif_decode_table[id] = gif_device;
		return gif_device;
	}
	else
	{
		pic_memfree(gif_device);
		pic_memfree(gif_temp);
		return NULL;
	}
}

//关闭gif, 释放资源
static void gif_decode_exit(int id)
{
	gifDecodeDevice *gif_D = gif_decode_table[id];
	gifDecodeTemp *gif_temp = gif_D->temp;

	if(gif_decode_table[id] == NULL)
		return;

	MTF_close(gif_temp->gfile);
	pic_memfree(gif_D);
	pic_memfree(gif_temp);
	gif_decode_table[id] = NULL;
}

//循环执行解码, gif_decode_loop gif_decode_init gif_decode_set三者必需互斥执行
unsigned char gif_decode_loop(void)
{
	u16 width, height;
	u8 res;
	u8 stop_release; //结束释放
	u8 stop_init;	 //一次循环结束, 初始化部分参数
	int id;
	char _run_decode; //运行一次解码

	//设备变量
	gif89a *mygif89a;
	mFILE *gfile;
	u16 *dtime;			  //解码延时
	u8 *gifdecoding_loop; //标记解码运行中

	u8 state;
	u8 *loop_cnt;
	u16 x;
	u16 y;

	for (id = 0; id < GIF_MAX_CNT; id++)
	{
		if (gif_decode_table[id] == NULL)
		{
			continue;
		}
		else
		{
			width = 0, height = 0;
			res = 0;
			stop_release = 0; //结束释放
			stop_init = 0;	  //一次循环结束, 初始化部分参数
			_run_decode = 0; //运行一次解码

			mygif89a = gif_decode_table[id]->temp->mygif89a;
			gfile = gif_decode_table[id]->temp->gfile;
			dtime = &(gif_decode_table[id]->temp->dtime);						 //解码延时
			gifdecoding_loop = &(gif_decode_table[id]->temp->gifdecoding_loop); //标记解码运行中

			state = gif_decode_table[id]->state;
			loop_cnt = &gif_decode_table[id]->loop_cnt;
			x = gif_decode_table[id]->x;
			y = gif_decode_table[id]->y;

			if (state == 0) //停止
			{
				if (*gifdecoding_loop)
				{
					stop_release = 1;
					res = 0;
				}
				else
				{
					return 0;
				}
			}
			else if (state == 1) //暂停
			{
				return 0;
			}
			else if (*gifdecoding_loop) //循环解码一帧
			{
				*dtime = *dtime - 1;
				if (*dtime)
					return 0;
				_run_decode = 1;
			}
			else //开始解码
			{
				if (*loop_cnt) //有没循环次数
				{
					if (*loop_cnt != 255) //255为无限循环
						*loop_cnt -= 1;
				}
				else
				{
					return 0;
				}

				MTF_seek(gfile, 0, SEEK_SET);
				if (gif_check_head(gfile))
					res = PIC_FORMAT_ERR;
				if (gif_getinfo(gfile, mygif89a))
					res = PIC_FORMAT_ERR;

				width = mygif89a->gifLSD.width;
				height = mygif89a->gifLSD.height;
				if (x >= 0 && x <= lcddev.width - 1 && y >= 0 && y <= lcddev.height - 1)
				{
					if (x + width - 1 >= 0 && x + width - 1 <= lcddev.width - 1 &&
						y + height - 1 >= 0 && y + height - 1 <= lcddev.height - 1)
					{
						x = (width - mygif89a->gifLSD.width) / 2 + x;
						y = (height - mygif89a->gifLSD.height) / 2 + y;
					}
					else
					{
						res = PIC_WINDOW_ERR;
					}
				}
				else
				{
					res = PIC_WINDOW_ERR;
				}

				*gifdecoding_loop = 1;			   //decode运行
				_run_decode = 1;
			}

			if (_run_decode)
			{
				if (*gifdecoding_loop && res == 0) //解码过程
				{
					res = gif_drawimage(gfile, mygif89a, x, y); //显示一张图片
					if (mygif89a->gifISD.flag & 0x80)
						gif_recovergctbl(mygif89a); //恢复全局颜色表
					if (mygif89a->delay)
						*dtime = mygif89a->delay;
					else
						*dtime = 10; //默认延时
					// while ((*dtime)-- && *gifdecoding_loop)
					// 	delay_ms(10); //延迟
					if (res == 2)
					{
						stop_init = 1;
						res = 0;
						//break;
					}
					LCD_Exec(); //更新一帧到屏幕
				}
				else
				{
					stop_release = 1;
				}
			}

			if (stop_init) //一次循环结束, 初始参数
			{
				res = 0X40; //gif停止
				*gifdecoding_loop = 0;
			}
			if (stop_release) //结束释放
			{
				res = 0X40; //gif停止
				gif_decode_exit(id);
			}
			return res;
		}
	}
	return 0;
}

//初始化gif资源
//解码一个gif文件
//filename:带路径的gif文件名字
char gif_decode_init(const char *filename, int id)
{
	gifDecodeDevice *gif_D;
	gifDecodeTemp *gif_temp;

	gif_D = gif_decode_id_create(id);
	if(gif_D!=NULL)
	{
		gif_temp = gif_D->temp;
		gif_temp->mygif89a = &gif_temp->gif89astruct;
		gif_temp->mygif89a->lzw = &gif_temp->LZWstruct;
		gif_temp->dtime = 0;			//解码延时
		gif_temp->gifdecoding_loop = 0; //标记解码运行中
		gif_temp->gfile = MTF_open((const char *)filename, "rb");
		if (gif_temp->gfile == NULL)
		{
			pic_memfree(gif_D);
			pic_memfree(gif_temp);
			gif_decode_table[id] = NULL;
			return -4;
		}
		return gif_D->id;
	}
	else
	{
		return -1;
	}
}

//设置gif播放方式
//state: 播放控制: 0停止, 1暂停, 2开始; x,y: 显示坐标.
void gif_decode_set(int id, char state, unsigned char loop_cnt, int x, int y)
{
	gifDecodeDevice *gif_D = gif_decode_table[id];

	if (gif_decode_table[id] == NULL)
		return;

	gif_D->loop_cnt = loop_cnt;
	gif_D->state = state;
	gif_D->x = x;
	gif_D->y = y;

	if (state == 0)
		gif_decode_exit(id);
}
