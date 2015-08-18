#include "Engine/PrecompiledHeader.h"
#include "Engine/ParticleSystem.h"
#include "Engine/DataFormats.h"
#include "ResourceCompiler/ParticleSystemGenerator.h"
#include "Core/ClassTable.h"

DEFINE_RESOURCE_TYPE_INFO(ParticleSystem);
DEFINE_RESOURCE_GENERIC_FACTORY(ParticleSystem);

ParticleSystem::ParticleSystem(const ResourceTypeInfo *pTypeInfo /*= &s_TypeInfo*/)
    : m_updateInterval(1.0f)
{

}

ParticleSystem::~ParticleSystem()
{
    for (uint32 i = 0; i < m_emitters.GetSize(); i++)
        delete m_emitters[i];
}

void ParticleSystem::AddEmitter(const ParticleSystemEmitter *pEmitter)
{
    m_emitters.Add(pEmitter);
    m_updateInterval = Min(m_updateInterval, pEmitter->GetUpdateInterval());
}

void ParticleSystem::RemoveEmitter(const ParticleSystemEmitter *pEmitter)
{
    int32 index = m_emitters.IndexOf(pEmitter);
    DebugAssert((uint32)index < m_emitters.GetSize());
    m_emitters.OrderedRemove(index);
}

// bool ParticleSystem::InitializeInstance(InstanceData *pInstanceData) const
// {
// 
// }
// 
// void ParticleSystem::CleanupInstance(InstanceData *pInstanceData) const
// {
// 
// }
// 
// void ParticleSystem::UpdateInstance(InstanceData *pInstanceData, float deltaTime) const
// {
// 
// }

bool ParticleSystem::LoadFromStream(const char *name, ByteStream *pStream)
{
    BinaryReader binaryReader(pStream);
    uint64 headerOffset = binaryReader.GetStreamPosition();

    // read header
    DF_PARTICLE_SYSTEM_HEADER particleSystemHeader;
    if (!binaryReader.SafeReadBytes(&particleSystemHeader, sizeof(particleSystemHeader)))
        return false;

    // check header
    if (particleSystemHeader.HeaderSize < sizeof(DF_PARTICLE_SYSTEM_HEADER) ||
        particleSystemHeader.Magic != DF_PARTICLE_SYSTEM_HEADER_MAGIC ||
        particleSystemHeader.EmitterCount == 0)
    {
        return false;
    }

    // load class table
    ClassTable classTable;
    if (!binaryReader.SafeSeekAbsolute(headerOffset + particleSystemHeader.ClassTableOffset) || !classTable.LoadFromStream(pStream, true))
        return false;

    // allocate emitters
    m_emitters.Reserve(particleSystemHeader.EmitterCount);
    if (!binaryReader.SafeSeekAbsolute(headerOffset + particleSystemHeader.EmitterOffset))
        return false;

    // load emitters
    for (uint32 emitterIndex = 0; emitterIndex < particleSystemHeader.EmitterCount; emitterIndex++)
    {
        DF_PARTICLE_SYSTEM_EMITTER_HEADER emitterHeader;
        uint64 emitterStartOffset = binaryReader.GetStreamPosition();
        if (!binaryReader.SafeReadBytes(&emitterHeader, sizeof(emitterHeader)))
            return false;

        // seek to emitter properties
        if (!binaryReader.SafeSeekAbsolute(emitterHeader.PropertiesOffset))
            return false;

        // create emitter object
        Object *pEmitterObject = classTable.UnserializeObject(pStream, emitterHeader.TypeIndex);
        ParticleSystemEmitter *pEmitter = (pEmitterObject != nullptr) ? pEmitterObject->SafeCast<ParticleSystemEmitter>() : nullptr;
        if (pEmitter == nullptr)
        {
            delete pEmitterObject;
            return false;
        }

        // reserve modules
        pEmitter->m_modules.Reserve(emitterHeader.ModuleCount);
        m_emitters.Add(pEmitter);

        // move to start of modules
        if (!binaryReader.SafeSeekAbsolute(emitterHeader.ModuleOffset))
            return false;

        // load modules
        for (uint32 moduleIndex = 0; moduleIndex < emitterHeader.ModuleCount; moduleIndex++)
        {
            DF_PARTICLE_SYSTEM_MODULE_HEADER moduleHeader;
            uint64 moduleStartOffset = binaryReader.GetStreamPosition();
            if (!binaryReader.SafeReadBytes(&moduleHeader, sizeof(moduleHeader)))
                return false;

            Object *pModuleObject = classTable.UnserializeObject(pStream, moduleHeader.TypeIndex);
            ParticleSystemModule *pModule = (pModuleObject != nullptr) ? pModuleObject->SafeCast<ParticleSystemModule>() : nullptr;
            if (pModuleObject == nullptr)
            {
                delete pModuleObject;
                return false;
            }

            pEmitter->m_modules.Add(pModule);

            // seek to next module
            if (!binaryReader.SafeSeekAbsolute(moduleStartOffset + moduleHeader.TotalSize))
                return false;
        }

        // update interval fixup
        m_updateInterval = (emitterIndex == 0) ? pEmitter->GetUpdateInterval() : Min(m_updateInterval, pEmitter->GetUpdateInterval());

        // seek to next emitter offset
        if (!binaryReader.SafeSeekAbsolute(emitterStartOffset + emitterHeader.TotalSize))
            return false;
    }

    // done
    m_strName = name;
    return true;
}
