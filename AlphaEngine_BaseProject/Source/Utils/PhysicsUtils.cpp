/*!
@file	PhysicsUtils.cpp
@author	Ethan Ong
@brief	Defines Physics functions

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/

#include "PhysicsUtils.h"
#include <cmath>

bool PhysicsUtils::AABB(const AEVec2& aPos, const AEVec2& aSize,
                        const AEVec2& bPos, const AEVec2& bSize)
{
    return  std::fabs(aPos.x - bPos.x) <= (aSize.x + bSize.x) * 0.5f &&
            std::fabs(aPos.y - bPos.y) <= (aSize.y + bSize.y) * 0.5f;
}
