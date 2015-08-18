#pragma once
#include "BaseGame/GameEntity.h"

class InteractiveEntity : public GameEntity
{
    DECLARE_ENTITY_TYPEINFO(InteractiveEntity, GameEntity);
    DECLARE_ENTITY_NO_FACTORY(InteractiveEntity);

public:
    InteractiveEntity(const EntityTypeInfo *pTypeInfo = &s_typeInfo);
    virtual ~InteractiveEntity();


};
