/**
 * @file
 * @brief DSP OS support.
 *
 * Home Audio System Kernel V0.0.1
 *
 *
 */
#ifndef __HAOS_H__
#define __HAOS_H__

#include "stdint.h"

#define __fg_primitive_call
#define __bg_primitive_call
#define __fg_call
#define __bg_call

// 	if defined __HAOS_RUN_EOF:  run simulation using EOF value from input stream
//	else: run simulation using while loop
// #define __HAOS_RUN_EOF

// if defined __BITRIPPER: use bitripper in decoder
// else: read directly from file in decoder

#define BRICK_SIZE 16
#define IO_BUFFER_SIZE_PER_CHAN 64
#define IO_BUFFER_PER_CHAN_MODULO 4
#define NUMBER_OF_IO_CHANNELS 16
#define FRAMES_PER_MS 256
#define MAX_NUM_CHANNELS 16
#define SAMPLE_SCALE -(double)(1 << 31)
/** Pointer to audio data in OS-provided IO Buffer.
 */
typedef float *HAOS_PcmSamplePtr_t;

/* Define the various types of framework entry points */

/**
 *  @brief a function that is called in the foreground.
 *
 *  Every mct_t function callback, except the preKickStart and
 *  background callbacks, are foreground functions.
 *
 *  @param (none)
 *
 *  @ingroup module_api
 */
typedef void __fg_call HAOS_ForegroundCallback_t(void);

/**
 *  @brief A pre kickstart function gets called before the KickStart message is received and acted-on.
 *
 *  @param mif_ptr Pointer to the module interface.
 *
 *  @ingroup module_api
 */
typedef void __fg_call HAOS_PrekickCallback_t(void* mif);

/**
 *  @brief a function that is called in the background
 *
 *  A background function is used to perform interruptible, lower priority processing.
 *  Called from an infinite-loop in the main body of the OS code.
 *
 *  @param (none)
 *
 *  @ingroup module_api
 */
typedef void __bg_call HAOS_BackgroundCallback_t(void);

/**
 *  @brief The Module Call Table (MCT) is a table of framework entry points.
 *
 *  This table contains 8 optional entry points that may be called by
 *  the OS Framework.
 *
 *  @param framework_prekick_entry_point_t* <b>prekick_func_ptr</b>
 *  Unconditional initializations.
 *  Initialize vars whose values don't depend on MCV.
 *  This function gets called before the KickStart message is received and acted-on.
 *  Called only when the OS re-boots.
 *
 *  @param framework_entry_point_t* <b>postkick_func_ptr</b>
 *  Unconditional initializations OR conditional initializations.
 *  This function gets called immediately after the KickStart message is received and acted-on (i.e., prior to any other Framework Entry-point).
 *  This function also gets called upon App-Restart (which happens after recovering from an audio under-flow).
 *  This function gets called before any block-processing functions are called.
 *  An example of the kind of initialization that must happen here would be any filter state that must be cleared
 *  prior to starting audio again.
 *
 *  @param framework_entry_point_t* <b>timer_func_ptr</b>
 *   Foreground function called less frequently than frame or block.
 *   Called from the Block ISR (a.k.a., Brick ISR or Foreground ISR)
 *   Called only when the Timer flag is set (gets set by the Timer ISR)
 *   Use this to act on MCV changes when the block and frame functions are not being called
 *
 *  @param framework_entry_point_t* <b>frame_func_ptr</b>
 *   called on input frame boundaries..
 *   Called from the Block ISR (a.k.a., Brick ISR or Foreground ISR).
 *   This function gets called on frame boundaries (integer multiples of sample-blocks)
 *     -#:  AAC Frame-size is 1024 samples.
 *     -#:  Our PCM frame-size was chosen to be 256 samples.
 *   Use this as another place to act on MCV changes.  This is called more often than
 *   the Timer functions, but is only called when the DAO clocks are active (only an issue
 *   with Decoder chips).
 *
 *  @param framework_entry_point_t* <b>block_func_ptr</b>
 *   Called once for each brick (16 samples) of PCM input.
 *   Called from the Block ISR (a.k.a., Brick ISR or Foreground ISR) whenever there is one or more
 *   blocks of unprocessed PCM in the IO Buffer.
 *   This function can process only one block of any channel, but all channels are available.
 *   This function is inherently an in-place processor.
 *   There is an array of I/O buffer pointers, located at YMEM addresses 0x60 - 0x6F.
 *   These pointers will point at block N for all channels.
 *
 *  @param 	void* <b>reserved1</b>
 *
 *  @param bg_framework_entry_point_t* <b>background_func_ptr</b>
 *  Perform interruptible, lower priority processing.
 *  Called from an infinite-loop in the main body of the OS code.
 *  Can be interrupted, so exercise caution when using global vars that are used in the foreground ISR.
 *
 *  @param framework_entry_point_t* <b>postmalloc_func_ptr</b>
 *  Called after successful malloc by all modules in system.
 *  Called from the Block ISR (a.k.a., Brick ISR or Foreground ISR).
 *  Called if the mallocations requested by the modules' Pre-Mallocs succeeds.
 *  This gives the module designer a chance to act upon the newly acquired buffers
 *  if so desired.  For example, a malloced FIR-filter history-buffer should be
 *  cleared here.
 *  This function is also a good place to set a malloc-success flag for the benefit
 *  of the Block function.  That is, the Block function should never try to operate on
 *  a buffer that has not yet been malloc'ed.
 *
 *  @param framework_entry_point_t* <b>premalloc_func_ptr</b>
 *   Request memory from the heap.
 *   Called from the Block ISR (a.k.a., Brick ISR or Foreground ISR).
 *   Called immediately after the frame functions are called and immediately prior to the PostMalloc functions.
 *   This function is only called if one or more modules set the X_VX_NextFrm_Reinit_Req flag (in the Frame function).
 *   When this happens, all modules' PreMalloc functions get called so that each module has the chance
 *   to ask for heap memory.
 *   If this function gets called, the OS has already freed all of the heap, and all heap-using modules
 *   MUST request heap again.
 *   There are 6 types of malloc requests: X, Y, L modulo and non-modulo.
 *   Mallocation is all or nothing.  If there is not enough available heap to accommodate
 *   the entire set of requests, the systems halts.
 *   If the Malloc succeeds, the OS calls the Post Malloc functions.
 *
 *  @ingroup module_api
 */
typedef struct {
	HAOS_PrekickCallback_t*		Prekick;
	HAOS_ForegroundCallback_t* 	Postkick;
	HAOS_ForegroundCallback_t* 	Timer;
	HAOS_ForegroundCallback_t* 	Frame;
	HAOS_ForegroundCallback_t* 	Brick;
	HAOS_ForegroundCallback_t* 	AFAP;
	HAOS_BackgroundCallback_t* 	Background;
	HAOS_ForegroundCallback_t* 	Postmalloc;
	HAOS_ForegroundCallback_t* 	Premalloc;
} HAOS_Mct_t;

/**
 *  @brief Structure pointed to by the ODT, which points to the MCV and MCT.
 *
 *  @ingroup module_api
 */
typedef struct
{
	void* 		MCV;
	HAOS_Mct_t* MCT;
	void* reserved1;
	void* reserved2;
	void* reserved3;
	void* reserved4;
	void* reserved5;
	void* reserved6;
} HAOS_Mif_t;

/** @brief one row in the ODT.
 *
 *  @param __memY <b>mif_t* mifptr</b>
 *   Pointer to the MIF for one module.
 *
 *  @param unsigned <b>int moduleID</b>
 *   Module ID for one module.
 *
 *  @ingroup module_api
*/
typedef struct
{
	HAOS_Mif_t* MIF;
	uint32_t 	moduleID;
} HAOS_OdtEntry_t;

/** @typedef odt_entry_t odt_t[];
 *
 *  @brief the Overlay Definition Table (ODT) or module table.
 *
 *  A table of up to 16 odt_entry_t rows, in long (XY) memory.
 *  The table is terminated by a zero entry.
 */
typedef HAOS_OdtEntry_t HAOS_Odt_t[];

typedef struct
{
	int inputChannelMask;
	int outputChannelMask;
	int sampleRate;
	int decodeInfo;
	// int* downmixCoefsPtr;
	// int delayValues[16];
	// int delayCtrl;
} HAOS_FrameData_t;

typedef struct
{
	HAOS_FrameData_t* frameData;
	float* IOBufferPtrs[NUMBER_OF_IO_CHANNELS];
} HAOS_CopyToIOPtrs_t;

void HAOS_copyBrickToIO(HAOS_CopyToIOPtrs_t* copyToIOPtrs);

extern HAOS_PcmSamplePtr_t __HAOS_IOBUFFER_PTRS[NUMBER_OF_IO_CHANNELS];
extern HAOS_PcmSamplePtr_t __HAOS_IOBUFFER_INP_PTRS[NUMBER_OF_IO_CHANNELS];
extern const HAOS_PcmSamplePtr_t __HAOS_TEMPBUFFER_PTRS[NUMBER_OF_IO_CHANNELS];

/** Bit map used to determine what channels are valid coming in current processing block.
 * Change this map if you add or remove channels (for the benefit of down-stream modules)
 * @ingroup os_interface
 */

extern int __HAOS_PPM_VALID_CHANNELS;
extern bool __HAOS_AT_EOF;
extern int __HAOS_IO_FREE;

#endif /* __HAOS_H__ */
