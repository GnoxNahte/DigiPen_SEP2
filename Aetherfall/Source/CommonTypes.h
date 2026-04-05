/*!
@file	CommonTypes.h
@author	Ethan Ong
@brief	Declares any common types for other files to use.
		Helps with circular dependencies

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/

#pragma once

// Enumeration types for the incoming damage type.
enum DAMAGE_TYPE
{
	DAMAGE_TYPE_NORMAL,
	DAMAGE_TYPE_CRIT,
	DAMAGE_TYPE_RESIST,
	DAMAGE_TYPE_HEAL,
	DAMAGE_TYPE_MISS,
	DAMAGE_TYPE_ENEMY_ATTACK,
	DAMAGE_TYPE_ENEMY_MISS,
	DAMAGE_TYPE_TRAP
};
