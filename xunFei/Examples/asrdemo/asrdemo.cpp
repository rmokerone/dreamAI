// asrdemo.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "../../include/qisr.h"

#define TRUE 1
#define FALSE 0

int get_grammar_id( int upload );//获取语法ID
int run_asr(const char* asrfile);//测试识别效果
const char*  get_audio_file(void);//选择音频文件

char GrammarID[128];
const int BUFFER_NUM = 4096;
const int MAX_KEYWORD_LEN = 4096;

int main(int argc, char* argv[])
{
	int ret = MSP_SUCCESS;
	const char* asrfile = get_audio_file();
	//appid 请勿随意改动
	ret = QISRInit("appid=52f44528");
	if(ret != MSP_SUCCESS)
	{
		printf("QISRInit with errorCode: %d \n", ret);
		return 0;
	}

	memset(GrammarID, 0, sizeof(GrammarID));
	ret = get_grammar_id( FALSE );
	if(ret != MSP_SUCCESS)
	{
		printf("get_grammar_id with errorCode: %d \n", ret);
		return 0;
	}

	ret = run_asr(asrfile);
	QISRFini();
	char key = getchar();
	return 0;
}

int get_grammar_id( int upload)
{
	int ret = MSP_SUCCESS;
	const char * sessionID = NULL;
	if (FALSE == upload)
	{
		strcpy(GrammarID, "e7eb1a443ee143d5e7ac52cb794810fe");
		//这个ID是我上传之后记录下来的。语法上传之后永久保存在服务器上，所以不要反复上传同样的语法
		return 0;
	}
	
	//如果想要重新上传语法，传入参数upload置为TRUE，就可以走下面的上传语法流程
	sessionID = QISRSessionBegin(NULL, "ssm=1,sub=asr", &ret);
	if(ret != MSP_SUCCESS)
	{
		printf("QISRSessionBegin with errorCode: %d \n", ret);
		return ret;
	}

	char UserData[MAX_KEYWORD_LEN];
	memset(UserData, 0, MAX_KEYWORD_LEN);
	FILE* fp = fopen("asr_keywords_utf8.txt", "rb");//关键字列表文件必须是utf8格式
	if (fp == NULL)
	{
		printf("keyword file cannot open\n");
		return -1;
	}
	unsigned int len = (unsigned int)fread(UserData, 1, MAX_KEYWORD_LEN, fp);
	UserData[len] = 0;
	fclose(fp);
	const char* testID = QISRUploadData(sessionID, "contact", UserData, len, "dtt=keylist", &ret);
	if(ret != MSP_SUCCESS)
	{
		printf("QISRUploadData with errorCode: %d \n", ret);
		return ret;
	}
	memcpy((void*)GrammarID, testID, strlen(testID));
	printf("GrammarID: \"%s\" \n", GrammarID);//将获得的GrammarID输出到屏幕上

	QISRSessionEnd(sessionID, "normal");
	return 0;
}

int run_asr(const char* asrfile)
{
	int ret = MSP_SUCCESS;
	int i = 0;
	FILE* fp = NULL;
	char buff[BUFFER_NUM];
	unsigned int len;
	int status = MSP_AUDIO_SAMPLE_CONTINUE, ep_status = -1, rec_status = -1, rslt_status = -1;

	const char* param = "rst=plain,sub=asr,ssm=1,aue=speex,auf=audio/L16;rate=16000";//注意sub=asr
	const char* sess_id = QISRSessionBegin(GrammarID, param, &ret);//将语法ID传入QISRSessionBegin
	if ( MSP_SUCCESS != ret )
	{
		printf("QISRSessionBegin err %d\n", ret);	
		return ret;
	}

	fp = fopen( asrfile , "rb");//我们提供了几个音频文件，测试时根据需要在这里更换
	if ( NULL == fp )
	{
		printf("failed to open file,please check the file.\n");
		QISRSessionEnd(sess_id, "normal");
		return -1;
	}

	printf("writing audio...\n");
	while ( !feof(fp) )
	{
		len = (unsigned int)fread(buff, 1, BUFFER_NUM, fp);
		feof(fp) ? status = MSP_AUDIO_SAMPLE_LAST : status = MSP_AUDIO_SAMPLE_CONTINUE;//最后一块音频要使用last
		ret = QISRAudioWrite(sess_id, buff, len, status, &ep_status, &rec_status);
		if ( ret != MSP_SUCCESS )
		{
			printf("\nQISRAudioWrite err %d\n", ret);
			break;
		}

		if ( rec_status == MSP_REC_STATUS_SUCCESS )
		{
			const char* result = QISRGetResult(sess_id, &rslt_status, 0, &ret);
			if (ret != MSP_SUCCESS )
			{
				printf("error code: %d\n", ret);
				break;
			}
			else if( rslt_status == MSP_REC_STATUS_NO_MATCH )
				printf("get result nomatch\n");
			else
			{
				if ( result != NULL )
					printf("get result[%d/%d]:len:%d\n %s\n", ret, rslt_status,strlen(result), result);
			}
		}
		printf(".");
		usleep(200000);//因为是模拟录音，为了避免数据发送太快造成缓冲区溢出，所以这里暂停200ms，如果是实时录音，不必暂停
	}
	printf("\n");

	if (ret == MSP_SUCCESS)
	{	
		printf("get reuslt\n");
		char asr_result[1024] = "";
		unsigned int pos_of_result = 0;
		int loop_count = 0;
		do 
		{
			const char* result = QISRGetResult(sess_id, &rslt_status, 0, &ret);
			if ( ret != 0 )
			{
				printf("QISRGetResult err %d\n", ret);
				break;
			}

			if( rslt_status == MSP_REC_STATUS_NO_MATCH )
			{
				printf("get result nomatch\n");
			}
			else if ( result != NULL )
			{
				printf("[%d]:get result[%d/%d]: %s\n", (loop_count), ret, rslt_status, result);
				strcpy(asr_result+pos_of_result,result);
				pos_of_result += (unsigned int)strlen(result);
			}
			else
			{
				printf("[%d]:get result[%d/%d]\n",(loop_count), ret, rslt_status);
			}
			usleep(500000);
		} while (rslt_status != MSP_REC_STATUS_COMPLETE && loop_count++ < 30);
		if (strcmp(asr_result,"")==0)
		{
			printf("no result\n");
		}

	}

	QISRSessionEnd(sess_id, NULL);
	printf("QISRSessionEnd.\n");
	fclose(fp); 

	return 0;
}

const char*  get_audio_file(void)
{
	char key = 0;
	while(key != 27)//按Esc则退出
	{
		system("clear");//清屏
		printf("请选择音频文件：\n");
		printf("1.阿里山龙胆\n");
		printf("2.科大讯飞\n");
		printf("3.齐鲁石化\n");
		printf("4.一二三四五六七八九十\n");
		printf("注意：第三条的音频故意结巴说出来的，用于展示效果。\n      关键字列表中没有第四条，展示如果用户说的词语不在列表中，会得到什么结果。\n");
		key = getchar();
		switch(key)
		{
		case '1':
			printf("1.阿里山龙胆\n");
			return "wav/阿里山龙胆.wav";
		case '2':
			printf("2.科大讯飞\n");
			return "wav/科大讯飞.wav";
		case '3':
			printf("3.齐鲁石化\n");
			return "wav/齐鲁石化.wav";
		case '4':
			printf("4.一二三四五六七八九十\n");
			return "wav/一二三四五六七八九十.wav";
		default:
			continue;
		}
	}
	exit(0);
	return NULL;
}
