#include "idt.h"
#include <stdint.h>
#include <util/binary.h>

typedef struct {
    uint16_t baseLow;
    uint16_t segmentSelector;
    uint8_t _Reserved;
    uint8_t flags;
    uint16_t baseHigh;
} __attribute__((packed)) IDTEntry;

typedef struct {
    uint16_t limit;
    IDTEntry *ptr;
} __attribute__((packed)) IDTDescriptor;

IDTEntry g_IDT[256];
IDTDescriptor g_IDTDescriptor = {sizeof(g_IDT) - 1, g_IDT};

void __attribute__((cdecl)) i686_IDT_Load(IDTDescriptor *descriptor);

void i686_IDT_SetGate(int interrupt, void *base, uint16_t segmentDescriptor, uint8_t flags) {
    g_IDT[interrupt].baseLow = ((uint32_t)base) & 0xFFFF;
    g_IDT[interrupt].segmentSelector = segmentDescriptor;
    g_IDT[interrupt]._Reserved = 0;
    g_IDT[interrupt].flags = flags;
    g_IDT[interrupt].baseHigh = ((uint32_t)base >> 16) & 0xFFFF;
}

void i686_IDT_EnableGate(int interrupt) {
    FLAG_SET(g_IDT[interrupt].flags, IDT_FLAG_PRESENT);
}

void i686_IDT_DisableGate(int interrupt) {
    FLAG_UNSET(g_IDT[interrupt].flags, IDT_FLAG_PRESENT);
}

void i686_IDT_Initialize() {
    i686_IDT_Load(&g_IDTDescriptor);
}
