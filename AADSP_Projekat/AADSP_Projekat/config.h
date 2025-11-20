#pragma once

#define DEFAULT_ENABLE 1
#define DEFAULT_INPUT_GAIN -6.0
#define DEFAULT_HEADROOM_GAIN -3.0
#define DEFAULT_CHANNELS "2_0_0"

typedef struct
{
	int enable;				// {1 (On), 0 (Off)}
	double inputGain;		// [0, -inf]
	double headroomGain;	// [0, -inf]
	char outputCh[8];		//"2_0_0", "2_0_1" ..
} Config;

struct ModeMap
{
	const char* modeName;
	uint8_t mask;
};

ModeMap modes[] =
{
	{"2_0_0", 0b00000011},
	{"2_0_1", 0b00100011},
	{"0_2_0", 0b00011000},
	{"0_2_1", 0b00111000},
	{"3_2_0", 0b00011111},
	{"3_2_1", 0b00111111}
};

