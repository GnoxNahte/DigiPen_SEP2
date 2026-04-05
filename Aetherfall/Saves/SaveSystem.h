#pragma once
#include <filesystem>
#include <string>   
#include "SaveData.h"

class SaveSystem
{
public:
    static void Init(const std::filesystem::path& dir = {});
    static bool Save(int slot, const SaveData& data);
    static bool Load(int slot, SaveData& outData);
    static bool Exists(int slot);
    static bool Delete(int slot);
    static std::filesystem::path GetSaveDir();

private:
    static std::filesystem::path s_saveDir;
    static std::filesystem::path MakeSlotPath(int slot);
    static bool WriteAllBytes(const std::filesystem::path& p, const void* bytes, size_t n);
    static bool ReadAllBytes(const std::filesystem::path& p, std::string& outBytes);
};
