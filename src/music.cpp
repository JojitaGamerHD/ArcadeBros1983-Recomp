// Original host-side music overlay. No game/ROM data is embedded here.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <wrl/client.h>
#include "music.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <new>
#include <string>
#include <vector>

namespace {
using Microsoft::WRL::ComPtr;
constexpr unsigned kOutputRate = 48000;
constexpr size_t kMaxFrames = 32u * 1024u * 1024u;
std::vector<int16_t> music;
size_t cursor = 0;

struct MediaSession {
    HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    bool started = false;
    ~MediaSession() {
        if (started) MFShutdown();
        if (SUCCEEDED(com)) CoUninitialize();
    }
};

HRESULT decode(const wchar_t *path, std::vector<int16_t> &result) {
    MediaSession session;
    if (FAILED(session.com) && session.com != RPC_E_CHANGED_MODE) return session.com;
    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) return hr;
    session.started = true;
    ComPtr<IMFSourceReader> reader;
    hr = MFCreateSourceReaderFromURL(path, nullptr, &reader);
    if (FAILED(hr)) return hr;
    hr = reader->SetStreamSelection(MF_SOURCE_READER_ALL_STREAMS, FALSE);
    if (FAILED(hr)) return hr;
    hr = reader->SetStreamSelection(MF_SOURCE_READER_FIRST_AUDIO_STREAM, TRUE);
    if (FAILED(hr)) return hr;
    ComPtr<IMFMediaType> requested;
    hr = MFCreateMediaType(&requested);
    if (FAILED(hr)) return hr;
    if (FAILED(hr = requested->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio)) ||
        FAILED(hr = requested->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM)) ||
        FAILED(hr = requested->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16))) return hr;
    hr = reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, requested.Get());
    if (FAILED(hr)) return hr;
    ComPtr<IMFMediaType> format;
    hr = reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &format);
    if (FAILED(hr)) return hr;
    UINT32 rate = 0, channels = 0, bits = 0, alignment = 0;
    GUID subtype{};
    if (FAILED(hr = format->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &rate)) ||
        FAILED(hr = format->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &channels)) ||
        FAILED(hr = format->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &bits)) ||
        FAILED(hr = format->GetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, &alignment)) ||
        FAILED(hr = format->GetGUID(MF_MT_SUBTYPE, &subtype))) return hr;
    if (subtype != MFAudioFormat_PCM || bits != 16 || channels < 1 || channels > 8 ||
        alignment != channels * 2 || rate < 8000 || rate > 192000) return E_INVALIDARG;
    std::vector<int16_t> native;
    for (size_t packets = 0; packets < 200000; ++packets) {
        DWORD flags = 0;
        ComPtr<IMFSample> sample;
        hr = reader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, nullptr,
                                &flags, nullptr, &sample);
        if (FAILED(hr)) return hr;
        if (flags & (MF_SOURCE_READERF_ERROR | MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED))
            return E_FAIL;
        if (sample) {
            ComPtr<IMFMediaBuffer> buffer;
            hr = sample->ConvertToContiguousBuffer(&buffer);
            if (FAILED(hr)) return hr;
            BYTE *bytes = nullptr;
            DWORD length = 0;
            hr = buffer->Lock(&bytes, nullptr, &length);
            if (FAILED(hr)) return hr;
            const size_t frames = length / alignment;
            if (length % alignment || frames > kMaxFrames - native.size()) {
                buffer->Unlock();
                return E_INVALIDARG;
            }
            // Reserve before reading while locked; always unlock on allocation failure.
            try {
                if (native.size() + frames > native.capacity())
                    native.reserve(std::min(kMaxFrames, std::max(native.size() + frames,
                        std::max(size_t(4096), native.capacity() * 2))));
                for (size_t f = 0; f < frames; ++f) {
                    int sum = 0;
                    for (unsigned c = 0; c < channels; ++c) {
                        int16_t value;
                        std::memcpy(&value, bytes + f * alignment + c * 2, sizeof(value));
                        sum += value;
                    }
                    native.push_back(static_cast<int16_t>(sum / static_cast<int>(channels)));
                }
            } catch (...) { buffer->Unlock(); throw; }
            buffer->Unlock();
        }
        if (flags & MF_SOURCE_READERF_ENDOFSTREAM) {
            if (native.empty()) return E_FAIL;
            const uint64_t output = static_cast<uint64_t>(native.size()) * kOutputRate / rate;
            if (!output || output > kMaxFrames) return E_INVALIDARG;
            result.resize(static_cast<size_t>(output));
            for (size_t i = 0; i < result.size(); ++i) {
                const uint64_t position = static_cast<uint64_t>(i) * rate;
                const size_t a = static_cast<size_t>(position / kOutputRate);
                const size_t b = std::min(a + 1, native.size() - 1);
                const double fraction = static_cast<double>(position % kOutputRate) / kOutputRate;
                result[i] = static_cast<int16_t>(std::lround(
                    native[a] * (1.0 - fraction) + native[b] * fraction));
            }
            return S_OK;
        }
    }
    return E_FAIL; // Bounded even if a broken source never signals EOS.
}

int load(const wchar_t *path) {
    music.clear();
    cursor = 0;
    std::vector<int16_t> decoded;
    HRESULT hr = E_FAIL;
    try { hr = decode(path, decoded); }
    catch (const std::bad_alloc &) { hr = E_OUTOFMEMORY; }
    catch (...) { hr = E_FAIL; }
    if (FAILED(hr)) {
        std::fprintf(stderr, "[arcade music] pista no disponible (0x%08lX); audio NES conservado\n",
                     static_cast<unsigned long>(hr));
        return 0;
    }
    music.swap(decoded);
    std::fprintf(stderr, "[arcade music] pista cargada: %.2f s, mono PCM16, 48000 Hz\n",
                 static_cast<double>(music.size()) / kOutputRate);
    return 1;
}
}

extern "C" int arcade_music_load_utf8(const char *path) {
    if (!path) { music.clear(); cursor = 0; return 0; }
    const int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, nullptr, 0);
    if (!n) { music.clear(); cursor = 0; return 0; }
    try {
        std::wstring wide(static_cast<size_t>(n), L'\0');
        if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, wide.data(), n)) return 0;
        return load(wide.c_str());
    } catch (...) { music.clear(); cursor = 0; return 0; }
}
extern "C" int arcade_music_load_default(void) {
    wchar_t file[32768];
    const DWORD n = GetModuleFileNameW(nullptr, file, 32768);
    if (!n || n >= 32768) { music.clear(); cursor = 0; return 0; }
    try {
        // Portable user data lives in Datos/, the launcher's working directory.
        // A privately supplied track stays outside the generic Bin/ package.
        wchar_t local[32768];
        const DWORD length = GetFullPathNameW(L"Music\\stage1.m4a", 32768, local, nullptr);
        if (length && length < 32768) {
            const DWORD attributes = GetFileAttributesW(local);
            if (attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY))
                return load(local);
        }
        std::wstring path(file, n);
        const size_t slash = path.find_last_of(L"\\/");
        if (slash == std::wstring::npos) return 0;
        path.resize(slash + 1);
        path += L"assets\\music\\stage1.m4a";
        return load(path.c_str());
    } catch (...) { music.clear(); cursor = 0; return 0; }
}
extern "C" void arcade_music_reset(void) { cursor = 0; }
extern "C" size_t arcade_music_frame_count(void) { return music.size(); }
extern "C" size_t arcade_music_cursor(void) { return cursor; }
extern "C" void arcade_music_mix(int16_t *samples, size_t count, int enabled, int volume) {
    if (!samples || !enabled || music.empty()) return;
    volume = std::clamp(volume, 0, 100);
    for (size_t i = 0; i < count; ++i) {
        const int mixed = static_cast<int>(samples[i]) +
            static_cast<int>(music[cursor]) * volume / 100;
        samples[i] = static_cast<int16_t>(std::clamp(mixed, -32768, 32767));
        if (++cursor == music.size()) cursor = 0;
    }
}
