/*!
@file	ScopedTimer.h
@author	Ethan Ong
@brief	Declares ScopedTimer, a simple timer for profiling

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
#pragma once
#include <chrono>

/**
 * @brief	Simple timer for profiling. 
 *			To use, just create a variable of type ScopedTimer.
 *			Once it goes out of scope it'll output the time taken
 */
class ScopedTimer
{
public:
	ScopedTimer(const std::string& _name);
	~ScopedTimer();

	void Print(std::ostream& os);
private:
	std::string name;
	std::chrono::time_point<std::chrono::high_resolution_clock> start;
};

void operator <<(std::ostream& os, ScopedTimer& timer);
