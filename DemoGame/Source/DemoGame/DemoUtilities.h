#pragma once
#include "Engine/Common.h"
#include "Engine/Entity.h"
#include "Engine/Material.h"

class DirectionalLightEntity;

namespace DemoUtilities
{

    // create a plane shape
    Entity *CreatePlaneShape(World *pWorld, const float3 &normal, const float3 &origin, float size, const Material *pMaterial, float textureRepeatInterval = 1.0f, uint32 shadowFlags = ENTITY_SHADOW_FLAG_RECEIVE_DYNAMIC_SHADOWS);

    // create sun light
    DirectionalLightEntity *CreateSunLight(World *pWorld, const float3 &color = float3(1.0f, 1.0f, 1.0f), const float brightness = 1.0f, const float ambientContribution = 0.2f, const float3 &rotation = float3(45.0f, 0.0f, 0.0f));


}       // namespace DemoUtilties