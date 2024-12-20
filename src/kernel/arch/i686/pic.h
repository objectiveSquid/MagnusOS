#pragma once

#include "irq.h"
#include <stdbool.h>
#include <stdint.h>

typedef uint16_t picmask_t;

#define PICMASK_PIC1(mask) (mask & 0xFF)
#define PICMASK_PIC2(mask) (mask >> 8)

#define PICMASK_ALL 0xFFFF
#define PICMASK_NONE 0x0000

typedef struct {
    // driver name
    const char *name;

    // send end of interrupt
    void (*sendEndOfInterrupt)(irq_t irq);

    // check if device exists
    bool (*probe)();

    // disable device
    void (*disable)();

    // initialize device
    void (*initialize)(uint8_t offsetPic1, uint8_t offsetPic2, bool autoEndOfInterrupt);

    // unmask interrupt
    void (*unmask)(irq_t irq);
    // mask interrupt
    void (*mask)(irq_t irq);
} PICDriver;
