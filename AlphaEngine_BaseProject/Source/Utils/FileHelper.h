#pragma once
#include <string>
#include <rapidjson/document.h>

namespace FileHelper {
    bool TryReadJsonFile(const std::string& path, rapidjson::Document& doc);
}
