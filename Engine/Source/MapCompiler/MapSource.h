#pragma once
#include "MapCompiler/MapCompiler.h"
#include "MapCompiler/MapSourceGeometryData.h"
#include "Engine/Common.h"
#include "Engine/TerrainRenderer.h"
#include "Core/PropertyTable.h"
#include "YBaseLib/ProgressCallbacks.h"

class XMLReader;
class XMLWriter;

class EntityTypeInfo;
class ObjectTemplate;

class TerrainLayerListGenerator;

class MapSourceRegion;
class MapSourceEntityData;
class MapSourceTerrainData;

class MapSource
{
    friend class MapCompiler;
    friend class MapSourceRegion;

public:
    MapSource();
    ~MapSource();

    ZipArchive *GetMapArchive() const { return m_pMapArchive; }

    // object template, may or may not be present
    const ObjectTemplate *GetObjectTemplate() const { return m_pObjectTemplate; }

    bool IsChanged() const;
    void FlagAsChanged();

    // helper methods
    const int2 GetRegionForPosition(const float3 &position) const;
    const int2 GetRegionForTerrainSection(int32 sectionX, int32 sectionY) const;

    // Serialization.
    void Create(uint32 regionSize = 1024);
    bool Load(const char *fileName, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);
    bool Save(ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);
    bool SaveAs(const char *newFileName, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);

    // Loading from stream. Save() will not be possible, SaveAs() will be possible, however.
    bool LoadFromStream(ByteStream *pArchiveStream, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);

    // Properties
    const uint32 GetVersion() const { return m_version; }
    const uint32 GetRegionSize() const { return m_regionSize; }
    const uint32 GetRegionLODLevels() const { return m_regionLODLevels; }
    const PropertyTable *GetProperties() const { return &m_properties; }
    PropertyTable *GetProperties() { return &m_properties; }

    // Terrain Data
    bool HasTerrain() const { return (m_pTerrainData != NULL); }
    const MapSourceTerrainData *GetTerrainData() const { return m_pTerrainData; }
    MapSourceTerrainData *GetTerrainData() { return m_pTerrainData; }
    MapSourceTerrainData *CreateTerrainData(const TerrainLayerListGenerator *pLayerList, TERRAIN_HEIGHT_STORAGE_FORMAT heightStorageFormat, uint32 scale, uint32 sectionSize, uint32 renderLODCount, int32 minHeight, int32 maxHeight, int32 baseHeight, ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);
    void DeleteTerrainData(ProgressCallbacks *pProgressCallbacks = ProgressCallbacks::NullProgressCallback);

    // Geometry Data
    //const MapSourceGeometryData *GetEntityData() const { return &m_GeometryData; }
    //MapSourceGeometryData *GetEntityData() { return &m_GeometryData; }

    // Entity id allocation
    String GenerateEntityName(const char *typeName);

    // Entity adding/removal. Removing an entity from here by pointer leaves the cleanup to the caller.
    MapSourceEntityData *CreateEntity(const char *typeName, const char *entityName = nullptr);
    MapSourceEntityData *CreateEntityFromTemplate(const ObjectTemplate *pTemplate, const char *entityName = nullptr);
    MapSourceEntityData *CopyEntity(const MapSourceEntityData *pEntity, const char *entityName = nullptr);
    bool AddEntity(MapSourceEntityData *pEntityData);
    void RemoveEntity(MapSourceEntityData *pEntityData);
    bool RemoveEntity(const char *entityName);

    // Entity readback.
    uint32 GetEntityCount() const { return m_entities.GetMemberCount(); }
    const MapSourceEntityData *GetEntityData(const char *entityName) const;
    MapSourceEntityData *GetEntityData(const char *entityName);

    // iteration functions
    template<typename T>
    void EnumerateEntities(T callback)
    {
        for (EntityTable::Iterator itr = m_entities.Begin(); !itr.AtEnd(); itr.Forward())
            callback(itr->Value);
    }    
    template<typename T>
    void EnumerateEntities(T callback) const
    {
        for (EntityTable::ConstIterator itr = m_entities.Begin(); !itr.AtEnd(); itr.Forward())
            callback(itr->Value);
    }    

private:
    void SetDefaults();
    bool IsEntitiesChanged() const;

    // entities loading/saving
    bool LoadEntities(ProgressCallbacks *pProgressCallbacks);
    bool SaveEntities(ProgressCallbacks *pProgressCallbacks) const;

    // terrain loading/saving
    bool LoadTerrain(ProgressCallbacks *pProgressCallbacks);
    bool SaveTerrain(ProgressCallbacks *pProgressCallbacks) const;

    String m_filename;
    ZipArchive *m_pMapArchive;
    bool m_isChanged;

    // properties
    uint32 m_version;
    uint32 m_regionSize;
    uint32 m_regionLODLevels;
    const ObjectTemplate *m_pObjectTemplate;
    PropertyTable m_properties;

    // entities
    uint32 m_nextEntitySuffix;
    typedef CIStringHashTable<MapSourceEntityData *> EntityTable;
    EntityTable m_entities;
    mutable bool m_entityListChanged;

    // terrain
    MapSourceTerrainData *m_pTerrainData;

    // geometry
    //MapSourceGeometryData m_GeometryData;
};

class MapSourceEntityComponent
{
    friend MapSource;
    friend MapSourceEntityData;
    MapSourceEntityComponent();

public:
    ~MapSourceEntityComponent();

    const String &GetComponentName() const { return m_componentName; }
    const String &GetTypeName() const { return m_typeName; }
    const bool IsChanged() const { return m_changed; }

    void Create(const String &componentName, const String &typeName);
    void CreateFromTemplate(const String &componentName, const ObjectTemplate *pTemplate);

    bool LoadFromXML(XMLReader &xmlReader);
    void SaveToXML(XMLWriter &xmlWriter) const;

    const PropertyTable *GetPropertyTable() const { return &m_properties; }
    const String *GetPropertyValuePointer(const char *propertyName) const;
    String GetPropertyValueString(const char *propertyName) const;
    void SetPropertyValue(const char *propertyName, const char *propertyValue);
    void SetPropertyValue(const char *propertyName, const String &propertyValue);

    void SetChangedFlag() { m_changed = true; }
    void ClearChangedFlag() { m_changed = false; }

private:
    String m_componentName;
    String m_typeName;
    mutable bool m_changed;

    PropertyTable m_properties;
};

class MapSourceEntityData
{
    friend MapSource;
    MapSourceEntityData();

public:
    ~MapSourceEntityData();

    const String &GetEntityName() const { return m_entityName; }
    const String &GetTypeName() const { return m_typeName; }

    const bool IsChanged() const;
    void SetChangedFlag();
    void ClearChangedFlag();

    const PropertyTable *GetPropertyTable() const { return &m_properties; }
    const String *GetPropertyValuePointer(const char *propertyName) const;
    String GetPropertyValueString(const char *propertyName) const;
    void SetPropertyValue(const char *propertyName, const char *propertyValue);
    void SetPropertyValue(const char *propertyName, const String &propertyValue);

    const String &GetScript() const { return m_script; }
    void SetScript(const char *script) { m_script = script; m_changed = true; }
    void SetScript(const String &script) { m_script = script; m_changed = true; }

    const MapSourceEntityComponent *GetComponentByIndex(uint32 index) const { return m_components[index]; }
    const MapSourceEntityComponent *GetComponentByName(const char *name) const;
    const uint32 GetComponentCount() const { return m_components.GetSize(); }

    MapSourceEntityComponent *GetComponentByIndex(uint32 index) { return m_components[index]; }
    MapSourceEntityComponent *GetComponentByName(const char *name);
    MapSourceEntityComponent *CreateComponent(const char *typeName, const char *componentName = nullptr);
    MapSourceEntityComponent *CreateComponent(const ObjectTemplate *pTemplate, const char *componentName = nullptr);
    void AddComponent(MapSourceEntityComponent *pComponent);
    void RemoveComponent(MapSourceEntityComponent *pComponent);

    void Create(const String &entityName, const String &typeName);
    void CreateFromTemplate(const String &entityName, const ObjectTemplate *pTemplate);
    bool LoadFromXML(XMLReader &xmlReader);
    void SaveToXML(XMLWriter &xmlWriter) const;

    // typed property readers
    bool GetPropertyValueBool(const char *propertyName, bool *pValue) const;
    bool GetPropertyValueInt8(const char *propertyName, int8 *pValue) const;
    bool GetPropertyValueInt16(const char *propertyName, int16 *pValue) const;
    bool GetPropertyValueInt32(const char *propertyName, int32 *pValue) const;
    bool GetPropertyValueInt64(const char *propertyName, int64 *pValue) const;
    bool GetPropertyValueUInt8(const char *propertyName, uint8 *pValue) const;
    bool GetPropertyValueUInt16(const char *propertyName, uint16 *pValue) const;
    bool GetPropertyValueUInt32(const char *propertyName, uint32 *pValue) const;
    bool GetPropertyValueUInt64(const char *propertyName, uint64 *pValue) const;
    bool GetPropertyValueFloat(const char *propertyName, float *pValue) const;
    bool GetPropertyValueDouble(const char *propertyName, double *pValue) const;
    bool GetPropertyValueColor(const char *propertyName, uint32 *pValue) const;
    bool GetPropertyValueInt2(const char *propertyName, int2 *pValue) const;
    bool GetPropertyValueInt3(const char *propertyName, int3 *pValue) const;
    bool GetPropertyValueInt4(const char *propertyName, int4 *pValue) const;
    bool GetPropertyValueUInt2(const char *propertyName, uint2 *pValue) const;
    bool GetPropertyValueUInt3(const char *propertyName, uint3 *pValue) const;
    bool GetPropertyValueUInt4(const char *propertyName, uint4 *pValue) const;
    bool GetPropertyValueFloat2(const char *propertyName, float2 *pValue) const;
    bool GetPropertyValueFloat3(const char *propertyName, float3 *pValue) const;
    bool GetPropertyValueFloat4(const char *propertyName, float4 *pValue) const;
    bool GetPropertyValueQuaternion(const char *propertyName, Quaternion *pValue) const;

    // typed property readers with fallback values
    bool GetPropertyValueDefaultBool(const char *propertyName, bool defaultValue = false) const;
    int8 GetPropertyValueDefaultInt8(const char *propertyName, int8 defaultValue = 0) const;
    int16 GetPropertyValueDefaultInt16(const char *propertyName, int16 defaultValue = 0) const;
    int32 GetPropertyValueDefaultInt32(const char *propertyName, int32 defaultValue = 0) const;
    int64 GetPropertyValueDefaultInt64(const char *propertyName, int64 defaultValue = 0) const;
    uint8 GetPropertyValueDefaultUInt8(const char *propertyName, uint8 defaultValue = 0) const;
    uint16 GetPropertyValueDefaultUInt16(const char *propertyName, uint16 defaultValue = 0) const;
    uint32 GetPropertyValueDefaultUInt32(const char *propertyName, uint32 defaultValue = 0) const;
    uint64 GetPropertyValueDefaultUInt64(const char *propertyName, uint64 defaultValue = 0) const;
    float GetPropertyValueDefaultFloat(const char *propertyName, float defaultValue = 0) const;
    double GetPropertyValueDefaultDouble(const char *propertyName, double defaultValue = 0) const;
    uint32 GetPropertyValueDefaultColor(const char *propertyName, uint32 defaultValue = 0) const;
    int2 GetPropertyValueDefaultInt2(const char *propertyName, const int2 &defaultValue = int2::Zero) const;
    int3 GetPropertyValueDefaultInt3(const char *propertyName, const int3 &defaultValue = int3::Zero) const;
    int4 GetPropertyValueDefaultInt4(const char *propertyName, const int4 &defaultValue = int4::Zero) const;
    uint2 GetPropertyValueDefaultUInt2(const char *propertyName, const uint2 &defaultValue = uint2::Zero) const;
    uint3 GetPropertyValueDefaultUInt3(const char *propertyName, const uint3 &defaultValue = uint3::Zero) const;
    uint4 GetPropertyValueDefaultUInt4(const char *propertyName, const uint4 &defaultValue = uint4::Zero) const;
    float2 GetPropertyValueDefaultFloat2(const char *propertyName, const float2 &defaultValue = float2::Zero) const;
    float3 GetPropertyValueDefaultFloat3(const char *propertyName, const float3 &defaultValue = float3::Zero) const;
    float4 GetPropertyValueDefaultFloat4(const char *propertyName, const float4 &defaultValue = float4::Zero) const;
    Quaternion GetPropertyValueDefaultQuaternion(const char *propertyName, const Quaternion &defaultValue = Quaternion::Identity) const;

    // typed property writers
    void SetPropertyValueBool(const char *propertyName, bool propertyValue);
    void SetPropertyValueInt8(const char *propertyName, int8 propertyValue);
    void SetPropertyValueInt16(const char *propertyName, int16 propertyValue);
    void SetPropertyValueInt32(const char *propertyName, int32 propertyValue);
    void SetPropertyValueInt64(const char *propertyName, int64 propertyValue);
    void SetPropertyValueUInt8(const char *propertyName, uint8 propertyValue);
    void SetPropertyValueUInt16(const char *propertyName, uint16 propertyValue);
    void SetPropertyValueUInt32(const char *propertyName, uint32 propertyValue);
    void SetPropertyValueUInt64(const char *propertyName, uint64 propertyValue);
    void SetPropertyValueFloat(const char *propertyName, float propertyValue);
    void SetPropertyValueDouble(const char *propertyName, double propertyValue);
    void SetPropertyValueColor(const char *propertyName, uint32 propertyValue);
    void SetPropertyValueInt2(const char *propertyName, const int2 &propertyValue);
    void SetPropertyValueInt3(const char *propertyName, const int3 &propertyValue);
    void SetPropertyValueInt4(const char *propertyName, const int4 &propertyValue);
    void SetPropertyValueUInt2(const char *propertyName, const uint2 &propertyValue);
    void SetPropertyValueUInt3(const char *propertyName, const uint3 &propertyValue);
    void SetPropertyValueUInt4(const char *propertyName, const uint4 &propertyValue);
    void SetPropertyValueFloat2(const char *propertyName, const float2 &propertyValue);
    void SetPropertyValueFloat3(const char *propertyName, const float3 &propertyValue);
    void SetPropertyValueFloat4(const char *propertyName, const float4 &propertyValue);
    void SetPropertyValueQuaternion(const char *propertyName, const Quaternion &propertyValue);

private:
    String m_entityName;
    String m_typeName;
    PropertyTable m_properties;
    String m_script;
    mutable bool m_changed;

    typedef PODArray<MapSourceEntityComponent *> ComponentArray;
    ComponentArray m_components;
};

