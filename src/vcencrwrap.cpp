#include "vcencrwrap.h"

const char* encrwrap::bkey = ""/*key goes here*/;

std::string encrwrap::perform(const std::string& data)
{
    VCBlob key = vcMakeKey(bkey);
    VCBlob blob = vcCrypto(key, (void*)data.data(), data.length());
    std::string returndata = std::string((char*)blob.data, blob.length);
    vcFreeBlob(&blob);
    vcFreeBlob(&key);
    return returndata;
}