#pragma once


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
};
