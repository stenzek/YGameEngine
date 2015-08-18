#include "BaseGame/PrecompiledHeader.h"
#include "BaseGame/LivingEntity.h"

DEFINE_ENTITY_TYPEINFO(LivingEntity, 0);
BEGIN_ENTITY_PROPERTIES(LivingEntity)
    PROPERTY_TABLE_MEMBER_UINT("Health", 0, offsetof(LivingEntity, m_health), __PS_OnHealthChanged, nullptr)
    PROPERTY_TABLE_MEMBER_UINT("MaxHealth", 0, offsetof(LivingEntity, m_maxHealth), nullptr, nullptr)
END_ENTITY_PROPERTIES()
BEGIN_ENTITY_SCRIPT_FUNCTIONS(LivingEntity)
END_ENTITY_SCRIPT_FUNCTIONS()

LivingEntity::LivingEntity(const EntityTypeInfo *pTypeInfo /*= &s_typeInfo*/)
    : BaseClass(pTypeInfo),
      m_health(1),
      m_maxHealth(1)
{

}

LivingEntity::~LivingEntity()
{

}

void LivingEntity::SetHealth(uint32 health)
{
    m_health = health;
    OnHealthChanged();
}

void LivingEntity::SetMaxHealth(uint32 maxHealth)
{
    m_maxHealth = maxHealth;
    OnHealthChanged();
}

void LivingEntity::OnHealthChanged()
{

}

void LivingEntity::OnDeath()
{

}

void LivingEntity::OnRevive()
{

}

