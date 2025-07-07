#include "exports.h"
#include <lib/algorithm/arrays.h>
#include <lib/algorithm/string.h>
#include <stddef.h>

// get functions to export
#include "visual/stdio.h"

const FunctionExport g_ExportList[] = {
    {"putc", (void *)putc},
    {"puts", (void *)puts},
    {"printf", (void *)printf},
};

const FunctionExport *getExports() {
    return g_ExportList;
}

void *getExportFunction(const char *name) {
    for (size_t i = 0; i < ARRAY_SIZE(g_ExportList); i++) {
        if (strcmp(g_ExportList[i].name, name) == 0)
            return g_ExportList[i].address;
    }

    return NULL;
}
