#define MINIAUDIO_IMPLEMENTATION
#include "MusicManager.h"

#include <algorithm>
#include <stdio.h>

void MusicManager::Open() {
    if (soundReady) return;

    lastError.clear();
    bgmPath = FindBgmPath();
    if (bgmPath.empty()) {
        lastError = L"未找到 bgm.mp3";
        return;
    }

    ma_result result = ma_engine_init(nullptr, &engine);
    if (result != MA_SUCCESS) {
        engineReady = false;
        lastError = FormatResult(L"音频引擎初始化失败", result);
        return;
    }
    engineReady = true;

    result = ma_sound_init_from_file_w(&engine, bgmPath.c_str(), MA_SOUND_FLAG_STREAM, nullptr, nullptr, &bgm);
    if (result != MA_SUCCESS) {
        soundReady = false;
        lastError = FormatResult(L"音乐文件加载失败", result);
        ma_engine_uninit(&engine);
        engineReady = false;
        return;
    }

    soundReady = true;
    ma_sound_set_looping(&bgm, MA_TRUE);
    SetVolume(volume);
    if (enabled) Play();
}

void MusicManager::Play() {
    if (!soundReady) return;
    ma_result result = ma_sound_start(&bgm);
    if (result != MA_SUCCESS) lastError = FormatResult(L"音乐播放失败", result);
}

void MusicManager::Stop() {
    if (!soundReady) return;
    ma_sound_stop(&bgm);
    ma_sound_seek_to_pcm_frame(&bgm, 0);
}

void MusicManager::Close() {
    if (soundReady) {
        ma_sound_stop(&bgm);
        ma_sound_uninit(&bgm);
        soundReady = false;
    }
    if (engineReady) {
        ma_engine_uninit(&engine);
        engineReady = false;
    }
}

void MusicManager::SetEnabled(bool value) {
    enabled = value;
    if (enabled) {
        if (!soundReady) Open();
        Play();
    }
    else {
        Stop();
    }
}

void MusicManager::SetVolume(int value) {
    volume = std::max(0, std::min(100, value));
    if (soundReady) ma_sound_set_volume(&bgm, volume / 100.0f);
}

bool MusicManager::IsEnabled() const { return enabled; }
bool MusicManager::IsOpened() const { return soundReady; }
int MusicManager::GetVolume() const { return volume; }
const std::wstring& MusicManager::GetBgmPath() const { return bgmPath; }
const std::wstring& MusicManager::GetLastError() const { return lastError; }
const std::wstring& MusicManager::GetLastCommand() const { return emptyText; }

bool MusicManager::FileExists(const std::wstring& path) const {
    DWORD attributes = GetFileAttributes(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

std::wstring MusicManager::NormalizePath(const std::wstring& path) const {
    wchar_t fullPath[MAX_PATH]{};
    DWORD length = GetFullPathName(path.c_str(), MAX_PATH, fullPath, nullptr);
    if (length == 0 || length >= MAX_PATH) return path;
    return fullPath;
}

std::wstring MusicManager::DirectoryOf(const std::wstring& path) const {
    size_t slash = path.find_last_of(L"\\/");
    if (slash == std::wstring::npos) return L"";
    return path.substr(0, slash + 1);
}

std::wstring MusicManager::ParentDirectoryOf(const std::wstring& directory) const {
    if (directory.empty()) return L"";
    std::wstring trimmed = directory;
    while (!trimmed.empty() && (trimmed.back() == L'\\' || trimmed.back() == L'/')) trimmed.pop_back();
    return DirectoryOf(trimmed);
}

std::wstring MusicManager::CombinePath(const std::wstring& directory, const std::wstring& fileName) const {
    if (directory.empty()) return fileName;
    if (directory.back() == L'\\' || directory.back() == L'/') return directory + fileName;
    return directory + L"\\" + fileName;
}

void MusicManager::AddBgmCandidates(std::vector<std::wstring>& candidates, const std::wstring& directory) const {
    if (directory.empty()) return;
    candidates.push_back(CombinePath(directory, L"bgm.mp3"));
    candidates.push_back(CombinePath(CombinePath(directory, L"Release"), L"bgm.mp3"));
    candidates.push_back(CombinePath(CombinePath(directory, L"Debug"), L"bgm.mp3"));
}

std::wstring MusicManager::FindBgmPath() const {
    wchar_t exePath[MAX_PATH]{};
    GetModuleFileName(nullptr, exePath, MAX_PATH);

    wchar_t currentPath[MAX_PATH]{};
    GetCurrentDirectory(MAX_PATH, currentPath);

    std::wstring exeDir = DirectoryOf(exePath);
    std::wstring currentDir = currentPath;
    std::wstring exeParent = ParentDirectoryOf(exeDir);
    std::wstring currentParent = ParentDirectoryOf(currentDir);

    std::vector<std::wstring> candidates;
    AddBgmCandidates(candidates, exeDir);
    AddBgmCandidates(candidates, currentDir);
    AddBgmCandidates(candidates, exeParent);
    AddBgmCandidates(candidates, currentParent);

    for (const auto& candidate : candidates) {
        std::wstring normalized = NormalizePath(candidate);
        if (FileExists(normalized)) return normalized;
    }
    return L"";
}

std::wstring MusicManager::FormatResult(const wchar_t* prefix, ma_result result) const {
    wchar_t buffer[128];
    swprintf_s(buffer, L"%s：%d", prefix, (int)result);
    return buffer;
}
