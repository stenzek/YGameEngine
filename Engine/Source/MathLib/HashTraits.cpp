#include "MathLib/HashTraits.h"
#include "MathLib/Vectorf.h"
#include "MathLib/Vectori.h"
#include "MathLib/Vectoru.h"

HashType HashTrait<Vector2f>::GetHash(const Vector2f &Value)
{
    uint32 xhash = *(uint32 *)&Value.x;
    uint32 yhash = *(uint32 *)&Value.y;

    return (xhash + yhash * 37);
}

HashType HashTrait<Vector3f>::GetHash(const Vector3f &Value)
{
    uint32 xhash = *(uint32 *)&Value.x;
    uint32 yhash = *(uint32 *)&Value.y;
    uint32 zhash = *(uint32 *)&Value.z;

    return (xhash + yhash * 37 + zhash * 101);
}

HashType HashTrait<Vector4f>::GetHash(const Vector4f &Value)
{
    uint32 xhash = *(uint32 *)&Value.x;
    uint32 yhash = *(uint32 *)&Value.y;
    uint32 zhash = *(uint32 *)&Value.z;
    uint32 whash = *(uint32 *)&Value.w;

    return (xhash + yhash * 37 + zhash * 101 + whash * 241);
}

HashType HashTrait<Vector2i>::GetHash(const Vector2i &Value)
{
    uint32 xhash = *(uint32 *)&Value.x;
    uint32 yhash = *(uint32 *)&Value.y;

    return (xhash + yhash * 37);
}

HashType HashTrait<Vector3i>::GetHash(const Vector3i &Value)
{
    uint32 xhash = *(uint32 *)&Value.x;
    uint32 yhash = *(uint32 *)&Value.y;
    uint32 zhash = *(uint32 *)&Value.z;

    return (xhash + yhash * 37 + zhash * 101);
}

HashType HashTrait<Vector4i>::GetHash(const Vector4i &Value)
{
    uint32 xhash = *(uint32 *)&Value.x;
    uint32 yhash = *(uint32 *)&Value.y;
    uint32 zhash = *(uint32 *)&Value.z;
    uint32 whash = *(uint32 *)&Value.w;

    return (xhash + yhash * 37 + zhash * 101 + whash * 241);
}

HashType HashTrait<Vector2u>::GetHash(const Vector2u &Value)
{
    uint32 xhash = Value.x;
    uint32 yhash = Value.y;

    return (xhash + yhash * 37);
}

HashType HashTrait<Vector3u>::GetHash(const Vector3u &Value)
{
    uint32 xhash = Value.x;
    uint32 yhash = Value.y;
    uint32 zhash = Value.z;

    return (xhash + yhash * 37 + zhash * 101);
}

HashType HashTrait<Vector4u>::GetHash(const Vector4u &Value)
{
    uint32 xhash = Value.x;
    uint32 yhash = Value.y;
    uint32 zhash = Value.z;
    uint32 whash = Value.w;

    return (xhash + yhash * 37 + zhash * 101 + whash * 241);
}
