#pragma once
#include "BaseGame/GameEntity.h"

class LivingEntity : public GameEntity
{
    DECLARE_ENTITY_TYPEINFO(LivingEntity, GameEntity);
    DECLARE_ENTITY_NO_FACTORY(Entity);

public:
    LivingEntity(const EntityTypeInfo *pTypeInfo = &s_typeInfo);
    virtual ~LivingEntity();

    const bool IsAlive() const { return (m_health > 0); }
    const uint32 GetHealth() const { return m_health; }
    void SetHealth(uint32 health);

    const uint32 GetMaxHealth() const { return m_maxHealth; }
    void SetMaxHealth(uint32 maxHealth);

protected:
    // callbacks
    virtual void OnHealthChanged();

    // events
    virtual void OnDeath();
    virtual void OnRevive();

    // behavior variables
    uint32 m_health;
    uint32 m_maxHealth;

    // skeletal mesh, attachment points?

private:
    // hidden trampolines for property system
    static void __PS_OnHealthChanged(LivingEntity *pThis, const void *pUserData) { pThis->OnHealthChanged(); }
};

