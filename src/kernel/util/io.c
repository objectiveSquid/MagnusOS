#include "io.h"
#include "x86.h"

#define UNUSED_PORT 0x80

void IOWait() {
    x86_OutByte(UNUSED_PORT, 0);
}
