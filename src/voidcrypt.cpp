#include "voidcrypt/voidcrypt.h"
#include <stdio.h>
#include <string.h>


VCBlob vcMakeKey(const char* str) {
    VCBlob v = vcAllocBlob(strlen(str));
    memcpy(v.data, str, v.length);
    return v;
}

VCBlob vcCrypto(VCBlob key, void* data, size_t dataSize) {
    size_t blockCount = dataSize / key.length;
    if (dataSize % key.length > 0)
        blockCount++;

    VCBlob blob = vcAllocBlob(dataSize);

    uint8_t* pData = (uint8_t*)data;
    uint8_t* pKey = (uint8_t*)key.data;

    uint8_t* pBlob = (uint8_t*)blob.data;

    for (size_t block = 0; block < blockCount; block++) {
        for (size_t keyIt = 0; keyIt < key.length; keyIt++) {
            size_t it = block * key.length + keyIt;

            if (it >= dataSize) break;

            pBlob[it] = pData[it] ^ pKey[keyIt];
        }
    }

    return blob;
}

VCBlob vcAllocBlob(size_t length) {
    VCBlob blob;
    blob.length = length;
    blob.data = VC_MALLOC(length);
    return blob;
}

void vcFreeBlob(VCBlob* blob) {
    VC_FREE(blob->data);
}
