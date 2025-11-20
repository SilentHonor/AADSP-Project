#pragma once

#define NO_INVERSION 0.0
#define SILENCE_INVERSION 0.5
#define TOTAL_INVERSION 1.0

#define NO_INVERSION_GAIN 1.0

typedef struct {
	double degree;
	double gain;
} inverter_data_t;

void audio_invert_init(inverter_data_t* data, double degree, double gain);
void gst_audio_invert_transform(inverter_data_t* data, double* input, double* output, unsigned int num_samples);


