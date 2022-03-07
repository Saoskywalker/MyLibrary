#include “ff.h”  //unicode~GBK码表和函数
#include “malloc.h”
#include "types_plus.h"

/*************************************************************************************************
 * https://blog.csdn.net/bladeandmaster88/article/details/54837338
* 将UTF8编码转换成Unicode（UCS-2LE）编码  低地址存低位字节
* 参数：
*    char* pInput     输入字符串
*    char*pOutput   输出字符串
* 返回值：转换后的Unicode字符串的字节数，如果出错则返回-1
**************************************************************************************************/

//utf8转unicode 没测试,,,可能有BUG
int Utf8ToUnicode(char *pInput, char *pOutput)
{
    int outputSize = 0; //记录转换后的Unicode字符串的字节数

    while (*pInput)
    {
        if (*pInput > 0x00 && *pInput <= 0x7F) //处理单字节UTF8字符（英文字母、数字）
        {
            *pOutput = *pInput;
            pOutput++;
            *pOutput = 0; //小端法表示，在高地址填补0
        }
        else if (((*pInput) & 0xE0) == 0xC0) //处理双字节UTF8字符
        {
            char high = *pInput;
            pInput++;
            char low = *pInput;
            if ((low & 0xC0) != 0x80) //检查是否为合法的UTF8字符表示
            {
                return -1; //如果不是则报错
            }

            *pOutput = (high << 6) + (low & 0x3F);
            pOutput++;
            *pOutput = (high >> 2) & 0x07;
        }
        else if (((*pInput) & 0xF0) == 0xE0) //处理三字节UTF8字符
        {
            char high = *pInput;
            pInput++;
            char middle = *pInput;
            pInput++;
            char low = *pInput;
            if (((middle & 0xC0) != 0x80) || ((low & 0xC0) != 0x80))
            {
                return -1;
            }
            *pOutput = (middle << 6) + (low & 0x3F); //取出middle的低两位与low的低6位，组合成unicode字符的低8位
            pOutput++;
            *pOutput = (high << 4) + ((middle >> 2) & 0x0F); //取出high的低四位与middle的中间四位，组合成unicode字符的高8位
        }
        else //对于其他字节数的UTF8字符不进行处理
        {
            return -1;
        }
        pInput++; //处理下一个utf8字符
        pOutput++;
        outputSize += 2;
    }
    //unicode字符串后面，有两个\0
    *pOutput = 0;
    pOutput++;
    *pOutput = 0;
    return outputSize;
}

//unicode转utf8 没测试,,,可能有BUG
int UnicodeToUtf8(char *pInput, char *pOutput)
{
    int len = 0; //记录转换后的Utf8字符串的字节数
    while (*pInput)
    {
        //处理一个unicode字符
        char low = *pInput; //取出unicode字符的低8位
        pInput++;
        char high = *pInput; //取出unicode字符的高8位
        int w = high << 8;
        unsigned wchar = (high << 8) + low; //高8位和低8位组成一个unicode字符,加法运算级别高

        if (wchar <= 0x7F) //英文字符
        {
            pOutput[len] = (char)wchar; //取wchar的低8位
            len++;
        }
        else if (wchar >= 0x80 && wchar <= 0x7FF) //可以转换成双字节pOutput字符
        {
            pOutput[len] = 0xc0 | ((wchar >> 6) & 0x1f); //取出unicode编码低6位后的5位，填充到110yyyyy 10zzzzzz 的yyyyy中
            len++;
            pOutput[len] = 0x80 | (wchar & 0x3f); //取出unicode编码的低6位，填充到110yyyyy 10zzzzzz 的zzzzzz中
            len++;
        }
        else if (wchar >= 0x800 && wchar < 0xFFFF) //可以转换成3个字节的pOutput字符
        {
            pOutput[len] = 0xe0 | ((wchar >> 12) & 0x0f); //高四位填入1110xxxx 10yyyyyy 10zzzzzz中的xxxx
            len++;
            pOutput[len] = 0x80 | ((wchar >> 6) & 0x3f); //中间6位填入1110xxxx 10yyyyyy 10zzzzzz中的yyyyyy
            len++;
            pOutput[len] = 0x80 | (wchar & 0x3f); //低6位填入1110xxxx 10yyyyyy 10zzzzzz中的zzzzzz
            len++;
        }

        else //对于其他字节数的unicode字符不进行处理
        {
            return -1;
        }
        pInput++; //处理下一个unicode字符
    }
    //utf8字符串后面，有个\0
    pOutput[len] = 0;
    return len;
}

/************************************************************************ 
 * https://blog.csdn.net/qq_39575645/article/details/82053929
 * UNICODE和GBK互转使用FATFS的码表和函数
* 函数名称: StrProcess_UTF8toGBK 
* 函数功能: 将网络上 utf8 的字符串转换为 GBK 字符串 使得兼容 原子LCD屏驱动程序 
* 函数输入: void input: c_utf8 原utf8 字符串 同时 新的 gbk字符串会将其覆盖 
* length 你想要设置的 中间缓存块的大小 
* 函数输出: void output:字符总数 (一个中文算两个字节) 
* 作者 :author:@Kim_alittle_star 
* 文件依存: 
* #include “ff.h” 
* #include “malloc.h” 
************************************************************************/

unsigned int StrProcess_UTF8toGBK(unsigned char *c_utf8, unsigned int length)
{
    // ff_uni2oem(,FF_CODE_PAGE);
    /* !< 首先 将 utf8 转换成标准 Unicode 然后查表将 Unicode 转化为GBK */
    u16 outputSize = 0; //记录转换后的gbk字符串长度
    u8 *pInput = (u8 *)c_utf8;
    u8 *c_gbk = malloc(length); /* !< 申请内存,也可以外部传入一个 数据缓存空间 */
    u8 *pOutput = c_gbk;
    u16 *uni = (u16 *)c_gbk;
    u16 gbk;
    /* !< 以下中间代码来自于 CSDN @bladeandmaster88 公开的源码 */
    while (*pInput)
    {
        if (*pInput > 0x00 && *pInput <= 0x7F) //处理单字节UTF8字符（英文字母、数字）
        {
            *pOutput = *pInput;
            pOutput += 1;
            *pOutput = 0; //小端法表示，在高地址填补0
        }
        else if (((*pInput) & 0xE0) == 0xC0) //处理双字节UTF8字符
        {
            char high = *pInput;
            pInput += 1;
            char low = *pInput;
            if ((low & 0xC0) != 0x80) //检查是否为合法的UTF8字符表示
            {
                return -1; //如果不是则报错
            }
            *pOutput = (high << 6) + (low & 0x3F);
            pOutput++;
            *pOutput = (high >> 2) & 0x07;
        }
        else if (((*pInput) & 0xF0) == 0xE0) //处理三字节UTF8字符
        {
            char high = *pInput;
            pInput++;
            char middle = *pInput;
            pInput++;
            char low = *pInput;
            if (((middle & 0xC0) != 0x80) || ((low & 0xC0) != 0x80))
            {
                return -1;
            }
            *pOutput = (middle << 6) + (low & 0x3F); //取出middle的低两位与low的低6位，组合成unicode字符的低8位
            pOutput++;
            *pOutput = (high << 4) + ((middle >> 2) & 0x0F); //取出high的低四位与middle的中间四位，组合成unicode字符的高8位
        }
        else //对于其他字节数的UTF8字符不进行处理
        {
            return -1;
        }
        pInput++; //处理下一个utf8字符
        pOutput++;
    }
    //unicode字符串后面，有两个\0
    *pOutput = 0;
    pOutput++;
    *pOutput = 0;
    /* !< 感谢 @bladeandmaster88 的开源支持 */
    pInput = c_utf8;
    while (*uni != 0)
    {
        gbk = ff_uni2oem(*uni, FF_CODE_PAGE); /* !< Unicode 向 GBK 转换函数 此次没实现 */
        uni++;
        if (gbk & 0xff00)
        {
            *pInput = ((gbk & 0xff00) >> 8);
            pInput++;
            *pInput = (gbk & 0x00ff);
            pInput++;
            outputSize += 2;
        }
        else
        {
            *pInput = (gbk & 0x00ff);
            pInput++;
            outputSize++;
        }
    }
    *pInput = '\0';   /* !< 加上结束符号 */
    free(c_gbk); /* !< 释放内存 */
    return outputSize;
}
