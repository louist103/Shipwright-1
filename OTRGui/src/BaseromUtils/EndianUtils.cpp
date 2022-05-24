#include "EndianUtils.h"
#include <cstdint>
#include <cstdlib>
#include <cstring>

#ifndef _MSC_VER
#include <byteswap.h>
#define BSWAP32 bswap_32
#define BSWAP16 bswap_16
#else  //Windows
#define BSWAP32 _byteswap_ulong
#define BSWAP16 _byteswap_ushort
#endif


size_t gRomSize;
static uint8_t sFirstByte;


RomEndian GetRomEndian(FILE* romFile) {
    fseek(romFile, 0, SEEK_END);
    gRomSize = ftell(romFile);
    rewind(romFile);


    if ((gRomSize != ROM_64_MB) && (gRomSize != ROM_54_MB) && (gRomSize != ROM_32_MB)) {
        fprintf(stderr, "Invalid rom size.");
        return RomEndian::Error;
    }

    if (fread(&sFirstByte, sizeof(uint8_t), 1, romFile) != 1) {
        fprintf(stderr, "Failed to read first byte of baserom_original.z64\n");
        return RomEndian::Error;
    }
    if (sFirstByte == 0x40) { //Little Endian
        puts("Detected little endian rom");
        return RomEndian::Little;
    }
    else if (sFirstByte == 0x37) {
        puts("Detected half endian / byte-swapped rom");
        return RomEndian::Half;
    } else { //BE.  TODO get what this value is to properly handle an error
        puts("Detected big endian rom");
        return RomEndian::Big;
    }

}

void* ConvertToBigEndianRam(FILE* romFile) {
    void* romData;

    //The debug roms have some extra data "overdump" which we can just remove
    if (gRomSize == ROM_64_MB) {
        gRomSize = ROM_54_MB;
    }

    romData = (malloc(gRomSize));
    rewind(romFile);
    
    if (romData == nullptr) {
        fprintf(stderr, "Failed to allocate memory for byteswapping rom.\n");
        return romData;
    }

    switch (sFirstByte) {
        case 0x40: {
            uint32_t currentWord;

            for (size_t pos = 0; pos < (gRomSize / 4); pos++) {
                fread(&currentWord, sizeof(uint32_t), 1, romFile);
                currentWord = BSWAP32(currentWord);
                reinterpret_cast<uint32_t*>(romData)[pos] = currentWord;
            } 
            break;
        }
        case 0x37: {
            uint16_t currentShort;

            for (size_t pos = 0; pos < (gRomSize / 2); pos++) {
                fread(&currentShort, sizeof(uint16_t), 1, romFile);
                currentShort = BSWAP16(currentShort);
                reinterpret_cast<uint16_t*>(romData)[pos] = currentShort;
            }
            break;
        }
        case 0x80: {
            fread(romData, gRomSize, 1, romFile);
            break;
        }

    }

    //! Hack for the MQ debug rom.
    //! When we first got this rom the header was patched to a US regin for... reasons.
    //! This will mess up the MD5 calculation. So we need to fix it here.
    

    uint32_t crc = reinterpret_cast<uint32_t*>(romData)[0x10 / 4];
    crc = BSWAP32(crc);

    if (crc == 0x917D18F6) {
        //fseek(outFile, 0x3E, SEEK_SET);
        reinterpret_cast<uint8_t*>(romData)[0x3E] = 'P';
    }



    return romData;
    
}
