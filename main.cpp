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

	// //打开MP3文件
	// FILE* file=fopen("znsyxl-32kbps.mp3", "rb");
	// FILE* fout= fopen("out.pcm","wb");
	
	// //获取MP3文件长度
	// fseek(file, 0, SEEK_END);
	// data_size = (int)ftell(file);

	// //读取整个MP3文件
	// fseek(file, 0, SEEK_SET);
	// input_buf = (unsigned char *)malloc(data_size);
	// fread(input_buf, 1, data_size, file);

	// //初始化minimp3的解码器结构
	// static mp3dec_t mp3d;
	// mp3dec_init(&mp3d);

	// //定义mp3dec_frame_info_t
	// mp3dec_frame_info_t info;
	// short pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
	// int mp3len = 0;

	// //逐帧解码并且播放MP3
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
	//打开MP3文件
	FILE* file=fopen("znsyxl-16kbps.mp3", "rb");
	FILE* fout= fopen("out.pcm","wb");

	//获取MP3文件长度
	fseek(file, 0, SEEK_END);
	data_size = (int)ftell(file);

	//读取整个MP3文件
	fseek(file, 0, SEEK_SET);
	input_buf = (unsigned char *)malloc(data_size);
	fread(input_buf, 1, data_size, file);
	
	//初始化minimp3的解码器结构
	static mp3dec_t mp3d;
	mp3dec_init(&mp3d);
	
	//定义mp3dec_frame_info_t
	mp3dec_frame_info_t info;
	short pcm[1024*16];
	int mp3len = 0;
	int i=0;
	//逐帧解码并且播放MP3
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

int main()
{
	unsigned char input_buf[2048];

	//打开MP3文件
	FILE* file=fopen("znsyxl-16kbps.mp3", "rb");
	FILE* fout= fopen("out.pcm","wb");

	fseek(file, 0, SEEK_END);
	int data_size = (int)ftell(file);
	//初始化minimp3的解码器结构
	static mp3dec_t mp3d;
	mp3dec_init(&mp3d);
	
	//定义mp3dec_frame_info_t
	mp3dec_frame_info_t info;
	short pcm[1024*16];
	int mp3len = 0;
	int32_t inlen = 0;
	int s32Ret = 0;
	int32_t samples = 0;
	int i = 0;
	while(inlen<data_size){
		fseek(file,inlen,SEEK_SET);
		if(info.frame_offset==-1){
			s32Ret = fread(input_buf, sizeof(char), 1024, file);
			samples = mp3dec_decode_frame(&mp3d, input_buf, 1024, pcm, &info);
		}
		else
		{
			s32Ret = fread(input_buf, sizeof(char), info.frame_bytes, file);
			samples = mp3dec_decode_frame(&mp3d, input_buf, s32Ret, pcm, &info);
		}
			
		printf("s32Ret:%d\n",s32Ret);
		// samples = mp3dec_decode_frame(&mp3d, input_buf, 1024, pcm, &info);
		inlen += info.frame_bytes;
		printf("samples:%d\n",samples);
		printf("info:%d %d %d %d %d %d\n",info.bitrate_kbps,info.channels,info.frame_bytes,info.frame_offset,info.hz,info.layer); 
		if(samples!=0)
			fwrite(pcm, sizeof(short), samples, fout);
		i++;
	}
	printf("i:%d\n",i);
	fclose(fout);
	fclose(file);
}

#endif