#include "GameFramework/PrecompiledHeader.h"
#include "GameFramework/SpriteEntity.h"

DEFINE_ENTITY_TYPEINFO(SpriteEntity, 0);
DEFINE_ENTITY_GENERIC_FACTORY(SpriteEntity);
BEGIN_ENTITY_PROPERTIES(SpriteEntity)
END_ENTITY_PROPERTIES()
BEGIN_ENTITY_SCRIPT_FUNCTIONS(SpriteEntity)
END_ENTITY_SCRIPT_FUNCTIONS()

SpriteEntity::SpriteEntity(const EntityTypeInfo *pTypeInfo /* = &s_TypeInfo */)
    : BaseClass(pTypeInfo)
{

}

SpriteEntity::~SpriteEntity()
{

}
