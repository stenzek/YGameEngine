#include "ResourceCompiler/PrecompiledHeader.h"
#include "ResourceCompiler/ParticleSystemGenerator.h"
#include "ResourceCompiler/ClassTableGenerator.h"
#include "ResourceCompiler/ObjectTemplate.h"
#include "ResourceCompiler/ObjectTemplateManager.h"
#include "ResourceCompiler/ResourceCompiler.h"
#include "Engine/DataFormats.h"
#include "YBaseLib/XMLReader.h"
#include "YBaseLib/XMLWriter.h"
Log_SetChannel(ParticleSystemGenerator);

const char *ParticleSystemGenerator::Emitter::Properties::MaxActiveParticles = "MaxActiveParticles";
const char *ParticleSystemGenerator::Emitter::Properties::UpdateInterval = "UpdateInterval";
const char *ParticleSystemGenerator::Emitter::Properties::SpawnRate = "SpawnRate";
const char *ParticleSystemGenerator::Emitter::Properties::SpawnCount = "SpawnCount";

ParticleSystemGenerator::Module::Module(const char *typeName)
    : m_typeName(typeName)
{

}

ParticleSystemGenerator::Module::~Module()
{

}

ParticleSystemGenerator::Emitter::Emitter(const char *name, const char *typeName)
    : m_name(name),
      m_typeName(typeName)
{

}

ParticleSystemGenerator::Emitter::~Emitter()
{
    for (uint32 i = 0; i < m_modules.GetSize(); i++)
        delete m_modules[i];
}

const uint32 ParticleSystemGenerator::Emitter::GetMaxActiveParticles() const
{
    return m_properties.GetPropertyValueDefaultUInt32(Properties::MaxActiveParticles, 1);
}

const float ParticleSystemGenerator::Emitter::GetUpdateInterval() const
{
    return m_properties.GetPropertyValueDefaultFloat(Properties::UpdateInterval, 0.016f);
}

const float ParticleSystemGenerator::Emitter::GetSpawnRate() const
{
    return m_properties.GetPropertyValueDefaultFloat(Properties::SpawnRate, 1.0f);
}

const uint32 ParticleSystemGenerator::Emitter::GetSpawnCount() const
{
    return m_properties.GetPropertyValueDefaultUInt32(Properties::SpawnCount, 1);
}

void ParticleSystemGenerator::Emitter::SetName(const char *name)
{
    m_name = name;
}

void ParticleSystemGenerator::Emitter::SetMaxActiveParticles(uint32 maxActiveParticles)
{
    m_properties.SetPropertyValueUInt32(Properties::MaxActiveParticles, maxActiveParticles);
}

void ParticleSystemGenerator::Emitter::SetUpdateInterval(float updateInterval)
{
    m_properties.SetPropertyValueFloat(Properties::UpdateInterval, updateInterval);
}

void ParticleSystemGenerator::Emitter::SetSpawnRate(float spawnRate)
{
    m_properties.SetPropertyValueFloat(Properties::SpawnRate, spawnRate);
}

void ParticleSystemGenerator::Emitter::SetSpawnCount(uint32 spawnCount)
{
    m_properties.SetPropertyValueUInt32(Properties::MaxActiveParticles, spawnCount);
}

const uint32 ParticleSystemGenerator::Emitter::GetModuleCount() const
{
    return m_modules.GetSize();
}

const ParticleSystemGenerator::Module *ParticleSystemGenerator::Emitter::GetModuleByIndex(uint32 index) const
{
    return m_modules[index];
}

ParticleSystemGenerator::Module *ParticleSystemGenerator::Emitter::GetModuleByIndex(uint32 index)
{
    return m_modules[index];
}

void ParticleSystemGenerator::Emitter::AddModule(Module *pModule)
{
    m_modules.Add(pModule);
}

void ParticleSystemGenerator::Emitter::RemoveModule(Module *pModule)
{
    int32 index = m_modules.IndexOf(pModule);
    DebugAssert(index >= 0);
    m_modules.OrderedRemove(index);
}

ParticleSystemGenerator::ParticleSystemGenerator()
{

}

ParticleSystemGenerator::~ParticleSystemGenerator()
{
    for (uint32 i = 0; i < m_emitters.GetSize(); i++)
        delete m_emitters[i];
}

ParticleSystemGenerator::Emitter *ParticleSystemGenerator::CreateEmitter(const char *emitterName, const char *typeName)
{
    return new Emitter(emitterName, typeName);
}

ParticleSystemGenerator::Module *ParticleSystemGenerator::CreateModule(const char *typeName)
{
    return new Module(typeName);
}

bool ParticleSystemGenerator::LoadFromXML(const char *fileName, ByteStream *pStream)
{
    XMLReader xmlReader;
    if (!xmlReader.Create(pStream, fileName))
        return false;

    // find root element
    if (!xmlReader.SkipToElement("particlesystem"))
        return false;

    for (;;)
    {
        if (!xmlReader.NextToken())
            return false;

        if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
        {
            int32 particleSystemSelection = xmlReader.Select("emitter");
            if (particleSystemSelection < 0)
                return false;

            switch (particleSystemSelection)
            {
                // emitter
            case 0:
                {
                    const char *emitterNameStr = xmlReader.FetchAttribute("name");
                    const char *emitterTypeStr = xmlReader.FetchAttribute("type");
                    if (emitterNameStr == nullptr || emitterTypeStr == nullptr)
                    {
                        xmlReader.PrintError("missing emitter name/type");
                        return false;
                    }

                    if (xmlReader.IsEmptyElement())
                    {
                        xmlReader.PrintError("invalid emitter declaration");
                        return false;
                    }

                    Emitter *pEmitter = new Emitter(emitterNameStr, emitterTypeStr);
                    m_emitters.Add(pEmitter);

                    // extract properties and modules
                    for (;;)
                    {
                        if (!xmlReader.NextToken())
                            return false;

                        if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
                        {
                            int32 emitterSelection = xmlReader.Select("properties|modules");
                            if (emitterSelection < 0)
                                return false;

                            switch (emitterSelection)
                            {
                                // properties
                            case 0:
                                {
                                    if (!pEmitter->m_properties.LoadFromXML(xmlReader))
                                        return false;

                                    DebugAssert(xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT && Y_stricmp(xmlReader.GetNodeName(), "properties") == 0);
                                }
                                break;

                                // modules
                            case 1:
                                {
                                    if (!xmlReader.IsEmptyElement())
                                    {
                                        for (;;)
                                        {
                                            if (!xmlReader.NextToken())
                                                return false;

                                            if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
                                            {
                                                int32 modulesSelection = xmlReader.Select("module");
                                                if (modulesSelection < 0)
                                                    return false;

                                                const char *moduleTypeStr = xmlReader.FetchAttribute("type");
                                                if (moduleTypeStr == nullptr)
                                                {
                                                    xmlReader.PrintError("missing module type");
                                                    return false;
                                                }

                                                Module *pModule = new Module(moduleTypeStr);
                                                pEmitter->m_modules.Add(pModule);
                                                
                                                if (!xmlReader.IsEmptyElement())
                                                {
                                                    for (;;)
                                                    {
                                                        if (!xmlReader.NextToken())
                                                            return false;

                                                        if (xmlReader.GetTokenType() == XMLREADER_TOKEN_ELEMENT)
                                                        {
                                                            int32 moduleSelection = xmlReader.Select("properties");
                                                            if (moduleSelection < 0)
                                                                return false;

                                                            if (!pModule->m_properties.LoadFromXML(xmlReader))
                                                                return false;

                                                            DebugAssert(xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT && Y_stricmp(xmlReader.GetNodeName(), "properties") == 0);
                                                        }
                                                        else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
                                                        {
                                                            DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "module") == 0);
                                                            break;
                                                        }
                                                        else
                                                        {
                                                            UnreachableCode();
                                                            return false;
                                                        }
                                                    }
                                                }
                                            }
                                            else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
                                            {
                                                DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "modules") == 0);
                                                break;
                                            }
                                            else
                                            {
                                                UnreachableCode();
                                                return false;
                                            }
                                        }
                                    }
                                }
                                break;
                            }
                        }
                        else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
                        {
                            DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "emitter") == 0);
                            break;
                        }
                        else
                        {
                            UnreachableCode();
                            return false;
                        }
                    }

                    // sanity checks
                    if (pEmitter->GetModuleCount() == 0)
                    {
                        xmlReader.PrintError("Emitter declared without any modules");
                        return false;
                    }
                }
                break;
            }
        }
        else if (xmlReader.GetTokenType() == XMLREADER_TOKEN_END_ELEMENT)
        {
            DebugAssert(Y_stricmp(xmlReader.GetNodeName(), "particlesystem") == 0);
            break;
        }
    }

    // sanity check on system
    if (GetEmitterCount() == 0)
    {
        xmlReader.PrintError("particle system has no emitters");
        return false;
    }

    return true;
}

bool ParticleSystemGenerator::SaveToXML(ByteStream *pStream)
{
    return false;
}

const uint32 ParticleSystemGenerator::GetEmitterCount() const
{
    return m_emitters.GetSize();
}

const ParticleSystemGenerator::Emitter *ParticleSystemGenerator::GetEmitterByIndex(uint32 index) const
{
    return m_emitters[index];
}

ParticleSystemGenerator::Emitter *ParticleSystemGenerator::GetEmitterByIndex(uint32 index)
{
    return m_emitters[index];
}

const ParticleSystemGenerator::Emitter *ParticleSystemGenerator::GetEmitterByName(const char *name) const
{
    for (uint32 i = 0; i < m_emitters.GetSize(); i++)
    {
        if (m_emitters[i]->GetName().Compare(name))
            return m_emitters[i];
    }

    return nullptr;
}

ParticleSystemGenerator::Emitter *ParticleSystemGenerator::GetEmitterByName(const char *name)
{
    for (uint32 i = 0; i < m_emitters.GetSize(); i++)
    {
        if (m_emitters[i]->GetName().Compare(name))
            return m_emitters[i];
    }

    return nullptr;
}

bool ParticleSystemGenerator::Compile(ResourceCompilerCallbacks *pCallbacks, ByteStream *pStream) const
{
    // write particle system header
    uint64 streamStartOffset = pStream->GetPosition();
    uint64 endOffset;
    DF_PARTICLE_SYSTEM_HEADER systemHeader;
    systemHeader.Magic = DF_PARTICLE_SYSTEM_HEADER_MAGIC;
    systemHeader.HeaderSize = sizeof(systemHeader);
    systemHeader.ClassTableOffset = 0;
    systemHeader.ClassTableSize = 0;
    systemHeader.EmitterOffset = 0;
    systemHeader.EmitterCount = 0;
    if (!pStream->Write2(&systemHeader, sizeof(systemHeader)))
        return false;

    // create a class table
    ClassTableGenerator classTable;

    // for each emitter
    for (uint32 emitterIndex = 0; emitterIndex < m_emitters.GetSize(); emitterIndex++)
    {
        const Emitter *pEmitter = m_emitters[emitterIndex];

        // lookup the class
        const ObjectTemplate *pEmitterTemplate = ObjectTemplateManager::GetInstance().GetObjectTemplate(pCallbacks, pEmitter->GetTypeName());
        if (pEmitterTemplate == nullptr)
        {
            Log_ErrorPrintf("ParticleSystemGenerator::Compile: Emitter type '%s' is not known.", pEmitter->GetTypeName().GetCharArray());
            return false;
        }

        // does it exist in the class table?
        const ClassTableGenerator::Type *pEmitterClassTableType = classTable.GetTypeByName(pEmitterTemplate->GetTypeName());
        if (pEmitterClassTableType == nullptr)
        {
            // add it
            pEmitterClassTableType = classTable.CreateTypeFromPropertyTemplate(pEmitterTemplate->GetTypeName(), pEmitterTemplate->GetPropertyTemplate());
            if (pEmitterClassTableType == nullptr)
            {
                Log_ErrorPrintf("ParticleSystemGenerator::Compile: Failed to add type '%s' to class table.", pEmitter->GetTypeName().GetCharArray());
                return false;
            }
        }

        // add each of the modules
        for (uint32 moduleIndex = 0; moduleIndex < pEmitter->GetModuleCount(); moduleIndex++)
        {
            const Module *pModule = pEmitter->GetModuleByIndex(moduleIndex);

            // lookup the class
            const ObjectTemplate *pModuleTemplate = ObjectTemplateManager::GetInstance().GetObjectTemplate(pCallbacks, pModule->GetTypeName());
            if (pModuleTemplate == nullptr)
            {
                Log_ErrorPrintf("ParticleSystemGenerator::Compile: Module type '%s' is not known.", pModule->GetTypeName().GetCharArray());
                return false;
            }

            // does it exist in the class table?
            const ClassTableGenerator::Type *pModuleClassTableType = classTable.GetTypeByName(pModuleTemplate->GetTypeName());
            if (pModuleClassTableType == nullptr)
            {
                // add it
                pModuleClassTableType = classTable.CreateTypeFromPropertyTemplate(pModuleTemplate->GetTypeName(), pModuleTemplate->GetPropertyTemplate());
                if (pModuleClassTableType == nullptr)
                {
                    Log_ErrorPrintf("ParticleSystemGenerator::Compile: Failed to add type '%s' to class table.", pModule->GetTypeName().GetCharArray());
                    return false;
                }
            }
        }
    }

    // write the class table
    {
        uint64 classTableStartOffset = pStream->GetPosition();
        if (!classTable.Compile(pStream))
            return false;

        // update sizes
        systemHeader.ClassTableOffset = static_cast<uint32>(classTableStartOffset - streamStartOffset);
        systemHeader.ClassTableSize = static_cast<uint32>(pStream->GetPosition() - classTableStartOffset);
    }

    // update pointer to start of emitters
    systemHeader.EmitterOffset = static_cast<uint32>(pStream->GetPosition() - streamStartOffset);

    // for each emitter
    for (uint32 emitterIndex = 0; emitterIndex < m_emitters.GetSize(); emitterIndex++)
    {
        const Emitter *pEmitter = m_emitters[emitterIndex];

        // lookup the class
        const ObjectTemplate *pEmitterTemplate = ObjectTemplateManager::GetInstance().GetObjectTemplate(pCallbacks, pEmitter->GetTypeName());
        DebugAssert(pEmitterTemplate != nullptr);

        // does it exist in the class table?
        const ClassTableGenerator::Type *pEmitterClassTableType = classTable.GetTypeByName(pEmitterTemplate->GetTypeName());
        DebugAssert(pEmitterClassTableType != nullptr);

        // create the header for it
        uint64 emitterHeaderOffset = pStream->GetPosition();
        DF_PARTICLE_SYSTEM_EMITTER_HEADER emitterHeader;
        emitterHeader.TotalSize = sizeof(emitterHeader);
        emitterHeader.TypeIndex = pEmitterClassTableType->GetIndex();
        emitterHeader.PropertiesOffset = 0;
        emitterHeader.ModuleOffset = 0;
        emitterHeader.ModuleCount = 0;
        pStream->Write2(&emitterHeader, sizeof(emitterHeader));

        // write emitter object
        uint32 bytesWritten;
        emitterHeader.PropertiesOffset = static_cast<uint32>(pStream->GetPosition() - streamStartOffset);
        if (!classTable.SerializeObjectBinary(pStream, pEmitterTemplate->GetTypeName(), pEmitterTemplate->GetPropertyTemplate(), pEmitter->GetPropertyTable(), &emitterHeader.TypeIndex, &bytesWritten))
            return false;

        // update size
        emitterHeader.ModuleOffset = static_cast<uint32>(pStream->GetPosition() - streamStartOffset);
        emitterHeader.TotalSize += bytesWritten;

        // write each of the modules
        for (uint32 moduleIndex = 0; moduleIndex < pEmitter->GetModuleCount(); moduleIndex++)
        {
            const Module *pModule = pEmitter->GetModuleByIndex(moduleIndex);

            // lookup the class
            const ObjectTemplate *pModuleTemplate = ObjectTemplateManager::GetInstance().GetObjectTemplate(pCallbacks, pModule->GetTypeName());
            DebugAssert(pModuleTemplate != nullptr);

            // does it exist in the class table?
            const ClassTableGenerator::Type *pModuleClassTableType = classTable.GetTypeByName(pModuleTemplate->GetTypeName());
            DebugAssert(pModuleClassTableType != nullptr);

            // write module header
            uint64 moduleHeaderOffset = pStream->GetPosition();
            DF_PARTICLE_SYSTEM_MODULE_HEADER moduleHeader;
            moduleHeader.TotalSize = sizeof(moduleHeader);
            moduleHeader.TypeIndex = pModuleClassTableType->GetIndex();
            pStream->Write2(&moduleHeader, sizeof(moduleHeader));

            // write module object
            if (!classTable.SerializeObjectBinary(pStream, pModuleTemplate->GetTypeName(), pModuleTemplate->GetPropertyTemplate(), pModule->GetPropertyTable(), &moduleHeader.TypeIndex, &bytesWritten))
                return false;

            // update size
            moduleHeader.TotalSize += bytesWritten;
            emitterHeader.TotalSize += moduleHeader.TotalSize;
            emitterHeader.ModuleCount++;

            // fix up header
            endOffset = pStream->GetPosition();
            if (!pStream->SeekAbsolute(moduleHeaderOffset) || !pStream->Write2(&moduleHeader, sizeof(moduleHeader)) || !pStream->SeekAbsolute(endOffset))
                return false;
        }

        // fix up header
        endOffset = pStream->GetPosition();
        if (!pStream->SeekAbsolute(emitterHeaderOffset) || !pStream->Write2(&emitterHeader, sizeof(emitterHeader)) || !pStream->SeekAbsolute(endOffset))
            return false;

        // update emitter count
        systemHeader.EmitterCount++;
    }

    // rewrite system header
    endOffset = pStream->GetPosition();
    if (!pStream->SeekAbsolute(streamStartOffset) || !pStream->Write2(&systemHeader, sizeof(systemHeader)) || !pStream->SeekAbsolute(endOffset))
        return false;

    // done
    return true;
}

// Interface
BinaryBlob *ResourceCompiler::CompileParticleSystem(ResourceCompilerCallbacks *pCallbacks, const char *name)
{
    SmallString sourceFileName;
    sourceFileName.Format("%s.ParticleSystem.xml", name);

    BinaryBlob *pSourceData = pCallbacks->GetFileContents(sourceFileName);
    if (pSourceData == nullptr)
    {
        Log_ErrorPrintf("ResourceCompiler::CompileMaterialShader: Failed to read '%s'", sourceFileName.GetCharArray());
        return nullptr;
    }

    ByteStream *pStream = ByteStream_CreateReadOnlyMemoryStream(pSourceData->GetDataPointer(), pSourceData->GetDataSize());
    ParticleSystemGenerator *pGenerator = new ParticleSystemGenerator();
    if (!pGenerator->LoadFromXML(sourceFileName, pStream))
    {
        delete pGenerator;
        pStream->Release();
        pSourceData->Release();
        return nullptr;
    }

    pStream->Release();
    pSourceData->Release();

    ByteStream *pOutputStream = ByteStream_CreateGrowableMemoryStream();
    if (!pGenerator->Compile(pCallbacks, pOutputStream))
    {
        pOutputStream->Release();
        delete pGenerator;
        return nullptr;
    }

    BinaryBlob *pReturnBlob = BinaryBlob::CreateFromStream(pOutputStream);
    pOutputStream->Release();
    delete pGenerator;
    return pReturnBlob;
}
    

