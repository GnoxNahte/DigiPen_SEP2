/*!
@file	Box.h
@author	Ethan Ong
@brief	Declares a Box class to store position and size.

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/

#pragma once
#include "AEEngine.h"

// No .cpp for now, just stores data for now
struct Box
{
	AEVec2 position;
	AEVec2 size;
};

