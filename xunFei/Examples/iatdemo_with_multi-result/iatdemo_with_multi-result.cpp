// iatdemo_with_multi-result.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "../../include/qisr.h"

#define TRUE 1
#define FALSE 0

const int BUFFER_NUM = 1024 * 4;

void run_iat(const char* src_wav_filename , const char* des_text_filename , const char* param)
{
	bool error = false;
	int ret = MSP_SUCCESS;
	int i = 0;
	FILE* fp = NULL;
	FILE* fout = NULL;
	char buff[BUFFER_NUM];
	unsigned int len;
	int status = MSP_AUDIO_SAMPLE_CONTINUE, ep_status = -1, rec_status = -1, rslt_status = -1;

	///第二个参数为传递的参数，使用会话模式，使用speex编解码，使用16k16bit的音频数据
	///第三个参数为返回码
	const char* sess_id = QISRSessionBegin(NULL, param, &ret);
	if ( MSP_SUCCESS != ret )
	{
		printf("QISRSessionBegin err %d\n", ret);	
		error = true;
	}

	///模拟录音，输入音频
	if (error == false)
	{
		fp = fopen( src_wav_filename , "rb");
		if ( NULL == fp )
		{
			printf("failed to open file,please check the file.\n");
			error = true;
		}
	}

	///结果输出到文件
	if (error == false)
	{
		fout = fopen( des_text_filename , "ab");
		if( NULL == fout )
		{
			printf("failed to open file,please check the file.\n");
			error = true;
		}
	}
	if (error == false)
	{
		printf("writing audio...\n");

		char param_value[32] = "";//参数值的字符串形式
		unsigned int value_len = 32;	//字符串长度或buffer长度
		int volume = 0;//音量数值

		while ( !feof(fp) )
		{
			len = (unsigned int)fread(buff, 1, BUFFER_NUM, fp);
			printf(".");
			feof(fp) ? status = MSP_AUDIO_SAMPLE_LAST : status = MSP_AUDIO_SAMPLE_CONTINUE;//最后一块音频要使用last
			///开始向服务器发送音频数据
			ret = QISRAudioWrite(sess_id, buff, len, status, &ep_status, &rec_status);
			if ( ret != MSP_SUCCESS )
			{
				printf("\nQISRAudioWrite err %d\n", ret);
				error = true;
				break;
			}
			/*********获取当前发送音频的音量信息**********/
			value_len = 32;//value_len既是传入参数，又是传出参数，每次调用QTTSGetParam时要调整为buffer长度
			ret = QISRGetParam(sess_id,"volume",param_value,&value_len);//获取音量信息，获取上行流量和下行流量的例子见ttsdemo
			if ( ret != MSP_SUCCESS )
			{
				printf("QISRGetParam: qisr get param failed Error code %d.\n",ret);
				char key = getchar();
				break;
			}
			volume = atoi(param_value);//获取到的音量信息可以用于在界面上用不同的图片展示动态效果
			//printf("volume== %d \n",volume);
			for (int i=0;i<volume;i++)
			{
				printf(".");
			}
			printf("\n");
			/*******获取当前发送音频的音量信息结束**********/
			if (ep_status == MSP_EP_AFTER_SPEECH)//检测到音频后端点
			{
				printf("QISRAudioWrite: ep_status == MSP_EP_AFTER_SPEECH.\n");
				break;
			}

			///服务器返回部分结果
			if ( rec_status == MSP_REC_STATUS_SUCCESS )
			{
				const char* result = QISRGetResult(sess_id, &rslt_status, 0, &ret);
				if( rslt_status == MSP_REC_STATUS_NO_MATCH )
					printf("get result nomatch\n");
				else
				{
					if ( result != NULL )
						fwrite(result, 1, strlen(result), fout);
					printf("get result[%d/%d]: %s\n", ret, rslt_status, result);
				}
			}
			usleep(200000);//因为是模拟录音，为了避免数据发送太快造成缓冲区溢出，所以这里暂停200ms，如果是实时录音，不必暂停
		}
		printf("\n");
		fclose(fp); 
	}

	///最后一块数据发完之后，循环从服务器端获取结果
	///考虑到网络环境不好的情况下，可以对循环次数作限定
	if (error == false)
	{	
		printf("get reuslt\n");
		int loop_count = 0;
		do 
		{
			const char* result = QISRGetResult(sess_id, &rslt_status, 0, &ret);
			if ( ret != MSP_SUCCESS )
			{
				printf("QISRGetResult err %d\n", ret);
				error = true;
				break;
			}

			if( rslt_status == MSP_REC_STATUS_NO_MATCH )
				printf("get result nomatch\n");
			else
			{
				if ( result != NULL )
					fwrite(result, 1, strlen(result), fout);
				printf("[%d]:get result[%d/%d]: %s\n", (loop_count), ret, rslt_status, result);
			}
			usleep(500000);
		} while (rslt_status != MSP_REC_STATUS_COMPLETE && loop_count++ < 30);
	}

	if( NULL != fout )
	{
		const char* result = "\r\n";
		fwrite(result, 1, strlen(result), fout);
		fclose(fout);
	}

	ret = QISRSessionEnd(sess_id, NULL);
	if ( ret != MSP_SUCCESS )
	{
		printf("QISRSessionEnd err %d\n", ret);
		return;
	}
	printf("QISRSessionEnd.\n");
	return;
}

int main(int argc, char* argv[])
{
	int ret = MSP_SUCCESS;
	const char* param1 = "sub=iat,ssm=1,auf=audio/L16;rate=16000,aue=speex,ent=sms16k,rst=json,nbest=10";//最多5个候选项，格式只能为json，编码只能为utf8
	const char* param2 = "sub=iat,ssm=1,auf=audio/L16;rate=16000,aue=speex,ent=sms16k,rst=json,nbest=-1";//最多2个候选项，格式只能为json，编码只能为utf8,nbest的取值范围为1~5
	const char* output_file = "iat_result.txt";
	///引擎初始化，只需初始化一次
	///APPID请勿随意改动
	ret = QISRInit("appid=52f44528");

	//开始一路转写会话
	run_iat("wav/一二三四五六七八九十.wav" , output_file , param1);
	run_iat("wav/一二三四五六七八九十.wav" , output_file , param2);

	//逆初始化，在程序结束前调用一次即可
	ret = QISRFini();
	char key = getchar();
	return 0;
}

