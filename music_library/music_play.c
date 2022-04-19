#include "audio_port.h"
#include "MTF_io.h"
#include <string.h>

#define MUSIC_DEBUG(...) //printf(__VA_ARGS__)

static int play_exit = 1; //播放退出标志
static unsigned char playMusicState = 0;
static unsigned char wav_buff[2][70000] __attribute__((aligned(4)));
struct wav_
{
    uint16_t audio_format;       //格式
    uint16_t num_channels;       //通道数
    uint32_t format_blok_lenght; //格式块长度
    uint32_t sample_rate;        //采集频率
    uint32_t byte_rate;          //数据传输率
    uint16_t bits_per_sample;    //采样位宽度
    uint32_t data_size;          //数据区大小
    uint32_t play_time_ms;       //播放时长
};
static struct wav_ wav;

static audio_pcm_dev_type wanted_spec;

//上锁, 避免多线程下冲突
#include "system_port.h"
static MTF_mutex *music_mutex = NULL;
static char music_lock(void)
{
    if (music_mutex == NULL)
    {
        music_mutex = MTF_CreateMutex();
    }
    return (char)MTF_LockMutex(music_mutex);
}

static char music_try_lock(void)
{
    return (char)MTF_TryLockMutex(music_mutex);
}

static char music_unlock(void)
{
	return (char)MTF_UnlockMutex(music_mutex);
}

//输入路径
int _WAV_Play(char *path)
{
#define ReadSize (512 * 40) /*一次读取数据大小*/
    int i;
    int a, b, u, Inx, r = 0;
    uint8_t buff[8192];
    int res = 0;
    mFILE *fp1 = NULL;

    MUSIC_DEBUG("WAV play\r\n");

    if (path == NULL)
    {
        MUSIC_DEBUG("no path input..\r\n");
        return -4;
    }
    fp1 = MTF_open(path, "rb");
    if (fp1 == NULL)
        res = 4;
    else
        res = 0;
    if (res == 5)
    {
        MUSIC_DEBUG("no path..\r\n");
        return -3;
    }
    if (res == 4)
    {
        MUSIC_DEBUG("WAV file not exist..\r\n");
        return -2;
    }
    if (res == 0)
    {
        MUSIC_DEBUG("WAV file open successful..\r\n");

        /*解WAV文件*/
        MTF_read(buff, 1, 58, fp1);
        //for(i=0;i<44;i++)MUSIC_DEBUG("%02x ",buff[i]);
        wav.audio_format = buff[21] << 8 | buff[20];
        wav.num_channels = buff[23] << 8 | buff[22];
        wav.bits_per_sample = buff[35] << 8 | buff[34];
        wav.byte_rate = buff[31] << 24 | buff[30] << 16 | buff[29] << 8 | buff[28];
        wav.sample_rate = buff[27] << 24 | buff[26] << 16 | buff[25] << 8 | buff[24];
        wav.format_blok_lenght = buff[19] << 24 | buff[18] << 16 | buff[17] << 8 | buff[16];
        i = 16 + wav.format_blok_lenght + 4 + 4;
        wav.data_size = buff[i + 3] << 24 | buff[i + 2] << 16 | buff[i + 1] << 8 | buff[i + 0];
        wav.play_time_ms = (int)((float)wav.data_size * (float)1000 / (float)wav.byte_rate);

        /*输出*/
        MUSIC_DEBUG("sample format:%d \r\n", wav.audio_format);
        MUSIC_DEBUG("sample channel:%d \r\n", wav.num_channels);
        MUSIC_DEBUG("sample bit:%d bit\r\n", wav.bits_per_sample);
        MUSIC_DEBUG("sample cycle:%d Hz\r\n", (int)wav.sample_rate);
        MUSIC_DEBUG("sample rate:%d Byte/S\r\n", (int)wav.byte_rate);
        MUSIC_DEBUG("format block lenght:%d Byte\r\n", (int)wav.format_blok_lenght);
        MUSIC_DEBUG("data size:%d Byte\r\n", (int)wav.data_size);
        // int wav_play_time = 0; //播放时长
        // wav_play_time = wav.play_time_ms / 1000;
        // MUSIC_DEBUG("play time:%02d:%02d:%02d \r\n",
        //        wav_play_time / 3600, wav_play_time % 3600 / 60, wav_play_time % 3600 % 60);

        /*无压缩格式*/
        if (wav.audio_format == 1)
        {
            a = wav.data_size / ReadSize;
            b = wav.data_size % ReadSize;
            /*读出数据区*/
            if (a == 1)
            {
                MTF_seek(fp1, i + 4, SEEK_SET);
                MTF_read(wav_buff[0], 1, ReadSize, fp1);
                if (b > 0)
                {
                    MTF_seek(fp1, i + 4 + ReadSize, SEEK_SET);
                    MTF_read(wav_buff[1], 1, b, fp1);
                }
                r = ReadSize;
            }
            else if (a >= 2)
            {
                MTF_seek(fp1, i + 4, SEEK_SET);
                MTF_read(wav_buff[0], 1, ReadSize, fp1);
                MTF_seek(fp1, i + 4 + ReadSize, SEEK_SET);
                MTF_read(wav_buff[1], 1, ReadSize, fp1);
                r = ReadSize;
            }
            else if ((a == 0) && (b > 0))
            {
                MTF_seek(fp1, i + 4, SEEK_SET);
                MTF_read(wav_buff[0], 1, b, fp1);
                r = b;
                b = 0;
            }

            wanted_spec.silence = 0;
            wanted_spec.samples = 1024;
            wanted_spec.freq = wav.sample_rate;
            wanted_spec.channels = wav.num_channels;
            if (wav.bits_per_sample == 16)
                wanted_spec.format = AUDIO_S16SYS;
            else if (wav.bits_per_sample == 24)
                wanted_spec.format = AUDIO_S24SYS;
            else if (wav.bits_per_sample == 32)
                wanted_spec.format = AUDIO_S32SYS;
            else
                return -6;

            if (MTF_audio_pcm_output_init(&wanted_spec, NULL) == 0) //音频硬件初始化
            {
                MTF_audio_pcm_output(&wanted_spec, wav_buff[0], r); //输出数据
                MUSIC_DEBUG("play...\r\n");
                Inx = 0;
                u = 0;
                while (1)
                {
                    if (MTF_audio_pcm_output_busy(&wanted_spec) == 0) //上一部分数据播放完
                    {
                        if (a)
                            a--;
                        u++;
                        Inx++;
                        if (a >= 1)
                        {
                            MTF_audio_pcm_output(&wanted_spec, wav_buff[Inx % 2], ReadSize);
                            MTF_seek(fp1, i + 4 + ((1 + u) * ReadSize), SEEK_SET); //读取WAV数据, 数据本身为PCM
                            if (a == 1)                                            //读剩余数据
                            {
                                if ((Inx % 2) == 0)
                                    MTF_read(wav_buff[1], 1, b, fp1);
                                if ((Inx % 2) == 1)
                                    MTF_read(wav_buff[0], 1, b, fp1);
                            }
                            else
                            {
                                if ((Inx % 2) == 0)
                                    MTF_read(wav_buff[1], 1, ReadSize, fp1);
                                if ((Inx % 2) == 1)
                                    MTF_read(wav_buff[0], 1, ReadSize, fp1);
                            }
                            //									MUSIC_DEBUG("用时= %d ms\r\n",Read_time_ms()-t);
                        }
                        else
                        {
                            if (b > 0)
                            {
                                MTF_audio_pcm_output(&wanted_spec, wav_buff[Inx % 2], b);
                                b = 0;
                            }
                            else
                            {
                                MUSIC_DEBUG("play end\r\n");
                                MTF_audio_pcm_output_exit(&wanted_spec);
                                break;
                            }
                        }

                        // int t = MTF_audio_output_time_get() / 100;
                        // int tj;
                        // if (t != tj)
                        // {
                        // 	tj = t;
                        // 	MUSIC_DEBUG("%02d:%02d:%02d----%02d:%02d:%02d \r\n",
                        // 		   wav_play_time / 3600, wav_play_time % 3600 / 60,
                        // 		   wav_play_time % 3600 % 60, t / 3600, t % 3600 / 60, t % 3600 % 60);
                        // }
                    }
                    /*播放退出*/
                    if (play_exit == 1)
                    {
                        play_exit = 0;
                        MUSIC_DEBUG("paly exit\r\n");
                        MTF_audio_pcm_output_exit(&wanted_spec);
                    }
                }
            }
            else
                MUSIC_DEBUG("parameter error\r\n");
        }
        else
            MUSIC_DEBUG("format error\r\n");

        MUSIC_DEBUG("file close\r\n");
        MTF_close(fp1);
    }
    return 0;
}

static uint32_t read_size = 512 * 40; /*一次读取数据大小*/
static int iblocklen;
static int _integer, _decimal, _unit, WavBuffInx;
static mFILE *fpWav = NULL;
static int wavTotalTime = 0; //文件播放时长
static int _timeTemp = 1;
static int audio_keep_time = 0;

int wav_init(char *path, int play_time)
{
    int res = 0;
    int r = 0;
    uint8_t buff[8192];
    
    music_lock();
    if (playMusicState) //强制停止
    {
        // while (MTF_audio_pcm_output_busy(&wanted_spec));
        MTF_audio_pcm_output_exit(&wanted_spec);
        MUSIC_DEBUG("paly exit\r\n");
        _timeTemp = 1;
        play_exit = 1;
        playMusicState = 0;
        MUSIC_DEBUG("file close\r\n");
        MTF_close(fpWav);
    }
    music_unlock();

    MUSIC_DEBUG("WAV play\r\n");

    if (path == NULL)
    {
        MUSIC_DEBUG("no path input..\r\n");
        return -4;
    }
    fpWav = MTF_open(path, "rb");
    if (fpWav == NULL)
        res = 4;
    else
        res = 0;
    if (res == 5)
    {
        MUSIC_DEBUG("no path..\r\n");
        return -3;
    }
    if (res == 4)
    {
        MUSIC_DEBUG("WAV file not exist..\r\n");
        return -2;
    }
    if (res == 0)
    {
        MUSIC_DEBUG("WAV file open successful..\r\n");

        /*解WAV文件*/
        MTF_read(buff, 1, 58, fpWav);
        //for(iblocklen=0;iblocklen<44;iblocklen++)MUSIC_DEBUG("%02x ",buff[iblocklen]);
        wav.audio_format = buff[21] << 8 | buff[20];
        wav.num_channels = buff[23] << 8 | buff[22];
        wav.bits_per_sample = buff[35] << 8 | buff[34];
        wav.byte_rate = buff[31] << 24 | buff[30] << 16 | buff[29] << 8 | buff[28];
        wav.sample_rate = buff[27] << 24 | buff[26] << 16 | buff[25] << 8 | buff[24];
        wav.format_blok_lenght = buff[19] << 24 | buff[18] << 16 | buff[17] << 8 | buff[16];
        iblocklen = 16 + wav.format_blok_lenght + 4 + 4;
        wav.data_size = buff[iblocklen + 3] << 24 | buff[iblocklen + 2] << 16 | buff[iblocklen + 1] << 8 | buff[iblocklen + 0];
        wav.play_time_ms = (int)((float)wav.data_size * (float)1000 / (float)wav.byte_rate);

        /*输出*/
        MUSIC_DEBUG("sample format:%d \r\n", wav.audio_format);
        MUSIC_DEBUG("sample channel:%d \r\n", wav.num_channels);
        MUSIC_DEBUG("sample bit:%d bit\r\n", wav.bits_per_sample);
        MUSIC_DEBUG("sample cycle:%d Hz\r\n", (int)wav.sample_rate);
        MUSIC_DEBUG("sample rate:%d Byte/S\r\n", (int)wav.byte_rate);
        MUSIC_DEBUG("format block lenght:%d Byte\r\n", (int)wav.format_blok_lenght);
        MUSIC_DEBUG("data size:%d Byte\r\n", (int)wav.data_size);
        wavTotalTime = wav.play_time_ms / 1000;
        MUSIC_DEBUG("play time:%02d:%02d:%02d \r\n",
               wavTotalTime / 3600, wavTotalTime % 3600 / 60, wavTotalTime % 3600 % 60);

        /*无压缩格式*/
        if (wav.audio_format == 1)
        {
            _integer = wav.data_size / read_size;
            _decimal = wav.data_size % read_size;
            /*读出数据区*/
            if (_integer == 1)
            {
                MTF_seek(fpWav, iblocklen + 4, SEEK_SET);
                MTF_read(wav_buff[0], 1, read_size, fpWav);
                if (_decimal > 0)
                {
                    MTF_seek(fpWav, iblocklen + 4 + read_size, SEEK_SET);
                    MTF_read(wav_buff[1], 1, _decimal, fpWav);
                }
                r = read_size;
            }
            else if (_integer >= 2)
            {
                MTF_seek(fpWav, iblocklen + 4, SEEK_SET);
                MTF_read(wav_buff[0], 1, read_size, fpWav);
                MTF_seek(fpWav, iblocklen + 4 + read_size, SEEK_SET);
                MTF_read(wav_buff[1], 1, read_size, fpWav);
                r = read_size;
            }
            else if ((_integer == 0) && (_decimal > 0))
            {
                MTF_seek(fpWav, iblocklen + 4, SEEK_SET);
                MTF_read(wav_buff[0], 1, _decimal, fpWav);
                r = _decimal;
                _decimal = 0;
            }

            wanted_spec.silence = 0;
            wanted_spec.samples = 1024;
            wanted_spec.freq = wav.sample_rate;
            wanted_spec.channels = wav.num_channels;
            if (wav.bits_per_sample == 16)
                wanted_spec.format = AUDIO_S16SYS;
            else if (wav.bits_per_sample == 24)
                wanted_spec.format = AUDIO_S24SYS;
            else if (wav.bits_per_sample == 32)
                wanted_spec.format = AUDIO_S32SYS;
            else
                return -6;

            if (MTF_audio_pcm_output_init(&wanted_spec, NULL) == 0) //音频硬件初始化
            {
                MTF_audio_pcm_output(&wanted_spec, wav_buff[0], r); //输出数据
                MUSIC_DEBUG("play...\r\n");
                WavBuffInx = 0;
                _unit = 0;
                audio_keep_time = play_time;
                play_exit = 0; //开始播放
            }
            else
                MUSIC_DEBUG("parameter error\r\n");
        }
        else
            MUSIC_DEBUG("format error\r\n");
    }
    return 0;
}

void _WAV_Play2(void)
{
    // if (play_exit && playMusicState) //强制停止
    // {
    //     // while (MTF_audio_pcm_output_busy(&wanted_spec));
    //     MTF_audio_pcm_output_exit(&wanted_spec);
    //     MUSIC_DEBUG("paly exit\r\n");
    //     _timeTemp = 1;
    //     play_exit = 1;
    //     playMusicState = 0;
    //     MUSIC_DEBUG("file close\r\n");
    //     MTF_close(fpWav);
    // }

    if (play_exit == 0)
    {
        if (music_try_lock() != 0)
            return;
        playMusicState = 1;                               //播放处理中
        if (MTF_audio_pcm_output_busy(&wanted_spec) == 0) //上一部分数据播放完
        {
            _integer--;
            _unit++;
            WavBuffInx++;
            if (_integer >= 1)
            {
                MTF_audio_pcm_output(&wanted_spec, wav_buff[WavBuffInx % 2], read_size);
                MTF_seek(fpWav, iblocklen + 4 + ((1 + _unit) * read_size), SEEK_SET);
                if (_integer == 1) //读剩余数据
                {
                    if ((WavBuffInx % 2) == 0)
                        MTF_read(wav_buff[1], 1, _decimal, fpWav);
                    if ((WavBuffInx % 2) == 1)
                        MTF_read(wav_buff[0], 1, _decimal, fpWav);
                }
                else
                {
                    if ((WavBuffInx % 2) == 0)
                        MTF_read(wav_buff[1], 1, read_size, fpWav);
                    if ((WavBuffInx % 2) == 1)
                        MTF_read(wav_buff[0], 1, read_size, fpWav);
                }
                //MUSIC_DEBUG("用时= %d ms\r\n",Read_time_ms()-t);
            }
            else
            {
                if (_decimal > 0)
                {
                    MTF_audio_pcm_output(&wanted_spec, wav_buff[WavBuffInx % 2], _decimal);
                    _decimal = 0;
                }
                else
                {
                    MTF_audio_pcm_output_exit(&wanted_spec);
                    MUSIC_DEBUG("play end\r\n");
                    _timeTemp = 1;
                    play_exit = 1;      //停止播放
                    playMusicState = 0; //已停止播放处理
                    MUSIC_DEBUG("file close\r\n");
                    MTF_close(fpWav);
                    music_unlock();
                    return;
                }
            }
            int t = MTF_audio_output_time_get() / 100;
            if (t != _timeTemp) //播放持续时间, 单位1s
            {
                _timeTemp = t;
                MUSIC_DEBUG("%02d:%02d:%02d----%02d:%02d:%02d \r\n",
                       wavTotalTime / 3600, wavTotalTime % 3600 / 60,
                       wavTotalTime % 3600 % 60, t / 3600, t % 3600 / 60, t % 3600 % 60);
                if (audio_keep_time != 0XFF) //0XFF为播放整个文件
                {
                    if (t >= audio_keep_time) //停止播放
                    {
                        MTF_audio_pcm_output_exit(&wanted_spec);
                        MUSIC_DEBUG("paly exit\r\n");
                        _timeTemp = 1;
                        play_exit = 1;
                        playMusicState = 0;
                        MUSIC_DEBUG("file close\r\n");
                        MTF_close(fpWav);
                    }
                }
            }
        }
        music_unlock();
    }
}

/*****mp3解码******/
//mp3
#if defined(__CC_ARM)
#define _MP3_LIB_H
#endif

#ifdef _MP3_LIB_H

#include "mp3dec.h" //汇编文件没改为GCC版本,编不了
#include "mp3common.h"

int Get_ID3V2(mFILE *f, char *TIT2, char *TPE1, char *TALB);
int Get_ID3V1(mFILE *f, char *TIT2, char *TPE1, char *TALB);
int Get_mp3_play_time_ms(int ID3size, int ID3V1_size, mFILE *f, MP3FrameInfo _mp3FrameInfo);
int MP3_to_PCM_DATA(HMP3Decoder Mp3Decoder, MP3FrameInfo *mp3FrameInfo, mFILE *f, unsigned char *outbuff, int data_init, int read_addr);
// //网页资料链接
// //https://blog.csdn.net/wlsfling/article/details/5875959
// //https://blog.csdn.net/zftzftzft/article/details/79550396
// //https://blog.csdn.net/zftzftzft/article/details/79550396
// //输入路径
int _MP3_Play(char *path)
{
    int tj = 1;
    int Frame_size;
    int mp3_play_time = 0; //播放时长
    int offset = 0;
    unsigned char *readPtr = 0; //输入数据缓冲
    int Padding = 0;            //Padding位
    int bytesLeft = 0;
    MP3FrameInfo mp3FrameInfo;
    HMP3Decoder Mp3Decoder;
    //TIT2：歌曲标题名字
    //TPE1：作者名字
    //TALB：作品专辑
    char TIT2[100];
    char TPE1[100];
    char TALB[100];
    int ID3v1 = 0, ID3v2 = 0; //ID3大小
    //
    int res;
    mFILE *f = NULL;
    int pcm_data_size;

    MUSIC_DEBUG("MP3播放\r\n");

    if (path == NULL)
    {
        MUSIC_DEBUG("没有路径输入..\r\n");
        return -4;
    }
    f = MTF_open(path, "rb");
    if (f == NULL)
        res = 4;
    else
        res = 0;
    if (res == 5)
    {
        MUSIC_DEBUG("文件夹路径不存在..\r\n");
        return -3;
    }
    if (res == 4)
    {
        MUSIC_DEBUG("MP3文件不在..\r\n");
        return -3;
    }
    if (res == 0)
        MUSIC_DEBUG("MP3文件打开成功..\r\n");
    //
    readPtr = wav_buff[0];
    /*读入一段mp3文件*/
    MTF_read(readPtr, 1, 10000, f);
    bytesLeft = 10000;
    //读ID3
    ID3v1 = Get_ID3V1(f, TIT2, TPE1, TALB);
    ID3v2 = Get_ID3V2(f, TIT2, TPE1, TALB);
    //超出，从新读入数据
    if (ID3v2 > 10000)
    {
        MUSIC_DEBUG("ID3v2超长\r\n");
        readPtr = wav_buff[0];
        MTF_seek(f, ID3v2, SEEK_SET);
        MTF_read(readPtr, 1, 10000, f);
        bytesLeft = 10000;
    }
    else
    {
        if (ID3v2 > 0)
            offset += ID3v2; //如果有ID3V2偏移到下一个帧
        readPtr += offset;
        bytesLeft -= offset;
    }
    /*【参数1：输入数据的缓存指针】【 参数2: 输入数据帧的剩余大小】*/
    offset = MP3FindSyncWord(readPtr, bytesLeft); //找帧头
    readPtr += offset;
    bytesLeft -= offset;

    if (offset < 0)
    {
        MUSIC_DEBUG("没有帧头\r\n");
        return -1;
    }
    else
    {
        Mp3Decoder = MP3InitDecoder();
        if (Mp3Decoder == 0)
        {
            MUSIC_DEBUG("Init Mp3Decoder failed!\r\n");
            return -1;
        }
        /*返回帧头信息*/
        if (ERR_MP3_NONE == MP3GetNextFrameInfo(Mp3Decoder, &mp3FrameInfo, readPtr))
        {
            /*打印帧头信息*/
            MUSIC_DEBUG("FrameInfo bitrate=%d \r\n", mp3FrameInfo.bitrate);
            MUSIC_DEBUG("FrameInfo nChans=%d \r\n", mp3FrameInfo.nChans);
            MUSIC_DEBUG("FrameInfo samprate=%d \r\n", mp3FrameInfo.samprate);
            MUSIC_DEBUG("FrameInfo bitsPerSample=%d \r\n", mp3FrameInfo.bitsPerSample);
            MUSIC_DEBUG("FrameInfo outputSamps=%d \r\n", mp3FrameInfo.outputSamps);
            MUSIC_DEBUG("FrameInfo layer=%d \r\n", mp3FrameInfo.layer);
            MUSIC_DEBUG("FrameInfo version=%d \r\n", mp3FrameInfo.version);
            /*计算数据帧大小 帧大小 = ( 每帧采样次数 × 比特率(bit/s) ÷ 8 ÷采样率) + Padding*/
            Frame_size = (int)samplesPerFrameTab[mp3FrameInfo.version][mp3FrameInfo.layer - 1] * mp3FrameInfo.bitrate / 8 / mp3FrameInfo.samprate + Padding;
            MUSIC_DEBUG("FrameInfo Frame_size=%d \r\n", Frame_size);
            //返回播放时长信息
            int t = Get_mp3_play_time_ms(ID3v2, ID3v1, f, mp3FrameInfo);
            mp3_play_time = t / 1000;
            MUSIC_DEBUG("播放时长=%02d:%02d:%02d \r\n", mp3_play_time / 3600, mp3_play_time % 3600 / 60, mp3_play_time % 3600 % 60);
        }
    }
    /*只能播放MP3文件*/
    if ((mp3FrameInfo.layer < 3) || (mp3FrameInfo.bitrate <= 0) || (mp3FrameInfo.samprate <= 0))
    {
        MUSIC_DEBUG("格式不是MP3... \r\n");
        MP3FreeDecoder(Mp3Decoder);
        MTF_close(f);
        return -1;
    }

    /*初始化参数*/
    wav.audio_format = 1;
    wav.num_channels = mp3FrameInfo.nChans;
    wav.bits_per_sample = mp3FrameInfo.bitsPerSample;
    wav.sample_rate = mp3FrameInfo.samprate;

    /*写入参数*/
    if (wav.audio_format == 1) //PCM格式
    {
        wanted_spec.silence = 0;
        wanted_spec.samples = 1024;
        wanted_spec.freq = wav.sample_rate;
        wanted_spec.channels = wav.num_channels;
        if (wav.bits_per_sample == 16)
            wanted_spec.format = AUDIO_S16SYS;
        else if (wav.bits_per_sample == 24)
            wanted_spec.format = AUDIO_S24SYS;
        else if (wav.bits_per_sample == 32)
            wanted_spec.format = AUDIO_S32SYS;
        else
            return -6;

        if (MTF_audio_pcm_output_init(&wanted_spec, NULL) == 0) //音频硬件初始化
        {
            //dma初始化-需要先解码两个缓存-播放一个，一个备用
            pcm_data_size = MP3_to_PCM_DATA(Mp3Decoder, &mp3FrameInfo, f, wav_buff[0], 1, ID3v2); //读取一部分MP3数据并转为PCM格式, 与wav不同的地方
            if (pcm_data_size > 0)
            {
                MTF_audio_pcm_output(&wanted_spec, wav_buff[0], pcm_data_size); //输出数据
                MUSIC_DEBUG("play...\r\n");
            }
            else
            {
                MUSIC_DEBUG("decode error D1...\r\n");
                MTF_audio_pcm_output_exit(&wanted_spec);
                MP3FreeDecoder(Mp3Decoder);
                MTF_close(f);
                return -2;
            }

            pcm_data_size = MP3_to_PCM_DATA(Mp3Decoder, &mp3FrameInfo, f, wav_buff[1], 0, 0);

            //检测发送完成中断-中断DMA指向下一缓存，读SD后解码一个缓存
            int Inx = 0;
            while (1)
            {
                if (MTF_audio_pcm_output_busy(&wanted_spec) == 0) //上一部分数据播放完
                {
                    /*设置DMA一下缓存*/
                    Inx++;
                    if (pcm_data_size > 0)
                    {
                        int x = Inx % 2;
                        MTF_audio_pcm_output(&wanted_spec, wav_buff[x], pcm_data_size);
                        /*解一下缓存*/
                        //int times=Read_time_ms();
                        if (x == 0)
                            pcm_data_size = MP3_to_PCM_DATA(Mp3Decoder, &mp3FrameInfo, f, wav_buff[1], 0, 0);
                        if (x == 1)
                            pcm_data_size = MP3_to_PCM_DATA(Mp3Decoder, &mp3FrameInfo, f, wav_buff[0], 0, 0);
                        //MUSIC_DEBUG("解码用时= %d ms\r\n",Read_time_ms()-times);
                        int t = MTF_audio_output_time_get() / 100;
                        if (t != tj)
                        {
                            tj = t;
                            MUSIC_DEBUG("%02d:%02d:%02d----%02d:%02d:%02d \r\n", mp3_play_time / 3600, mp3_play_time % 3600 / 60,
                                   mp3_play_time % 3600 % 60, t / 3600, t % 3600 / 60, t % 3600 % 60);
                        }
                    }
                    else
                    {
                        MTF_audio_pcm_output_exit(&wanted_spec);
                        MUSIC_DEBUG("play end\r\n");
                        break; //播放放成退出
                    }
                }

                /*播放退出*/
                if (play_exit == 1)
                {
                    play_exit = 0;
                    MUSIC_DEBUG("paly exit\r\n");
                    MTF_audio_pcm_output_exit(&wanted_spec);
                }
            }
        }
        else
            MUSIC_DEBUG("参数错误\r\n");
    }
    else
        MUSIC_DEBUG("格式错误\r\n");
    MP3FreeDecoder(Mp3Decoder);
    MTF_close(f);
    return 0;
}

// /*
// 解码1帧
// 返回数据大小
// Mp3Decoder 初始化指针
// mp3FrameInfo 解码信息指针
// f FATFS文件指针
// outbuff 输出数据指针
// data_init 初始化参数
// read_addr 第一帧地址 配合data_init用
// 返回-3 找帧头错误
// 返回-2 解码错误过多
// 返回-1 播放完成
// */
#define DATSIZE (512 * 30)
#define FM 5 //连续解码FM帧
unsigned char rbuff[DATSIZE] __attribute__((aligned(4)));
unsigned char rbuff2[DATSIZE] __attribute__((aligned(4)));
int MP3_to_PCM_DATA(HMP3Decoder Mp3Decoder, MP3FrameInfo *mp3FrameInfo, mFILE *f, unsigned char *outbuff, int data_init, int read_addr)
{
    int offset;
    static int res = 0;
    static int ssddtt = 0;
    static int _bytesLeft = 0;     //有效长度
    static unsigned char *readPtr; //读出指针
    unsigned int br;
    int outsize;
    /*初始化参数*/
    if (data_init == 1)
    {
        res = 0;
        _bytesLeft = 0;
        ssddtt = read_addr;
        readPtr = NULL;
    }
    /*连续解码FM帧*/
    for (int s = 0; s < FM; s++)
    {
        /*判断是否需要读入数据*/
        if (_bytesLeft < (MAINBUF_SIZE * 2))
        {
            memcpy(&rbuff[0], &rbuff[DATSIZE - _bytesLeft], _bytesLeft);
            /*读入数据*/
            MTF_seek(f, ssddtt, SEEK_SET);
            br = MTF_read(rbuff + _bytesLeft, 1, DATSIZE - _bytesLeft, f);
            /*数据读完-播放完成*/
            if (br == 0)
            {
                if (s > 0) //有解码数据-返回大小
                {
                    outsize = (mp3FrameInfo->outputSamps * 2 * s);
                    return outsize;
                }
                return -1; //没有解码返回
            }
            ssddtt += (DATSIZE - _bytesLeft); //偏移到下一下地址
            _bytesLeft = DATSIZE;             //有效字节数
            readPtr = rbuff;                  //数据指针偏移到0
        }
        /*找帧头*/
        offset = MP3FindSyncWord(readPtr, _bytesLeft);
        if (offset >= 0)
        {
            readPtr += offset;
            _bytesLeft -= offset;
            /*解码帧-【数据指针+帧长度】与【有效长度-帧长度】*/
            res = MP3Decode(Mp3Decoder, &readPtr, &_bytesLeft, (short *)outbuff, 0);
            outbuff += (mp3FrameInfo->outputSamps * 2);
            if (res < 0)
                return -2;
        }
        else
            return -3; //没有帧头
    }
    /*返回解码大小*/
    outsize = (mp3FrameInfo->outputSamps * 2 * FM);
    return outsize;
}

// /*识别ID3V2+
// f 文件句柄
// //TIT2：歌曲标题名字
// //TPE1：作者名字
// //TALB：作品专辑
// //编码UNICODE
// 返回数据大小
// //https://blog.csdn.net/qq_18661257/article/details/54908775
// */
int Get_ID3V2(mFILE *f, char *TIT2, char *TPE1, char *TALB)
{
    int s = 0, i, l;
    int ID3_VER;
    int ID3_SIZE;
    char readbuff[5000];
    int TIT2_size = 0;
    int TPE1_size = 0;
    int TALB_size = 0;
    int TIT2_F = 0;
    int TPE1_F = 0;
    int TALB_F = 0;
    /*读入一段mp3文件*/
    MTF_seek(f, 0, SEEK_SET);
    MTF_read(readbuff, 1, 5000, f);
    /*ID3_V2识别*/
    for (i = 0; i < 50; i++)
    {
        if ((readbuff[i + 0] == 'I') && (readbuff[i + 1] == 'D') && (readbuff[i + 2] == '3'))
        {
            ID3_VER = readbuff[i + 3] * 10 + readbuff[i + 4];
            ID3_SIZE = ((readbuff[i + 9] & 0x7f) | ((readbuff[i + 8] & 0x7f) << 7) | ((readbuff[i + 7] & 0x7f) << 14) | ((readbuff[i + 6] & 0x7f) << 21)) + 10;
            MUSIC_DEBUG("ID3V%d.%d \r\n", ID3_VER / 10, ID3_VER % 10);
            MUSIC_DEBUG("标签大小=%d\r\n", ID3_SIZE);
            s = ID3_SIZE;
            if (ID3_SIZE > 5000)
                ID3_SIZE = 5000; //超长ID3 可能有图片类
            for (; i < ID3_SIZE; i++)
            {
                if (TIT2_F == 0)
                    if ((readbuff[i + 0] == 'T') && (readbuff[i + 1] == 'I') && (readbuff[i + 2] == 'T') && (readbuff[i + 3] == '2')) //TIT2
                    {
                        MUSIC_DEBUG("TIT2=%d \r\n", i);
                        TIT2_size = readbuff[i + 4 + 0] << 24 | readbuff[i + 4 + 1] << 16 | readbuff[i + 4 + 2] << 8 | readbuff[i + 4 + 3] << 0;
                        //					MUSIC_DEBUG("TIT2_size=%d \r\n",TIT2_size);
                        for (l = 0; l < (TIT2_size - 3); l++)
                            TIT2[l] = readbuff[i + 10 + 3 + l];
                        //					for(l=0;l<(TIT2_size-3);l++)MUSIC_DEBUG("0x%02x,",TIT2[l]);
                        //					MUSIC_DEBUG("\r\n");
                        TIT2_F = 1;
                    }
                if (TPE1_F == 0)
                    if ((readbuff[i + 0] == 'T') && (readbuff[i + 1] == 'P') && (readbuff[i + 2] == 'E') && (readbuff[i + 3] == '1')) //TPE1
                    {
                        MUSIC_DEBUG("TPE1=%d \r\n", i);
                        TPE1_size = readbuff[i + 4 + 0] << 24 | readbuff[i + 4 + 1] << 16 | readbuff[i + 4 + 2] << 8 | readbuff[i + 4 + 3] << 0;
                        //					MUSIC_DEBUG("TPE1_size=%d \r\n",TPE1_size);
                        for (l = 0; l < (TPE1_size - 3); l++)
                            TPE1[l] = readbuff[i + 10 + 3 + l];
                        //					for(l=0;l<(TPE1_size-3);l++)MUSIC_DEBUG("0x%02x,",TPE1[l]);
                        //					MUSIC_DEBUG("\r\n");
                        TPE1_F = 1;
                    }
                if (TALB_F == 0)
                    if ((readbuff[i + 0] == 'T') && (readbuff[i + 1] == 'A') && (readbuff[i + 2] == 'L') && (readbuff[i + 3] == 'B')) //TALB
                    {
                        MUSIC_DEBUG("TALB=%d \r\n", i);
                        TALB_size = readbuff[i + 4 + 0] << 24 | readbuff[i + 4 + 1] << 16 | readbuff[i + 4 + 2] << 8 | readbuff[i + 4 + 3] << 0;
                        //					MUSIC_DEBUG("TALB_size=%d \r\n",TALB_size);
                        for (l = 0; l < (TALB_size - 3); l++)
                            TALB[l] = readbuff[i + 10 + 3 + l];
                        //					for(l=0;l<(TALB_size-3);l++)MUSIC_DEBUG("0x%02x,",TALB[l]);
                        //					MUSIC_DEBUG("\r\n");
                        TALB_F = 1;
                    }
            }
        }
    }
    return s;
}

// /*识别ID3V1
// f 文件句柄
// 返回数据大小
// //编码GBK2312
// */
int Get_ID3V1(mFILE *f, char *TIT2, char *TPE1, char *TALB)
{
    int l;
    /*读最后128字节*/
    char buffl[128];
    int Fsize = MTF_size(f);
    MTF_seek(f, Fsize - 128, SEEK_SET);
    MTF_read(buffl, 1, 128, f);
    if ((buffl[0] == 'T') && (buffl[1] == 'A') && (buffl[2] == 'G'))
    {
        MUSIC_DEBUG("ID3V1\r\n");
        for (l = 0; l < 30; l++)
            TIT2[l] = buffl[3 + l];
        for (l = 0; l < 30; l++)
            TPE1[l] = buffl[33 + l];
        for (l = 0; l < 30; l++)
            TALB[l] = buffl[63 + l];
        //编码GBK2312
        //		for(l=0;l<10;l++)MUSIC_DEBUG("0x%02x,",TIT2[l]);
        //		MUSIC_DEBUG("\r\n");
        return 128;
    }
    return 0;
}

// /*返回播放时长-ms
// ID3size ID3大小
// ID3V1_size ID3V1 大小
// f 文件句柄
// _mp3FrameInfo mp3信息指针
// */
int Get_mp3_play_time_ms(int ID3size, int ID3V1_size, mFILE *f, MP3FrameInfo _mp3FrameInfo)
{
    int time, i;
    int VBR_Frame_num; //总帧数
    int Fsize;         //文件大小
    char readbuff[5000];

    /*读入一段mp3文件*/
    MTF_seek(f, ID3size, SEEK_SET);
    MTF_read(readbuff, 1, 5000, f);
    //VBR 可变码率
    for (i = 0; i < 60; i++)
    {
        if (((readbuff[i + 0] == 'X') && (readbuff[i + 1] == 'i') && (readbuff[i + 2] == 'n') && (readbuff[i + 3] == 'g')) ||
            ((readbuff[i + 0] == 'I') && (readbuff[i + 1] == 'n') && (readbuff[i + 2] == 'f') && (readbuff[i + 3] == 'o')))
        {
            MUSIC_DEBUG("Xing=%d \r\n", i);
            /*读总帧数*/
            VBR_Frame_num = readbuff[i + 8 + 0] << 24 | readbuff[i + 8 + 1] << 16 | readbuff[i + 8 + 2] << 8 | readbuff[i + 8 + 3] << 0;
            /*计算播放时长*/
            time = (float)VBR_Frame_num * ((float)1000000 / (float)_mp3FrameInfo.samprate * (float)samplesPerFrameTab[_mp3FrameInfo.version][_mp3FrameInfo.layer - 1]) / (float)1000;
            return time;
        }
        if ((readbuff[i + 0] == 'V') && (readbuff[i + 1] == 'B') && (readbuff[i + 2] == 'R') && (readbuff[i + 3] == 'I'))
        {
            MUSIC_DEBUG("VBRI=%d \r\n", i);
            /*读总帧数*/
            VBR_Frame_num = readbuff[i + 14 + 0] << 24 | readbuff[i + 14 + 1] << 16 | readbuff[i + 14 + 2] << 8 | readbuff[i + 14 + 3] << 0;
            /*计算播放时长*/
            time = (float)VBR_Frame_num * ((float)1000000 / (float)_mp3FrameInfo.samprate * (float)samplesPerFrameTab[_mp3FrameInfo.version][_mp3FrameInfo.layer - 1]) / (float)1000;
            return time;
        }
    }
    //CBR 固定码率
    Fsize = MTF_size(f);
    //播放时长 = ( 文件大小 – ID3大小 ) × 8 ÷ 比特率(kbit/s)
    time = (Fsize - ID3size - ID3V1_size) * 8 / (_mp3FrameInfo.bitrate / 1000);
    return time;
}

#endif

/*播放WAV MP3文件*/
void MP3WAVplay(char *path)
{
    char *Fn = path + (strlen(path) - 4);

    MUSIC_DEBUG("path %s \r\n", path);
    if (strcmp(Fn, ".mp3") == 0)
    {
#ifdef _MP3_LIB_H
        _MP3_Play(path);
#else
        MUSIC_DEBUG("no support mp3\r\n");
#endif
    }
    else if (strcmp(Fn, ".wav") == 0)
    {
        _WAV_Play(path);
    }
    else
    {
        MUSIC_DEBUG("it isn't mp3/wav\r\n");
    }
}

/*退出播放*/
void MP3WAVplay_exit(void)
{
    play_exit = 1;
}

/*音频测试*/
void AUDIO_Demo(void)
{
    char *path[13] =
        {
            "./so_shi_te_ni_ma.mp3",
            "./kiss_me_goodbye.wav",
            "./a1.wav",
            "./a2.mp3",
            "./m1.wav",
            "./m2.mp3",
            "./m3.mp3",
            "./m4.mp3",
            NULL};

    int s = 0;

    while (1)
    {
        /*播放*/
        MP3WAVplay(path[s]);
        /*下一曲*/
        s++;
        if (s >= 6)
            s = 0;
    }
}
