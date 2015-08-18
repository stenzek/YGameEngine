#include "Engine/Entity.h"

class SpriteEntity : public Entity
{
    DECLARE_ENTITY_TYPEINFO(SpriteEntity, Entity);
    DECLARE_ENTITY_GENERIC_FACTORY(SpriteEntity);

public:
    SpriteEntity(const EntityTypeInfo *pTypeInfo = &s_typeInfo);
    virtual ~SpriteEntity();

};

