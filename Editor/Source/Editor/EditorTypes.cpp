#include "Editor/PrecompiledHeader.h"
#include "Editor/Editor.h"

#define REGISTER_VERTEXFACTORY(Type) VERTEX_FACTORY_MUTABLE_TYPE_INFO(Type)->RegisterType()
#define REGISTER_ENTITY(Type) ENTITY_MUTABLE_TYPEINFO(Type)->RegisterType()

void Editor::RegisterEditorTypes()
{
}

#undef REGISTER_VERTEXFACTORY
#undef REGISTER_ENTITY
