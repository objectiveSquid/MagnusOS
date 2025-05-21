#include "io.h"
#include "x86.h"

#define UNUSED_PORT 0x80

// reading from an unused port seemingly uses a few hundred nanoseconds as i understand
void waitFewHundredNs(uint8_t count) {
    for (uint8_t i = 0; i < count; ++i)
        x86_InByte(UNUSED_PORT);
}

void IOWait() {
    x86_OutByte(UNUSED_PORT, 0);
}
