#pragma once
#include "Core/Common.h"
#include "Core/Property.h"
#include "YBaseLib/ByteStream.h"

class ObjectSerializer
{
public:
    static bool SerializePropertyValueString(PROPERTY_TYPE propertyType, const char *propertyValueString, ByteStream *pOutputStream, uint32 *pWrittenBytes);
};