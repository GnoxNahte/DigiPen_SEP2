#include "SaveSystem.h"

#include <fstream>
#include <vector>
#include <cstring>
#include <type_traits>
#include <system_error>

std::filesystem::path SaveSystem::s_saveDir = std::filesystem::path("Saves");

namespace
{
    constexpr char kMagic[8] = { 'A','L','P','H','A','S','A','V' };

    template<typename T>
    void Append(std::vector<uint8_t>& buf, const T& v)
    {
        static_assert(std::is_trivially_copyable_v<T>);
        const uint8_t* p = reinterpret_cast<const uint8_t*>(&v);
        buf.insert(buf.end(), p, p + sizeof(T));
    }

    template<typename T>
    bool ReadAt(const std::string& bytes, size_t& offset, T& out)
    {
        static_assert(std::is_trivially_copyable_v<T>);
        if (offset + sizeof(T) > bytes.size()) return false;
        std::memcpy(&out, bytes.data() + offset, sizeof(T));
        offset += sizeof(T);
        return true;
    }
}

void SaveSystem::Init(const std::filesystem::path& dir)
{
    if (!dir.empty())
        s_saveDir = dir;

    std::error_code ec;
    std::filesystem::create_directories(s_saveDir, ec);
}

std::filesystem::path SaveSystem::GetSaveDir()
{
    return s_saveDir;
}

std::filesystem::path SaveSystem::MakeSlotPath(int slot)
{
    return s_saveDir / ("save_slot_" + std::to_string(slot) + ".sav");
}

bool SaveSystem::Exists(int slot)
{
    std::error_code ec;
    return std::filesystem::exists(MakeSlotPath(slot), ec);
}

bool SaveSystem::Delete(int slot)
{
    std::error_code ec;
    return std::filesystem::remove(MakeSlotPath(slot), ec);
}

bool SaveSystem::Save(int slot, const SaveData& data)
{
    std::error_code ec;
    std::filesystem::create_directories(s_saveDir, ec);

    // [magic 8][version u32][payloadSize u32][payload...]
    // payload(v1): [levelId i32][hp i32][totalSeconds f32]

    std::vector<uint8_t> payload;
    Append(payload, data.levelId);
    Append(payload, data.hp);
    Append(payload, data.totalSeconds);

    uint32_t payloadSize = static_cast<uint32_t>(payload.size());

    std::vector<uint8_t> fileBytes;
    fileBytes.insert(fileBytes.end(), kMagic, kMagic + sizeof(kMagic));
    Append(fileBytes, data.version);
    Append(fileBytes, payloadSize);
    fileBytes.insert(fileBytes.end(), payload.begin(), payload.end());

    return WriteAllBytes(MakeSlotPath(slot), fileBytes.data(), fileBytes.size());
}

bool SaveSystem::Load(int slot, SaveData& outData)
{
    std::string bytes;
    if (!ReadAllBytes(MakeSlotPath(slot), bytes)) return false;

    size_t off = 0;

    if (bytes.size() < sizeof(kMagic)) return false;
    if (std::memcmp(bytes.data(), kMagic, sizeof(kMagic)) != 0) return false;
    off += sizeof(kMagic);

    uint32_t version = 0;
    uint32_t payloadSize = 0;
    if (!ReadAt(bytes, off, version)) return false;
    if (!ReadAt(bytes, off, payloadSize)) return false;
    if (off + payloadSize > bytes.size()) return false;

    SaveData loaded{};
    loaded.version = version;

    if (version == 1)
    {
        if (!ReadAt(bytes, off, loaded.levelId)) return false;
        if (!ReadAt(bytes, off, loaded.hp)) return false;
        if (!ReadAt(bytes, off, loaded.totalSeconds)) return false;
    }
    else
    {
        return false; // 未来版本可在这里做迁移
    }

    outData = loaded;
    return true;
}

bool SaveSystem::WriteAllBytes(const std::filesystem::path& p, const void* bytes, size_t n)
{
    std::ofstream ofs(p, std::ios::binary | std::ios::trunc);
    if (!ofs) return false;

    ofs.write(reinterpret_cast<const char*>(bytes), static_cast<std::streamsize>(n));
    return ofs.good();
}

bool SaveSystem::ReadAllBytes(const std::filesystem::path& p, std::string& outBytes)
{
    std::ifstream ifs(p, std::ios::binary);
    if (!ifs) return false;

    ifs.seekg(0, std::ios::end);
    std::streamsize size = ifs.tellg();
    if (size <= 0) return false;
    ifs.seekg(0, std::ios::beg);

    outBytes.resize(static_cast<size_t>(size));
    ifs.read(outBytes.data(), size);
    return ifs.good();
}
