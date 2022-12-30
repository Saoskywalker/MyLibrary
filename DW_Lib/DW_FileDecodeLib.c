#include "MTF_io.h"
#include <stdint.h>
#include <malloc.h>
#include <string.h>
#include "file_operate_hal.h" 
#include "DW_FileDecodeLib.h"
#include "cJSON.h"
#include "dwDriver.h"
#include "dwDriveD2.h"

/* Create a bunch of objects as demonstration. */
static int DW_print_preallocated(char *name, cJSON *root)
{
    /* declarations */
    char *out = NULL;
    char *buf = NULL;
    char *buf_fail = NULL;
    size_t len = 0;
    size_t len_fail = 0;
    mFILE *fp = NULL;
    size_t total = 1;

    /* formatted print 注意释放out内存*/
    out = cJSON_Print(root);

    /* create buffer to succeed */
    /* the extra 5 bytes are because of inaccuracies when reserving memory */
    len = strlen(out) + 5;
    buf = (char*)malloc(len);
    if (buf == NULL)
    {
        printf("Failed to allocate memory.\n");
        return 1;
    }

    /* create buffer to fail */
    len_fail = strlen(out);
    buf_fail = (char*)malloc(len_fail);
    if (buf_fail == NULL)
    {
        printf("Failed to allocate memory.\n");
        return 1;
    }

    /* Print to buffer */
    if (!cJSON_PrintPreallocated(root, buf, (int)len, 1)) {
        printf("cJSON_PrintPreallocated failed!\n");
        if (strcmp(out, buf) != 0) {
            printf("cJSON_PrintPreallocated not the same as cJSON_Print!\n");
            printf("cJSON_Print result:\n%s\n", out);
            printf("cJSON_PrintPreallocated result:\n%s\n", buf);
        }
        free(out);
        free(buf_fail);
        free(buf);
        return -1;
    }

    /* success */
    printf("%s\n", buf); //打印输出JSON

    /*输出JSON文件*/
    if (name != NULL)
    {
        while (buf[total]) //检查字节数
        {
            total++;
        }
        fp = MTF_open(name, "wb+"); //读写生成打开
        MTF_write(buf, 1, total, fp);
        MTF_close(fp);
    }

    /* force it to fail */
    if (cJSON_PrintPreallocated(root, buf_fail, (int)len_fail, 1)) {
        printf("cJSON_PrintPreallocated failed to show error with insufficient memory!\n");
        printf("cJSON_Print result:\n%s\n", out);
        printf("cJSON_PrintPreallocated result:\n%s\n", buf_fail);
        free(out);
        free(buf_fail);
        free(buf);
        return -1;
    }

    free(out);
    free(buf_fail);
    free(buf);
    return 0;
}

#define DW_MAX_PAGE 256
#define STRING_TEMP_SIZE 256*3 //256为待转换最大字节数, 转换后, 1字节变两个字符加一个空格(字符串末尾空格变为0), 所以为3

static size_t _old_num = 65535; //由于新旧对比生成新页, 禁止头为65535, 不然此页生成出错
static size_t hmiCmdCnt = 0;
static uint8_t *string_temp = NULL;
static cJSON *HMIConfig = NULL, *HmiCfg_display_number = NULL, *HmiCfg_array = NULL, *HmiCfg_control = NULL,
        *HmiCfg_page_run = NULL;

//库初始化
int HMI_JSON_create_init(void)
{
    _old_num = 65535;
    hmiCmdCnt = 0;
    string_temp = (uint8_t *)malloc(STRING_TEMP_SIZE); 

    if(string_temp==NULL)
        return 1; //失败
    else
        return 0; //成功
}

//库释放
int HMI_JSON_remove(void)
{
    free(string_temp);
    return 0; //成功 
}

/*********用于把运行命令生成JSON字符对象**************/
static uint8_t *HmiJsonBtos(uint8_t *b, size_t count)
{
    size_t j = 0, i = 0;
    uint8_t t = 0;

    if (b == NULL || count == 0)
        return NULL;

    for (i = 0, j = 0; j < count; i += 3, j++)
    {
        t = (b[j] & 0XF0) >> 4;
        if (t > 9)
            string_temp[i] = t + '7'; //A~F
        else
            string_temp[i] = t + '0'; //0~9

        t = b[j] & 0X0F;

        if (t > 9)
            string_temp[i + 1] = t + '7'; //A~F
        else
            string_temp[i + 1] = t + '0'; //0~9

        string_temp[i + 2] = ' ';
    }
    string_temp[i - 1] = 0; //字符串结尾

    return string_temp;
} 

static size_t HmiRunCodeNumCreate(void)
{
    //生成新运行代码号
    return ++hmiCmdCnt;
}

static cJSON *HmiArray(cJSON *object, char *sss)
{
    //生成某个数组
    cJSON *_type = NULL;
    
    _type = cJSON_GetObjectItem(object, sss); //检查是否生成过
    if (_type == NULL)                           //生成新组
    {
        _type = cJSON_CreateArray();
        cJSON_AddItemToObject(object, sss, _type);
    }
    return _type;
}

static int HmiRunCodeGenerate(char *sss)
{
    uint8_t *data = NULL, *ss = NULL;
    unsigned int *cnt = NULL;

	dwGetSendData(&data, &cnt); //生成命令数组
    if (*cnt * 3 > STRING_TEMP_SIZE)
        return 0;                //过长
    ss = HmiJsonBtos(data, (size_t)(*cnt)); //命令数组转换为字符串
    if(ss==NULL)
    {
        return 0; //失败
    }
    else
    {
        return cJSON_AddItemToArray(HmiArray(HMIConfig, sss), 
                                    cJSON_CreateString((const char *)ss)); //放入JSON
    }
}
/***********************************************/

//页生成, 并初始化控件类型
static void HmiPageCreate(size_t new_num)
{
    char sss[32] = {0};

    if (_old_num != new_num) //新旧页号不一致
    {
        sprintf(sss, "display-number@%d", (int)new_num);
        HmiCfg_display_number = cJSON_GetObjectItem(HMIConfig, sss); //检查页面是否生成过
        if (HmiCfg_display_number == NULL)
        {
            HmiCfg_display_number = cJSON_AddObjectToObject(HMIConfig, sss); //新增页号

            memset(sss, 0, sizeof(sss)); //进入页时自动运行的命令
            sprintf(sss, "code-group@%d", (int)HmiRunCodeNumCreate());
            HmiCfg_page_run = cJSON_AddStringToObject(HmiCfg_display_number, "run", sss);
        }
        else
        {
            HmiCfg_page_run = cJSON_GetObjectItem(HmiCfg_display_number, "run");
        }
        _old_num = new_num;
    }
}

//按键值返回
static void HmiTouchKey(uint8_t *_fptr, size_t *afterOffset)
{
    //新增控件
    HmiCfg_control = cJSON_CreateObject();
    HmiCfg_array = HmiArray(HmiCfg_display_number, "key"); //检查并生成数组
    cJSON_AddItemToArray(HmiCfg_array, HmiCfg_control); //放到key组

    cJSON_AddStringToObject(HmiCfg_control, "name", ""); 
    cJSON_AddStringToObject(HmiCfg_control, "type", "click");
    cJSON_AddBoolToObject(HmiCfg_control, "storage-bool", false);

    if (_fptr[14] != 0XFF) //判断是否发送键值
    {
        if (_fptr[14] == 0XFE) //上传
        {
            if (_fptr[16] == 0XFE) //DGUS每个触控的扩展标志(以16个字节为一组, 第二组开始, 每组开头0XFE)
            {
                cJSON_AddNumberToObject(HmiCfg_control, "key-code", 
                                            ((uint16_t)_fptr[20] << 8) | (_fptr[21])); //键值
                cJSON_AddNumberToObject(HmiCfg_control, "address",
                                        ((uint16_t)_fptr[17] << 8) + _fptr[18]); //控件号
                cJSON_AddBoolToObject(HmiCfg_control, "auto-send", true); //自动上传键值
            }
            *afterOffset = 32; //下一控件存储偏移值
        }
        else 
        {
            if (_fptr[14] == 0XFD) //不上传
            {
                if (_fptr[16] == 0XFE) //DGUS每个触控的扩展标志(以16个字节为一组, 第二组开始, 每组开头0XFE)
                {
                    cJSON_AddNumberToObject(HmiCfg_control, "key-code", 
                                            ((uint16_t)_fptr[20] << 8) | (_fptr[21])); //键值
                    cJSON_AddNumberToObject(HmiCfg_control, "address",
                                            ((uint16_t)_fptr[17] << 8) + _fptr[18]); //控件号
                    cJSON_AddBoolToObject(HmiCfg_control, "auto-send", false);    //不上传键值
                }
                *afterOffset = 32; //下一控件存储偏移值
            }
        }
    }
}

//指令集版本, 按键值返回
static void HmiTouchKey_Command(uint8_t *_fptr, size_t *afterOffset)
{
    //新增控件
    HmiCfg_control = cJSON_CreateObject();
    HmiCfg_array = HmiArray(HmiCfg_display_number, "key"); //检查并生成数组
    cJSON_AddItemToArray(HmiCfg_array, HmiCfg_control); //放到key组
    
    cJSON_AddStringToObject(HmiCfg_control, "name", "");
    cJSON_AddStringToObject(HmiCfg_control, "type", "click");
    cJSON_AddBoolToObject(HmiCfg_control, "storage-bool", false);

    if (_fptr[14] != 0XFF) //判断是否发送键值
    {
        cJSON_AddNumberToObject(HmiCfg_control, "key-code", 
                                ((uint16_t)_fptr[14] << 8) | (_fptr[15])); //键值
        cJSON_AddNumberToObject(HmiCfg_control, "address", 4224); //固定控件号: 0X1080
        cJSON_AddBoolToObject(HmiCfg_control, "auto-send", true); //自动上传键值
    }
    *afterOffset = 16; //下一控件存储偏移值
}

int DW_TouchFileDecode(uint8_t *_fptr, size_t file_size)
{
    char sss[16] = {0};
    size_t dest = 0, afterOffset = 0;
    int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    uint8_t version = 0;

    dest = 0;
    while (_fptr[dest] != 0XFF && _fptr[dest + 1] != 0XFF)
    {
        if(dest >= file_size)
            return 0;
        
        HmiPageCreate(_fptr[dest + 1]); //需要判断是否同一页

        //识别触控功能类型
        if(_fptr[dest + 14]==0XFD||_fptr[dest + 14]==0XFE) //区分指令集版本和DGUS版本
        { //DGUS版本
            if (_fptr[dest + 15] == 0X05) //按键值返回
                HmiTouchKey(&_fptr[dest], &afterOffset);
            //在此添加类型
            else
                break; //遇到无适配类型就退出解释
        }
        else
        { //指令集版本
            version = 1;
            HmiTouchKey_Command(&_fptr[dest], &afterOffset);
        }

        //按键范围
        x1 = ((int)_fptr[dest + 2] << 8) + _fptr[dest + 3];
        y1 = ((int)_fptr[dest + 4] << 8) + _fptr[dest + 5];
        x2 = ((int)_fptr[dest + 6] << 8) + _fptr[dest + 7];
        y2 = ((int)_fptr[dest + 8] << 8) + _fptr[dest + 9];
        cJSON_AddNumberToObject(HmiCfg_control, "x1", x1); 
        cJSON_AddNumberToObject(HmiCfg_control, "y1", y1); 
        cJSON_AddNumberToObject(HmiCfg_control, "x2", x2); 
        cJSON_AddNumberToObject(HmiCfg_control, "y2", y2);  

        //按下
        if (((uint16_t)_fptr[dest + 12] << 8) + _fptr[dest + 13] != 0XFF00)
        { //有按下效果
            dwCutPic(_fptr[dest + 13], x1, y1, x2, y2, x1, y1);
            //生成按下运行命令
            memset(sss, 0, sizeof(sss));
            sprintf(sss, "code-group@%d", (int)HmiRunCodeNumCreate());
            cJSON_AddStringToObject(HmiCfg_control, "run-down", sss);
            HmiRunCodeGenerate(sss);
        }

        //松开
        if (((uint16_t)_fptr[dest + 10] << 8) + _fptr[dest + 11] != 0XFF00)
        { //跳转页面
            dwDisPicNoL(_fptr[dest + 11]);
            memset(sss, 0, sizeof(sss));
            sprintf(sss, "code-group@%d", (int)HmiRunCodeNumCreate());
            cJSON_AddStringToObject(HmiCfg_control, "run-up", sss);
            HmiRunCodeGenerate(sss);
        }
        else if (((uint16_t)_fptr[dest + 12] << 8) + _fptr[dest + 13] != 0XFF00)
        { //恢复按钮
            dwCutPic(_fptr[dest + 1], x1, y1, x2, y2, x1, y1);
            memset(sss, 0, sizeof(sss));
            sprintf(sss, "code-group@%d", (int)HmiRunCodeNumCreate());
            cJSON_AddStringToObject(HmiCfg_control, "run-up", sss);
            HmiRunCodeGenerate(sss);
        }

        dest = dest + afterOffset; //下一个控件偏移
    }

    if (version == 1)
    {
        //更新版本号: 指令集HMI版本号
        cJSON_ReplaceItemInObject(HMIConfig, "version", cJSON_CreateString("MTF_HMI_COMMMAND"));
    }

    return 0; //success
}

/************显示控件组成***********/
//ctrl[0]==0X5A //头, 固定
//ctrl[1] //控件类型
//((uint16_t)ctrl[2]<<8)|ctrl[3] //SP指针: 指定控件描述内容的存放位置(从ROM加载后存储到RAM的地址指针, 一般0XFFFF, 系统自定)
//(((uint16_t)ctrl[4]<<8)|ctrl[5])*2 //控件描述内容长度(字节)
//((uint16_t)ctrl[6]<<8)|ctrl[7] //VP指针: 控件号0x0000-0xFFFF，有些无需指定地址的，写0x0000即可。
//后面为详细描述内容
//控件描述内容(组成: 控件号至控件描述内容长度最后字节)
/********************************************/

static uint32_t _RGB565to888(uint16_t pencolor) //rgb565转rgb888,透明通道为非透
{
	uint32_t tempColor = 0;
	tempColor = (pencolor & 0X0000001F) << 3;  //B
	tempColor |= (pencolor & 0X000007E0) << 5; //G
	tempColor |= ((pencolor & 0X0000F800) << 8) | 0XFF000000; //R & alpha
	return tempColor;
}

//文本显示
int HmiDisText(uint8_t *_fptr)
{
    uint8_t *ss = NULL;
    uint8_t temp[3] = {0};
    int x1 = 0, y1 = 0;
    uint32_t color = 0;

    if(_fptr[0]!=0X5A) //头
        return 1; //fail

    _fptr += 6; //控件描述内容偏移

    //新增控件
    HmiCfg_control = cJSON_CreateObject();
    HmiCfg_array = HmiArray(HmiCfg_display_number, "text"); //检查并生成数组
    cJSON_AddItemToArray(HmiCfg_array, HmiCfg_control); //放到text组

    cJSON_AddStringToObject(HmiCfg_control, "name", ""); 
    cJSON_AddNumberToObject(HmiCfg_control, "address", ((uint16_t)_fptr[0] << 8) + _fptr[1]); //控件号

    x1 = ((int)_fptr[2]<<8)|_fptr[3];
    y1 = ((int)_fptr[4]<<8)|_fptr[5];
    cJSON_AddNumberToObject(HmiCfg_control, "x1", x1); 
    cJSON_AddNumberToObject(HmiCfg_control, "y1", y1);

    color = _RGB565to888(((uint16_t)_fptr[6]<<8)|_fptr[7]);
    temp[0] = (color&0X00FF0000)>>16; //R
    temp[1] = (color&0X0000FF00)>>8; //G
    temp[2] = (color&0X000000FF); //B
    ss = HmiJsonBtos(temp, 3);
    cJSON_AddStringToObject(HmiCfg_control, "color", (char *)ss);

    cJSON_AddNumberToObject(HmiCfg_control, "font", _fptr[19]);
    cJSON_AddNumberToObject(HmiCfg_control, "size", _fptr[21]); 

    //每页进入时自动运行命令, 显示
    dwD2DisString(((uint16_t)_fptr[0] << 8) + _fptr[1], (unsigned char *)"");
    HmiRunCodeGenerate(HmiCfg_page_run->valuestring);

    return 0; //success
}

//变量图标显示
int HmiDisPic(uint8_t *_fptr)
{
    int x1 = 0, y1 = 0;

    if (_fptr[0] != 0X5A) //头
        return 1;         //fail

    _fptr += 6; //控件描述内容偏移

    //新增控件
    HmiCfg_control = cJSON_CreateObject();
    HmiCfg_array = HmiArray(HmiCfg_display_number, "picture"); //检查并生成数组
    cJSON_AddItemToArray(HmiCfg_array, HmiCfg_control);     //放到text组

    cJSON_AddStringToObject(HmiCfg_control, "name", "");
    cJSON_AddNumberToObject(HmiCfg_control, "address", ((uint16_t)_fptr[0] << 8) + _fptr[1]); //控件号

    x1 = ((int)_fptr[2] << 8) | _fptr[3];
    y1 = ((int)_fptr[4] << 8) | _fptr[5];
    cJSON_AddNumberToObject(HmiCfg_control, "x1", x1);
    cJSON_AddNumberToObject(HmiCfg_control, "y1", y1);

    cJSON_AddNumberToObject(HmiCfg_control, "photo", _fptr[14]);
    cJSON_AddNumberToObject(HmiCfg_control, "mode", _fptr[15]);
    
    //每页进入时自动运行命令, 显示
    dwD2DisICO(((uint16_t)_fptr[0] << 8) + _fptr[1], 0);
    HmiRunCodeGenerate(HmiCfg_page_run->valuestring);

    return 0; //success
}

static int DW_DisCtrlDecode(uint8_t *_fptr)
{ //control decode and put to json
    int res = 0;
    
    switch (_fptr[1]) //check ctrl type
    {
    case 0X11: res = HmiDisText(_fptr); break; //文本显示
    case 0X00: res = HmiDisPic(_fptr); break; //变量图标显示
    default: res = 2; break; //fail 
    }

    return res; //success
}

int DW_DisFileDecode(uint8_t *_fptr, size_t file_size)
{
    size_t i = 0, j = 0;
    //this file composition: 1.file head; 2. every page control number; 3.control information
    const char head_code[] = {0X14, 0X44, 0X47, 0X55, 0X53, 0X5F, 0X32}; //file head
    uint8_t *fptr_page = _fptr+18; //every page control number
    uint8_t *fptr_control = _fptr+0X4000; //control information start

    //check head 
    for (i = 0; i < sizeof(head_code); i++)
    {
        if (_fptr[i]!=head_code[i])
            return 1; //fail
    }

    for (i = 0; i < 256; i++) //here page number max 256
    {
        if ((*fptr_page & 0XF0) == 0X40) //page tail symbol
        {
            //this page has control number
            if (*(fptr_page - 2) != 0) //此页有控件
            {
                HmiPageCreate(i); //生成此页JSON

                for (j = 0; j < *(fptr_page - 2); j++)
                {
                    if (fptr_control - _fptr >= file_size)
                        return 2;              //fail
                    if (*fptr_control == 0XFF) //file tail is 0XFF
                        return 0;
                    if (*fptr_control == 0X5A) //control start is 0X5A
                        DW_DisCtrlDecode(fptr_control);
                    fptr_control += 32; //next controll info offset is 32
                }
            }

            fptr_page += 4; //next number info offset is 4
        }
        else
        {
            return 2; //fail;
        }
    }

    return 0; //success
}

void DW_FileDecode(char *nameDest, char *nameTouchSrc, char *nameDisSrc)
{
    int error = 0;
    unsigned char *_fptr = NULL;
    size_t file_size = 0;
    unsigned char _ok = 0;

    HMI_JSON_create_init();
    dwMount();

    HMIConfig = cJSON_CreateObject();
    cJSON_AddStringToObject(HMIConfig, "version", "MTF_HMI"); //HMI版本号

    dwD2DisPicNoL(0); //默认开机0号图片
    HmiRunCodeNumCreate();
    cJSON_AddStringToObject(HMIConfig, "run", "code-group@1"); //开机运行命令组为1 
    HmiRunCodeGenerate("code-group@1");               
    
    error = MTF_load_file(&_fptr, &file_size, nameTouchSrc);
    if(!error)
    {
        error = DW_TouchFileDecode(_fptr, file_size);
        if (!error)
            _ok = 1;
        free(_fptr);
    }

    error = MTF_load_file(&_fptr, &file_size, nameDisSrc);
    if(!error)
    {
        error = DW_DisFileDecode(_fptr, file_size);
        if (!error)
            _ok = 1;
        free(_fptr);
    }

    if (_ok)
    {
        /* Print to text */
        if (DW_print_preallocated(nameDest, HMIConfig) != 0)
            printf("output error\r\n");
        cJSON_Delete(HMIConfig);
    }
    else
    {
        printf("decode error: %d\r\n", error);
    }

    dwRemove();
    HMI_JSON_remove();
}

void *DW_FileDecodeNoCreate(char *nameTouchSrc, char *nameDisSrc)
{
    int error = 0;
    unsigned char *_fptr = NULL;
    size_t file_size = 0;
    unsigned char _ok = 0;
    cJSON *res = NULL;

    HMI_JSON_create_init();
    dwMount();

    HMIConfig = cJSON_CreateObject();
    cJSON_AddStringToObject(HMIConfig, "version", "MTF_HMI"); //HMI版本号

    dwD2DisPicNoL(0); //默认开机0号图片
    HmiRunCodeNumCreate();
    cJSON_AddStringToObject(HMIConfig, "run", "code-group@1"); //开机运行命令组为1 
    HmiRunCodeGenerate("code-group@1");               
    
    error = MTF_load_file(&_fptr, &file_size, nameTouchSrc);
    if(!error)
    {
        error = DW_TouchFileDecode(_fptr, file_size);
        if (!error)
            _ok = 1;
        free(_fptr);
    }

    error = MTF_load_file(&_fptr, &file_size, nameDisSrc);
    if(!error)
    {
        error = DW_DisFileDecode(_fptr, file_size);
        if (!error)
            _ok = 1;
        free(_fptr);
    }

    if (_ok)
    {
        /* Print to text */
        // if (DW_print_preallocated(nameDest, HMIConfig) != 0)
        //     printf("output error\r\n");
        // cJSON_Delete(HMIConfig);
        res = HMIConfig;
    }
    else
    {
        printf("decode error: %d\r\n", error);
    }

    dwRemove();
    HMI_JSON_remove();

    return res;
}
