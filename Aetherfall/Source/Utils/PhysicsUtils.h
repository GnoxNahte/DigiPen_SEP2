/*!
@file	PhysicsUtils.h
@author	Ethan Ong
@brief	Declares Physics functions

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/

#pragma once
#include <AEVec2.h>

namespace PhysicsUtils
{
	bool AABB(const AEVec2& aPos, const AEVec2& aSize, const AEVec2& bPos, const AEVec2& bSize);
};

