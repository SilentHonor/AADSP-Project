#pragma once

#define __fg_call


#define BLOCK_SIZE 16
#define MAX_NUM_CHANNEL 24
#define OUTPUT_NUM_CHANNEL 6


void __fg_call Module_postkickFunction();
void __fg_call Module_brickFunction();

void processBlock(double inputBuffer[][BLOCK_SIZE], double outputBuffer[][BLOCK_SIZE]);