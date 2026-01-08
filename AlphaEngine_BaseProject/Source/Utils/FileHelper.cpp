#include <rapidjson/istreamwrapper.h>
#include <rapidjson/error/en.h>
#include <fstream>
#include <iostream>

#include "FileHelper.h"

using namespace rapidjson;

bool FileHelper::TryReadJsonFile(std::string file, Document& doc)
{
	// rapidjson docs: https://rapidjson.org/md_doc_stream.html#iostreamWrapper
	std::ifstream ifs(file);
	if (!ifs.is_open())
	{
		std::cout << "Failed to open file: " << file << std::endl;
		return false;
	}

	/*std::string str;
	while (ifs >> str)
		std::cout << str << ' ';

	ifs.clear();
	ifs.seekg(0, std::ios::beg);*/

	IStreamWrapper isw(ifs);

	doc.ParseStream(isw);
	if (doc.HasParseError())
	{
		auto errorCode = doc.GetParseError();
		std::cout << "JSON parse error: %s (%u)", rapidjson::GetParseError_En(errorCode);
		return false;
	}

	return true;
}
