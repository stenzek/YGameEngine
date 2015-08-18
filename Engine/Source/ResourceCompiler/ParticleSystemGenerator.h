#pragma once
#include "ResourceCompiler/Common.h"
#include "Core/PropertyTable.h"

struct ResourceCompilerCallbacks;

class ParticleSystemGenerator
{
public:
    class Module
    {
        friend class ParticleSystemGenerator;

    public:
        Module(const char *typeName);
        ~Module();

        const String &GetTypeName() const { return m_typeName; }
        const PropertyTable *GetPropertyTable() const { return &m_properties; }
        PropertyTable *GetPropertyTable() { return &m_properties; }

    private:
        String m_typeName;
        PropertyTable m_properties;
    };

    class Emitter
    {
        class Properties
        {
        public:
            static const char *MaxActiveParticles;
            static const char *UpdateInterval;
            static const char *SpawnRate;
            static const char *SpawnCount;
        };

        friend class ParticleSystemGenerator;

    public:
        Emitter(const char *name, const char *typeName);
        ~Emitter();

        const String &GetName() const { return m_name; }
        const String &GetTypeName() const { return m_typeName; }
        const PropertyTable *GetPropertyTable() const { return &m_properties; }
        PropertyTable *GetPropertyTable() { return &m_properties; }

        const uint32 GetMaxActiveParticles() const;
        const float GetUpdateInterval() const;
        const float GetSpawnRate() const;
        const uint32 GetSpawnCount() const;

        void SetName(const char *name);
        void SetMaxActiveParticles(uint32 maxActiveParticles);
        void SetUpdateInterval(float updateInterval);
        void SetSpawnRate(float spawnRate);
        void SetSpawnCount(uint32 spawnCount);

        const uint32 GetModuleCount() const;
        const Module *GetModuleByIndex(uint32 index) const;
        Module *GetModuleByIndex(uint32 index);
        void AddModule(Module *pModule);
        void RemoveModule(Module *pModule);

    private:
        String m_name;
        String m_typeName;

        PropertyTable m_properties;

        PODArray<Module *> m_modules;
    };

 public:
    ParticleSystemGenerator();
    ~ParticleSystemGenerator();

    // creating emitters/modules
    static Emitter *CreateEmitter(const char *emitterName, const char *typeName);
    static Module *CreateModule(const char *typeName);

    // Loading/saving interface
    bool LoadFromXML(const char *fileName, ByteStream *pStream);
    bool SaveToXML(ByteStream *pStream);

    // emitter management
    const uint32 GetEmitterCount() const;
    const Emitter *GetEmitterByIndex(uint32 index) const;
    const Emitter *GetEmitterByName(const char *name) const;
    Emitter *GetEmitterByIndex(uint32 index);
    Emitter *GetEmitterByName(const char *name);

    // compiling
    bool Compile(ResourceCompilerCallbacks *pCallbacks, ByteStream *pStream) const;

private:
    PODArray<Emitter *> m_emitters;

};