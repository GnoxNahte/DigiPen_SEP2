/*!
@file	EditorUtils.h
@author	Ethan Ong
@brief	Declares EditorUtils classes. 
		- Inspectable: Any class that wants to links with the Editor needs to inherit this class

Copyright (C) 2026 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*/
#pragma once

#include <AEVec2.h>

/**
 * @brief An interface for objects that can be inspected (Shows up in editor)
 * @todo make some parts debug only
 */
class Inspectable
{
public:
	bool enableInspector = false;
	bool isSystem = false;
	bool isInspectorOpen = false;

	Inspectable(bool isSystem = false);
	~Inspectable();
	virtual void DrawInspector() = 0;

	/**
	 * @brief  Check if the mouse clicked on this object
	 * @return Returns if mouse pos is in object
	 */
	virtual bool CheckIfClicked(const AEVec2& mousePos);
};
