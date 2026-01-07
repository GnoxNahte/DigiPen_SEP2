#include <fstream>
#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/stringbuffer.h>
#include "rapidjson/writer.h"
#include <iostream>
#include <string>

#include "PlayerStats.h"

PlayerStats::PlayerStats(std::string file)
{
	std::cout << "loading file: " << file << std::endl;

	// rapidjson docs: https://rapidjson.org/md_doc_stream.html#iostreamWrapper
	std::ifstream ifs("config/player-stats.json");
	if (!ifs.is_open())
	{
		std::cout << "Failed to open file";
		return;
	}

	/*std::string str;
	while (ifs >> str)
	{
		std::cout << str << ' ';
	}*/

	rapidjson::IStreamWrapper isw(ifs);

	rapidjson::Document d;
	d.ParseStream(isw);

	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	d.Accept(writer);

	std::cout << buffer.GetString() << std::endl;
}

void PlayerStats::WatchFile(std::string file)
{
	std::cout << "watching file: " << file << std::endl;
}
