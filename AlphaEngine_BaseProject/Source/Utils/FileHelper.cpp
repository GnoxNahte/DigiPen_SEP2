#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/error/en.h>
#include <fstream>
#include <iostream>

#include "FileHelper.h"

bool FileHelper::TryReadJsonFile(const std::string& path, rapidjson::Document& doc)
{
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open())
    {
        std::cout << "Failed to open file: " << path << std::endl;
        return false;
    }

    rapidjson::IStreamWrapper isw(ifs);
    doc.ParseStream(isw);

    if (doc.HasParseError())
    {
        std::cout << "JSON parse error: "
            << rapidjson::GetParseError_En(doc.GetParseError())
            << " (offset " << doc.GetErrorOffset() << ")"
            << std::endl;
        return false;
    }

    if (!doc.IsObject())
    {
        std::cout << "JSON root is not an object!" << std::endl;
        return false;
    }

    return true;
}

bool FileHelper::TryWriteJsonFile(const std::string& path, rapidjson::Document& doc)
{
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs.is_open())
    {
        std::cout << "Failed to open file: " << path << std::endl;
        return false;
    }

    rapidjson::OStreamWrapper osw(ofs);
    rapidjson::PrettyWriter<rapidjson::OStreamWrapper> writer(osw);
    writer.SetMaxDecimalPlaces(3);
    doc.Accept(writer);


    return true;
}
