#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    char vbeSignature[4];     // == "VESA"
    uint16_t vbeVersion;      // == 0x0300 for VBE 3.0
    uint16_t oemStringPtr[2]; // isa vbeFarPtr
    uint8_t capabilities[4];
    uint32_t videoModePtr; // isa vbeFarPtr
    uint16_t totalMemory;  // as # of 64KB blocks
    uint8_t _Reserved[236 + 256];
} __attribute__((packed)) VbeInfoBlock;

typedef struct {
    uint16_t attributes;  // deprecated, only bit 7 should be of interest to you, and it indicates the mode supports a linear frame buffer.
    uint8_t window_a;     // deprecated
    uint8_t window_b;     // deprecated
    uint16_t granularity; // deprecated; used while calculating bank numbers
    uint16_t windowSize;
    uint16_t segment_a;
    uint16_t segment_b;
    uint32_t windowFunctionPtr; // deprecated; used to switch banks from protected mode without returning to real mode
    uint16_t pitch;             // number of bytes per horizontal line
    uint16_t width;             // width in pixels
    uint16_t height;            // height in pixels
    uint8_t wChar;              // unused...
    uint8_t yChar;              // ...
    uint8_t planes;
    uint8_t bitsPerPixel; // bits per pixel in this mode
    uint8_t banks;        // deprecated; total number of banks in this mode
    uint8_t memoryModel;
    uint8_t bankSize; // deprecated; size of a bank, almost always 64 KB but may be 16 KB...
    uint8_t imagePages;
    uint8_t _reserved;

    uint8_t redMask;
    uint8_t redPosition;
    uint8_t greenMask;
    uint8_t greenPosition;
    uint8_t blueMask;
    uint8_t bluePosition;
    uint8_t reservedMask;
    uint8_t reservedPosition;
    uint8_t directColorAttributes;

    uint32_t framebuffer; // physical address of the linear frame buffer; write here to draw to the screen
    uint32_t offScreenMemoryOffset;
    uint16_t offScreenMemorySize; // size of memory in the framebuffer but not being displayed on the screen
    uint8_t __reserved[206];
} __attribute__((packed)) VbeModeInfo;

bool VBE_GetControllerInfo(VbeInfoBlock *infoOutput);
bool VBE_GetModeInfo(uint16_t mode, VbeModeInfo *infoOutput);
bool VBE_SetVideoMode(uint16_t mode);