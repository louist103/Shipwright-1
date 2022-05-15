#ifndef EXTRACT_BASEROM_H
#define EXTRACT_BASEROM_H

#include <atomic>
#include <mutex>
#include <condition_variable>


#define NTSC_10 0xEC7011B7
#define NTSC_11 0xD43DA81F
#define NTSC_12 0x693BA2AE
#define PAL_10 0xB044B569
#define PAL_11 0xB2055FBD
#define NTSC_JP_GC_CE 0xF7F52DB8
#define NTSC_JP_GC 0xF611F4BA
#define NTSC_US_GC 0xF3DD35BA
#define PAL_GC 0x09465AC3
#define NTSC_JP_MQ 0xF43B45BA
#define NTSC_US_MQ 0xF034001A
#define PAL_MQ 0x1D4136F3
#define PAL_GC_DBG1 0x871E1C92 // 03-21-2002 build
#define PAL_GC_DBG2 0x87121EFE // 03-13-2002 build
#define PAL_GC_MQ_DBG 0x917D18F6
#define IQUE_TW 0x3D81FB3E
#define IQUE_CN 0xB1E1E07B

typedef enum ExtractorState {
	ExtractorWaiting,
	ExtractorExtracting,
	ExtractorDone,
}ExtractorState;

extern std::mutex gMutex;

extern int gWaitForImGuiImput;
extern int gWaitForImGuiReason;
extern int gImGuiResponse;
extern char currentFile[35];

extern volatile ExtractorState gExtractorState;

int extract(const char* fileName);
uint32_t getVersion(FILE* rom, FILE** fileList);


#endif
