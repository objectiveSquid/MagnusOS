#pragma once

typedef struct {
    const char *name;
    void *address;
} FunctionExport;

void *getExportFunction(const char *name);
const FunctionExport *getExports();
