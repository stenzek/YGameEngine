#include "Engine/PrecompiledHeader.h"
#include "Engine/Engine.h"
#include "Engine/Entity.h"
#include "Renderer/VertexFactory.h"

// Engine Resource Types
#include "Engine/Texture.h"
#include "Engine/MaterialShader.h"
#include "Engine/Material.h"
#include "Engine/StaticMesh.h"
#include "Engine/Font.h"
#include "Engine/BlockPalette.h"
#include "Engine/BlockMesh.h"
#include "Engine/Skeleton.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalAnimation.h"

// Components
#include "Engine/Component.h"
#include "GameFramework/StaticMeshComponent.h"
#include "GameFramework/BlockMeshComponent.h"
#include "GameFramework/PointLightComponent.h"
#include "GameFramework/PositionInterpolatorComponent.h"
#include "GameFramework/RotationInterpolatorComponent.h"
#include "GameFramework/InterpolatorComponent.h"
#include "GameFramework/ParticleEmitterComponent.h"

// Entities
#include "Engine/Entity.h"
#include "GameFramework/StaticMeshBrush.h"
#include "GameFramework/BlockMeshBrush.h"
#include "GameFramework/DirectionalLightEntity.h"
#include "GameFramework/PointLightEntity.h"
#include "GameFramework/SpriteEntity.h"
#include "GameFramework/StaticMeshEntity.h"
#include "GameFramework/BlockMeshEntity.h"
#include "GameFramework/StaticMeshRigidBodyEntity.h"
#include "GameFramework/ParticleEmitterEntity.h"

// Particle system
#include "Engine/ParticleSystem.h"
#include "Engine/ParticleSystemBuiltinEmitters.h"
#include "Engine/ParticleSystemBuiltinModules.h"

// Macro for type registration
#define REGISTER_TYPE(Type) Type::StaticMutableTypeInfo()->RegisterType()

void Engine::RegisterEngineResourceTypes()
{
    REGISTER_TYPE(Material);
    REGISTER_TYPE(MaterialShader);
    REGISTER_TYPE(Texture);
    REGISTER_TYPE(Texture2D);
    REGISTER_TYPE(Texture2DArray);
    REGISTER_TYPE(TextureCube);
    REGISTER_TYPE(StaticMesh);
    REGISTER_TYPE(Font);
    REGISTER_TYPE(BlockPalette);
    REGISTER_TYPE(BlockMesh);
    REGISTER_TYPE(Skeleton);
    REGISTER_TYPE(SkeletalMesh);
    REGISTER_TYPE(SkeletalAnimation);
}

void Engine::RegisterEngineComponentTypes()
{
    REGISTER_TYPE(Component);

    REGISTER_TYPE(StaticMeshComponent);
    REGISTER_TYPE(BlockMeshComponent);
    REGISTER_TYPE(PointLightComponent);
    REGISTER_TYPE(PositionInterpolatorComponent);
    REGISTER_TYPE(RotationInterpolatorComponent);
    REGISTER_TYPE(InterpolatorComponent);
    REGISTER_TYPE(ParticleEmitterComponent);
}

void Engine::RegisterEngineEntityTypes()
{
    REGISTER_TYPE(Entity);

    // TODO REMOVE REFERENCE TO GAMEFRAMEWORK AFTER THIS CRUD CLEANUP
    REGISTER_TYPE(StaticMeshBrush);
    REGISTER_TYPE(BlockMeshBrush);

    REGISTER_TYPE(DirectionalLightEntity);
    REGISTER_TYPE(PointLightEntity);
    REGISTER_TYPE(SpriteEntity);
    REGISTER_TYPE(StaticMeshEntity);
    REGISTER_TYPE(BlockMeshEntity);
    REGISTER_TYPE(StaticMeshRigidBodyEntity);
    REGISTER_TYPE(ParticleEmitterEntity);
}

void Engine::RegisterExternalTypes()
{

}

void Engine::RegisterEngineTypes()
{
    // base object type, kinda necessary
    REGISTER_TYPE(Object);

    // engine types
    RegisterEngineResourceTypes();
    RegisterEngineComponentTypes();
    RegisterEngineEntityTypes();
    RegisterExternalTypes();

    // particle system
    REGISTER_TYPE(ParticleSystem);
    REGISTER_TYPE(ParticleSystemEmitter);
    REGISTER_TYPE(ParticleSystemModule);
    ParticleSystemEmitter::RegisterBuiltinEmitters();
    ParticleSystemModule::RegisterBuiltinModules();
}

void Engine::UnregisterTypes()
{
    // objects
    {
        ObjectTypeInfo::RegistryType &typeRegistry = ShaderComponentTypeInfo::GetRegistry();
        for (uint32 i = 0; i < typeRegistry.GetNumTypes(); i++)
        {
            ObjectTypeInfo *pTypeInfo = typeRegistry.GetRegisteredTypeInfoByIndex(i).pTypeInfo;
            if (pTypeInfo != NULL)
                pTypeInfo->UnregisterType();
        }
    }
}
