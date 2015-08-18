#pragma once
#include "Engine/Common.h"

#ifdef Y_COMPILER_MSVC
    #pragma warning(push)
    #pragma warning(disable: 4127)  // warning C4127: conditional expression is constant
    #pragma warning(disable: 4706)  // warning C4706: assignment within conditional expression
#endif

#include "btBulletCollisionCommon.h"
#include "btBulletDynamicsCommon.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"

namespace Physics
{
    inline btVector3 Float3ToBulletVector(const float3 &v) { return btVector3(v.x, v.y, v.z); }
    inline float3 BulletVector3ToFloat3(const btVector3 &v) { return float3(v.x(), v.y(), v.z()); }
    inline btVector3 SIMDVector3fToBulletVector(const SIMDVector3f &v) { return btVector3(v.x, v.y, v.z); }
    inline float3 BulletVector3ToSIMDVector3f(const btVector3 &v) { return SIMDVector3f(v.x(), v.y(), v.z()); }
    inline btQuaternion QuaternionToBulletQuaternion(const Quaternion &v) { return btQuaternion(v.x, v.y, v.z, v.w); }
    inline Quaternion BulletQuaternionToQuaternion(const btQuaternion &v) { return Quaternion(v.x(), v.y(), v.z(), v.w()); }

    // drops any scale component of the transform
    inline btTransform TransformToBulletTransform(const Transform &transform)
    {
        btTransform res;
        res.setRotation(QuaternionToBulletQuaternion(transform.GetRotation()));
        res.setOrigin(Float3ToBulletVector(transform.GetPosition()));
        return res;
    }
    inline Transform BulletTransformToTransform(const btTransform &bulletTransform, const float3 &scale)
    {
        Transform res;
        res.SetRotation(BulletQuaternionToQuaternion(bulletTransform.getRotation()));
        res.SetPosition(BulletVector3ToFloat3(bulletTransform.getOrigin()));
        res.SetScale(scale);
        return res;
    }    
}

#ifdef Y_COMPILER_MSVC
    #pragma warning(pop)
#endif
