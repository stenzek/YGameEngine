#include "Engine/PrecompiledHeader.h"
#include "Engine/World.h"
#include "Engine/Physics/PhysicsWorld.h"
#include "Engine/Brush.h"
#include "Engine/Entity.h"
#include "Engine/ParticleSystemRenderProxy.h"
#include "Renderer/RenderWorld.h"
#include "Renderer/Renderer.h"

World::World()
{
    m_worldBoundingBox = AABox::Zero;
    m_worldBoundingSphere = Sphere::Zero;
    m_pPhysicsWorld = new Physics::PhysicsWorld();
    m_pRenderWorld = new RenderWorld();    
    m_nextEntityID = 1;
    m_gameTime = 0.0f;
}

World::~World()
{
    DebugAssert(m_observers.GetSize() == 0);
    DebugAssert(m_activeEntities.GetSize() == 0);
    DebugAssert(m_activeAsyncEntities.GetSize() == 0);

    // clean up emitters
    while (m_temporaryParticleEffects.GetSize() > 0)
    {
        TemporaryParticleEffect &effect = m_temporaryParticleEffects[m_temporaryParticleEffects.GetSize() - 1];
        m_pRenderWorld->RemoveRenderable(effect.pRenderProxy);
        effect.pRenderProxy->Release();
        m_temporaryParticleEffects.RemoveBack();
    }

    delete m_pPhysicsWorld;

    // render world destruction should come from the render thread
    //RENDERER_QUEUE_BLOCKING_LAMBDA_COMMAND([this]() {
        m_pRenderWorld->Release();
    //});
}

void World::QueueRemoveEntity(Entity *pEntity)
{
    if (m_removeQueue.Contains(pEntity))
        return;

    m_removeQueue.Add(pEntity);
}

void World::AddObserver(const void *identifier, const float3 &location /* = float3::Zero */)
{
    for (ObserverEntry &observer : m_observers)
    {
        if (observer.Key == identifier)
        {
            observer.Value = location;
            return;
        }
    }

    m_observers.Add(ObserverEntry(identifier, location));
}

void World::UpdateObserver(const void *identifier, const float3 &location)
{
    for (ObserverEntry &observer : m_observers)
    {
        if (observer.Key == identifier)
        {
            observer.Value = location;
            return;
        }
    }
}

void World::RemoveObserver(const void *identifier)
{
    for (uint32 i = 0; i < m_observers.GetSize(); i++)
    {
        if (m_observers[i].Key == identifier)
        {
            m_observers.OrderedRemove(i);
            return;
        }
    }
}

void World::BeginFrame(float deltaTime)
{
    m_gameTime += deltaTime;
}

void World::UpdateAsync(float deltaTime)
{
    // update physics world async
    m_pPhysicsWorld->UpdateAsync(deltaTime);

    // update active entities async
    for (uint32 i = 0; i < m_activeAsyncEntities.GetSize(); i++)
    {
        EntityUpdateData &updateData = m_activeAsyncEntities[i];

        updateData.TimeSinceLastUpdate += deltaTime;

        if (updateData.TimeSinceLastUpdate >= updateData.UpdateInterval)
        {
            float timeSinceEntityLastUpdate = updateData.TimeSinceLastUpdate;
            updateData.TimeSinceLastUpdate = 0.0f;

            updateData.pEntity->UpdateAsync(timeSinceEntityLastUpdate);
        }
    }
}

void World::Update(float deltaTime)
{
    // update physics world
    m_pPhysicsWorld->Update(deltaTime);

    // update active entities
    for (uint32 i = 0; i < m_activeEntities.GetSize(); i++)
    {
        EntityUpdateData &updateData = m_activeEntities[i];

        updateData.TimeSinceLastUpdate += deltaTime;

        if (updateData.TimeSinceLastUpdate >= updateData.UpdateInterval)
        {
            float timeSinceEntityLastUpdate = updateData.TimeSinceLastUpdate;
            updateData.TimeSinceLastUpdate = 0.0f;

            updateData.pEntity->Update(timeSinceEntityLastUpdate);
        }
    }

    // update particle systems
    UpdateTemporaryParticleEffects(deltaTime);
}

void World::EndFrame()
{
    // remove anything that has to die
    while (m_removeQueue.GetSize() > 0)
    {
        Entity *pEntity = m_removeQueue[0];

        // slow.. fixme at some point
        m_removeQueue.PopFront();

        // invoke remove
        RemoveEntity(pEntity);
    }
}

void World::RegisterEntityForUpdate(Entity *pEntity, float interval)
{
    // check for existing
    for (uint32 i = 0; i < m_activeEntities.GetSize(); i++)
    {
        EntityUpdateData &updateData = m_activeEntities[i];
        if (updateData.pEntity == pEntity)
        {
            updateData.UpdateInterval = interval;
            return;
        }        
    }

    // create new
    EntityUpdateData updateData;
    updateData.pEntity = pEntity;
    updateData.UpdateInterval = interval;
    updateData.TimeSinceLastUpdate = 0.0f;
    m_activeEntities.Add(updateData);

    // re-sort
    SortActiveEntities();
}

void World::RegisterEntityForAsyncUpdate(Entity *pEntity, float interval)
{
    // check for existing
    for (uint32 i = 0; i < m_activeAsyncEntities.GetSize(); i++)
    {
        EntityUpdateData &updateData = m_activeAsyncEntities[i];
        if (updateData.pEntity == pEntity)
        {
            updateData.UpdateInterval = interval;
            return;
        }
    }

    // create new
    EntityUpdateData updateData;
    updateData.pEntity = pEntity;
    updateData.UpdateInterval = interval;
    updateData.TimeSinceLastUpdate = 0.0f;
    m_activeAsyncEntities.Add(updateData);

    // re-sort
    SortActiveAsyncEntities();
}

void World::UnregisterEntityForUpdate(Entity *pEntity)
{
    for (uint32 i = 0; i < m_activeEntities.GetSize(); i++)
    {
        EntityUpdateData &updateData = m_activeEntities[i];
        if (updateData.pEntity == pEntity)
        {
            m_activeEntities.FastRemove(i);
            SortActiveEntities();
            return;
        }
    }
}

void World::UnregisterEntityForAsyncUpdate(Entity *pEntity)
{
    for (uint32 i = 0; i < m_activeAsyncEntities.GetSize(); i++)
    {
        EntityUpdateData &updateData = m_activeAsyncEntities[i];
        if (updateData.pEntity == pEntity)
        {
            m_activeAsyncEntities.FastRemove(i);
            SortActiveAsyncEntities();
            return;
        }
    }
}

void World::SortActiveEntities()
{

}

void World::SortActiveAsyncEntities()
{

}

bool World::RayCast(const Ray &ray, Entity **ppHitEntity, float3 *pContactNormal, float3 *pContactPoint)
{
    const Physics::PhysicsProxy *pHitPhysicsProxy;
    if (!m_pPhysicsWorld->RayCast(ray, &pHitPhysicsProxy, pContactNormal, pContactPoint))
        return false;

    if (ppHitEntity != nullptr)
        *ppHitEntity = (pHitPhysicsProxy != nullptr && pHitPhysicsProxy->GetEntityID() != 0) ? GetEntityByID(pHitPhysicsProxy->GetEntityID()) : nullptr;

    return true;
}

void World::SpawnParticleEmitter(const ParticleSystem *pParticleSystem, float lifeSpan, const float3 &location, const Quaternion &rotation /* = Quaternion::Identity */, const float3 &scale /* = float3::One */, const float3 &initialVelocity /* = float3::Zero */, float mass /* = 0.0f */)
{
    TemporaryParticleEffect effect;
    effect.pRenderProxy = new ParticleSystemRenderProxy(pParticleSystem, 0);
    effect.BaseTransform = Transform(location, rotation, scale);
    effect.TimeRemaining = lifeSpan;
    effect.HasVelocity = (initialVelocity.SquaredLength() > Y_FLT_EPSILON);
    effect.Velocity = initialVelocity;
    effect.Mass = mass;
    m_temporaryParticleEffects.Add(effect);
    m_pRenderWorld->AddRenderable(effect.pRenderProxy);
}

void World::UpdateTemporaryParticleEffects(float deltaTime)
{
    for (uint32 i = 0; i < m_temporaryParticleEffects.GetSize(); )
    {
        TemporaryParticleEffect &effect = m_temporaryParticleEffects[i];
        if (deltaTime >= effect.TimeRemaining)
        {
            m_pRenderWorld->RemoveRenderable(effect.pRenderProxy);
            effect.pRenderProxy->Release();
            m_temporaryParticleEffects.OrderedRemove(i);
            continue;
        }

        // handle velocity
        if (effect.HasVelocity)
        {
            // affect position
            effect.BaseTransform.SetPosition(effect.BaseTransform.GetPosition() + effect.Velocity * deltaTime);

            // apply gravity
            if (effect.Mass != 0.0f)
                effect.Velocity += m_pPhysicsWorld->GetGravity() * (effect.Mass * deltaTime);
        }

        effect.TimeRemaining -= deltaTime;
        effect.pRenderProxy->Update(&effect.BaseTransform, deltaTime);
        i++;
    }
}
