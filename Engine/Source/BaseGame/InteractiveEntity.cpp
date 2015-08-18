#include "BaseGame/PrecompiledHeader.h"
#include "BaseGame/InteractiveEntity.h"

DEFINE_ENTITY_TYPEINFO(InteractiveEntity, 0);
BEGIN_ENTITY_PROPERTIES(InteractiveEntity)
END_ENTITY_PROPERTIES()
BEGIN_ENTITY_SCRIPT_FUNCTIONS(InteractiveEntity)
END_ENTITY_SCRIPT_FUNCTIONS()

InteractiveEntity::InteractiveEntity(const EntityTypeInfo *pTypeInfo /* = &s_typeInfo */)
    : GameEntity(pTypeInfo)
{

}

InteractiveEntity::~InteractiveEntity()
{

}
