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
