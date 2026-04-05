/*!
@file	FileHelper.h
@author	Ethan Ong
@brief  Declares a few File IO helper functions to make reading and writing 
        json files through the rapidjson API easier

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
#pragma once
#include <string>
#include <rapidjson/document.h>

namespace FileHelper {
    bool TryReadJsonFile(const std::string& path, rapidjson::Document& doc);
    bool TryWriteJsonFile(const std::string& path, rapidjson::Document& doc, bool createIfNotFound = false);
}
