#include "Renderer/PrecompiledHeader.h"
#include "Renderer/ShaderProgram.h"
#include "Renderer/ShaderComponent.h"
#include "Renderer/VertexFactory.h"
#include "Renderer/Renderer.h"
#include "Engine/MaterialShader.h"
Log_SetChannel(ShaderProgram);

ShaderProgram::ShaderProgram(GPUShaderProgram *pGPUProgram, uint32 globalShaderFlags, const ShaderComponentTypeInfo *pBaseShaderTypeInfo, uint32 baseShaderFlags, const VertexFactoryTypeInfo *pVertexFactoryTypeInfo, uint32 vertexFactoryFlags, const MaterialShader *pMaterialShader, uint32 materialShaderFlags)
    : m_pBaseShaderTypeInfo(pBaseShaderTypeInfo),
      m_pVertexFactoryTypeInfo(pVertexFactoryTypeInfo),
      m_pMaterialShader(pMaterialShader),
      m_globalShaderFlags(globalShaderFlags),
      m_baseShaderFlags(baseShaderFlags),
      m_vertexFactoryFlags(vertexFactoryFlags),
      m_materialShaderFlags(materialShaderFlags),
      m_pGPUProgram(pGPUProgram),
      m_pBaseShaderParameterMap(nullptr),
      m_pVertexFactoryParameterMap(nullptr),
      m_pMaterialShaderUniformParameterMap(nullptr),
      m_pMaterialShaderTextureParameterMap(nullptr)
{
    GenerateBaseShaderParameterMap();

    if (m_pVertexFactoryTypeInfo != nullptr)
        GenerateVertexFactoryParameterMap();

    if (m_pMaterialShader != nullptr)
    {
        GenerateMaterialShaderUniformParameterMap();
        GenerateMaterialShaderTextureParameterMap();
    }
}

ShaderProgram::~ShaderProgram()
{
    delete[] m_pMaterialShaderTextureParameterMap;
    delete[] m_pMaterialShaderUniformParameterMap;
    delete[] m_pVertexFactoryParameterMap;
    delete[] m_pBaseShaderParameterMap;

    //m_pGPUProgram->Release();

    uint32 newRefCount = m_pGPUProgram->Release();
    DebugAssert(newRefCount == 0);
    UNREFERENCED_PARAMETER(newRefCount);
}

void ShaderProgram::GenerateBaseShaderParameterMap()
{
    uint32 nParameterBindings = m_pBaseShaderTypeInfo->GetParameterBindingCount();
    uint32 programParameterCount = m_pGPUProgram->GetParameterCount();
    if (nParameterBindings == 0 || programParameterCount == 0)
        return;

    const SHADER_COMPONENT_PARAMETER_BINDING *pParameterBinding = m_pBaseShaderTypeInfo->GetParameterBindings();
    m_pBaseShaderParameterMap = new int32[nParameterBindings];
    for (uint32 parameterBindingIndex = 0; parameterBindingIndex < nParameterBindings; parameterBindingIndex++, pParameterBinding++)
    {
        // lookup a parameter in the program, this needs to be optimized
        uint32 parameterIndex;
        for (parameterIndex = 0; parameterIndex < programParameterCount; parameterIndex++)
        {
            const char *parameterName;
            SHADER_PARAMETER_TYPE parameterType;
            uint32 arraySize;
            m_pGPUProgram->GetParameterInformation(parameterIndex, &parameterName, &parameterType, &arraySize);
            if (Y_strcmp(parameterName, pParameterBinding->ParameterName) == 0)
            {
                // check type
                if (parameterType != pParameterBinding->ExpectedType)
                    Log_WarningPrintf("ShaderMap::Program::GenerateBaseShaderParameterMap: Parameter '%s' type mismatch (expected %s got %s)", parameterName, NameTable_GetNameString(NameTables::ShaderParameterType, pParameterBinding->ExpectedType), NameTable_GetNameString(NameTables::ShaderParameterType, parameterType));

                break;
            }
        }

        if (parameterIndex != programParameterCount)
            m_pBaseShaderParameterMap[parameterBindingIndex] = static_cast<int32>(parameterIndex);
        else
            m_pBaseShaderParameterMap[parameterBindingIndex] = -1;
    }
}

void ShaderProgram::GenerateVertexFactoryParameterMap()
{
    uint32 nParameterBindings = m_pVertexFactoryTypeInfo->GetParameterBindingCount();
    uint32 programParameterCount = m_pGPUProgram->GetParameterCount();
    if (nParameterBindings == 0 || programParameterCount == 0)
        return;

    const SHADER_COMPONENT_PARAMETER_BINDING *pParameterBinding = m_pVertexFactoryTypeInfo->GetParameterBindings();
    m_pVertexFactoryParameterMap = new int32[nParameterBindings];
    for (uint32 parameterBindingIndex = 0; parameterBindingIndex < nParameterBindings; parameterBindingIndex++, pParameterBinding++)
    {
        // lookup a parameter in the program, this needs to be optimized
        uint32 parameterIndex;
        for (parameterIndex = 0; parameterIndex < programParameterCount; parameterIndex++)
        {
            const char *parameterName;
            SHADER_PARAMETER_TYPE parameterType;
            uint32 arraySize;
            m_pGPUProgram->GetParameterInformation(parameterIndex, &parameterName, &parameterType, &arraySize);
            if (Y_strcmp(parameterName, pParameterBinding->ParameterName) == 0)
            {
                // check type
                if (parameterType != pParameterBinding->ExpectedType)
                    Log_WarningPrintf("ShaderMap::Program::GenerateVertexFactoryParameterMap: Parameter '%s' type mismatch (expected %s got %s)", parameterName, NameTable_GetNameString(NameTables::ShaderParameterType, pParameterBinding->ExpectedType), NameTable_GetNameString(NameTables::ShaderParameterType, parameterType));

                break;
            }
        }

        if (parameterIndex != programParameterCount)
            m_pVertexFactoryParameterMap[parameterBindingIndex] = static_cast<int32>(parameterIndex);
        else
            m_pVertexFactoryParameterMap[parameterBindingIndex] = -1;
    }
}

void ShaderProgram::GenerateMaterialShaderUniformParameterMap()
{
    uint32 nUniformBindings = m_pMaterialShader->GetUniformParameterCount();
    uint32 programParameterCount = m_pGPUProgram->GetParameterCount();
    if (nUniformBindings == 0 || programParameterCount == 0)
        return;

    m_pMaterialShaderUniformParameterMap = new int32[nUniformBindings];

    for (uint32 uniformIndex = 0; uniformIndex < nUniformBindings; uniformIndex++)
    {
        const MaterialShader::UniformParameter *pUniformParameter = m_pMaterialShader->GetUniformParameter(uniformIndex);

        SmallString shaderParameterName;
        shaderParameterName.Format("MTLUniformParameter_%s", pUniformParameter->Name.GetCharArray());

        uint32 parameterIndex;
        for (parameterIndex = 0; parameterIndex < programParameterCount; parameterIndex++)
        {
            const char *parameterName;
            SHADER_PARAMETER_TYPE parameterType;
            uint32 arraySize;
            m_pGPUProgram->GetParameterInformation(parameterIndex, &parameterName, &parameterType, &arraySize);
            if (Y_strcmp(parameterName, shaderParameterName) == 0)
            {
                // check type
                if (parameterType != pUniformParameter->Type)
                    Log_WarningPrintf("ShaderMap::Program::GenerateMaterialShaderUniformParameterMap: Parameter '%s' type mismatch (expected %s got %s)", parameterName, NameTable_GetNameString(NameTables::ShaderParameterType, pUniformParameter->Type), NameTable_GetNameString(NameTables::ShaderParameterType, parameterType));

                break;
            }
        }

        if (parameterIndex != programParameterCount)
            m_pMaterialShaderUniformParameterMap[uniformIndex] = static_cast<int32>(parameterIndex);
        else
            m_pMaterialShaderUniformParameterMap[uniformIndex] = -1;
    }
}

void ShaderProgram::GenerateMaterialShaderTextureParameterMap()
{
    // get texture parameter count
    uint32 nTextureParameters = m_pMaterialShader->GetTextureParameterCount();
    uint32 programParameterCount = m_pGPUProgram->GetParameterCount();
    if (nTextureParameters == 0 || programParameterCount == 0)
        return;

    // post processed materials adds a few textures -- todo turn this into "material system parameters"
    static const char *postProcessParameterNames[] = { "MTLPostProcessParameter_SceneColor", "MTLPostProcessParameter_SceneDepth" };
    if (m_pMaterialShader->GetRenderMode() == MATERIAL_RENDER_MODE_POST_PROCESS)
        nTextureParameters += countof(postProcessParameterNames);

    // alloc
    m_pMaterialShaderTextureParameterMap = new int32[nTextureParameters];

    for (uint32 textureIndex = 0; textureIndex < m_pMaterialShader->GetTextureParameterCount(); textureIndex++)
    {
        const MaterialShader::TextureParameter *pTextureParameter = m_pMaterialShader->GetTextureParameter(textureIndex);

        SmallString shaderParameterName;
        shaderParameterName.Format("MTLTextureParameter_%s", pTextureParameter->Name.GetCharArray());

        uint32 parameterIndex;
        for (parameterIndex = 0; parameterIndex < programParameterCount; parameterIndex++)
        {
            const char *parameterName;
            SHADER_PARAMETER_TYPE parameterType;
            uint32 arraySize;
            m_pGPUProgram->GetParameterInformation(parameterIndex, &parameterName, &parameterType, &arraySize);
            if (Y_strcmp(parameterName, shaderParameterName) == 0)
            {
                /*// check type
                if (parameterType != pTextureParameter->Type)
                    Log_WarningPrintf("ShaderMap::Program::GenerateMaterialShaderTextureParameterMap: Parameter '%s' type mismatch (expected %s got %s)", parameterName, NameTable_GetNameString(NameTables::ShaderParameterType, pTextureParameter->Type), NameTable_GetNameString(NameTables::ShaderParameterType, parameterType));*/

                break;
            }
        }

        if (parameterIndex != programParameterCount)
            m_pMaterialShaderTextureParameterMap[textureIndex] = static_cast<int32>(parameterIndex);
        else
            m_pMaterialShaderTextureParameterMap[textureIndex] = -1;
    }

    // add post processed textures
    if (m_pMaterialShader->GetRenderMode() == MATERIAL_RENDER_MODE_POST_PROCESS)
    {
        for (uint32 i = 0; i < countof(postProcessParameterNames); i++)
        {
            uint32 parameterIndex;
            for (parameterIndex = 0; parameterIndex < programParameterCount; parameterIndex++)
            {
                const char *parameterName;
                SHADER_PARAMETER_TYPE parameterType;
                uint32 arraySize;
                m_pGPUProgram->GetParameterInformation(parameterIndex, &parameterName, &parameterType, &arraySize);
                if (Y_strcmp(parameterName, postProcessParameterNames[i]) == 0)
                    break;
            }

            if (parameterIndex != programParameterCount)
                m_pMaterialShaderTextureParameterMap[m_pMaterialShader->GetTextureParameterCount() + i] = static_cast<int32>(parameterIndex);
            else
                m_pMaterialShaderTextureParameterMap[m_pMaterialShader->GetTextureParameterCount() + i] = -1;
        }
    }
}

int32 ShaderProgram::Compare(const ShaderProgram *a, const ShaderProgram *b)
{
    ptrdiff_t diff;
    if ((diff = a->m_pBaseShaderTypeInfo - b->m_pBaseShaderTypeInfo) != 0)
        return (int32)diff;
    if ((diff = a->m_pVertexFactoryTypeInfo - b->m_pVertexFactoryTypeInfo) != 0)
        return (int32)diff;
    if ((diff = a->m_pMaterialShader - b->m_pMaterialShader) != 0)
        return (int32)diff;

    int32 fdiff;
    if ((fdiff = (int32)a->m_globalShaderFlags - (int32)b->m_globalShaderFlags) != 0)
        return (int32)diff;

    if ((fdiff = (int32)a->m_baseShaderFlags - (int32)b->m_baseShaderFlags) != 0)
        return (int32)diff;

    if ((fdiff = (int32)a->m_vertexFactoryFlags - (int32)b->m_vertexFactoryFlags) != 0)
        return (int32)diff;

    if ((fdiff = (int32)a->m_materialShaderFlags - (int32)b->m_materialShaderFlags) != 0)
        return (int32)diff;
    
    // Hashing would be better than this....
    return 0;
}

void ShaderProgram::SetBaseShaderParameterValue(GPUCommandList *pCommandList, uint32 index, SHADER_PARAMETER_TYPE type, const void *pValue) const
{
    DebugAssert(index < m_pBaseShaderTypeInfo->GetParameterBindingCount());
    if (m_pBaseShaderParameterMap[index] >= 0)
        pCommandList->SetShaderParameterValue(m_pBaseShaderParameterMap[index], type, pValue);
}

void ShaderProgram::SetBaseShaderParameterValueArray(GPUCommandList *pCommandList, uint32 index, SHADER_PARAMETER_TYPE type, const void *pValues, uint32 firstElement, uint32 numElements) const
{
    DebugAssert(index < m_pBaseShaderTypeInfo->GetParameterBindingCount());
    if (m_pBaseShaderParameterMap[index] >= 0)
        pCommandList->SetShaderParameterValueArray(m_pBaseShaderParameterMap[index], type, pValues, firstElement, numElements);
}

void ShaderProgram::SetBaseShaderParameterStruct(GPUCommandList *pCommandList, uint32 index, const void *pValue, uint32 valueSize) const
{
    DebugAssert(index < m_pBaseShaderTypeInfo->GetParameterBindingCount());
    if (m_pBaseShaderParameterMap[index] >= 0)
        pCommandList->SetShaderParameterStruct(m_pBaseShaderParameterMap[index], pValue, valueSize);
}

void ShaderProgram::SetBaseShaderParameterStructArray(GPUCommandList *pCommandList, uint32 index, const void *pValue, uint32 valueSize, uint32 firstElement, uint32 numElements) const
{
    DebugAssert(index < m_pBaseShaderTypeInfo->GetParameterBindingCount());
    if (m_pBaseShaderParameterMap[index] >= 0)
        pCommandList->SetShaderParameterStructArray(m_pBaseShaderParameterMap[index], pValue, valueSize, firstElement, numElements);
}

void ShaderProgram::SetBaseShaderParameterResource(GPUCommandList *pCommandList, uint32 index, GPUResource *pResource) const
{
    DebugAssert(index < m_pBaseShaderTypeInfo->GetParameterBindingCount());
    if (m_pBaseShaderParameterMap[index] >= 0)
        pCommandList->SetShaderParameterResource(m_pBaseShaderParameterMap[index], pResource);
}

void ShaderProgram::SetBaseShaderParameterTexture(GPUCommandList *pCommandList, uint32 index, GPUTexture *pTexture, GPUSamplerState *pSamplerState) const
{
    DebugAssert(index < m_pBaseShaderTypeInfo->GetParameterBindingCount());
    if (m_pBaseShaderParameterMap[index] >= 0)
        pCommandList->SetShaderParameterTexture(m_pBaseShaderParameterMap[index], pTexture, pSamplerState);
}

void ShaderProgram::SetVertexFactoryParameterValue(GPUCommandList *pCommandList, uint32 index, SHADER_PARAMETER_TYPE type, const void *pValue) const
{
    DebugAssert(index < m_pVertexFactoryTypeInfo->GetParameterBindingCount());
    if (m_pVertexFactoryParameterMap[index] >= 0)
        pCommandList->SetShaderParameterValue(m_pVertexFactoryParameterMap[index], type, pValue);
}

void ShaderProgram::SetVertexFactoryParameterValueArray(GPUCommandList *pCommandList, uint32 index, SHADER_PARAMETER_TYPE type, const void *pValues, uint32 firstElement, uint32 numElements) const
{
    DebugAssert(index < m_pVertexFactoryTypeInfo->GetParameterBindingCount());
    if (m_pVertexFactoryParameterMap[index] >= 0)
        pCommandList->SetShaderParameterValueArray(m_pVertexFactoryParameterMap[index], type, pValues, firstElement, numElements);
}

void ShaderProgram::SetVertexFactoryParameterStruct(GPUCommandList *pCommandList, uint32 index, const void *pValue, uint32 valueSize) const
{
    DebugAssert(index < m_pVertexFactoryTypeInfo->GetParameterBindingCount());
    if (m_pVertexFactoryParameterMap[index] >= 0)
        pCommandList->SetShaderParameterStruct(m_pVertexFactoryParameterMap[index], pValue, valueSize);
}

void ShaderProgram::SetVertexFactoryParameterStructArray(GPUCommandList *pCommandList, uint32 index, const void *pValue, uint32 valueSize, uint32 firstElement, uint32 numElements) const
{
    DebugAssert(index < m_pVertexFactoryTypeInfo->GetParameterBindingCount());
    if (m_pVertexFactoryParameterMap[index] >= 0)
        pCommandList->SetShaderParameterStructArray(m_pVertexFactoryParameterMap[index], pValue, valueSize, firstElement, numElements);
}

void ShaderProgram::SetVertexFactoryParameterResource(GPUCommandList *pCommandList, uint32 index, GPUResource *pResource) const
{
    DebugAssert(index < m_pVertexFactoryTypeInfo->GetParameterBindingCount());
    if (m_pVertexFactoryParameterMap[index] >= 0)
        pCommandList->SetShaderParameterResource(m_pVertexFactoryParameterMap[index], pResource);
}

void ShaderProgram::SetVertexFactoryParameterTexture(GPUCommandList *pCommandList, uint32 index, GPUTexture *pTexture, GPUSamplerState *pSamplerState) const
{
    DebugAssert(index < m_pVertexFactoryTypeInfo->GetParameterBindingCount());
    if (m_pVertexFactoryParameterMap[index] >= 0)
        pCommandList->SetShaderParameterTexture(m_pVertexFactoryParameterMap[index], pTexture, pSamplerState);
}

void ShaderProgram::SetMaterialParameterValue(GPUCommandList *pCommandList, uint32 index, SHADER_PARAMETER_TYPE type, const void *pValue) const
{
    DebugAssert(index < m_pMaterialShader->GetUniformParameterCount());
    if (m_pMaterialShaderUniformParameterMap[index] >= 0)
        pCommandList->SetShaderParameterValue(m_pMaterialShaderUniformParameterMap[index], type, pValue);
}

void ShaderProgram::SetMaterialParameterValueArray(GPUCommandList *pCommandList, uint32 index, SHADER_PARAMETER_TYPE type, const void *pValues, uint32 firstElement, uint32 numElements) const
{
    DebugAssert(index < m_pMaterialShader->GetUniformParameterCount());
    if (m_pMaterialShaderUniformParameterMap[index] >= 0)
        pCommandList->SetShaderParameterValueArray(m_pMaterialShaderUniformParameterMap[index], type, pValues, firstElement, numElements);
}

void ShaderProgram::SetMaterialParameterResource(GPUCommandList *pCommandList, uint32 index, GPUResource *pResource) const
{
    DebugAssert(index < m_pMaterialShader->GetTextureParameterCount());
    if (m_pMaterialShaderTextureParameterMap[index] >= 0)
        pCommandList->SetShaderParameterResource(m_pMaterialShaderTextureParameterMap[index], pResource);
}

void ShaderProgram::SetMaterialParameterTexture(GPUCommandList *pCommandList, uint32 index, GPUTexture *pTexture, GPUSamplerState *pSamplerState) const
{
    DebugAssert(index < m_pMaterialShader->GetTextureParameterCount() + (m_pMaterialShader->GetRenderMode() == MATERIAL_RENDER_MODE_POST_PROCESS) * 2);
    if (m_pMaterialShaderTextureParameterMap[index] >= 0)
        pCommandList->SetShaderParameterTexture(m_pMaterialShaderTextureParameterMap[index], pTexture, pSamplerState);
}

