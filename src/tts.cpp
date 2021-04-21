#include "tts.h"

#include "STDAfx.h"
#include <sapi.h>

std::wstring tts::s2ws(const std::string& s)
{
    int len;
    int slength = (int)s.length() + 1;
    len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
    wchar_t* buf = new wchar_t[len];
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
    std::wstring r(buf);
    delete[] buf;
    return r;
}

int tts::Speak(const std::string& words)
{
    ISpVoice* pVoice = NULL;

    if (FAILED(::CoInitialize(NULL)))
        return FALSE;

    HRESULT hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void**)&pVoice);
    if (SUCCEEDED(hr))
    {
        std::wstring stemp = s2ws(words);
        LPCWSTR result = stemp.c_str();
        hr = pVoice->Speak(result, SPF_DEFAULT, NULL);

        // Change pitch
        pVoice->Release();
        pVoice = NULL;
    }
    ::CoUninitialize();
    return TRUE;
}
