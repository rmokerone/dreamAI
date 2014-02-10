// ttsdemo_with_speechmark.cpp : Defines the entry point for the console application.
//
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "../../include/qtts.h"

#define TRUE 1
#define FALSE 0

typedef int SR_DWORD;
typedef short int SR_WORD ;

//音频头部格式
struct wave_pcm_hdr
{
	char            riff[4];                        // = "RIFF"
	SR_DWORD        size_8;                         // = FileSize - 8
	char            wave[4];                        // = "WAVE"
	char            fmt[4];                         // = "fmt "
	SR_DWORD        dwFmtSize;                      // = 下一个结构体的大小 : 16

	SR_WORD         format_tag;              // = PCM : 1
	SR_WORD         channels;                       // = 通道数 : 1
	SR_DWORD        samples_per_sec;        // = 采样率 : 8000 | 6000 | 11025 | 16000
	SR_DWORD        avg_bytes_per_sec;      // = 每秒字节数 : dwSamplesPerSec * wBitsPerSample / 8
	SR_WORD         block_align;            // = 每采样点字节数 : wBitsPerSample / 8
	SR_WORD         bits_per_sample;         // = 量化比特数: 8 | 16

	char            data[4];                        // = "data";
	SR_DWORD        data_size;                // = 纯数据长度 : FileSize - 44 
} ;

//默认音频头部数据
struct wave_pcm_hdr default_pcmwavhdr = 
{
	{ 'R', 'I', 'F', 'F' },
	0,
	{'W', 'A', 'V', 'E'},
	{'f', 'm', 't', ' '},
	16,
	1,
	1,
	16000,
	32000,
	2,
	16,
	{'d', 'a', 't', 'a'},
	0  
};

int text_to_speech(const char* src_text ,const char* des_path ,const char* params)
{
	struct wave_pcm_hdr pcmwavhdr = default_pcmwavhdr;
	const char* sess_id = NULL;
	int ret = 0;
	unsigned int text_len = 0;
	char* audio_data;
	unsigned int audio_len = 0;
	int synth_status = MSP_TTS_FLAG_STILL_HAVE_DATA;
	FILE* fp = NULL;

	int pos = 0;			//用于标记上一次已经合成到的位置
	int loop_count = 0;		//用于标记，取了几次结果
	int upload_flow = 0,download_flow = 0;//上传流量和下载流量
	char param_value[32] = "";//参数值的字符串形式
	unsigned int value_len = 32;	//字符串长度或buffer长度

	printf("begin to synth...\n");
	if (NULL == src_text || NULL == des_path)
	{
		printf("params is null!\n");
		return -1;
	}
	text_len = (unsigned int)strlen(src_text);
	fp = fopen(des_path,"wb");
	if (NULL == fp)
	{
		printf("open file %s error\n",des_path);
		return -1;
	}
	sess_id = QTTSSessionBegin(params, &ret);
	if ( ret != MSP_SUCCESS )
	{
		printf("QTTSSessionBegin: qtts begin session failed Error code %d.\n",ret);
		return ret;
	}

	ret = QTTSTextPut(sess_id, src_text, text_len, NULL );
	if ( ret != MSP_SUCCESS )
	{
		printf("QTTSTextPut: qtts put text failed Error code %d.\n",ret);
		QTTSSessionEnd(sess_id, "TextPutError");
		return ret;
	}

	fwrite(&pcmwavhdr, 1, sizeof(pcmwavhdr), fp);

	while ( true )
	{
		audio_data = (char*)QTTSAudioGet( sess_id ,&audio_len , &synth_status , &ret );
		//获取拼音标注
		const char* audio_info = "";
		if ( 0 < audio_len)
		{
			audio_info = QTTSAudioInfo(sess_id);
		}
		printf("[%d]:get result[err==%d / status==%d / audio_len==%d]:%s\n", (loop_count), ret, synth_status,audio_len,audio_info);
		loop_count++;										//取结果次数自增
		
		if ( ret != MSP_SUCCESS )
		{
			printf("QTTSAudioGet: qtts get audio failed Error code %d.\n",ret);
			break;
		}
		fwrite(audio_data, 1, audio_len, fp);
		pcmwavhdr.data_size += audio_len;//修正pcm数据的大小
		if ( MSP_TTS_FLAG_DATA_END == synth_status )
		{
			printf("QTTSAudioGet: get end of data.\n");
			break;
		}
	}
	//修正pcm文件头数据的大小
	pcmwavhdr.size_8 += pcmwavhdr.data_size + 36;

	//将修正过的数据写回文件头部
	fseek(fp, 4, 0);
	fwrite(&pcmwavhdr.size_8,sizeof(pcmwavhdr.size_8), 1, fp);
	fseek(fp, 40, 0);
	fwrite(&pcmwavhdr.data_size,sizeof(pcmwavhdr.data_size), 1, fp);

	fclose(fp);

	ret = QTTSSessionEnd(sess_id, "Normal");
	if ( ret != MSP_SUCCESS )
	{
		printf("QTTSSessionEnd: qtts end failed Error code %d.\n",ret);
	}
	return ret;
}

int main(int argc, char* argv[])
{
	///APPID请勿随意改动
	const char* m_configs = "appid=52f44528";
	const char* text1  = "讯飞语音，沟通无限。";
	const char*  filename1 = "text_to_speech_test_1.wav";
	const char* param1 = "ssm=1,auf=audio/L16;rate=16000,ent=AisoundPro,vcn=jiajia,smk=2";//带参数smk=3，获取详细的语音标注，json格式，请自行解析
	const char* text2  = "讯飞语音，沟通无限。";
	const char*  filename2 = "text_to_speech_test_2.wav";
	const char* param2 = "ssm=1,auf=audio/L16;rate=16000,ent=AisoundPro,vcn=jiajia,smk=1";//不带smk参数，获取到的是普通的位置信息
	int ret = 0;
	char key = 0;

	//引擎初始化
	ret = QTTSInit( m_configs);
	if ( ret != MSP_SUCCESS )
	{
		printf("QTTSInit: failed, Error code %d.\n", ret);
		key = getchar();
		return ret;
	}
	//合成文本
	ret = text_to_speech(text1,filename1,param1);
	if ( ret != MSP_SUCCESS )
	{
		printf("text_to_speech: failed , Error code %d.\n",ret);
	}
	ret = text_to_speech(text2,filename2,param2);
	if ( ret != MSP_SUCCESS )
	{
		printf("text_to_speech: failed , Error code %d.\n",ret);
	}
	//引擎关闭
	ret = QTTSFini();
	if ( ret != MSP_SUCCESS )
	{
		printf("QTTSFini: failed , Error code %d.\n",ret);
	}
	else
	{
		printf("Complete!\nPress any key to exit.\n");
	}
	key = getchar();
	return 0;
}


