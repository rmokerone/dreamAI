/*
 * =====================================================================================
 *
 *       Filename:  main.c
 *
 *    Description: alsa output form wav 
 *
 *        Version:  1.0
 *        Created:  02/08/2014 03:57:35 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Moker (UESC@AHPU), rmokerone@gmail.com
 *        Company:  
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alsa/asoundlib.h>

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
	SR_WORD         bits_per_sample;          // = 量化比特数: 8 | 16

	char            data[4];                        // = "data";
	SR_DWORD        data_size;                // = 纯数据长度 : FileSize - 44 
} waveHdr;

const char fileName[] = "text_to_speech_test_1.wav";

int wav_play (FILE *fp);

int main (void)
{
	FILE *fp;
	int n_read;

	fp = fopen (fileName,"rb");
	if (fp == NULL)
	{
		printf ("Openle error!\n");
		exit (0);
	}
	else
		printf ("Open file sucess!\n");
	n_read = fread (&waveHdr,sizeof(struct wave_pcm_hdr),1,fp);
//	printf("n_read = %d\n", n_read); 
	printf ("riff = %s\n", waveHdr.riff);	
	
	wav_play(fp);

	return 0;
	
}

int wav_play (FILE * fp)
{
	int rc;
	int ret;
	int dir = 0;
	int size;
	char *buffer;

	int bit = waveHdr.bits_per_sample;
	int channels = waveHdr.channels;
	int frequency = waveHdr.samples_per_sec;
	int datablock = waveHdr.block_align;

	snd_pcm_t * handle;
	snd_pcm_hw_params_t *params;
	snd_pcm_uframes_t frames;

	snd_pcm_open (&handle,"default",SND_PCM_STREAM_PLAYBACK,0);
	snd_pcm_hw_params_alloca(&params);
	snd_pcm_hw_params_any(handle,params);
	snd_pcm_hw_params_set_access(handle,params,SND_PCM_ACCESS_RW_INTERLEAVED);
	      //采样位数
	switch(bit/8)
        {
               case 1:snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_U8);
               break ;
               case 2:snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);
               break ;
               case 3:snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S24_LE);
               break ;
        }
	printf ("bit = %d channels = %d, datablock = %d\n", bit, channels
		,datablock);
        snd_pcm_hw_params_set_channels(handle, params, channels); 
	snd_pcm_hw_params_set_rate_near(handle,params, &frequency,&dir);
 	snd_pcm_hw_params(handle,params);
	rc = snd_pcm_hw_params_get_period_size(params,&frames,&dir);
	
	size = frames * datablock;
	buffer = (char *)malloc (size);
	fseek (fp,58,SEEK_SET);
	
	while (1)
	{
		memset(buffer,0,sizeof (buffer));
		ret = fread (buffer,size,1,fp);
		if (ret == 0)
		{
			printf ("write succes!\n");
			break;
		}
		while (ret = snd_pcm_writei(handle,buffer,frames) <0)
		{
			usleep (2000);
			if (ret == -EPIPE)
			{
				fprintf (stderr,"underrun occurred\n");
				snd_pcm_prepare (handle);
			}
			else if (ret < 0);
			{
				fprintf(stderr,"error from writei:5s\n",
					snd_strerror(ret));
			}
		}
	}
	
	snd_pcm_drain(handle);
	snd_pcm_close(handle);
	free(buffer);
	return 0;
	if (rc < 0)
		printf  ("rc = %d snd_pcm open error!\n",rc);
	printf ("rc = %d size = %d\n",rc,size);
}
