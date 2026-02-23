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
	Inspectable();
	~Inspectable();
	virtual void DrawInspector() = 0;

	/**
	 * @brief  Check if the mouse clicked on this object
	 * @return Returns if mouse pos is in object
	 */
	virtual bool CheckIfClicked(const AEVec2& mousePos);
};
