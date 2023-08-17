#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if 0
int main()
{
	// unsigned char *input_buf = NULL;
	// int data_size = 0;

	// //��MP3�ļ�
	// FILE* file=fopen("znsyxl-32kbps.mp3", "rb");
	// FILE* fout= fopen("out.pcm","wb");
	
	// //��ȡMP3�ļ�����
	// fseek(file, 0, SEEK_END);
	// data_size = (int)ftell(file);

	// //��ȡ����MP3�ļ�
	// fseek(file, 0, SEEK_SET);
	// input_buf = (unsigned char *)malloc(data_size);
	// fread(input_buf, 1, data_size, file);

	// //��ʼ��minimp3�Ľ������ṹ
	// static mp3dec_t mp3d;
	// mp3dec_init(&mp3d);

	// //����mp3dec_frame_info_t
	// mp3dec_frame_info_t info;
	// short pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
	// int mp3len = 0;

	// //��֡���벢�Ҳ���MP3
	// int samples = mp3dec_decode_frame(&mp3d, input_buf, data_size, pcm, &info);
	// while(samples) {
	// 	//play(pcm, samples);
		
	// 	mp3len += info.frame_bytes;
	// 	samples = mp3dec_decode_frame(&mp3d, input_buf + mp3len, data_size - mp3len, pcm, &info);
	// 	fwrite(pcm, sizeof(short), samples, fout);
	// }

	// free(input_buf);
	// fclose(fout);
	// fclose(file);

	unsigned char *input_buf = NULL;
	int data_size = 0;
	//��MP3�ļ�
	FILE* file=fopen("znsyxl-16kbps.mp3", "rb");
	FILE* fout= fopen("out.pcm","wb");

	//��ȡMP3�ļ�����
	fseek(file, 0, SEEK_END);
	data_size = (int)ftell(file);

	//��ȡ����MP3�ļ�
	fseek(file, 0, SEEK_SET);
	input_buf = (unsigned char *)malloc(data_size);
	fread(input_buf, 1, data_size, file);
	
	//��ʼ��minimp3�Ľ������ṹ
	static mp3dec_t mp3d;
	mp3dec_init(&mp3d);
	
	//����mp3dec_frame_info_t
	mp3dec_frame_info_t info;
	short pcm[1024*16];
	int mp3len = 0;
	int i=0;
	//��֡���벢�Ҳ���MP3
	int samples = mp3dec_decode_frame(&mp3d, input_buf, data_size, pcm, &info);
	while(samples){
	    mp3len += info.frame_bytes;
	    samples = mp3dec_decode_frame(&mp3d, input_buf + mp3len, data_size - mp3len, pcm, &info);
		printf("samples:%d\n",samples);
	    printf("info:%d %d %d %d %d %d\n",info.bitrate_kbps,info.channels,info.frame_bytes,info.frame_offset,info.hz,info.layer); 

		fwrite(pcm, sizeof(short), samples, fout);
		i++;
	}
	printf("i=%d\n",i);
	free(input_buf);
	fclose(fout);
	fclose(file);
}

#else

#define REDA_SIZE 2048
typedef enum
{
	MONO			= 0,	//Single track
	DUAL_CHANNEL	= 1,	//Dual track
	DIFFERENTIAL	= 2,	//Differential signal
}ChannelSel;
uint8_t outpcmchannel[3]= {1,2,2};

int audio_play_mp3file(const char * filein,ChannelSel channelsel_)
{
	
	//open mp3 file
	FILE* file=fopen(filein, "rb");
	FILE* fout= fopen("out.pcm","wb");

	//init minimp3 coder struct
	static mp3dec_t mp3d;
	mp3dec_init(&mp3d);

	//define mp3dec_frame_info_t
	mp3dec_frame_info_t info;
	unsigned char input_buf[REDA_SIZE];
	short pcm[REDA_SIZE];
	short outpcm[REDA_SIZE*2];
	int inlen = 0;
	int read_once_size = REDA_SIZE;

	while(1){
		fseek(file,inlen,SEEK_SET);
		int s32Ret = fread(input_buf, sizeof(char), read_once_size, file);
		if(s32Ret<=0)
			break;
		int32_t samples = mp3dec_decode_frame(&mp3d, input_buf, read_once_size, pcm, &info);
	
		inlen += info.frame_bytes;
		read_once_size = info.frame_bytes;

		printf("s32Ret:%d\n",s32Ret);
		printf("inlen:%d\n",inlen);
		printf("samples:%d\n",samples);
		printf("info:%d %d %d %d %d %d\n",info.bitrate_kbps,info.channels,info.frame_bytes,info.frame_offset,info.hz,info.layer); 

		//success coder
		if(samples>0 && info.frame_bytes>0){
			if(channelsel_ == MONO){
				if(info.channels == 1){
					memcpy(outpcm,pcm,samples*sizeof(short));
				}
				else if(info.channels == 2){
					for(int index=0; index<samples; index++){
						outpcm[index] = pcm[index*info.channels];
					}
				}
			}
			else if(channelsel_ == DUAL_CHANNEL){
				if(info.channels == 1){
					for(int index=0; index<samples; index++){
						outpcm[index*2]   = pcm[index];
						outpcm[index*2+1] = pcm[index];
					}
				}
				else if(info.channels == 2){
					memcpy(outpcm,pcm,samples*sizeof(short)*info.channels);
				}
			}
			else if(channelsel_ == DIFFERENTIAL){
				if(info.channels == 1){
					for(int index=0; index<samples; index++){
						outpcm[index*2] = pcm[index];
						outpcm[index*2+1] = 0xffff - pcm[index];
					}
				}
				else if(info.channels == 2){
					for(int index=1; index<samples; index++){
						outpcm[index*2] = pcm[index*2];
						outpcm[index*2+1] = 0xffff - pcm[index*2+1];
					}
				}
				
			}
			fwrite(outpcm, sizeof(short), samples * outpcmchannel[channelsel_], fout);
		}
		//The decoder skipped ID3 or invalid data
		else if(samples == 0 && info.frame_bytes > 0){
			continue;
		}
		//Insufficient data
		else if (samples == 0 && info.frame_bytes == 0){
			printf("data error!!!\n");
		}
	}
	fclose(fout);
	fclose(file);
}
int main()
{
	audio_play_mp3file("znsyxl-16kbps.mp3",MONO);
}

#endif