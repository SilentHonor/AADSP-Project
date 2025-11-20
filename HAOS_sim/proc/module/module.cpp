#pragma warning(disable : 4996)
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include "haos.h"
#include "wavefile.h"
#include "config.h"
#include "delay.h"
#include "inverter.h"
#include "module.h"

// Global buffers
double sampleBufferInput[MAX_NUM_CHANNEL][BLOCK_SIZE];
double sampleBufferOutput[OUTPUT_NUM_CHANNEL][BLOCK_SIZE];
double tempBuffer[OUTPUT_NUM_CHANNEL][BLOCK_SIZE];
double sumBuffer[BLOCK_SIZE];

DelayLine delayLine;
inverter_data_t inverter;



double gain(double db) {
	return pow(10.0, db / 20.0);
}


uint8_t getChannels(const char* mode) {
	for (const auto& m : modes)
	{
		if (strcmp(mode, m.modeName) == 0)
		{
			return m.mask;
		}
	}
	printf("Invalid Output mode!");
	exit(1);
}

struct {
	int enable;				// {1 (On), 0 (Off)}
	double inputGain;		// [0, -inf]
	double headroomGain;	// [0, -inf]
	char outputCh[8];		//"2_0_0", "2_0_1" ..
}Module_mcv = {
	1,
	-4,
	-5,
	"3_2_1"
};

HAOS_Mct_t Module_mct{
	0,
	Module_postkickFunction,
	0,
	0,
	Module_brickFunction,
	0,
	0,
	0,
	0
};

HAOS_Mif_t Module_mif = { &Module_mcv, &Module_mct };

HAOS_Odt_t Module_odt = {
	{ &Module_mif, 0x70 },
	{ 0, 0 }
};

void processBlock(double inputBuffer[][BLOCK_SIZE], double outputBuffer[][BLOCK_SIZE]) {
	for (int j = 0; j < BLOCK_SIZE; ++j) {
		inputBuffer[0][j] *= gain(Module_mcv.inputGain);
		inputBuffer[2][j] *= gain(Module_mcv.inputGain);

		sumBuffer[j] = inputBuffer[0][j] + inputBuffer[2][j];
		sumBuffer[j] *= gain(Module_mcv.headroomGain);

		Delayline_processBlock(&delayLine, &inputBuffer[0][j], &outputBuffer[3][j], 1);
		Delayline_processBlock(&delayLine, &inputBuffer[2][j], &outputBuffer[4][j], 1);


		outputBuffer[5][j] = sumBuffer[j];												//LFE
		gst_audio_invert_transform(&inverter, &sumBuffer[j], &outputBuffer[1][j], 1);	//C

		sumBuffer[j] *= gain(-6);
		outputBuffer[0][j] = sumBuffer[j];												//L
		outputBuffer[2][j] = sumBuffer[j];												//R


		outputBuffer[3][j] *= gain(-2);
		outputBuffer[4][j] *= gain(-2);

		outputBuffer[3][j] += sumBuffer[j];												//Ls
		outputBuffer[4][j] += sumBuffer[j];												//Rs

	}

	return;
}

void __fg_call Module_postkickFunction()
{
	// Initialize 1ms delay for Ls and Rs channels
	DelayLine_init(&delayLine, DELAY_SAMPLES);

	//Initialize inverter with total inversion and no increase in gain
	audio_invert_init(&inverter, TOTAL_INVERSION, NO_INVERSION_GAIN);
}

void __fg_call Module_brickFunction()
{
	if (Module_mcv.enable == 1) {
		std::cout << "Start brick Module" << std::endl;

		int mask = __HAOS_PPM_VALID_CHANNELS;
		int maskOut = getChannels(Module_mcv.outputCh);
		int channel = 0;

		while (mask != 0) {
			if (mask & 1) {
				for (int sample = 0; sample < BLOCK_SIZE; sample++) {
					sampleBufferInput[channel][sample] = __HAOS_IOBUFFER_PTRS[channel][sample];
				}
			}
			mask >>= 1;
			channel++;
		}

		processBlock(sampleBufferInput, sampleBufferOutput);

		__HAOS_PPM_VALID_CHANNELS = maskOut;
		channel = 0;
		while (maskOut != 0) {
			if (maskOut & 1) {
				for (int sample = 0; sample < BLOCK_SIZE; sample++) {
					__HAOS_IOBUFFER_PTRS[channel][sample] = sampleBufferOutput[channel][sample];
				}
			}
			maskOut >>= 1;
			channel++;
		}
		std::cout << "End brick Module" << std::endl;
	}

}
