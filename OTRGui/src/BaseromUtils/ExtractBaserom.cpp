#if defined (_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <thread>
#include <filesystem>
#ifndef _MSC_VER
#define BSWAP32 __bswap_32
#else  //Windows
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

int gExtractorWaiting = false;
int gWaitForImGuiReason = 0;
ImGuiResponse gImGuiResponse = ImGuiResponse::None;
char currentFile[35];
uint32_t gRomCrc = 0xFFFFFFFF;

std::mutex gMutex;
std::mutex gFileMutex;
std::condition_variable cv;

volatile ExtractorState gExtractorState = ExtractorState::Waiting;


uint32_t getVersion(void* rom, FILE** fileList, bool fromRam) {
    uint32_t romCRC;

    if (!fromRam) {
        fseek((FILE*)rom, 0x10, SEEK_SET);

        if (fread(&romCRC, sizeof(uint32_t), 1, (FILE*)rom) != 1) {
            fprintf(stderr, "Could not read rom CRC\n");
            exit(1);
        }
    }
    else {
        romCRC = reinterpret_cast<uint32_t*>(rom)[0x10 / 4];
    }

    romCRC = BSWAP32(romCRC);

    switch (romCRC) {
    case NTSC_10:
        puts("Detected version NTSC 1.0");
        *(fileList) = fopen("assets/extractor/filelists/filelist_ntsc_n64.txt", "r");
        dmaTableOffset = OFF_NTSC_10;
        break;
    case NTSC_11:
        puts("Detected version NTSC 1.1");
        *(fileList) = fopen("assets/extractor/filelists/filelist_ntsc_n64.txt", "r");
        dmaTableOffset = OFF_NTSC_11;
        break;
    case NTSC_12:
        puts("Detected version NTSC 1.2");
        *(fileList) = fopen("assets/extractor/filelists/filelist_ntsc_n64.txt", "r");
        dmaTableOffset = OFF_NTSC_12;
        break;
    case PAL_10:
        puts("Detected version PAL N64");
        *(fileList) = fopen("assets/extractor/filelists/filelist_pal_n64.txt", "r");
        dmaTableOffset = OFF_PAL_10;
        break;
    case PAL_11:
        puts("Detected version PAL N64");
        *(fileList) = fopen("assets/extractor/filelists/filelist_pal_n64.txt", "r");
        dmaTableOffset = OFF_PAL_11;
        break;
    case NTSC_JP_GC:
        puts("Detected version JP GameCube (MQ Disk)");
        *(fileList) = fopen("assets/extractor/filelists/filelist_gamecube+mq.txt", "r");
        dmaTableOffset = OFF_JP_GC;
        break;
    case NTSC_JP_GC_CE:
        puts("Detected version GameCube (Collectors Edition Disk)");
        *(fileList) = fopen("assets/extractor/filelists/filelist_gamecube+mq.txt", "r");
        dmaTableOffset = OFF_JP_GC_CE;
        break;
    case NTSC_JP_MQ:
        puts("Detected version JP Master Quest");
        *(fileList) = fopen("assets/extractor/filelists/filelist_gamecube+mq.txt", "r");
        dmaTableOffset = OFF_JP_MQ;
        break;
    case NTSC_US_MQ:
        puts("Detected version US Master Quest");
        *(fileList) = fopen("assets/extractor/filelists/filelist_gamecube+mq.txt", "r");
        dmaTableOffset = OFF_JP_MQ;
        break;
    case NTSC_US_GC:
        puts("Detected version US GameCube");
        *(fileList) = fopen("assets/extractor/filelists/filelist_gamecube+mq.txt", "r");
        dmaTableOffset = OFF_US_MQ;
        break;
    case PAL_GC:
        puts("Detected version PAL GameCube");
        *(fileList) = fopen("assets/extractor/filelists/filelist_pal_mq.txt", "r");
        dmaTableOffset = OFF_PAL_GC;
        break;
    case PAL_MQ:
        puts("Detected version PAL Master Quest");
        *(fileList) = fopen("assets/extractor/filelists/gamecube_pal.txt", "r");
        dmaTableOffset = OFF_PAL_MQ;
        break;
    case PAL_GC_DBG1:
        printf("Detected version GameCube Debug\n");
        *(fileList) = fopen("assets/extractor/filelists/dbg.txt", "r");
        dmaTableOffset = OFF_PAL_GC_DBG1;
        break;
    case PAL_GC_DBG2:
        printf("Detected version GameCube Debug\n");
        *(fileList) = fopen("assets/extractor/filelists/dbg.txt", "r");
        dmaTableOffset = OFF_PAL_GC_DBG2;
        break;
    case PAL_GC_MQ_DBG:
        printf("Detected version Master Quest GameCube Debug\n");
        *(fileList) = fopen("assets/extractor/filelists/dbg.txt", "r");
        dmaTableOffset = OFF_PAL_MQ_DBG;
        break;
    case IQUE_CN:
        puts("Detected version CN IQUE");
        *(fileList) = fopen("assets/extractor/filelists/filelist_ique.txt", "r");
        dmaTableOffset = OFF_CN_IQUE;
        break;
    case IQUE_TW:
        puts("Detected version TW IQUE");
        *(fileList) = fopen("assets/extractor/filelists/filelist_ique.txt", "r");
        dmaTableOffset = OFF_TW_IQUE;
        break;
    default:
        fprintf(stderr, "Unknown CRC %x given: \n", romCRC);
    }
    if (*(fileList) == nullptr) {
        fprintf(stderr, "Could not open filelist\n");
    }
    return romCRC;
}

static void createDir(const char* dirName) {
    if (!std::filesystem::create_directory(dirName)) {
        std::unique_lock<std::mutex> lock(gFileMutex);
        gExtractorWaiting = true;
        cv.wait(lock);
        switch (gImGuiResponse) {
            case ImGuiResponse::DeleteFiles:
                std::filesystem::remove_all(dirName);
                std::filesystem::create_directory(dirName);
                break;
            case ImGuiResponse::KeepFiles:
                puts("Keeping files. Exiting the thread");
                return;
        }
    }
}


int extract(FILE* rom, FILE* fileList) {
    FILE* outFile;
    uint32_t dataAddr[4];

    char folderName[17];
    sprintf(folderName, "baserom_%X", gRomCrc);

    createDir(folderName);
    if (gImGuiResponse == ImGuiResponse::KeepFiles) {
        gImGuiResponse = ImGuiResponse::None;
        return 1;
    }

    gExtractorState = ExtractorState::Extracting;

    rewind(fileList);

    for (uint32_t i = 0; fgets(currentFile, 35, fileList) != nullptr; i++) {
        //auto lock = std::lock_guard<std::mutex>(gMutex);
        char outFileName[55];
        sprintf(outFileName, "baserom_%X/", gRomCrc);
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

        if (data == nullptr) {
            fprintf(stderr, "Could not allocate memory for %s\n", outFileName);
            exit(1);
        }

        if (outFile == nullptr) {
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
    //fclose(rom);
    gExtractorState = ExtractorState::Done;
    return 0;
}

int extractRam(const void* rom, FILE* fileList) {
    FILE* outFile;
    uint32_t dataAddr[4];

    char folderName[17];
    sprintf(folderName, "baserom_%X", gRomCrc);

    createDir(folderName);
    if (gImGuiResponse == ImGuiResponse::KeepFiles) {
        gImGuiResponse = ImGuiResponse::None;
        return 1;
    }

    gExtractorState = ExtractorState::Extracting;

    rewind(fileList);

    for (uint32_t i = 0; fgets(currentFile, 35, fileList) != nullptr; i++) {
        //auto lock = std::lock_guard<std::mutex>(gMutex);
        char outFileName[55];
        sprintf(outFileName, "baserom_%X/", gRomCrc);
        void* data;
        bool isCompressed;

        memcpy(dataAddr, (uint8_t*)((uintptr_t)rom + (dmaTableOffset + (DMA_ENTRY_SIZE * i))), DMA_ENTRY_SIZE);

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

        if (data == nullptr) {
            fprintf(stderr, "Could not allocate memory for %s\n", outFileName);
            exit(1);
        }

        if (outFile == nullptr) {
            fprintf(stderr, "Could not open output file for %s\n", outFileName);
            exit(1);
        }

        if (!isCompressed) {
            memcpy(data, (uint8_t*)((uintptr_t)rom + dataAddr[virtualStart]), dataAddr[virtualEnd] - dataAddr[virtualStart]);
        }
        else {
            memcpy(data, (uint8_t*)((uintptr_t)rom + dataAddr[physicalStart]), dataAddr[physicalEnd] - dataAddr[physicalStart]);
        }

        if (!isCompressed) {
            if (fwrite(data, dataAddr[virtualEnd] - dataAddr[virtualStart], 1, outFile) != 1) {
                fprintf(stderr, "Could not write rom file %s\n", currentFile);
                exit(1);
            }
        }
        else {
            size_t size = dataAddr[virtualEnd] - dataAddr[virtualStart];
            void* decompressedData = malloc(size);
            if (decompressedData == nullptr) {
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
    gExtractorState = ExtractorState::Done;
    return 0;
}

