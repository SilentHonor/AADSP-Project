#pragma warning(disable : 4996)
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
#include "WAVheader.h"
#include "config.h"
#include "delay.h"
#include "inverter.h"

#define BLOCK_SIZE 16
#define MAX_NUM_CHANNEL 24
#define OUTPUT_NUM_CHANNEL 6

// Global buffers
double sampleBufferInput[MAX_NUM_CHANNEL][BLOCK_SIZE];
double sampleBufferOutput[OUTPUT_NUM_CHANNEL][BLOCK_SIZE];
double tempBuffer[OUTPUT_NUM_CHANNEL][BLOCK_SIZE];
double sumBuffer[BLOCK_SIZE];

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



void X86_parseCmdLine(int argc, char* argv[], Config* config){

	//Only input & Output file paths were provided
	if (argc == 3) { 
		config->enable = DEFAULT_ENABLE;
		config->inputGain = DEFAULT_INPUT_GAIN;
		config->headroomGain = DEFAULT_HEADROOM_GAIN;
		strcpy(config->outputCh, DEFAULT_CHANNELS);
		return;
	}

	//Input, Output & enable parameters provided
	else if (argc == 4) {
		if (strcmp(argv[3], "On") == 0) {
			config->enable = DEFAULT_ENABLE;
			config->inputGain = DEFAULT_INPUT_GAIN;
			config->headroomGain = DEFAULT_HEADROOM_GAIN;
			strcpy(config->outputCh, DEFAULT_CHANNELS);
			return;
		}
		else if (strcmp(argv[3], "Off") == 0)
		{
			config->enable = 0;
			config->inputGain = 0;
			config->headroomGain = 0;
			strcpy(config->outputCh, DEFAULT_CHANNELS);
			return;
		}
	}

	//All paramaters provided
	else if (argc == 7) {
		// Parse enable (On/Off)
		if (strcmp(argv[3], "On") == 0) {
			config->enable = DEFAULT_ENABLE;

			// Parse gains
			config->inputGain = atof(argv[4]);
			if (config->inputGain > 0) {
				printf("Error: Input Gain must be negative value.\n");
				exit(1);
			}
			config->headroomGain = atof(argv[5]);
			if (config->headroomGain > 0) {
				printf("Error: Headroom Gain must be negative value.\n");
				exit(1);
			}

			//parsing output channels
			strncpy(config->outputCh, argv[6], sizeof(config->outputCh) - 1);
			config->outputCh[sizeof(config->outputCh) - 1] = '\0';
			return;
		}
		else if (strcmp(argv[3], "Off") == 0) {
			config->enable = 0;
			config->inputGain = 0;
			config->headroomGain = 0;
			strcpy(config->outputCh, DEFAULT_CHANNELS);
			return;
		}
		
	}

	printf("Invalid command line arguments structure!\n");
	printf("Valid structure: <INPUT_FILE_PATH> <OUTPUT_FILE_PATH> [On/Off](Enable parameter) [0, -inf](Input gain) [0, -inf](Headroom gain) <Output_mode>");
	exit(1);
	
}

int main(int argc, char* argv[])
{
	FILE* wav_in = NULL;
	FILE* wav_out = NULL;
	char WavInputName[256];
	char WavOutputName[256];
	WAV_HEADER inputWAVhdr;
	WAV_HEADER outputWAVhdr;
	
	DelayLine delayLine;
	inverter_data_t inverter;
	Config config;

	X86_parseCmdLine(argc, argv, &config);

	// Initialize 1ms delay for Ls and Rs channels
	DelayLine_init(&delayLine, DELAY_SAMPLES);

	//Initialize inverter with total inversion and no increase in gain
	audio_invert_init(&inverter, TOTAL_INVERSION, NO_INVERSION_GAIN);

	// Init channel buffers
	for (int i = 0; i < MAX_NUM_CHANNEL; i++)
	{
		memset(&sampleBufferInput[i], 0, BLOCK_SIZE * sizeof(double));
	}

	// Open input and output wav files
	strcpy(WavInputName, argv[1]);
	wav_in = OpenWavFileForRead(WavInputName, "rb");
	strcpy(WavOutputName, argv[2]);
	wav_out = OpenWavFileForRead(WavOutputName, "wb");

	// Read input wav header
	ReadWavHeader(wav_in, inputWAVhdr);

	// Set up output WAV header
	outputWAVhdr = inputWAVhdr;
	outputWAVhdr.fmt.NumChannels = OUTPUT_NUM_CHANNEL;

	WriteWavHeader(wav_out, outputWAVhdr);

	int sample;
	int BytesPerSample = inputWAVhdr.fmt.BitsPerSample / 8;
	const double SAMPLE_SCALE = -(double)(1 << 31) - 1.0;
	int iNumSamples = inputWAVhdr.data.SubChunk2Size / (inputWAVhdr.fmt.NumChannels * inputWAVhdr.fmt.BitsPerSample / 8);

	// Processing loop
	for (int i = 0; i < iNumSamples / BLOCK_SIZE; i++)
	{
		for (int j = 0; j < BLOCK_SIZE; j++)
		{
			for (int k = 0; k < inputWAVhdr.fmt.NumChannels; k++)
			{
				sample = 0; //debug
				fread(&sample, BytesPerSample, 1, wav_in);
				sample = sample << (32 - inputWAVhdr.fmt.BitsPerSample);		// force signextend
				sampleBufferInput[k][j] = sample / SAMPLE_SCALE;				// scale sample to 1.0/-1.0 range	
			}
		}

		if (config.enable)
		{
			for (int j = 0; j < BLOCK_SIZE; j++)
			{
				
				sampleBufferInput[0][j] *= gain(config.inputGain);
				sampleBufferInput[1][j] *= gain(config.inputGain);

				sumBuffer[j] = sampleBufferInput[0][j] + sampleBufferInput[1][j];
				sumBuffer[j] *= gain(config.headroomGain);

				Delayline_processBlock(&delayLine, &sampleBufferInput[0][j], &tempBuffer[3][j], 1);
				Delayline_processBlock(&delayLine, &sampleBufferInput[1][j], &tempBuffer[4][j], 1);


				tempBuffer[5][j] = sumBuffer[j];											//LFE
				gst_audio_invert_transform(&inverter, &sumBuffer[j], &tempBuffer[2][j], 1);	//C

				sumBuffer[j] *= gain(-6);
				tempBuffer[0][j] = sumBuffer[j];											//L
				tempBuffer[1][j] = sumBuffer[j];											//R
				

				tempBuffer[3][j] *= gain(-2);
				tempBuffer[4][j] *= gain(-2);

				tempBuffer[3][j] += sumBuffer[j];											//Ls
				tempBuffer[4][j] += sumBuffer[j];											//Rs
				


			}
			
		}
		else
		{
			for (int j = 0; j < BLOCK_SIZE; j++)
			{
				tempBuffer[0][j] = sampleBufferInput[0][j];
				tempBuffer[1][j] = sampleBufferInput[1][j];
			}
		}

		int channel = 0;
		int maskOut = getChannels(config.outputCh);

		while (maskOut != 0)
		{
			if (maskOut & 1)
			{
				for (int sample = 0; sample < BLOCK_SIZE; sample++)
				{
					sampleBufferOutput[channel][sample] = tempBuffer[channel][sample];
				}
			}
			channel++;
			maskOut >>= 1;
		}

		for (int j = 0; j < BLOCK_SIZE; j++)
		{
			for (int k = 0; k < OUTPUT_NUM_CHANNEL; k++)
			{
				sample = sampleBufferOutput[k][j] * SAMPLE_SCALE;
				sample = sample >> (32 - inputWAVhdr.fmt.BitsPerSample);
				fwrite(&sample, BytesPerSample, 1, wav_out);
			}
		}
	}

	fclose(wav_in);
	fclose(wav_out);

	return 0;
}
