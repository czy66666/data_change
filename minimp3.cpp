#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"
#include <stdio.h>
#include <string.h>
mp3dec_frame_info_t info;
uint8_t input_buf[1024*16];
mp3d_sample_t outpcm[16*1024];
/*unsigned char *input_buf; - input byte stream*/
static mp3dec_t mp3d;

int main()
{
	
	mp3dec_init(&mp3d);
	int buf_size = 0;
	
	FILE *pf; 
//    pf = fopen("16-16-1.mp3", "rb");
    pf = fopen("znsyxl-16kbps-no.mp3", "rb");
    FILE *outfile = fopen("out.pcm","wb");
    if (pf == NULL)
    {
        printf("open file for reading");
        exit(0);
    }
    printf("open file ok\n");
	while(!feof(pf)){
//		fread(input_buf, sizeof(uint8_t), sizeof(input_buf), pf);
//		printf("data:%d\n",input_buf[1]);
		
//		printf("bitrate_kbps:%d\n",info.bitrate_kbps);
//		memset(input_buf, 0, sizeof(input_buf));
        int len = fread(input_buf, 1, 1024, pf);
        for(int i = 0;i<len;i++)
        	printf("%x",input_buf[i]);
        printf("\n");
		int samples = mp3dec_decode_frame(&mp3d, input_buf, 1024, outpcm, &info);
        printf("len: %d, samples: %d \n", len, samples);
        printf("bitrate_kbps: %d, channels: %d, frame_bytes: %d, frame_offset: %d, hz: %d, layer: %d\n", info.bitrate_kbps,info.channels,info.frame_bytes,info.frame_offset, info.hz,info.layer);
		fwrite(outpcm, 2, samples*2, outfile);
		printf("outpcm\n");
//        for(int i = 0;i<sizeof(outpcm)/sizeof(outpcm[0]);i++)
//			printf("%x ",outpcm[i]);
//		printf("\n");	
	}
	fclose(pf);
	fclose(outfile);
	printf("read file ok\n");
	return 0;
 } 

