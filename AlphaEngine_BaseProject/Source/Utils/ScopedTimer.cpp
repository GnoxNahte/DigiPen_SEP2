#include "ScopedTimer.h"
#include <iostream>

ScopedTimer::ScopedTimer(const std::string& _name) :
	name(_name),
	start(std::chrono::high_resolution_clock::now())
{
}

ScopedTimer::~ScopedTimer()
{
	Print(std::cout);
}

void ScopedTimer::Print(std::ostream& os)
{
	// Docs: https://en.cppreference.com/w/cpp/chrono/duration/duration_cast
	auto now = std::chrono::high_resolution_clock::now();
	const std::chrono::duration<double, std::milli> duration = now - start;
	os << name << " took " << duration << "ms\n";
}

void operator<<(std::ostream& os, ScopedTimer& timer)
{
	timer.Print(os);
}
