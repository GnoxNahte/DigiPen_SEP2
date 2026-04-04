/*!
@file	EditorUtils.cpp
@author	Ethan Ong
@brief	Defines EditorUtils classes.
		- Inspectable: Any class that wants to links with the Editor needs to inherit this class

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
#include "EditorUtils.h"
#include "Editor.h"

Inspectable::Inspectable(bool isSystem) : isSystem(isSystem)
{
	// if is a system, register in parent class (since need name)
	if (isSystem)
		return;

	Editor::Register(this);
}

Inspectable::~Inspectable()
{
	if (isSystem)
		return;

	Editor::Unregister(this);
}

// Takes in mousePos
// Returns false by default
bool Inspectable::CheckIfClicked(const AEVec2& )
{
	//(void*)&mousePos;
	return false;
}
