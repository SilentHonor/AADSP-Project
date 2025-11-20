#include "delay.h"


void DelayLine_init(DelayLine* delayLine, int delaySamples) {
	for (int i = 0; i < MAX_DELAY; ++i) {
		delayLine->delayBuffer[i] = 0.0;
	}
	delayLine->writePos = 0;
	delayLine->delaySamples = delaySamples;
}


void Delayline_processBlock(DelayLine* delayLine, double* input, double* output, int numSamples) {
	for (int i = 0; i < numSamples; ++i) {
		int readPos = delayLine->writePos - delayLine->delaySamples;

		if (readPos < 0) readPos += MAX_DELAY;


		double delay = delayLine->delayBuffer[readPos];

		delayLine->delayBuffer[delayLine->writePos] = input[i];

		output[i] = delay;

		delayLine->writePos++;
		if (delayLine->writePos >= MAX_DELAY) {
			delayLine->writePos = 0;
		}

	}

}




