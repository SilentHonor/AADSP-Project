/////////////////////////////////////////////////////////
// 	pcmdec_sim.cpp
//
//  Author.......: Katarina Cvetkovic (katarinac)
//	Contact......: katarina.cvetkovic@rt-rk.com
//	Date.........: Apr 15, 2024
//
//	This file contains definitions of all PCM decoder function
//	called in main.cpp that should read samples from input file
//
/////////////////////////////////////////////////////////

#include <iostream>
#include <string>
#include "haos.h"
#include "pcmdec_sim.h"
#include "bitripper_sim.h"
#include "wavefile.h"

#define NO_SOURCE 	(0x08000000)
#define BLOCK_SIZE 	128
#define FRAME_SIZE	256

extern int nInputChannels;
extern int samplesPerChannel;
extern int bitsPerSample;
extern int sampleRate;

float tempSampleBuffer[MAX_NUM_CHANNELS*BRICK_SIZE];
float sampleBuffer[MAX_NUM_CHANNELS*BRICK_SIZE];
unsigned int inputChannelMask = 0;
float* rdPtr;  //read pointer of temp input buffer

struct
{
	unsigned int pcmEnable;
	unsigned int srcActiveChannels[16];
	unsigned int reserved[32];   // just to be sure no host comm message stomps on something
} PcmDecoder_mcv=
{
							//default values for PcmDecoderMCV
	0x00000001,				// enable pcm decoder
	0,						// input wave channel 0,
	NO_SOURCE,				// input wave channel 2,
	1,						// input wave channel 1,
	NO_SOURCE,				// input wave channel 3,
	NO_SOURCE,				// input wave channel 4,
	NO_SOURCE,				// input wave channel 5,
	NO_SOURCE,				// input wave channel 6,
	NO_SOURCE,				// input wave channel 7,
	NO_SOURCE,				// input wave channel 8,
	NO_SOURCE,				// input wave channel 9,
	NO_SOURCE,				// input wave channel 10,
	NO_SOURCE,				// input wave channel 11,
	NO_SOURCE,				// input wave channel 12,
	NO_SOURCE,				// input wave channel 13,
	NO_SOURCE,				// input wave channel 14,
	NO_SOURCE				// input wave channel 15
};

HAOS_Mct_t PcmDecoder_mct =
{
	PcmDecoder_prekickFunction,
	PcmDecoder_postkickFunction,
	0,
	0,
	PcmDecoder_brickFunction,
	0,
	0,
	0,
	0
};

HAOS_Mif_t PcmDecoder_mif = {&PcmDecoder_mcv, &PcmDecoder_mct};

HAOS_Odt_t PcmDecoder_odt =
{
	{&PcmDecoder_mif, 0x10},
	{0, 0}
};

HAOS_OdtEntry_t* PcmDecoder_odtPtr = PcmDecoder_odt;

HAOS_FrameData_t PcmDecoder_frameData = {0};
HAOS_CopyToIOPtrs_t PcmDecoder_copyToIOPtrs = {0};

void __fg_call PcmDecoder_prekickFunction(void* PcmDecoder_mifPtr)
{
	std::cout << "start prekick PCM" << std::endl;

	std::cout << "end prekick PCM" << std::endl;
}

void __fg_call PcmDecoder_postkickFunction()
{
	std::cout << "start postkick PCM" << std::endl;

	//set values of frame data structure

	for (int ch = 0; ch < NUMBER_OF_IO_CHANNELS; ch++)
	{
		if (PcmDecoder_mcv.srcActiveChannels[ch] != NO_SOURCE)
		{
			//init io masks of active channels
			PcmDecoder_frameData.inputChannelMask |= (1<<ch);
			PcmDecoder_frameData.outputChannelMask |= (1<<ch);
		}
	}
	PcmDecoder_frameData.sampleRate = sampleRate;
	PcmDecoder_frameData.decodeInfo = 0x1;
	PcmDecoder_copyToIOPtrs.frameData = &PcmDecoder_frameData;
	__HAOS_PPM_VALID_CHANNELS = PcmDecoder_frameData.outputChannelMask;

	std::cout << "end postkick PCM" << std::endl;
}

void __fg_call PcmDecoder_brickFunction()
{
	//1. extract bits from FIFO0 to temp buffers
	//2. copy buffers to IO
	//3. repeat process
	int inputChannelMask;
	int i = 0;
	int j = 0;
	int sample = 0;
	int loopSize = BRICK_SIZE*nInputChannels;

	if (PcmDecoder_mcv.pcmEnable)
	{
		if(!__HAOS_AT_EOF)
		{
			for(sample = 0; sample < BRICK_SIZE*nInputChannels;)
			{
				for (int ch = 0; ch < nInputChannels; ch++)
				{
					tempSampleBuffer[sample] = BitRipper_extractBits(32) / SAMPLE_SCALE;
					sample++;
				}
				samplesPerChannel--;
			}

			//copy samples to proper channel
			for (int ch = 0; ch < nInputChannels; ch++)
			{
				for (int sample = ch; sample < loopSize; sample += nInputChannels)
				{
					sampleBuffer[i] = tempSampleBuffer[sample];
					i++;
				}
			}

			//set read pointer
			rdPtr = sampleBuffer;
			inputChannelMask = PcmDecoder_frameData.inputChannelMask;
			{
				PcmDecoder_frameData.outputChannelMask = PcmDecoder_frameData.inputChannelMask;;
				while (inputChannelMask != 0)
				{
					if (inputChannelMask & 1)
					{
						PcmDecoder_copyToIOPtrs.IOBufferPtrs[j] = rdPtr;
						rdPtr+=BRICK_SIZE;
					}
					j++;
					inputChannelMask = inputChannelMask >> 1;
				}
			}

			//copy Brick to IO
			HAOS_copyBrickToIO(&PcmDecoder_copyToIOPtrs);
		}
	}
}
