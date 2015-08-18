#include "Engine/PrecompiledHeader.h"
#include "Engine/Common.h"

Y_Define_NameTable(NameTables::MaterialBlendingMode)
    Y_NameTable_Entry("None", MATERIAL_BLENDING_MODE_NONE)
    Y_NameTable_Entry("Additive", MATERIAL_BLENDING_MODE_ADDITIVE)
    Y_NameTable_Entry("Straight", MATERIAL_BLENDING_MODE_STRAIGHT)
    Y_NameTable_Entry("Premultiplied", MATERIAL_BLENDING_MODE_PREMULTIPLIED)
    Y_NameTable_Entry("Masked", MATERIAL_BLENDING_MODE_MASKED)
    Y_NameTable_Entry("SoftMasked", MATERIAL_BLENDING_MODE_SOFTMASKED)
Y_NameTable_End()

Y_Define_NameTable(NameTables::MaterialLightingType)
    Y_NameTable_Entry("Emissive", MATERIAL_LIGHTING_TYPE_EMISSIVE)
    Y_NameTable_Entry("Reflective", MATERIAL_LIGHTING_TYPE_REFLECTIVE)
    Y_NameTable_Entry("ReflectiveEmissive", MATERIAL_LIGHTING_TYPE_REFLECTIVE_EMISSIVE)
    Y_NameTable_Entry("ReflectiveTwoSided", MATERIAL_LIGHTING_TYPE_REFLECTIVE_TWO_SIDED)
Y_NameTable_End()

Y_Define_NameTable(NameTables::MaterialLightingModel)
    Y_NameTable_Entry("Phong", MATERIAL_LIGHTING_MODEL_PHONG)
    Y_NameTable_Entry("BlinnPhong", MATERIAL_LIGHTING_MODEL_BLINN_PHONG)
    Y_NameTable_Entry("PhysicallyBased", MATERIAL_LIGHTING_MODEL_PHYSICALLY_BASED)
    Y_NameTable_Entry("Custom", MATERIAL_LIGHTING_MODEL_CUSTOM)
Y_NameTable_End()

Y_Define_NameTable(NameTables::MaterialLightingNormalSpace)
    Y_NameTable_Entry("World", MATERIAL_LIGHTING_NORMAL_SPACE_WORLD_SPACE)
    Y_NameTable_Entry("Tangent", MATERIAL_LIGHTING_NORMAL_SPACE_TANGENT_SPACE)
Y_NameTable_End()

Y_Define_NameTable(NameTables::MaterialRenderMode)
    Y_NameTable_Entry("Normal", MATERIAL_RENDER_MODE_NORMAL)
    Y_NameTable_Entry("Wireframe", MATERIAL_RENDER_MODE_WIREFRAME)
    Y_NameTable_Entry("PostProcess", MATERIAL_RENDER_MODE_POST_PROCESS)
Y_NameTable_End()

Y_Define_NameTable(NameTables::MaterialRenderLayer)
    Y_NameTable_Entry("SkyBox", MATERIAL_RENDER_LAYER_SKYBOX)
    Y_NameTable_Entry("Normal", MATERIAL_RENDER_LAYER_NORMAL)
Y_NameTable_End()
