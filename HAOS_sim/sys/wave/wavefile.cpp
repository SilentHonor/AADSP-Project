/**
 * wavefile.cpp
 *
 *  Created on: Apr 10, 2024
 *      Author: pekez
 */
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include "wavefile.h"

//================================ local helper types ================================
struct RIFFHDR
{
    char chunkID[4];                    // Must be "RIFF"
    unsigned int fileSize;              // Size of the file
    char riffType[4];                   // Must be "WAVE"
};

struct FORMATHDR
{
    char           fmtID[4];           /* Must be "fmt " */
    unsigned int   fmtSize;            /* Format hdr size */
    unsigned short wFormatTag;         /* format type */
    unsigned short nChannels;          /* number of channels (i.e. mono, stereo...) */
    unsigned int   nSamplesPerSec;     /* sample rate */
    unsigned int   nAvgBytesPerSec;    /* for buffer estimation */
    unsigned short nBlockAlign;        /* block size of data */
    unsigned short wBitsPerSample;     /* number of bits per sample of mono data */
} ;

struct DATAHDR
{
    char            dataID[4];
    unsigned int    dataSize;
};

struct COMBINEDHDR
{
    RIFFHDR     riffHdr;
    FORMATHDR   formatHdr;
    DATAHDR     dataHdr;
};

#define     WAVE_FORMAT_PCM         1

struct wavefile_info {
    FILE *          fileHandle;
    unsigned short  nChannels;
    unsigned short  nSamplesPerSecond;
    unsigned int    bitsPerSample;
    int             nCurrentSample;
    FORMATHDR       formatHdr;
    int             nFrames;
};


static int read_wave_hdr(FILE *handle, FORMATHDR *waveFormat, RIFFHDR *riffHdr, DATAHDR *dataHdr);
static int mod_wave_header(const wavefile_info * info);

//============================ published functions from dsplib/wavefile.h ===============

WAVREAD_HANDLE * cl_wavread_open(char * filename)
{
  std::string fileName;
  FILE *      fileHandle;
  FORMATHDR   formatHdr;
  RIFFHDR     riffHdr;
  DATAHDR     dataHdr;
  errno_t     err;

  //
  // Read in the path and filename.
  //
  fileName = filename;

  //
  // Perform the actual function call.
  //
  err = fopen_s(&fileHandle,fileName.c_str(), "rb");
  if(err != 0)
  {
      return NULL;
  }

  if(read_wave_hdr(fileHandle, &formatHdr, &riffHdr, &dataHdr) )
  {
      fclose(fileHandle);
      return NULL;
  }

  if(fileHandle != NULL) {
    int bytesPerFrame = formatHdr.nChannels * (formatHdr.wBitsPerSample / 8);
    wavefile_info * info = new wavefile_info();
    info->fileHandle = fileHandle;
    info->nChannels = formatHdr.nChannels;
    info->bitsPerSample = formatHdr.wBitsPerSample;
    info->nCurrentSample = 0;
    info->nFrames = dataHdr.dataSize / bytesPerFrame;
    info->formatHdr = formatHdr;
    info->nSamplesPerSecond = formatHdr.nSamplesPerSec;
    return (WAVREAD_HANDLE *)(info);
  } else {
    return NULL;
  }
}

int cl_wavread_close(WAVREAD_HANDLE * handle)
{
  if (handle!=NULL) {
    wavefile_info * info = (wavefile_info *)handle;
    FILE * fileHandle = info->fileHandle;
    delete(info);
    int result = fclose(fileHandle);
    handle = NULL;
    return result;
  } else {
    return -1;
  }
}

int cl_wavread_getnchannels(WAVREAD_HANDLE *handle)
{
  if (handle!=NULL) {
    wavefile_info * info = (wavefile_info *)handle;
    return info->nChannels;
  } else {
    return -1;
  }
}

int cl_wavread_bits_per_sample(WAVREAD_HANDLE *handle)
{
  if (handle!=NULL) {
    wavefile_info * info = (wavefile_info *)handle;
    return info->bitsPerSample;
  } else {
    return -1;
  }
}

int cl_wavread_frame_rate(WAVREAD_HANDLE *handle)
{
  if (handle!=NULL) {
    wavefile_info * info = (wavefile_info *)handle;
    return info->nSamplesPerSecond;
  } else {
    return -1;
  }
}

int cl_wavread_number_of_frames(WAVREAD_HANDLE *handle)
{
  if (handle!=NULL) {
    wavefile_info * info = (wavefile_info *)handle;
    return info->nFrames;
  } else {
    return -1;
  }
}

int cl_wavread_sample_number(WAVREAD_HANDLE *handle)
{
  if (handle!=NULL) {
    wavefile_info * info = (wavefile_info *)handle;
    return info->nCurrentSample;
  } else {
    return -1;
  }
}

int cl_wavread_eof(WAVREAD_HANDLE *handle)
{
  if (handle!=NULL) {
    wavefile_info * info = (wavefile_info *)handle;
    return feof(info->fileHandle);
  } else {
    return 1;
  }
}

int cl_wavread_recvsample(WAVREAD_HANDLE *handle)
{
  if (handle==NULL) {
    return 0;
  } else {
    wavefile_info * info = (wavefile_info *)handle;
    unsigned char           ucData[4] = {0,0,0,0};
    int                     nItemsRead;
    int                     bytesPerSample;
    int retValue, sample;


    FILE *fp = info->fileHandle;
    bytesPerSample = info->bitsPerSample / 8;
    nItemsRead = fread(&sample, bytesPerSample, 1, fp);
    retValue = sample << (32 - info->bitsPerSample); // force signextend


    if(nItemsRead != 1)
    {
      return 0;
    }

    info->nCurrentSample++;
    return retValue;
  }
}

WAVWRITE_HANDLE * cl_wavwrite_open(char *filename, int wBitsPerSample, int nChannels, int nFrameRate)
{
  std::string fileName;
  FILE *      fileHandle;
  COMBINEDHDR hdr;
  int         numItems;
  wavefile_info * info;

  //
  // Check to make sure that the bits per channel is
  // 8, 16, 24, or 32.
  //
  if(wBitsPerSample != 8 && wBitsPerSample != 16 && wBitsPerSample != 24 && wBitsPerSample!=32)
  {
    return NULL;
  }

  //
  // Read in the path and filename.
  //
  fileName = filename;

  //
  // Perform the actual function call.
  //
  errno_t err = fopen_s(&fileHandle, fileName.c_str(), "wb");
  if(err != 0)
  {
    return NULL;
  }

  //
  // Create the riff header.
  //
  memcpy(hdr.riffHdr.chunkID, "RIFF", 4);
  hdr.riffHdr.fileSize = 0;
  memcpy(hdr.riffHdr.riffType, "WAVE", 4);

  // a0 - bits per sample
  // a1 - nChannels
  // b0   nFrameRate
  memcpy(hdr.formatHdr.fmtID, "fmt ", 4);
  hdr.formatHdr.fmtSize = sizeof(FORMATHDR) - sizeof(hdr.formatHdr.fmtID) - sizeof(hdr.formatHdr.fmtSize);
  hdr.formatHdr.nAvgBytesPerSec = nFrameRate * nChannels * wBitsPerSample / 8;
  hdr.formatHdr.nBlockAlign     = wBitsPerSample * nChannels / 8;
  hdr.formatHdr.nChannels       = nChannels;
  hdr.formatHdr.nSamplesPerSec  = nFrameRate;
  hdr.formatHdr.wBitsPerSample  = wBitsPerSample;
  hdr.formatHdr.wFormatTag      = WAVE_FORMAT_PCM;

  //
  // Create the data header.
  //
  memcpy(hdr.dataHdr.dataID, "data", 4);
  hdr.dataHdr.dataSize = 0;

  numItems = fwrite(&hdr, sizeof(COMBINEDHDR), 1, fileHandle);

  if(numItems == 0)
  {
    fclose(fileHandle);
    return NULL;
  }

  //
  // create an info structure for this file
  //
  info = new wavefile_info();
  if(info != NULL)
  {
    info->fileHandle = fileHandle;
    info->nChannels = nChannels;
    info->bitsPerSample = wBitsPerSample;
    info->formatHdr = hdr.formatHdr;
    info->nCurrentSample = 0;
    info->nFrames = 0;
    info->nSamplesPerSecond = hdr.formatHdr.nSamplesPerSec;
  }

  return info;
}

WAVWRITE_HANDLE * cl_wavwrite_reopen(char *filename, int samplenumber)
{
	std::string fileName;
	FILE *      fileHandle;
	FORMATHDR   formatHdr;
	RIFFHDR     riffHdr;
	DATAHDR     dataHdr;
	wavefile_info * info;

	//
	// Read in the path and filename.
	//
	fileName = filename;

	//
	// Perform the actual function call.
	//
	errno_t err = fopen_s(&fileHandle, fileName.c_str(), "rb+");
	if(err != 0)
	{
		return NULL;
	}

	//
	// Read wave header.
	if(read_wave_hdr(fileHandle, &formatHdr, &riffHdr, &dataHdr) )
	{
		fclose(fileHandle);
	    return NULL;
	}

	//
	// Go to the end of file to continue writing.
	//
	if(fseek(fileHandle, 0, SEEK_END))
	{
		fclose(fileHandle);
		return NULL;
	}

	//
	// Create an info structure for this file.
	//
	info = new wavefile_info();
	if(fileHandle != NULL && info != NULL)
	{
		int bytesPerFrame = formatHdr.nChannels * (formatHdr.wBitsPerSample / 8);
		info->fileHandle = fileHandle;
	    info->nChannels = formatHdr.nChannels;
	    info->bitsPerSample = formatHdr.wBitsPerSample;
	    info->formatHdr = formatHdr;
	    info->nCurrentSample = samplenumber;

	    if (bytesPerFrame == 0)
	    {
	    	info->nFrames = 0;
	    }
	    else
	    {
	    	info->nFrames = dataHdr.dataSize / bytesPerFrame;
	    }

	    info->nSamplesPerSecond = formatHdr.nSamplesPerSec;

	    return (WAVREAD_HANDLE *)(info);
	}

	return NULL;
}

int cl_wavwrite_close(WAVWRITE_HANDLE *handle)
{
  FILE * fileHandle;
  wavefile_info * pWaveWriteInfo;

  //
  // Look up the file handle.
  //
  if (handle==0) {
    return -1;
  }

  pWaveWriteInfo = (wavefile_info*)handle;
  fileHandle = pWaveWriteInfo->fileHandle;

  mod_wave_header(pWaveWriteInfo);
  delete(pWaveWriteInfo);
  //
  // Perform the actual fclose on the pc.
  //
  return fclose(fileHandle);
}

int cl_wavwrite_sendsample(WAVWRITE_HANDLE *handle, int sample)
{
  wavefile_info *         pWaveWriteInfo;
  FILE *                  fileHandle;
  /*unsigned char*/
  float           ucTemp[3] = { 0, 0, 0};
  int                     retValue;
  int sampleToWrite;
  //
  // Look up the file handle.
  //
  if (handle==NULL) {
    return EOF;
  }
  pWaveWriteInfo = (wavefile_info *) handle;
  fileHandle = pWaveWriteInfo->fileHandle;

  sample = sample >> (32 - pWaveWriteInfo->bitsPerSample);
  fwrite(&sample, pWaveWriteInfo->bitsPerSample / 8, 1, fileHandle);

  pWaveWriteInfo->nCurrentSample++;

  return 0;
}

int cl_wavwrite_sample_number(WAVWRITE_HANDLE *handle)
{
  if (handle!=NULL) {
    wavefile_info * info = (wavefile_info *)handle;
    return info->nCurrentSample;
  } else {
    return 0;
  }
}

int cl_wavwrite_update_wave_header(WAVWRITE_HANDLE *handle, int wBitsPerSample, int nChannels, int nFrameRate)
{
	int retCode;

	if (handle != NULL)
	{
		wavefile_info * info = (wavefile_info *)handle;
		FILE * fileHandle = info->fileHandle;

		// write number of channels
		retCode = fseek(fileHandle, 22, SEEK_SET);

		if(retCode)
		{
			return retCode;
		}

		retCode = fwrite(&nChannels, 2, 1, fileHandle);

		if(retCode != 1)
		{
			return -1;
		}

		// write number of average bytes per second
		retCode = fseek(fileHandle, 28, SEEK_SET);

		if(retCode)
		{
			return retCode;
		}

		int nAvgBytesPerSec = (nFrameRate * wBitsPerSample * nChannels) / 8;
		retCode = fwrite(&nAvgBytesPerSec, 4, 1, fileHandle);

		if(retCode != 1)
		{
			return -1;
		}

		// write block align
		retCode = fseek(fileHandle, 32, SEEK_SET);

		if(retCode)
		{
			return retCode;
		}

		int nBlockAlign = (wBitsPerSample * nChannels) / 8;
		retCode = fwrite(&nBlockAlign, 2, 1, fileHandle);

		if(retCode != 1)
		{
			return -1;
		}

		// return to the end of the file
		retCode = fseek(fileHandle, 0, SEEK_END);

		if(retCode)
		{
			return retCode;
		}

	    return 0;
	}

	return -1;
}

//====================== local helper functions ==============================
int read_wave_hdr(FILE *handle, FORMATHDR *waveFormat, RIFFHDR *riffHdr, DATAHDR *dataHdr)
{
    int i;
    int fmtExtraSize;
    if(fread(riffHdr, sizeof(RIFFHDR), 1, handle) == 0)
    {
        return -1;
    }

    if(memcmp(riffHdr->chunkID,"RIFF", 4)   &&  // "RIFF"
       memcmp(riffHdr->riffType, "WAVE", 4))
    {
        return -1;
    }

    if(fread(waveFormat, sizeof(FORMATHDR), 1, handle) == 0)
    {
        return -1;
    }

    if(memcmp(waveFormat->fmtID, "fmt ", 4) &&
      !(waveFormat->fmtSize  == 16 || waveFormat->fmtSize == 18 ) &&
        waveFormat->wFormatTag != WAVE_FORMAT_PCM )
    {
        return -1;
    }

    if( waveFormat->wBitsPerSample != 8  &&
        waveFormat->wBitsPerSample != 16 &&
        waveFormat->wBitsPerSample == 24 &&
        waveFormat->wBitsPerSample == 32)
    {
        return -1;
    }

    //
    // If the format is 18, read the extra information.
    //
    if(waveFormat->fmtSize == 18)
    {
        fread(&fmtExtraSize, 1, sizeof(unsigned short), handle);
        for(i = 0; i < fmtExtraSize; i++)
        {
            fgetc(handle);
        }

    }

    if(fread(dataHdr, sizeof(DATAHDR), 1, handle) == 0)
    {
        return -1;
    }
    if(memcmp(dataHdr->dataID, "data", 4) != 0 )
    {
        return -1;
    }

    return 0;
}

// ****************************************************************************
// mod_wave_header
// ****************************************************************************
/// Performs a seek and modifies the wave header with the correct filesize and
/// data size.
///
/// @param[in]  writeInfo
///             Instance of the wavewrite class that contains the informations
///             about the wavew file being written to.
///
/// @retval     0               Sucess
/// @retval     nonzero         Failure
///
int mod_wave_header(const wavefile_info * info)
{
    FILE * fileHandle = info->fileHandle;
    unsigned int dwDataSize;
    unsigned int dwFileSize;
    int retCode;

    //
    // Current Sample * Number of bytes per sample.
    //
    dwDataSize = info->nCurrentSample * info->bitsPerSample / 8;
    dwFileSize = dwDataSize + 36;
    retCode = fseek(fileHandle, 4, SEEK_SET);
    if(retCode)
    {
        return retCode;
    }
    retCode = fwrite(&dwFileSize, sizeof(unsigned int), 1, fileHandle);
    if(retCode != 1)
    {
        return -1;
    }

    retCode = fseek(fileHandle, 40, SEEK_SET);
    if(retCode)
    {
        return retCode;
    }
    retCode = fwrite(&dwDataSize, sizeof(unsigned int), 1, fileHandle);
    if(retCode != 1)
    {
        return -1;
    }
    return 0;
}
