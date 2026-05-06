#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <string>
#include <vector>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244)
#endif
#include "third_party/miniaudio/miniaudio.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

class MusicManager {
public:
    void Open();
    void Play();
    void Stop();
    void Close();
    void SetEnabled(bool value);
    void SetVolume(int value);

    bool IsEnabled() const;
    bool IsOpened() const;
    int GetVolume() const;
    const std::wstring& GetBgmPath() const;
    const std::wstring& GetLastError() const;
    const std::wstring& GetLastCommand() const;

private:
    bool FileExists(const std::wstring& path) const;
    std::wstring NormalizePath(const std::wstring& path) const;
    std::wstring DirectoryOf(const std::wstring& path) const;
    std::wstring ParentDirectoryOf(const std::wstring& directory) const;
    std::wstring CombinePath(const std::wstring& directory, const std::wstring& fileName) const;
    void AddBgmCandidates(std::vector<std::wstring>& candidates, const std::wstring& directory) const;
    std::wstring FindBgmPath() const;
    std::wstring FormatResult(const wchar_t* prefix, ma_result result) const;

    ma_engine engine{};
    ma_sound bgm{};
    bool engineReady = false;
    bool soundReady = false;
    bool enabled = true;
    int volume = 60;
    std::wstring bgmPath;
    std::wstring lastError;
    std::wstring emptyText;
};
