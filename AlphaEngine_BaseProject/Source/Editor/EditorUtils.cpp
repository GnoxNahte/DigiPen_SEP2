#include "EditorUtils.h"
#include "Editor.h"

Inspectable::Inspectable()
{
	Editor::Register(*this);
}

Inspectable::~Inspectable()
{
	Editor::Unregister(*this);
}

// Takes in mousePos
// Returns false by default
bool Inspectable::CheckIfClicked(const AEVec2& )
{
	//(void*)&mousePos;
	return false;
}
