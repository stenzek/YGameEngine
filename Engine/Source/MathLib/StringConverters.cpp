#include "MathLib/StringConverters.h"
#include "YBaseLib/StringConverter.h"

Vector2f StringConverter::StringToVector2f(const char *Source)
{
    uint32 strLength = Y_strlen(Source);
    const char *strCur = Source;
    const char *strEnd = Source + strLength;
    uint32 c = 0;

    Vector2f res(Vector2f::Zero);

    for (; strCur < strEnd && c < 2; )
    {
        if (*strCur == '(' || *strCur == ')' || *strCur == ',' || *strCur == ' ')
        {
            strCur++;
            continue;
        }

        char *EndPtr;
        res[c++] = Y_strtofloat(strCur, &EndPtr);
        strCur = EndPtr;
    }

    return res;
}

Vector3f StringConverter::StringToVector3f(const char *Source)
{
    uint32 strLength = Y_strlen(Source);
    const char *strCur = Source;
    const char *strEnd = Source + strLength;
    uint32 c = 0;

    Vector3f res(Vector3f::Zero);

    for (; strCur < strEnd && c < 3; )
    {
        if (*strCur == '(' || *strCur == ')' || *strCur == ',' || *strCur == ' ')
        {
            strCur++;
            continue;
        }

        char *EndPtr;
        res[c++] = Y_strtofloat(strCur, &EndPtr);
        strCur = EndPtr;
    }

    return res;
}

Vector4f StringConverter::StringToVector4f(const char *Source)
{
    uint32 strLength = Y_strlen(Source);
    const char *strCur = Source;
    const char *strEnd = Source + strLength;
    uint32 c = 0;

    Vector4f res(Vector4f::Zero);

    for (; strCur < strEnd && c < 4; )
    {
        if (*strCur == '(' || *strCur == ')' || *strCur == ',' || *strCur == ' ')
        {
            strCur++;
            continue;
        }

        char *EndPtr;
        res[c++] = Y_strtofloat(strCur, &EndPtr);
        strCur = EndPtr;
    }

    return res;
}

void StringConverter::Vector2fToString(String &Destination, const Vector2f &Source)
{
    Destination.Format("%f %f", Source.x, Source.y);
}

void StringConverter::Vector3fToString(String &Destination, const Vector3f &Source)
{
    Destination.Format("%f %f %f", Source.x, Source.y, Source.z);
}

void StringConverter::Vector4fToString(String &Destination, const Vector4f &Source)
{
    Destination.Format("%f %f %f %f", Source.x, Source.y, Source.z, Source.w);
}

Vector2i StringConverter::StringToVector2i(const char *Source)
{
    uint32 strLength = Y_strlen(Source);
    const char *strCur = Source;
    const char *strEnd = Source + strLength;
    uint32 c = 0;

    Vector2i res(Vector2i::Zero);

    for (; strCur < strEnd && c < 2; )
    {
        if (*strCur == '(' || *strCur == ')' || *strCur == ',' || *strCur == ' ')
        {
            strCur++;
            continue;
        }

        char *EndPtr;
        res[c++] = Y_strtoint32(strCur, &EndPtr);
        strCur = EndPtr;
    }

    return res;
}

Vector3i StringConverter::StringToVector3i(const char *Source)
{
    uint32 strLength = Y_strlen(Source);
    const char *strCur = Source;
    const char *strEnd = Source + strLength;
    uint32 c = 0;

    Vector3i res(Vector3i::Zero);

    for (; strCur < strEnd && c < 3; )
    {
        if (*strCur == '(' || *strCur == ')' || *strCur == ',' || *strCur == ' ')
        {
            strCur++;
            continue;
        }

        char *EndPtr;
        res[c++] = Y_strtoint32(strCur, &EndPtr);
        strCur = EndPtr;
    }

    return res;
}

Vector4i StringConverter::StringToVector4i(const char *Source)
{
    uint32 strLength = Y_strlen(Source);
    const char *strCur = Source;
    const char *strEnd = Source + strLength;
    uint32 c = 0;

    Vector4i res(Vector4i::Zero);

    for (; strCur < strEnd && c < 4; )
    {
        if (*strCur == '(' || *strCur == ')' || *strCur == ',' || *strCur == ' ')
        {
            strCur++;
            continue;
        }

        char *EndPtr;
        res[c++] = Y_strtoint32(strCur, &EndPtr);
        strCur = EndPtr;
    }

    return res;
}

Vector2u StringConverter::StringToVector2u(const char *Source)
{
    uint32 strLength = Y_strlen(Source);
    const char *strCur = Source;
    const char *strEnd = Source + strLength;
    uint32 c = 0;

    Vector2u res(Vector2u::Zero);

    for (; strCur < strEnd && c < 2; )
    {
        if (*strCur == '(' || *strCur == ')' || *strCur == ',' || *strCur == ' ')
        {
            strCur++;
            continue;
        }

        char *EndPtr;
        res[c++] = Y_strtouint32(strCur, &EndPtr);
        strCur = EndPtr;
    }

    return res;
}

Vector3u StringConverter::StringToVector3u(const char *Source)
{
    uint32 strLength = Y_strlen(Source);
    const char *strCur = Source;
    const char *strEnd = Source + strLength;
    uint32 c = 0;

    Vector3u res(Vector3u::Zero);

    for (; strCur < strEnd && c < 3; )
    {
        if (*strCur == '(' || *strCur == ')' || *strCur == ',' || *strCur == ' ')
        {
            strCur++;
            continue;
        }

        char *EndPtr;
        res[c++] = Y_strtouint32(strCur, &EndPtr);
        strCur = EndPtr;
    }

    return res;
}

Vector4u StringConverter::StringToVector4u(const char *Source)
{
    uint32 strLength = Y_strlen(Source);
    const char *strCur = Source;
    const char *strEnd = Source + strLength;
    uint32 c = 0;

    Vector4u res(Vector4u::Zero);

    for (; strCur < strEnd && c < 4; )
    {
        if (*strCur == '(' || *strCur == ')' || *strCur == ',' || *strCur == ' ')
        {
            strCur++;
            continue;
        }

        char *EndPtr;
        res[c++] = Y_strtouint32(strCur, &EndPtr);
        strCur = EndPtr;
    }

    return res;
}

void StringConverter::Vector2iToString(String &Destination, const Vector2i &Source)
{
    Destination.Format("%i %i", Source.x, Source.y);
}

void StringConverter::Vector3iToString(String &Destination, const Vector3i &Source)
{
    Destination.Format("%i %i %i", Source.x, Source.y, Source.z);
}

void StringConverter::Vector4iToString(String &Destination, const Vector4i &Source)
{
    Destination.Format("%i %i %i %i", Source.x, Source.y, Source.z, Source.w);
}

void StringConverter::Vector2uToString(String &Destination, const Vector2u &Source)
{
    Destination.Format("%u %u", Source.x, Source.y);
}

void StringConverter::Vector3uToString(String &Destination, const Vector3u &Source)
{
    Destination.Format("%u %u %u", Source.x, Source.y, Source.z);
}

void StringConverter::Vector4uToString(String &Destination, const Vector4u &Source)
{
    Destination.Format("%u %u %u %u", Source.x, Source.y, Source.z, Source.w);
}

SIMDVector2f StringConverter::StringToSIMDVector2f(const char *Source)
{
    uint32 strLength = Y_strlen(Source);
    const char *strCur = Source;
    const char *strEnd = Source + strLength;
    uint32 c = 0;

    SIMDVector2f res(SIMDVector2f::Zero);

    for (; strCur < strEnd && c < 2; )
    {
        if (*strCur == '(' || *strCur == ')' || *strCur == ',' || *strCur == ' ')
        {
            strCur++;
            continue;
        }

        char *EndPtr;
        res[c++] = Y_strtofloat(strCur, &EndPtr);
        strCur = EndPtr;
    }

    return res;
}

SIMDVector3f StringConverter::StringToSIMDVector3f(const char *Source)
{
    uint32 strLength = Y_strlen(Source);
    const char *strCur = Source;
    const char *strEnd = Source + strLength;
    uint32 c = 0;

    SIMDVector3f res(SIMDVector3f::Zero);

    for (; strCur < strEnd && c < 3; )
    {
        if (*strCur == '(' || *strCur == ')' || *strCur == ',' || *strCur == ' ')
        {
            strCur++;
            continue;
        }

        char *EndPtr;
        res[c++] = Y_strtofloat(strCur, &EndPtr);
        strCur = EndPtr;
    }

    return res;
}

SIMDVector4f StringConverter::StringToSIMDVector4f(const char *Source)
{
    uint32 strLength = Y_strlen(Source);
    const char *strCur = Source;
    const char *strEnd = Source + strLength;
    uint32 c = 0;

    SIMDVector4f res(SIMDVector4f::Zero);

    for (; strCur < strEnd && c < 4; )
    {
        if (*strCur == '(' || *strCur == ')' || *strCur == ',' || *strCur == ' ')
        {
            strCur++;
            continue;
        }

        char *EndPtr;
        res[c++] = Y_strtofloat(strCur, &EndPtr);
        strCur = EndPtr;
    }

    return res;
}

void StringConverter::SIMDVector2fToString(String &Destination, const SIMDVector2f &Source)
{
    Destination.Format("%f %f", Source.x, Source.y);
}

void StringConverter::SIMDVector3fToString(String &Destination, const SIMDVector3f &Source)
{
    Destination.Format("%f %f %f", Source.x, Source.y, Source.z);
}

void StringConverter::SIMDVector4fToString(String &Destination, const SIMDVector4f &Source)
{
    Destination.Format("%f %f %f %f", Source.x, Source.y, Source.z, Source.w);
}

SIMDVector2i StringConverter::StringToSIMDVector2i(const char *Source)
{
    uint32 strLength = Y_strlen(Source);
    const char *strCur = Source;
    const char *strEnd = Source + strLength;
    uint32 c = 0;

    SIMDVector2i res(SIMDVector2i::Zero);

    for (; strCur < strEnd && c < 2; )
    {
        if (*strCur == '(' || *strCur == ')' || *strCur == ',' || *strCur == ' ')
        {
            strCur++;
            continue;
        }

        char *EndPtr;
        res[c++] = Y_strtoint32(strCur, &EndPtr);
        strCur = EndPtr;
    }

    return res;
}

SIMDVector3i StringConverter::StringToSIMDVector3i(const char *Source)
{
    uint32 strLength = Y_strlen(Source);
    const char *strCur = Source;
    const char *strEnd = Source + strLength;
    uint32 c = 0;

    SIMDVector3i res(SIMDVector3i::Zero);

    for (; strCur < strEnd && c < 3; )
    {
        if (*strCur == '(' || *strCur == ')' || *strCur == ',' || *strCur == ' ')
        {
            strCur++;
            continue;
        }

        char *EndPtr;
        res[c++] = Y_strtoint32(strCur, &EndPtr);
        strCur = EndPtr;
    }

    return res;
}

SIMDVector4i StringConverter::StringToSIMDVector4i(const char *Source)
{
    uint32 strLength = Y_strlen(Source);
    const char *strCur = Source;
    const char *strEnd = Source + strLength;
    uint32 c = 0;

    SIMDVector4i res(SIMDVector4i::Zero);

    for (; strCur < strEnd && c < 4; )
    {
        if (*strCur == '(' || *strCur == ')' || *strCur == ',' || *strCur == ' ')
        {
            strCur++;
            continue;
        }

        char *EndPtr;
        res[c++] = Y_strtoint32(strCur, &EndPtr);
        strCur = EndPtr;
    }

    return res;
}

void StringConverter::SIMDVector2iToString(String &Destination, const SIMDVector2i &Source)
{
    Destination.Format("%i %i", Source.x, Source.y);
}

void StringConverter::SIMDVector3iToString(String &Destination, const SIMDVector3i &Source)
{
    Destination.Format("%i %i %i", Source.x, Source.y, Source.z);
}

void StringConverter::SIMDVector4iToString(String &Destination, const SIMDVector4i &Source)
{
    Destination.Format("%i %i %i %i", Source.x, Source.y, Source.z, Source.w);
}

Quaternion StringConverter::StringToQuaternion(const char *Source)
{
    uint32 strLength = Y_strlen(Source);
    const char *strCur = Source;
    const char *strEnd = Source + strLength;
    uint32 c = 0;

    Vector4f res(0.0f, 0.0f, 0.0f, 1.0f);

    for (; strCur < strEnd && c < 4;)
    {
        if (*strCur == '(' || *strCur == ')' || *strCur == ',' || *strCur == ' ')
        {
            strCur++;
            continue;
        }

        char *EndPtr;
        res[c++] = Y_strtofloat(strCur, &EndPtr);
        strCur = EndPtr;
    }

    return Quaternion(res.x, res.y, res.z, res.w);
}

void StringConverter::QuaternionToString(String &Destination, const Quaternion &Source)
{
    Destination.Format("%f %f %f %f", Source.x, Source.y, Source.z, Source.w);
}

void StringConverter::TransformToString(String &dest, const Transform &transform)
{
    const Vector3f &translation = transform.GetPosition();
    const Vector4f rotation = transform.GetRotation().GetVectorRepresentation();
    const Vector3f &scale = transform.GetScale();

    dest.Format("(%f, %f, %f), (%f, %f, %f, %f), (%f, %f, %f)",
                translation.x, translation.y, translation.z,
                rotation.x, rotation.y, rotation.z, rotation.w,
                scale.x, scale.y, scale.z);
}

Transform StringConverter::StringToTranform(const String &str)
{
    if (str.GetLength() < 2)
        return Transform::Identity;

    uint32 i = 0;
    SmallString tempString;
    Transform outTransform;
    outTransform.SetIdentity();

    while (i < str.GetLength() && str[i++] != '(');

    if (i < str.GetLength())
    {
        while (i < str.GetLength() && str[i] != ')')
            tempString.AppendCharacter(str[i++]);

        outTransform.SetPosition(StringConverter::StringToVector3f(tempString));
        tempString.Clear();

        while (i < str.GetLength() && str[i++] != ',');
        while (i < str.GetLength() && str[i++] != '(');
        
        if (i < str.GetLength())
        {
            while (i < str.GetLength() && str[i] != ')')
                tempString.AppendCharacter(str[i++]);

            outTransform.SetRotation(Quaternion(StringConverter::StringToVector4f(tempString)));
            tempString.Clear();

            while (i < str.GetLength() && str[i++] != ',');
            while (i < str.GetLength() && str[i++] != '(');

            if (i < str.GetLength())
            {
                while (i < str.GetLength() && str[i] != ')')
                    tempString.AppendCharacter(str[i++]);

                outTransform.SetScale(StringConverter::StringToVector3f(tempString));
                tempString.Clear();
            }
        }
    }

    return outTransform;
}

TinyString StringConverter::Vector2fToString(const Vector2f &Source)
{
    TinyString ret;
    Vector2fToString(ret, Source);
    return ret;
}

TinyString StringConverter::Vector3fToString(const Vector3f &Source)
{
    TinyString ret;
    Vector3fToString(ret, Source);
    return ret;
}

TinyString StringConverter::Vector4fToString(const Vector4f &Source)
{
    TinyString ret;
    Vector4fToString(ret, Source);
    return ret;
}

TinyString StringConverter::Vector2iToString(const Vector2i &Source)
{
    TinyString ret;
    SIMDVector2iToString(ret, Source);
    return ret;
}

TinyString StringConverter::Vector3iToString(const Vector3i &Source)
{
    TinyString ret;
    SIMDVector3iToString(ret, Source);
    return ret;
}

TinyString StringConverter::Vector4iToString(const Vector4i &Source)
{
    TinyString ret;
    SIMDVector4iToString(ret, Source);
    return ret;
}

TinyString StringConverter::Vector2uToString(const Vector2u &Source)
{
    TinyString ret;
    Vector2uToString(ret, Source);
    return ret;
}

TinyString StringConverter::Vector3uToString(const Vector3u &Source)
{
    TinyString ret;
    Vector3uToString(ret, Source);
    return ret;
}

TinyString StringConverter::Vector4uToString(const Vector4u &Source)
{
    TinyString ret;
    Vector4uToString(ret, Source);
    return ret;
}

TinyString StringConverter::SIMDVector2fToString(const SIMDVector2f &Source)
{
    TinyString ret;
    SIMDVector2fToString(ret, Source);
    return ret;
}

TinyString StringConverter::SIMDVector3fToString(const SIMDVector3f &Source)
{
    TinyString ret;
    SIMDVector3fToString(ret, Source);
    return ret;
}

TinyString StringConverter::SIMDVector4fToString(const SIMDVector4f &Source)
{
    TinyString ret;
    SIMDVector4fToString(ret, Source);
    return ret;
}

TinyString StringConverter::SIMDVector2iToString(const SIMDVector2i &Source)
{
    TinyString ret;
    SIMDVector2iToString(ret, Source);
    return ret;
}

TinyString StringConverter::SIMDVector3iToString(const SIMDVector3i &Source)
{
    TinyString ret;
    SIMDVector3iToString(ret, Source);
    return ret;
}

TinyString StringConverter::SIMDVector4iToString(const SIMDVector4i &Source)
{
    TinyString ret;
    SIMDVector4iToString(ret, Source);
    return ret;
}

TinyString StringConverter::QuaternionToString(const Quaternion &Source)
{
    TinyString ret;
    QuaternionToString(ret, Source);
    return ret;
}

SmallString StringConverter::TransformToString(const Transform &Source)
{
    SmallString ret;
    TransformToString(ret, Source);
    return ret;
}

