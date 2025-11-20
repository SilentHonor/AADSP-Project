#pragma once

#define DELAY_SAMPLES 48 //48 kHz / 1 ms = 48
#define MAX_DELAY 512

typedef struct {
	double delayBuffer[MAX_DELAY];
	int writePos;
	int delaySamples;
}DelayLine;

void DelayLine_init(DelayLine* delayLine, int delaySamples);
void Delayline_processBlock(DelayLine* delayLine, double* input, double* output, int numSamples);



