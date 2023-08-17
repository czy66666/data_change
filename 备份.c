#include "audioDrv.h"
#include "slpman.h"

//>>>

#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"
#include <string.h>
#define INCBIN(symname, sizename, filename, section)                    \
    __asm__ (".section " section "; .align 4; .globl "#symname);        \
    __asm__ (""#symname ":\n.incbin \"" filename "\"");                 \
    __asm__ (".section " section "; .align 1;");                        \
    __asm__ (""#symname "_end:");                                       \
    __asm__ (".section " section "; .align 4; .globl "#sizename);       \
    __asm__ (""#sizename ": .long "#symname "_end - "#symname " - 1");  \
    extern unsigned char symname[];                                     \
    extern unsigned int sizename
INCBIN(mp3_data, mp3_data_size, "C:/Users/admin/Desktop/minimp3/znsyxl-16kbps-no.mp3", ".rodata");
// INCBIN(beep_data, beep_data_size, "C:/Users/admin/Desktop/minimp3/out.pcm", ".rodata");

//<<<
// Only 8k_DUAL will use codec8388, other demos uses 8311
#define AUDIO_DEMO_44K          0
#define AUDIO_DEMO_16K          1
#define AUDIO_DEMO_8K_MONO      0
#define AUDIO_DEMO_8K_DUAL      0
#define AUDIO_DEMO_48K			0
#define AUDIO_RECORD_PLAY		0

static volatile bool isTxTransferDone = false;
static volatile bool isRxTransferDone = false;

#if (AUDIO_DEMO_44K == 1 || AUDIO_RECORD_PLAY == 1)
extern uint8_t audio16bit_44k[];
uint8_t   *audioSrc = (uint8_t*)audio16bit_44k;
static i2sSampleRate_e sampleRateUse = (i2sSampleRate_e)SAMPLERATE_44_1K;

#elif (AUDIO_DEMO_16K == 1 )
__attribute__((aligned(8))) uint8_t playdata[1024*2] = {};
extern uint8_t audio16bit_16k[];
uint8_t   *audioSrc = (uint8_t*)playdata;
static i2sSampleRate_e sampleRateUse = SAMPLERATE_16K;

#elif (AUDIO_DEMO_8K_MONO == 1)
extern uint8_t audio16bit_8k[];
uint8_t   *audioSrc = (uint8_t*)audio16bit_8k;
static i2sSampleRate_e sampleRateUse = SAMPLERATE_8K;

#elif (AUDIO_DEMO_8K_DUAL == 1)
extern uint8_t audio16bit_8k[];
__attribute__((aligned(8))) uint8_t audio16bit_8k_D[92784] = {}; // length should *2 than mono
uint8_t   *audioSrc = (uint8_t*)audio16bit_8k_D;
static i2sSampleRate_e sampleRateUse = SAMPLERATE_8K;

#elif (AUDIO_DEMO_48K == 1 )
extern uint8_t audio16bit_48k[];
uint8_t   *audioSrc = (uint8_t*)audio16bit_48k;
static i2sSampleRate_e sampleRateUse = SAMPLERATE_48K;

#endif

extern uint32_t rawDataGetCnt(i2sSampleRate_e sampleRate);

i2sParamCtrl_t audioParamCtrl;
static uint32_t cnt = 0;
uint32_t totalNum;

//>>>
#define REDA_SIZE 2048
typedef enum
{
	MONO_CHANNEL	= 0,	//Single track
	DUAL_CHANNELS	= 1,	//Dual track
	DIFFERENTIAL	= 2,	//Differential signal
}ChannelSel;
uint8_t outpcmchannel[3]= {1,2,2};
uint8_t TransferFlag = 0;
int16_t pcm[REDA_SIZE];
int16_t outpcm[REDA_SIZE*2];
int audio_play_mp3file(ChannelSel channelsel_)//(const char * filein,ChannelSel channelsel_)
{
	
	//open mp3 file
	// FILE* file=fopen(filein, "rb");
	// FILE* fout= fopen("out.pcm","wb");

	//init minimp3 coder struct
	static mp3dec_t mp3d;
	mp3dec_init(&mp3d);

	//define mp3dec_frame_info_t
	mp3dec_frame_info_t info;
	unsigned char input_buf[REDA_SIZE];
	
	int inlen = 0;
	int read_once_size = REDA_SIZE;
int i = 0;
	while(1){
		// fseek(file,inlen,SEEK_SET);
		// int s32Ret = fread(input_buf, sizeof(char), read_once_size, file);
		// if(s32Ret<=0)
		// 	break;
        
        if(inlen>=mp3_data_size){
            printf("4444\n");
            break;
        }
        printf("1111\n");
        memccpy(input_buf,mp3_data+inlen,sizeof(uint8_t),read_once_size);
        for (size_t iii = 0; iii < read_once_size; iii++)
        {
            printf("%02x ",input_buf[iii]);
        }
        
        printf("2222\n");
		int32_t samples = mp3dec_decode_frame(&mp3d, input_buf, read_once_size, pcm, &info);
	    printf("3333\n");
		inlen += info.frame_bytes;
		read_once_size = info.frame_bytes;

		// printf("s32Ret:%d\n",s32Ret);
		printf("inlen:%d\n",inlen);
		printf("samples:%d\n",samples);
		printf("info:%d %d %d %d %d %d\n",info.bitrate_kbps,info.channels,info.frame_bytes,info.frame_offset,info.hz,info.layer); 

		//success coder
		if(samples>0 && info.frame_bytes>0){
			if(channelsel_ == MONO_CHANNEL){
				if(info.channels == 1){
					memcpy(outpcm,pcm,samples*sizeof(uint16_t));
				}
				else if(info.channels == 2){
					for(int index=0; index<samples; index++){
						outpcm[index] = pcm[index*info.channels];
					}
				}
			}
			else if(channelsel_ == DUAL_CHANNELS){
				if(info.channels == 1){
					for(int index=0; index<samples; index++){
						outpcm[index*2]   = pcm[index];
						outpcm[index*2+1] = pcm[index];
					}
				}
				else if(info.channels == 2){
					memcpy(outpcm,pcm,samples*sizeof(uint16_t)*info.channels);
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
			// fwrite(outpcm, sizeof(short), samples * outpcmchannel[channelsel_], fout);
            for (size_t ii = 0; ii < samples*sizeof(uint16_t)*info.channels; ii++)
            {
               printf("%04x ",pcm[ii]);
            }
            printf("\n");
            memcpy(playdata+i*samples*sizeof(uint16_t)*info.channels,outpcm,samples*sizeof(uint16_t)*info.channels);
            i++;
		}
		//The decoder skipped ID3 or invalid data
		else if(samples == 0 && info.frame_bytes > 0){
            printf("contuie\n");
			continue;
		}
		//Insufficient data
		else if (samples == 0 && info.frame_bytes == 0){
			printf("data error!!!\n");
		}
	}
	// fclose(fout);
	// fclose(file);
}
//<<<

static void i2sTxCb(uint32_t event, uint32_t arg)
{
	if (cnt < totalNum / AUDIO_TX_TRANSFER_SIZE)
	{
		cnt++;
		HAL_I2sTransfer(PLAY, (uint8_t*)(audioSrc + cnt*AUDIO_TX_TRANSFER_SIZE), AUDIO_TX_TRANSFER_SIZE);
	}
	else if (cnt == totalNum / AUDIO_TX_TRANSFER_SIZE)
	{
		// last trunk
		HAL_I2sTransfer(PLAY, (uint8_t*)(audioSrc + cnt*AUDIO_TX_TRANSFER_SIZE), totalNum-cnt*AUDIO_TX_TRANSFER_SIZE);
		cnt++;
	}
	else
	{
		cnt = 0;
		HAL_I2sTransfer(PLAY, (uint8_t*)(audioSrc + cnt*AUDIO_TX_TRANSFER_SIZE), AUDIO_TX_TRANSFER_SIZE);
	}
}

static uint32_t rxCnt;
static void i2sRxCb(uint32_t event, uint32_t arg)
{    

	if(event & ARM_SPI_EVENT_TRANSFER_COMPLETE)
    {
        isRxTransferDone = true;
        rxCnt++;
    }
}

void AUDIO_ExampleEntry2()
{    
    HAL_I2sInit(I2S_CLK_ENABLE, i2sTxCb, i2sRxCb);
    printf("my audio\n");
#if (AUDIO_DEMO_8K_DUAL == 1)
    audioParamCtrl.codecType     = CODEC_ES8388;
    uint64_t i=0, j=0;
    for (i=0; i<rawDataGetCnt((i2sSampleRate_e)sampleRateUse); i+=2)
    {
    	audio16bit_8k_D[j]   = audio16bit_8k[i];
    	audio16bit_8k_D[j+1] = audio16bit_8k[i+1];
    	audio16bit_8k_D[j+2] = audio16bit_8k[i];
    	audio16bit_8k_D[j+3] = audio16bit_8k[i+1];
        j+=4;
    }
#else
    audioParamCtrl.codecType     = CODEC_TM8211;
#endif 

    switch (audioParamCtrl.codecType)
    {
        case CODEC_ES8311: 
        case CODEC_ES7148:
        case CODEC_ES7149:
        case CODEC_TM8211:
        {
			slpManNormalIOVoltSet(IOVOLT_3_30V);
			break;
        }

        default:
            break;
    }
    
    audioParamCtrl.frameSize     = FRAME_SIZE_16_16;
	if ((audioParamCtrl.codecType == CODEC_ES7148) || (audioParamCtrl.codecType == CODEC_ES7149)||(audioParamCtrl.codecType == CODEC_ES8311))
	{
		audioParamCtrl.mode 	 = I2S_MODE; // 7148/7149 only supports I2S mode
	}
	else
	{
		audioParamCtrl.mode 	 = MSB_MODE; // other codec uses MSB as default
	}
    audioParamCtrl.playRecord    = RECORD;
    audioParamCtrl.role          = CODEC_SLAVE_MODE;

#if (AUDIO_DEMO_8K_DUAL == 1)
    audioParamCtrl.channelSel    = DUAL_CHANNEL;
#else
    audioParamCtrl.channelSel    = MONO;
#endif
    audioParamCtrl.sampleRate    = (i2sSampleRate_e)sampleRateUse;

#if (AUDIO_DEMO_8K_DUAL == 1)
    HAL_I2SSetTotalNum(rawDataGetCnt(2*(i2sSampleRate_e)sampleRateUse));
#else
    HAL_I2SSetTotalNum(rawDataGetCnt((i2sSampleRate_e)sampleRateUse));
#endif

    HAL_I2sConfig(audioParamCtrl);

#if (AUDIO_RECORD_PLAY == 1)
	totalNum = AUDIO_RX_TRANSFER_SIZE*I2S_DMA_RX_DESCRIPTOR_CHAIN_NUM;
#else
	totalNum = sizeof(playdata);//HAL_I2SGetTotalNum();//beep_data_size;
#endif

#if (AUDIO_RECORD_PLAY == 1)// If want to record sound and then play it, open this conditional compilation.
	if (audioParamCtrl.codecType == CODEC_ES8311) 
    {
        codecWriteVal(CODEC_ES8311, 0x32, 0xff); // record need big volumn
    }
    else if (audioParamCtrl.codecType == CODEC_ES8388)
    {
        // If need to record, increase the volume of codec, otherwise may record fail
        codecWriteVal(CODEC_ES8388, 0x1a, 0x00);
        delay_us(30*1000);
    }
	
    // Waiting until recording voice is done.
    HAL_I2sTransfer(RECORD, (uint8_t*)audioSrc, AUDIO_RX_TRANSFER_SIZE);
    while (isRxTransferDone == false);

#else
	if (audioParamCtrl.codecType == CODEC_ES8388)
	{
		// set es8388 working mode
		es8388SetMode(audioParamCtrl.mode);
	}
	else if ((audioParamCtrl.codecType == CODEC_ES7148) || (audioParamCtrl.codecType == CODEC_ES7149))
	{
		// decrease volumn
		HAL_I2sSrcAdjustVolumn((int16_t*)audioSrc, rawDataGetCnt((i2sSampleRate_e)sampleRateUse), 02);
	}
    else if(audioParamCtrl.codecType == CODEC_ES8311)
    {
        // set es8311 working mode
		es8311SetMode(audioParamCtrl.mode);
		codecWriteVal(CODEC_ES8311, 0x32, 180); // record need big volumn
    }
#endif

	audioParamCtrl.playRecord = PLAY;
	HAL_I2SSetPlayRecord(audioParamCtrl.playRecord);
     audio_play_mp3file(MONO_CHANNEL);
    HAL_I2sTransfer(PLAY, (uint8_t*)audioSrc, AUDIO_TX_TRANSFER_SIZE);
   
	while (1);
}
