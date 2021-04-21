#pragma once
#include <stdint.h>

#ifndef VC_USE_CUSTOM_ALLOC
#include <stdlib.h>
#define VC_MALLOC(x) malloc(x)
#define VC_FREE(x) free(x)
#endif

struct VCBlob {
    size_t length;
    void* data;
};

VCBlob vcMakeKey(const char* str);

VCBlob vcCrypto(VCBlob key, void* data, size_t dataSize);
VCBlob vcAllocBlob(size_t length);
void vcFreeBlob(VCBlob* blob);
