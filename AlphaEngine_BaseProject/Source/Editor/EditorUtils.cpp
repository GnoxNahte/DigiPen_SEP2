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
