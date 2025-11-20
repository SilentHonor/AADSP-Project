/*
 * bitripper_sim.cpp
 *
 *  Created on: Aug 12, 2024
 *      Author: katarinac
 */

#include <iostream>
#include "math.h"
#include "bitripper_sim.h"

int* HAOS_fillInputFIFO0(int* writePtr);

int64_t extractMask[MAX_BITS]=
{
	0x100000000,
	0x300000000,
	0x700000000,
	0xf00000000,
	0x1f00000000,
	0x3f00000000,
	0x7f00000000,
	0xff00000000,
	0x1ff00000000,
	0x3ff00000000,
	0x7ff00000000,
	0xfff00000000,
	0x1fff00000000,
	0x3fff00000000,
	0x7fff00000000,
	0xffff00000000,
	0x1ffff00000000,
	0x3ffff00000000,
	0x7ffff00000000,
	0xfffff00000000,
	0x1fffff00000000,
	0x3fffff00000000,
	0x7fffff00000000,
	0xffffff00000000,
	0x1ffffff00000000,
	0x3ffffff00000000,
	0x7ffffff00000000,
	0xfffffff00000000,
	0x1fffffff00000000,
	0x3fffffff00000000,
	0x7fffffff00000000,
	0xffffffff00000000
};

BitRipper_t currStateStruct = {0};
BitRipper_t auxStateStruct = {0};
BitRipper_t mainStructBackup = {0};
bool inAuxState;
int alignmentInfo;				// the value if BitsRemaining at alignment
int overflowCntFIFO0;
int maxBits = 32;
void BitRipper_init(BitRipper_t& currStateStruct)
{
	currStateStruct.currentWord = 0;
	currStateStruct.bitsRemaining = 0;
	currStateStruct.inputReadPtr = inputFIFO0;
	currStateStruct.inputEndAddrP1 = inputFIFO0 + sizeof(inputFIFO0) + 1;
	currStateStruct.inputBaseAddr =  inputFIFO0;
	currStateStruct.reserved[0] = 0;
	currStateStruct.reserved[1] = 0;
	currStateStruct.writePtr = inputFIFO0;
 	currStateStruct.writePtr = HAOS_fillInputFIFO0(currStateStruct.writePtr);

	inAuxState = false;
	overflowCntFIFO0 = 0;
	alignmentInfo = 0;
}

int BitRipper_extractBits(int bitsNeeded)
{
	int left2extract = 0;
	int extractedBits = 0;
	int64_t extractedBits64 = 0;
	int64_t maskExtracted64 = 0;
	int currWord = 0;

	if (currStateStruct.bitsRemaining == 0)
	{
		while (currStateStruct.inputReadPtr == currStateStruct.writePtr)
		{
			BitRipper_init(currStateStruct);
		}
		// load new current word
		currStateStruct.currentWord = *(currStateStruct.inputReadPtr);
		currStateStruct.bitsRemaining = maxBits;
		currStateStruct.inputReadPtr++;
		if (currStateStruct.inputReadPtr == currStateStruct.inputEndAddrP1)
		{
			currStateStruct.inputReadPtr = currStateStruct.inputBaseAddr;
		}
	}

	left2extract = bitsNeeded - currStateStruct.bitsRemaining;		// number of bits extracted from current word
	currWord = currStateStruct.currentWord;
	extractedBits64 = (int64_t) (currWord << bitsNeeded);
	currWord = currWord << bitsNeeded;

	if(left2extract > 0)
	{
		// Check whether there is data
		while (currStateStruct.inputReadPtr == currStateStruct.writePtr)
		{
			BitRipper_init(currStateStruct);
		}
		// load new current word
		currStateStruct.currentWord = *(currStateStruct.inputReadPtr);
		currWord = currStateStruct.currentWord;
		currStateStruct.bitsRemaining = maxBits;
		currStateStruct.inputReadPtr++;
		if (currStateStruct.inputReadPtr == currStateStruct.inputEndAddrP1)
		{
			currStateStruct.inputReadPtr = currStateStruct.inputBaseAddr;
		}

		// extract the remaining bits from current word
		currStateStruct.bitsRemaining -= left2extract;
		maskExtracted64 = (int64_t) (currWord << left2extract);
		currWord = currWord << left2extract;
		maskExtracted64 = maskExtracted64 & extractMask[left2extract - 1];
		extractedBits64 = extractedBits64 | maskExtracted64;
	}
	else
	{
		currStateStruct.bitsRemaining = -left2extract;
	}

	// more than enough bits or exactly the right number of bits
	currStateStruct.currentWord = currWord;

	extractedBits = (int)extractedBits64;

	return extractedBits;
}

void BitRipper_saveMainState()
{
	mainStructBackup.currentWord = currStateStruct.currentWord;
	mainStructBackup.bitsRemaining = currStateStruct.bitsRemaining;
	mainStructBackup.inputReadPtr = currStateStruct.inputReadPtr;
	mainStructBackup.inputEndAddrP1 = currStateStruct.inputEndAddrP1;
	mainStructBackup.inputBaseAddr = currStateStruct.inputBaseAddr;
	mainStructBackup.writePtr = currStateStruct.writePtr;

	inAuxState =  true;
}

void BitRipper_restoreMainState()
{
	currStateStruct.currentWord = mainStructBackup.currentWord;
	currStateStruct.bitsRemaining = mainStructBackup.bitsRemaining;
	currStateStruct.inputReadPtr = mainStructBackup.inputReadPtr;
	currStateStruct.inputEndAddrP1 = mainStructBackup.inputEndAddrP1;
	currStateStruct.inputBaseAddr = mainStructBackup.inputBaseAddr;
	currStateStruct.writePtr = mainStructBackup.writePtr;

	inAuxState =  false;
}

void BitRipper_loadMainState()
{
	currStateStruct.currentWord = auxStateStruct.currentWord;
	currStateStruct.bitsRemaining = auxStateStruct.bitsRemaining;
	currStateStruct.inputReadPtr = auxStateStruct.inputReadPtr;
	currStateStruct.inputEndAddrP1 = auxStateStruct.inputEndAddrP1;
	currStateStruct.inputBaseAddr = auxStateStruct.inputBaseAddr;
	currStateStruct.writePtr = auxStateStruct.writePtr;

	inAuxState =  false;
}

void BitRipper_saveAuxState()
{
	auxStateStruct.currentWord = currStateStruct.currentWord;
	auxStateStruct.bitsRemaining = currStateStruct.bitsRemaining;
	auxStateStruct.inputReadPtr = currStateStruct.inputReadPtr;
	auxStateStruct.inputEndAddrP1 = currStateStruct.inputEndAddrP1;
	auxStateStruct.inputBaseAddr = currStateStruct.inputBaseAddr;
	auxStateStruct.writePtr = currStateStruct.writePtr;
}

void BitRipper_loadAuxState()
{
	// do not copy the write pointer
	currStateStruct.currentWord = auxStateStruct.currentWord;
	currStateStruct.bitsRemaining = auxStateStruct.bitsRemaining;
	currStateStruct.inputReadPtr = auxStateStruct.inputReadPtr;
	currStateStruct.inputEndAddrP1 = auxStateStruct.inputEndAddrP1;
	currStateStruct.inputBaseAddr = auxStateStruct.inputBaseAddr;
}

int BitRipper_bitCntStates(BitRipper_t& currStateStruct, int* inputFIFO0)
{
	int dipstickBits = 0;
	int inputFIFOsize = 0;

	maxBits -= currStateStruct.bitsRemaining;
	maxBits += auxStateStruct.bitsRemaining;

	dipstickBits = currStateStruct.inputReadPtr - auxStateStruct.inputReadPtr;
	if (dipstickBits < 0)
	{
		inputFIFOsize = currStateStruct.inputEndAddrP1 - currStateStruct.inputBaseAddr;
		dipstickBits = dipstickBits + inputFIFOsize;
	}
	dipstickBits = dipstickBits - 1;
	dipstickBits = dipstickBits << 4;
	dipstickBits = dipstickBits << 1;
	dipstickBits = dipstickBits + maxBits;

	return dipstickBits;
}

int BitRipper_peek(int bitsNeeded)
{
	int left2peek = 0;
	int peekedBits = 0;
	int64_t peekedBits64 = 0;
	int64_t maskPeeked64 = 0;

	int currWord = currStateStruct.currentWord;
	int bitsRemaining = currStateStruct.bitsRemaining;
	int* inputReadPtr = currStateStruct.inputReadPtr;

	if (bitsRemaining == 0)
	{
		while (currStateStruct.inputReadPtr == currStateStruct.writePtr)
		{
			currStateStruct.writePtr = HAOS_fillInputFIFO0(currStateStruct.writePtr);
		}
		// load new current word
		currWord = *(inputReadPtr);
		bitsRemaining = maxBits;
		inputReadPtr++;
		if (inputReadPtr == currStateStruct.inputEndAddrP1)
		{
			inputReadPtr = currStateStruct.inputBaseAddr;
		}
	}

	left2peek = bitsNeeded - bitsRemaining;					// number of bits extracted from current word
	peekedBits64 = currWord << bitsNeeded;
	peekedBits64 = peekedBits64 & extractMask[bitsNeeded - 1];

	if(left2peek > 0)
	{
		// Check whether there is data
		while (currStateStruct.inputReadPtr == currStateStruct.writePtr)
		{
			currStateStruct.writePtr = HAOS_fillInputFIFO0(currStateStruct.writePtr);
		}
		// load new current word
		currWord = *(currStateStruct.inputReadPtr);
		bitsRemaining = maxBits;
		inputReadPtr++;
		if (inputReadPtr == currStateStruct.inputEndAddrP1)
		{
			inputReadPtr = currStateStruct.inputBaseAddr;
		}

		// peek the remaining bits from current word
		maskPeeked64 = currWord << left2peek;
		maskPeeked64 = maskPeeked64 & extractMask[left2peek - 1];
		peekedBits64 = peekedBits64 | maskPeeked64;
	}

	// more than enough bits or exactly the right number of bits
	peekedBits64 = peekedBits64 >> 32;

	peekedBits = peekedBits64;

	return peekedBits;
}

void BitRipper_skipBits(int bitsNum)
{
	int currWord = 0;
	int skipBits = 0;
	int bitsDiff = 0;

	if (currStateStruct.bitsRemaining == 0)
	{
		while (currStateStruct.inputReadPtr == currStateStruct.writePtr)
		{
			currStateStruct.writePtr = HAOS_fillInputFIFO0(currStateStruct.writePtr);
		}
		// load new current word
		currStateStruct.currentWord = *(currStateStruct.inputReadPtr);
		currStateStruct.bitsRemaining = maxBits;
		currStateStruct.inputReadPtr++;
		if (currStateStruct.inputReadPtr == currStateStruct.inputEndAddrP1)
		{
			//wrap around read pointer
			currStateStruct.inputReadPtr = currStateStruct.inputBaseAddr;
		}
	}

	bitsDiff = currStateStruct.bitsRemaining - bitsNum;

	if (bitsDiff < 0)
	{
		int bitsLeft = -bitsDiff; //get absolute value of bitsDiff number
		//check whether there is data
		while (currStateStruct.inputReadPtr == currStateStruct.writePtr)
		{
			currStateStruct.writePtr = HAOS_fillInputFIFO0(currStateStruct.writePtr);
		}
		// load new current word and skip adequate number of bits
		currStateStruct.currentWord = *(currStateStruct.inputReadPtr);
		currWord = currStateStruct.currentWord;
		currWord = currWord << bitsLeft;
		currStateStruct.bitsRemaining = maxBits - bitsLeft; // + bcs bitsDiff is negative value
	}
	else if (bitsDiff > 0)
	{
		currWord = currStateStruct.currentWord;
		currWord = currWord << bitsDiff;
		currStateStruct.currentWord = currWord;
		currStateStruct.bitsRemaining = maxBits - bitsDiff;
	}
	else
	{
		currStateStruct.currentWord = *(currStateStruct.inputReadPtr);
		currStateStruct.inputReadPtr++;
	}
}

void BitRipper_saveAlignment(int bitsOffset)
{
	alignmentInfo = currStateStruct.bitsRemaining - bitsOffset;

	if (alignmentInfo < 0)
	{
		alignmentInfo = -alignmentInfo;
	}

	alignmentInfo = alignmentInfo & 0x1f;
}

int BitRipper_alignCommon()
{
	int maxBits = 32;
	maxBits +=  currStateStruct.bitsRemaining;
	maxBits -= alignmentInfo;

	return maxBits;
}

void BitRipper_align2byte()
{
	int common = 0;
	int skipBits = 7;		// to do mod 8

	common = BitRipper_alignCommon();
	skipBits = skipBits & common;
	// call skipBits
	BitRipper_skipBits(skipBits);

}

void BitRipper_align2word()
{
	int common = 0;
	int skipBits = 0xf;		// to do mod 16

	common = BitRipper_alignCommon();
	skipBits = skipBits & common;
	// call SkipBits
	BitRipper_skipBits(skipBits);
}

void BitRipper_align2DWord()
{
	int common = 0;
	int skipBits = 0x1f;		// to do mod 32

	common = BitRipper_alignCommon();
	skipBits = skipBits & common;
	// call SkipBits
	BitRipper_skipBits(skipBits);
}

int BitRipper_readDipstick()
{
	int dipstickBits = 0;
	int writePtr;

	if(inAuxState)
	{
		currStateStruct.inputReadPtr = mainStructBackup.inputReadPtr;
		currStateStruct.bitsRemaining = mainStructBackup.bitsRemaining;
	}
	// read dipstick cont
	if (currStateStruct.inputReadPtr > currStateStruct.writePtr)
	{
		currStateStruct.writePtr += sizeof(inputFIFO0);
	}

	writePtr = *currStateStruct.writePtr;

	dipstickBits = writePtr << 4;
	dipstickBits = writePtr << 1;
	dipstickBits += currStateStruct.bitsRemaining;

	return dipstickBits;
}
