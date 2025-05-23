#include "io.h"
#include "x86.h"

#define UNUSED_PORT 0x80

// reading from an unused port seemingly uses a at least 30ns as i understand
void waitNsRough(uint32_t ns) {
    uint32_t count = (ns + 29) / 30; // divide by 30 and round up
    for (uint32_t i = 0; i < count; ++i)
        x86_InByte(UNUSED_PORT);
}

void IOWait() {
    x86_OutByte(UNUSED_PORT, 0);
}
