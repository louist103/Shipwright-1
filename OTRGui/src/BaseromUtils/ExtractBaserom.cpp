#if defined (_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <thread>
#ifndef _MSC_VER
#include <byteswap.h>
#define BSWAP32 bswap_32
#else  //Windows
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <fileapi.h>
#define BSWAP32 _byteswap_ulong
#endif

#include <stdint.h>
#include <stdbool.h>

#include "yaz0.h"
#include "ExtractBaserom.h"

#define DMA_ENTRY_SIZE 16

typedef enum Addresses {
    /* 0 */ virtualStart,
    /* 1 */ virtualEnd,
    /* 2 */ physicalStart,
    /* 3 */ physicalEnd
} Addresses;

//Crcs in a header file

//Rom DMA table start

#define OFF_NTSC_10_RC 0x7430
#define OFF_NTSC_10 0x7430
#define OFF_NTSC_11 0x7430
#define OFF_PAL_10 0x7950
#define OFF_NTSC_12 0x7960
#define OFF_PAL_11 0x7950
#define OFF_JP_GC 0x7170
#define OFF_JP_MQ 0x7170
#define OFF_US_GC 0x7170
#define OFF_US_MQ 0x7170
#define OFF_PAL_GC_DBG1 0x12F70
#define OFF_PAL_MQ_DBG 0x12F70
#define OFF_PAL_GC_DBG2 0x12F70
#define OFF_PAL_GC 0x7170
#define OFF_PAL_MQ 0x7170
#define OFF_JP_GC_CE 007170
#define OFF_CN_IQUE 0xB7A0
#define OFF_TW_IQUE 0xB240

static uint32_t dmaTableOffset;

int gWaitForImGuiImput = false;
int gWaitForImGuiReason = 0;
int gImGuiResponse = 0;
char currentFile[35];

std::mutex gMutex;

volatile ExtractorState gExtractorState = ExtractorWaiting;


uint32_t getVersion(FILE* rom, FILE** fileList) {
    uint32_t romCRC;

    fseek(rom, 0x10, SEEK_SET);

    if (fread(&romCRC, sizeof(uint32_t), 1, rom) != 1) {
        fprintf(stderr, "Could not read rom CRC\n");
        exit(1);
    }

    romCRC = BSWAP32(romCRC);

    switch (romCRC) {
    case NTSC_10:
        puts("Detected version NTSC 1.0");
        *(fileList) = fopen("assets/filelists/filelist_ntsc_n64.txt", "r");
        dmaTableOffset = OFF_NTSC_10;
        break;
    case NTSC_11:
        puts("Detected version NTSC 1.1");
        *(fileList) = fopen("assets/filelists/filelist_ntsc_n64.txt", "r");
        dmaTableOffset = OFF_NTSC_11;
        break;
    case NTSC_12:
        puts("Detected version NTSC 1.2");
        *(fileList) = fopen("assets/filelists/filelist_ntsc_n64.txt", "r");
        dmaTableOffset = OFF_NTSC_12;
        break;
    case PAL_10:
        puts("Detected version PAL N64");
        *(fileList) = fopen("assets/filelists/filelist_pal_n64.txt", "r");
        dmaTableOffset = OFF_PAL_10;
        break;
    case PAL_11:
        puts("Detected version PAL N64");
        *(fileList) = fopen("assets/filelists/filelist_pal_n64.txt", "r");
        dmaTableOffset = OFF_PAL_11;
        break;
    case NTSC_JP_GC:
        puts("Detected version JP GameCube (MQ Disk)");
        *(fileList) = fopen("assets/filelists/filelist_gamecube+mq.txt", "r");
        dmaTableOffset = OFF_JP_GC;
        break;
    case NTSC_JP_GC_CE:
        puts("Detected version GameCube (Collectors Edition Disk)");
        *(fileList) = fopen("assets/filelists/filelist_gamecube+mq.txt", "r");
        dmaTableOffset = OFF_JP_GC_CE;
        break;
    case NTSC_JP_MQ:
        puts("Detected version JP Master Quest");
        *(fileList) = fopen("assets/filelists/filelist_gamecube+mq.txt", "r");
        dmaTableOffset = OFF_JP_MQ;
        break;
    case NTSC_US_MQ:
        puts("Detected version US Master Quest");
        *(fileList) = fopen("assets/filelists/filelist_gamecube+mq.txt", "r");
        dmaTableOffset = OFF_JP_MQ;
        break;
    case NTSC_US_GC:
        puts("Detected version US GameCube");
        *(fileList) = fopen("assets/filelists/filelist_gamecube+mq.txt", "r");
        dmaTableOffset = OFF_US_MQ;
        break;
    case PAL_GC:
        puts("Detected version PAL GameCube");
        *(fileList) = fopen("assets/filelists/filelist_gamecube+mq.txt", "r");
        dmaTableOffset = OFF_PAL_GC;
        break;
    case PAL_MQ:
        puts("Detected version PAL Master Quest");
        *(fileList) = fopen("assets/filelists/filelist_pal_mq.txt", "r");
        dmaTableOffset = OFF_PAL_MQ;
        break;
    case PAL_GC_DBG1:
        printf("Detected version GameCube Debug\n");
        *(fileList) = fopen("assets/filelists/filelist_dbg.txt", "r");
        dmaTableOffset = OFF_PAL_GC_DBG1;
        break;
    case PAL_GC_DBG2:
        printf("Detected version GameCube Debug\n");
        *(fileList) = fopen("assets/filelists/filelist_dbg.txt", "r");
        dmaTableOffset = OFF_PAL_GC_DBG2;
        break;
    case PAL_GC_MQ_DBG:
        printf("Detected version Master Quest GameCube Debug\n");
        *(fileList) = fopen("assets/filelists/filelist_dbg.txt", "r");
        dmaTableOffset = OFF_PAL_MQ_DBG;
        break;
    case IQUE_CN:
        puts("Detected version CN IQUE");
        *(fileList) = fopen("assets/filelists/filelist_ique.txt", "r");
        dmaTableOffset = OFF_CN_IQUE;
        break;
    case IQUE_TW:
        puts("Detected version TW IQUE");
        *(fileList) = fopen("assets/filelists/filelist_ique.txt", "r");
        dmaTableOffset = OFF_TW_IQUE;
        break;
    default:
        fprintf(stderr, "Unknown CRC %x given: \n", romCRC);
    }
    if (*(fileList) == NULL) {
        fprintf(stderr, "Could not open filelist\n");
    }
    return romCRC;
}


void DeleteAllFiles(const wchar_t* folderPath)
{
    wchar_t fileFound[256];
    WIN32_FIND_DATA info;
    HANDLE hp;
    swprintf(fileFound, sizeof(fileFound), L"%s\\*.*", folderPath);
    hp = FindFirstFile(fileFound, &info);
    do
    {
        swprintf(fileFound, sizeof(fileFound), L"%s\\%ws", folderPath, info.cFileName);
        DeleteFile(fileFound);

    } while (FindNextFile(hp, &info)); 
    FindClose(hp);
}

static void createDir(const wchar_t* dirName) {
    if (!CreateDirectory(dirName, NULL)) {
        DWORD lastError = GetLastError();
        switch (lastError) {
        case ERROR_ALREADY_EXISTS:
            //gWaitForImGuiReason = 
            //WAIT_FOR_IMGUI_INPUT;
            DeleteAllFiles(dirName);
            RemoveDirectory(dirName);
            if (!CreateDirectory(dirName, NULL)) {
                MessageBox(NULL, L"Tried twice and couldn't make the \'baserom\' folder.", L"Failed to make folder", MB_OK);

            }

        }
    }
}

int extract(const char* romName) {
    FILE* rom = fopen(romName, "rb");
    FILE* outFile;
    FILE* fileList = NULL;
    uint32_t dataAddr[4];

    if (rom == NULL) {
        fprintf(stderr, "Could not open rom file\n");
        exit(1);
    }
    
    getVersion(rom, &fileList);

    createDir(L"baserom");

    gExtractorState = ExtractorExtracting;

    for (uint32_t i = 0; fgets(currentFile, 35, fileList) != NULL; i++) {
        auto lock = std::lock_guard<std::mutex>(gMutex);
        char outFileName[45] = "baserom/";
        void* data;
        bool isCompressed;

        fseek(rom, dmaTableOffset + (DMA_ENTRY_SIZE * i), SEEK_SET);

        if (fread(&dataAddr, sizeof(uint32_t), 4, rom) != 4) {
            fprintf(stderr, "Could not read addresses for file %s\n", currentFile);
            exit(1);
        }

        dataAddr[virtualStart] = BSWAP32(dataAddr[virtualStart]);
        dataAddr[virtualEnd] = BSWAP32(dataAddr[virtualEnd]);
        dataAddr[physicalStart] = BSWAP32(dataAddr[physicalStart]);
        dataAddr[physicalEnd] = BSWAP32(dataAddr[physicalEnd]);

        if (dataAddr[physicalEnd] == 0) {
            data = malloc(dataAddr[virtualEnd] - dataAddr[virtualStart]);
            isCompressed = false;
        }
        else {
            data = malloc(dataAddr[physicalEnd] - dataAddr[physicalStart]);
            isCompressed = true;
        }

        strcat(outFileName, (char*)currentFile);
        outFileName[strlen(outFileName) - 1] = 0;
        outFile = fopen(outFileName, "wb");

        if (data == NULL) {
            fprintf(stderr, "Could not allocate memory for %s\n", outFileName);
            exit(1);
        }

        if (outFile == NULL) {
            fprintf(stderr, "Could not open output file for %s\n", outFileName);
            exit(1);
        }

        if (!isCompressed) {
            fseek(rom, dataAddr[virtualStart], SEEK_SET);
            if (fread(data, dataAddr[virtualEnd] - dataAddr[virtualStart], 1, rom) != 1) {
                fprintf(stderr, "Could not read rom file %s\n", currentFile);
                exit(1);
            }
        }
        else {
            fseek(rom, dataAddr[physicalStart], SEEK_SET);
            if (fread(data, dataAddr[physicalEnd] - dataAddr[physicalStart], 1, rom) != 1) {
                fprintf(stderr, "Could not read rom file %s\n", currentFile);
                exit(1);
            }
        }

        if (!isCompressed) {
            if (fwrite(data, dataAddr[virtualEnd] - dataAddr[virtualStart], 1, outFile) != 1) {
                fprintf(stderr, "Could not write rom file %s\n", currentFile);
                exit(1);
            }
        } else {
            size_t size = dataAddr[virtualEnd] - dataAddr[virtualStart];
            void* decompressedData = malloc(size);
            if (decompressedData == NULL) {
                fprintf(stderr, "could not allocate memory for decompressed data\n");
                exit(1);
            }

            yaz0_decode((const unsigned char*)data, (unsigned char*)decompressedData, size);
            
            if (fwrite(decompressedData, size, 1, outFile) != 1) {
                fprintf(stderr, "Could not write rom file %s\n", currentFile);
                free(decompressedData);
                exit(1);
            }
            free(decompressedData);
        }

        fclose(outFile);

        free(data);
    }
    fclose(rom);
    gExtractorState = ExtractorDone;
    return 0;
}
