#include "ContentConverter/PrecompiledHeader.h"
#include "ContentConverter/OBJImporter.h"
#include "Engine/ResourceManager.h"
#include "ResourceCompiler/MaterialGenerator.h"
#include "ResourceCompiler/StaticMeshGenerator.h"
#include "Core/MeshUtilties.h"

// for map building
#include "MapCompiler/MapSource.h"
#include "ResourceCompiler/ObjectTemplate.h"
#include "ResourceCompiler/ObjectTemplateManager.h"

OBJImporter::OBJImporter(ProgressCallbacks *pProgressCallbacks)
    : BaseImporter(pProgressCallbacks)
{
    m_uSourceLineNumber = 0;
}

OBJImporter::~OBJImporter()
{

}

void OBJImporter::SetDefaultOptions(OBJImporterOptions *pOptions)
{
    pOptions->ImportMaterials = true;
    pOptions->DefaultMaterialName = "materials/engine/default";
    pOptions->BuildCollisionShapeType = "trianglemesh";
    pOptions->UseSmoothingGroups = true;
    pOptions->TransformMatrix.SetIdentity();
    pOptions->CoordinateSystem = COORDINATE_SYSTEM_Y_UP_RH;
    pOptions->FlipWinding = false;
    pOptions->CenterMeshes = StaticMeshGenerator::CenterOrigin_Count;
    pOptions->MergeGroups = true;
}

bool OBJImporter::Execute(const OBJImporterOptions *pOptions)
{
    Timer t;
    uint32 i;
    bool Result = false;

    if (pOptions->InputFileName.IsEmpty() ||
        pOptions->MeshDirectory.IsEmpty() ||
        (pOptions->ImportMaterials && pOptions->MaterialDirectory.IsEmpty()))
    {
        m_pProgressCallbacks->ModalError("One or more required parameters not set.");
        return false;
    }

    // store options
    m_pOptions = pOptions;
    m_pProgressCallbacks->SetProgressRange(6);
    m_pProgressCallbacks->SetProgressValue(0);

    m_pProgressCallbacks->SetStatusText("Parsing source...");
    m_pProgressCallbacks->PushState();
    if (!ParseSource())
    {
        m_pProgressCallbacks->PopState();
        goto EXIT;
    }
    m_pProgressCallbacks->PopState();
    m_pProgressCallbacks->SetProgressValue(1);

    m_pProgressCallbacks->DisplayFormattedInformation("ParseSource(): %.4f msec", t.GetTimeMilliseconds());
    t.Reset();

    m_pProgressCallbacks->SetStatusText("Parsing materials...");
    m_pProgressCallbacks->PushState();
    if (!ParseMaterials())
    {
        m_pProgressCallbacks->PopState();
        return false;;
    }
    m_pProgressCallbacks->PopState();
    m_pProgressCallbacks->SetProgressValue(2);

    m_pProgressCallbacks->DisplayFormattedInformation("ParseMaterials(): %.4f msec", t.GetTimeMilliseconds());
    t.Reset();

    m_pProgressCallbacks->SetStatusText("Generating materials...");
    m_pProgressCallbacks->PushState();
    if (!GenerateMaterials())
    {
        m_pProgressCallbacks->PopState();
        goto EXIT;
    }
    m_pProgressCallbacks->PopState();
    m_pProgressCallbacks->SetProgressValue(3);

    m_pProgressCallbacks->DisplayFormattedInformation("GenerateMaterials(): %.4f msec", t.GetTimeMilliseconds());
    t.Reset();

    if (m_pOptions->MergeGroups)
    {
        m_pProgressCallbacks->SetStatusText("Merging groups...");
        MergeGroups();

        m_pProgressCallbacks->DisplayFormattedInformation("MergeGroups(): %.4f msec", t.GetTimeMilliseconds());
        t.Reset();
    }

    m_pProgressCallbacks->SetStatusText("Generating triangles...");
    m_pProgressCallbacks->PushState();
    if (!GenerateTriangles())
    {
        m_pProgressCallbacks->PopState();
        goto EXIT;
    }
    m_pProgressCallbacks->PopState();
    m_pProgressCallbacks->SetProgressValue(4);

    m_pProgressCallbacks->DisplayFormattedInformation("GenerateTriangles(): %.4f msec", t.GetTimeMilliseconds());
    t.Reset();

    m_pProgressCallbacks->SetStatusText("Writing output...");
    m_pProgressCallbacks->PushState();
    if (!WriteOutput())
    {
        m_pProgressCallbacks->PopState();
        goto EXIT;
    }
    m_pProgressCallbacks->PopState();
    m_pProgressCallbacks->SetProgressValue(5);

    m_pProgressCallbacks->DisplayFormattedInformation("WriteOutput(): %.4f msec", t.GetTimeMilliseconds());
    t.Reset();

    m_pProgressCallbacks->SetStatusText("Generating map...");
    m_pProgressCallbacks->PushState();
    if (m_pOptions->OutputMapName.GetLength() > 0)
    {
        if (!WriteMap())
        {
            m_pProgressCallbacks->PopState();
            goto EXIT;
        }

        m_pProgressCallbacks->DisplayFormattedInformation("WriteMap(): %.4f msec", t.GetTimeMilliseconds());
        t.Reset();
    }
    m_pProgressCallbacks->PopState();
    m_pProgressCallbacks->SetProgressValue(6);

    Result = true;

EXIT:
    for (i = 0; i < m_arrSourceMaterialLibraries.GetSize(); i++)
        delete m_arrSourceMaterialLibraries[i];
    m_arrSourceMaterialLibraries.Clear();

    for (i = 0; i < m_arrSourceMaterialNames.GetSize(); i++)
        delete m_arrSourceMaterialNames[i];
    m_arrSourceMaterialNames.Clear();

    for (i = 0; i < m_arrSourceGroups.GetSize(); i++)
        delete m_arrSourceGroups[i];
    m_arrSourceGroups.Clear();

    for (i = 0; i < m_arrOutputMeshes.GetSize(); i++)
        delete m_arrOutputMeshes[i];
    m_arrOutputMeshes.Clear();

    m_arrOutputMaterials.Clear();

    return Result;
}

void OBJImporter::PrintSourceError(const char *Format, ...)
{
    va_list ap;
    va_start(ap, Format);

    String Message;
    Message.Format("%s:%u: ", m_pOptions->InputFileName.GetCharArray(), m_uSourceLineNumber);
    Message.AppendFormattedStringVA(Format, ap);

    va_end(ap);

    m_pProgressCallbacks->DisplayError(Message.GetCharArray());
}

void OBJImporter::PrintSourceWarning(const char *Format, ...)
{
    va_list ap;
    va_start(ap, Format);

    String Message;
    Message.Format("%s:%u: ", m_pOptions->InputFileName.GetCharArray(), m_uSourceLineNumber);
    Message.AppendFormattedStringVA(Format, ap);

    va_end(ap);

    m_pProgressCallbacks->DisplayWarning(Message.GetCharArray());
}

bool OBJImporter::ParseSource()
{
    uint32 i, j;

    ByteStream *pInputStream;
    if (!ByteStream_OpenFileStream(m_pOptions->InputFileName, BYTESTREAM_OPEN_READ, &pInputStream))
    {
        m_pProgressCallbacks->DisplayFormattedError("Could not open input file \"%s\"", m_pOptions->InputFileName.GetCharArray());
        return false;
    }

    TextReader Reader(pInputStream);
    char Line[1024];

    int32 CurrentMaterialIndex = -1;
    SourceGroup *pCurrentSourceGroup = NULL;
    uint32 CurrentSmoothingGroups = 1;
    uint32 TotalFaceCount = 0;
    bool Result = true;

    while (Reader.ReadLine(Line, countof(Line)))
    {
        m_pProgressCallbacks->UpdateProgressFromStream(pInputStream);
        m_uSourceLineNumber++;
        Y_strstrip(Line, "\r\n\t ");

        // skip comments
        if (Line[0] == '#')
            continue;

        // tokenize
        char *Tokens[128];
        uint32 nTokens;
        nTokens = Y_strsplit3(Line, ' ', Tokens, countof(Tokens));
        if (nTokens < 1)
            continue;

        // switch depending on token
        // reference material library
        if (!Y_stricmp(Tokens[0], "mtllib"))
        {
            if (nTokens < 2)
            {
                PrintSourceError("At least one material library must be provided.");
                Result = false;
                break;
            }

            for (i = 1; i < nTokens; i++)
                m_arrSourceMaterialLibraries.Add(new String(Tokens[i]));
        }
        // vertex declaration
        else if (!Y_stricmp(Tokens[0], "v"))
        {
            if (nTokens < 4)
            {
                PrintSourceError("Vertex must have three components.");
                Result = false;
                break;
            }

            float3 v;
            v.x = Y_strtofloat(Tokens[1]);
            v.y = Y_strtofloat(Tokens[2]);
            v.z = Y_strtofloat(Tokens[3]);

            m_arrSourceVertices.Add(v);
        }
        // texcoord declaration
        else if (!Y_stricmp(Tokens[0], "vt"))
        {
            if (nTokens < 3)
            {
                PrintSourceError("Texcoord must have three components.");
                Result = false;
                break;
            }

            float2 v;
            v.x = Y_strtofloat(Tokens[1]);
            v.y = Y_strtofloat(Tokens[2]);

            m_arrSourceTexCoords.Add(v);
        }
        // vertex normal declaration, ignore these for now
        else if (!Y_stricmp(Tokens[0], "vn"))
        {

        }
        // set face group
        else if (!Y_stricmp(Tokens[0], "g"))
        {
            if (nTokens < 2)
            {
                PrintSourceError("Expected a group name.");
                Result = false;
                break;
            }
            if (nTokens > 2)
            {
                PrintSourceError("More than one face group unsupported.");
                Result = false;
                break;
            }

            for (i = 0; i < m_arrSourceGroups.GetSize(); i++)
            {
                if (m_arrSourceGroups[i]->Name.CompareInsensitive(Tokens[1]))
                {
                    pCurrentSourceGroup = m_arrSourceGroups[i];
                    break;
                }
            }

            if (i == m_arrSourceGroups.GetSize())
            {
                pCurrentSourceGroup = new SourceGroup();
                pCurrentSourceGroup->Name.Assign(Tokens[1]);
                m_arrSourceGroups.Add(pCurrentSourceGroup);
            }
        }
        // set smoothing group
        else if (!Y_stricmp(Tokens[0], "s"))
        {
            if (nTokens < 2)
            {
                PrintSourceError("Expected a smoothing group mask or 'off'");
                Result = false;
                break;
            }

            if (!Y_stricmp(Tokens[1], "off"))
                CurrentSmoothingGroups = 0;
            else
                CurrentSmoothingGroups = Y_strtouint32(Tokens[1]);
        }
        // set face material
        else if (!Y_stricmp(Tokens[0], "usemtl"))
        {
            if (nTokens < 2)
            {
                PrintSourceError("A material name must be specified.");
                Result = false;
                break;
            }

            for (i = 0; i < m_arrSourceMaterialNames.GetSize(); i++)
            {
                if (m_arrSourceMaterialNames[i]->CompareInsensitive(Tokens[1]))
                    break;
            }

            if (i == m_arrSourceMaterialNames.GetSize())
                m_arrSourceMaterialNames.Add(new String(Tokens[1]));

            CurrentMaterialIndex = i;
        }
        // face declaration
        else if (!Y_stricmp(Tokens[0], "f"))
        {
            if (nTokens < 4)
            {
                PrintSourceError("A face must have at least three vertices.");
                Result = false;
                break;
            }
            if ((nTokens - 1) > OBJIMPORTER_MAX_VERTICES_PER_FACE)
            {
                PrintSourceError("Too many vertices in face (%u), max is %u", (nTokens - 1), (uint32)OBJIMPORTER_MAX_VERTICES_PER_FACE);
                Result = false;
                break;
            }

            // determine group
            if (pCurrentSourceGroup == NULL)
            {
                DebugAssert(m_arrSourceGroups.GetSize() == 0);

                pCurrentSourceGroup = new SourceGroup();
                pCurrentSourceGroup->Name = "Ungrouped";
                m_arrSourceGroups.Add(pCurrentSourceGroup);
            }

            // determine material index
            if (CurrentMaterialIndex < 0)
            {
                m_arrSourceMaterialNames.Add(new String());
                CurrentMaterialIndex = 0;
            }

            // determine num vertices
            uint32 NumVertices = nTokens - 1;

            // face struct
            SourceFace f;
            f.MaterialIndex = CurrentMaterialIndex;
            f.SmoothingGroups = CurrentSmoothingGroups;
            f.NumVertices = NumVertices;

            // components
            int32 VertexIndex[OBJIMPORTER_MAX_VERTICES_PER_FACE];
            //int32 NormalIndex[OBJIMPORTER_MAX_VERTICES_PER_FACE];
            int32 TexIndex[OBJIMPORTER_MAX_VERTICES_PER_FACE];
            //bool FaceHasNormal = true;
            bool FaceHasTex = true;

            // vertices
            char *ComponentTokens[3];
            for (i = 0; i < NumVertices; i++)
            {
                uint32 c = Y_strsplit(Tokens[i + 1], '/', ComponentTokens, countof(ComponentTokens));
                for (j = 0; j < c; j++)
                    Y_strstrip(ComponentTokens[j], " \r\n\t");

                if (c == 3)
                {
                    VertexIndex[i] = Y_strtoint32(ComponentTokens[0]);

                    // possible to have no texcoord index
                    if (*ComponentTokens[1] == '\0')
                        FaceHasTex = false;
                    else
                        TexIndex[i] = Y_strtoint32(ComponentTokens[1]);

                    //NormalIndex[i] = Y_strtoint32(ComponentTokens[2]);
                }
                else if (c == 2)
                {
                    //FaceHasNormal = false;
                    VertexIndex[i] = Y_strtoint32(ComponentTokens[0]);
                    TexIndex[i] = Y_strtoint32(ComponentTokens[1]);
                }
                else if (c == 1)
                {
                    //FaceHasNormal = false;
                    FaceHasTex = false;
                    VertexIndex[i] = Y_strtoint32(ComponentTokens[0]);
                }
                else
                {
                    PrintSourceError("Parse error in face.");
                    Result = false;
                    break;
                }
            }

            for (i = 0; i < NumVertices; i++)
            {
                if (VertexIndex[i] == 0 || VertexIndex[i] > (int32)m_arrSourceVertices.GetSize() || (VertexIndex[i] < 0 && ((-VertexIndex[i]) > (int32)m_arrSourceVertices.GetSize())))
                {
                    PrintSourceError("Vertex position index out of range.");
                    Result = false;
                    break;
                }

                f.VertexIndicies[i] = (VertexIndex[i] < 0) ? (m_arrSourceVertices.GetSize() - ((uint32)-VertexIndex[i])) : VertexIndex[i] - 1;

//                 if (FaceHasNormal)
//                 {
//                     if (NormalIndex[i] == 0 || NormalIndex[i] > (int32)m_arrVertexNormals.GetSize() || (NormalIndex[i] < 0 && ((-NormalIndex[i]) > (int32)m_arrVertexNormals.GetSize())))
//                     {
//                         PrintSourceError("Vertex normal index out of range.");
//                         return false;
//                     }
// 
//                     Face.NormalIndex[i] = (NormalIndex[i] < 0) ? (m_arrVertexNormals.GetSize() - ((uint32)-NormalIndex[i])) : NormalIndex[i] - 1;
//                 }
//                 else
//                 {
//                     Face.NormalIndex[i] = -1;
//                 }

                if (FaceHasTex)
                {
                    if (TexIndex[i] == 0 || TexIndex[i] > (int32)m_arrSourceTexCoords.GetSize() || (TexIndex[i] < 0 && ((-TexIndex[i]) > (int32)m_arrSourceTexCoords.GetSize())))
                    {
                        PrintSourceError("Vertex texcoord index out of range.");
                        Result = false;
                        break;
                    }

                    f.TexCoordIndices[i] = (TexIndex[i] < 0) ? (m_arrSourceTexCoords.GetSize() - ((uint32)-TexIndex[i])) : TexIndex[i] - 1;
                }
                else
                {
                    f.TexCoordIndices[i] = -1;
                }
            }

            for (; i < OBJIMPORTER_MAX_VERTICES_PER_FACE; i++)
            {
                f.VertexIndicies[i] = -1;
                f.TexCoordIndices[i] = -1;
            }

            // add it
            pCurrentSourceGroup->Faces.Add(f);
            TotalFaceCount++;
        }
        else
        {
            PrintSourceWarning("Unknown command: '%s'", Tokens[0]);
        }
    }

    pInputStream->Release();

    if (!Result)
        return false;

    m_pProgressCallbacks->DisplayFormattedInformation("Source has %u vertices, %u texcoords, %u faces in %u groups.", (uint32)m_arrSourceVertices.GetSize(), (uint32)m_arrSourceTexCoords.GetSize(), TotalFaceCount, (uint32)m_arrSourceGroups.GetSize());
    return true;
}

void OBJImporter::PrintMaterialError(uint32 i, const char *Format, ...)
{
    va_list ap;
    va_start(ap, Format);

    String Message;
    Message.Format("%s:%u: ", m_arrSourceMaterialLibraries[i]->GetCharArray(), m_uSourceLineNumber);
    Message.AppendFormattedStringVA(Format, ap);

    va_end(ap);

    m_pProgressCallbacks->DisplayError(Message.GetCharArray());
}

void OBJImporter::PrintMaterialWarning(uint32 i, const char *Format, ...)
{
    va_list ap;
    va_start(ap, Format);

    String Message;
    Message.Format("%s:%u: ", m_arrSourceMaterialLibraries[i]->GetCharArray(), m_uSourceLineNumber);
    Message.AppendFormattedStringVA(Format, ap);

    va_end(ap);

    m_pProgressCallbacks->DisplayWarning(Message.GetCharArray());
}

bool OBJImporter::ParseMaterials()
{
    uint32 i;

    for (i = 0; i < m_arrSourceMaterialLibraries.GetSize(); i++)
    {
        String MaterialLibraryPath;
        FileSystem::BuildPathRelativeToFile(MaterialLibraryPath, m_pOptions->InputFileName, m_arrSourceMaterialLibraries[i]->GetCharArray(), false, false);
        FileSystem::BuildOSPath(MaterialLibraryPath);

        ByteStream *pInputStream = FileSystem::OpenFile(MaterialLibraryPath.GetCharArray(), BYTESTREAM_OPEN_READ);
        if (pInputStream == NULL)
        {
            m_pProgressCallbacks->DisplayFormattedError("Could not open input material file \"%s\"", MaterialLibraryPath.GetCharArray());
            return false;
        }

        TextReader Reader(pInputStream);
        char Line[1024];

        SourceMaterial *pCurrentMaterial = NULL;
        bool Result = true;

        m_uSourceLineNumber = 0;
        while (Reader.ReadLine(Line, countof(Line)))
        {
            m_uSourceLineNumber++;
            Y_strstrip(Line, "\r\n\t ");

            // skip comments
            if (Line[0] == '#')
                continue;

            // tokenize
            char *Tokens[128];
            uint32 nTokens;
            nTokens = Y_strsplit3(Line, ' ', Tokens, countof(Tokens));
            if (nTokens < 1)
                continue;

            // switch depending on token
            // new material
            if (!Y_stricmp(Tokens[0], "newmtl"))
            {
                pCurrentMaterial = new SourceMaterial();
                pCurrentMaterial->Name.Assign(Tokens[1]);

                pCurrentMaterial->AmbientColor = float3::One;
                pCurrentMaterial->DiffuseColor = float3::One;
                pCurrentMaterial->SpecularColor = float3::One;
                pCurrentMaterial->EmissiveColor = float3::Zero;
                pCurrentMaterial->AlphaColor = 1.0f;
                pCurrentMaterial->Shininess = 10.0f;
                pCurrentMaterial->Illum = 2;

                // todo check dupe names
                m_arrSourceMaterials.Add(pCurrentMaterial);
                m_pProgressCallbacks->DisplayFormattedInformation("New source material: '%s'", pCurrentMaterial->Name.GetCharArray());
            }
            else
            {
                if (pCurrentMaterial == NULL)
                {
                    PrintMaterialError(i, "No material currently being defined.");
                    Result = false;
                    break;
                }

                // ambient color
                if (!Y_stricmp(Tokens[0], "Ka"))
                {
                    if (nTokens < 4)
                    {
                        PrintMaterialError(i, "Color has must three elements.");
                        Result = false;
                        break;
                    }

                    pCurrentMaterial->AmbientColor.x = Y_strtofloat(Tokens[1]);
                    pCurrentMaterial->AmbientColor.y = Y_strtofloat(Tokens[2]);
                    pCurrentMaterial->AmbientColor.z = Y_strtofloat(Tokens[3]);
                }
                // diffuse color
                else if (!Y_stricmp(Tokens[0], "Kd"))
                {
                    if (nTokens < 4)
                    {
                        PrintMaterialError(i, "Color has must three elements.");
                        Result = false;
                        break;
                    }

                    pCurrentMaterial->DiffuseColor.x = Y_strtofloat(Tokens[1]);
                    pCurrentMaterial->DiffuseColor.y = Y_strtofloat(Tokens[2]);
                    pCurrentMaterial->DiffuseColor.z = Y_strtofloat(Tokens[3]);
                }
                // specular color
                else if (!Y_stricmp(Tokens[0], "Ks"))
                {
                    if (nTokens < 4)
                    {
                        PrintMaterialError(i, "Color has must three elements.");
                        Result = false;
                        break;
                    }

                    pCurrentMaterial->SpecularColor.x = Y_strtofloat(Tokens[1]);
                    pCurrentMaterial->SpecularColor.y = Y_strtofloat(Tokens[2]);
                    pCurrentMaterial->SpecularColor.z = Y_strtofloat(Tokens[3]);
                }
                // shininess
                else if (!Y_stricmp(Tokens[0], "Ns"))
                {
                    if (nTokens < 2)
                    {
                        PrintMaterialError(i, "Missing parameter.");
                        Result = false;
                        break;
                    }

                    pCurrentMaterial->Shininess = Y_strtofloat(Tokens[1]);
                }
                // emissive color
                else if (!Y_stricmp(Tokens[0], "Ke"))
                {
                    if (nTokens < 4)
                    {
                        PrintMaterialError(i, "Color has must three elements.");
                        Result = false;
                        break;
                    }

                    pCurrentMaterial->EmissiveColor.x = Y_strtofloat(Tokens[1]);
                    pCurrentMaterial->EmissiveColor.y = Y_strtofloat(Tokens[2]);
                    pCurrentMaterial->EmissiveColor.z = Y_strtofloat(Tokens[3]);
                }
                // transparency
                else if (!Y_stricmp(Tokens[0], "d"))
                {
                    if (nTokens < 2)
                    {
                        PrintMaterialError(i, "Missing parameter.");
                        Result = false;
                        break;
                    }

                    pCurrentMaterial->AlphaColor = Y_strtofloat(Tokens[1]);
                }
                // illum
                else if (!Y_stricmp(Tokens[0], "illum"))
                {
                    if (nTokens < 2)
                    {
                        PrintMaterialError(i, "Missing parameter.");
                        Result = false;
                        break;
                    }

                    pCurrentMaterial->Illum = Y_strtouint32(Tokens[1]);
                }
                // ambient map
                else if (!Y_stricmp(Tokens[0], "map_Ka"))
                {
                    if (nTokens < 2)
                    {
                        PrintMaterialError(i, "Missing parameter.");
                        Result = false;
                        break;
                    }

                    FileSystem::BuildPathRelativeToFile(pCurrentMaterial->AmbientMap, MaterialLibraryPath.GetCharArray(), Tokens[1]);
                }
                // diffuse map
                else if (!Y_stricmp(Tokens[0], "map_Kd"))
                {
                    if (nTokens < 2)
                    {
                        PrintMaterialError(i, "Missing parameter.");
                        Result = false;
                        break;
                    }

                    FileSystem::BuildPathRelativeToFile(pCurrentMaterial->DiffuseMap, MaterialLibraryPath.GetCharArray(), Tokens[1]);
                }
                // specular map
                else if (!Y_stricmp(Tokens[0], "map_Ks"))
                {
                    if (nTokens < 2)
                    {
                        PrintMaterialError(i, "Missing parameter.");
                        Result = false;
                        break;
                    }

                    FileSystem::BuildPathRelativeToFile(pCurrentMaterial->SpecularMap, MaterialLibraryPath.GetCharArray(), Tokens[1]);
                }
                // alpha map
                else if (!Y_stricmp(Tokens[0], "map_d"))
                {
                    if (nTokens < 2)
                    {
                        PrintMaterialError(i, "Missing parameter.");
                        Result = false;
                        break;
                    }

                    FileSystem::BuildPathRelativeToFile(pCurrentMaterial->AlphaMap, MaterialLibraryPath.GetCharArray(), Tokens[1]);
                }
                // bump map
                else if (!Y_stricmp(Tokens[0], "map_bump") || !Y_stricmp(Tokens[0], "bump"))
                {
                    if (nTokens < 2)
                    {
                        PrintMaterialError(i, "Missing parameter.");
                        Result = false;
                        break;
                    }

                    FileSystem::BuildPathRelativeToFile(pCurrentMaterial->BumpMap, MaterialLibraryPath.GetCharArray(), Tokens[1]);
                }
                // unk
                else if (!Y_stricmp(Tokens[0], "Ni"))
                {

                }
                // unk
                else if (!Y_stricmp(Tokens[0], "Tr"))
                {

                }
                // unk
                else if (!Y_stricmp(Tokens[0], "Tf"))
                {

                }
                else
                {
                    PrintMaterialWarning(i, "Unknown command '%s'", Tokens[0]);
                }
            }
        }

        pInputStream->Release();

        if (!Result)
            return false;
    }
    
    return true;
}

bool OBJImporter::ImportMaterialTexture(const String &inputFileName, String &outputFullName, TEXTURE_USAGE usage, const String &maskTexture /* = EmptyString */)
{
    SmallString outputTextureName;
    PathString outputFileName;

    // copy name and remove extension
    int32 startLength = (int32)outputTextureName.GetLength();
    int32 lastSegment = Max(inputFileName.RFind('\\'), inputFileName.RFind('/'));
    if (lastSegment > 0)
        outputTextureName.AppendString(inputFileName.GetCharArray() + lastSegment + 1);
    else
        outputTextureName.AppendString(inputFileName);
    int32 extensionStartPos = outputTextureName.RFind('.');
    if (extensionStartPos >= startLength)
        outputTextureName.Erase(extensionStartPos);

    // sanitize name
    Resource::SanitizeResourceName(outputTextureName);
    
    // generate full name
    outputFullName.Format("%s/%s%s", m_pOptions->MaterialDirectory.GetCharArray(), m_pOptions->MaterialPrefix.GetCharArray(), outputTextureName.GetCharArray());
    outputFileName.Format("%s.tex.zip", outputFullName.GetCharArray());

    // see if it's already been written
    FILESYSTEM_STAT_DATA statData;
    if (g_pVirtualFileSystem->StatFile(outputFileName, &statData))
    {
        m_pProgressCallbacks->DisplayFormattedWarning("Texture '%s' already exists, not overwriting.", outputTextureName.GetCharArray());
        return true;
    }

    // log it
    m_pProgressCallbacks->DisplayFormattedInformation("Importing texture at '%s' as '%s'", inputFileName.GetCharArray(), outputFullName.GetCharArray());

    // build texture converter options
    TextureImporterOptions TIOptions;
    TextureImporter::SetDefaultOptions(&TIOptions);
    TIOptions.InputFileNames.Add(inputFileName);

    if (maskTexture.GetLength() > 0)
        TIOptions.InputMaskFileNames.Add(maskTexture);

    TIOptions.OutputName = outputFullName;
    TIOptions.Type = TEXTURE_TYPE_2D;
    TIOptions.Usage = usage;
    TIOptions.AddressU = TEXTURE_ADDRESS_MODE_WRAP;
    TIOptions.AddressV = TEXTURE_ADDRESS_MODE_WRAP;
    TIOptions.AddressW = TEXTURE_ADDRESS_MODE_WRAP;
    TIOptions.Optimize = false;
    TIOptions.Compile = false;

    // push state
    m_pProgressCallbacks->PushState();

    // create texture converter and run it
    TextureImporter TImporter(m_pProgressCallbacks);
    bool result = TImporter.Execute(&TIOptions);

    // pop state
    m_pProgressCallbacks->PopState();
    return result;
}

bool OBJImporter::GenerateMaterials()
{
    String fileName;

    // if we're not generating materials, we'll just create one, the default, and change all the indicies
    if (!m_pOptions->ImportMaterials)
    {
        m_arrOutputMaterials.Add(m_pOptions->DefaultMaterialName);

        for (uint32 i = 0; i < m_arrSourceGroups.GetSize(); i++)
        {
            for (uint32 j = 0; j < m_arrSourceGroups[i]->Faces.GetSize(); j++)
                m_arrSourceGroups[i]->Faces[j].MaterialIndex = 0;
        }

        return true;
    }

    // importing materials...
    for (uint32 i = 0; i < m_arrSourceMaterials.GetSize(); i++)
    {
        SourceMaterial *pSourceMaterial = m_arrSourceMaterials[i];

        // generate output material name
        SmallString outputMaterialName;
        outputMaterialName.AppendString(pSourceMaterial->Name);
        Resource::SanitizeResourceName(outputMaterialName);

        // append dir/prefix
        outputMaterialName.PrependFormattedString("%s/%s", m_pOptions->MaterialDirectory.GetCharArray(), m_pOptions->MaterialPrefix.GetCharArray());

        // build filename to material
        SmallString outputMaterialFileName;
        outputMaterialFileName.Format("%s.mtl.xml", outputMaterialName.GetCharArray());

        // does it already exist?
        FILESYSTEM_STAT_DATA statData;
        if (g_pVirtualFileSystem->StatFile(outputMaterialFileName, &statData))
        {
            m_pProgressCallbacks->DisplayFormattedWarning("Material '%s' already exists, not overwriting.", outputMaterialName.GetCharArray());
            pSourceMaterial->OutputName = outputMaterialName;
            continue;
        }

        SmallString DiffuseMapTextureName;
        SmallString SpecularMapTextureName;
        SmallString AlphaMapTextureName;
        SmallString NormalMapTextureName;

        // convert texture names and possibly import them
        if (pSourceMaterial->DiffuseMap.GetLength() > 0 && !ImportMaterialTexture(pSourceMaterial->DiffuseMap, DiffuseMapTextureName, TEXTURE_USAGE_COLOR_MAP, pSourceMaterial->AlphaMap))
            m_pProgressCallbacks->DisplayFormattedWarning("Failed to import diffuse map texture '%s'", pSourceMaterial->DiffuseMap.GetCharArray());
        if (pSourceMaterial->SpecularMap.GetLength() > 0 && !ImportMaterialTexture(pSourceMaterial->SpecularMap, SpecularMapTextureName, TEXTURE_USAGE_GLOSS_MAP))
            m_pProgressCallbacks->DisplayFormattedWarning("Failed to import specular map texture '%s'", pSourceMaterial->SpecularMap.GetCharArray());
        if (pSourceMaterial->BumpMap.GetLength() > 0 && !ImportMaterialTexture(pSourceMaterial->BumpMap, NormalMapTextureName, TEXTURE_USAGE_HEIGHT_MAP))
            m_pProgressCallbacks->DisplayFormattedWarning("Failed to import bump map texture '%s'", pSourceMaterial->AlphaMap.GetCharArray());

        // determine parent material
        MaterialGenerator materialGenerator;
        if ((AlphaMapTextureName.GetLength() > 0 || pSourceMaterial->AlphaColor != 1.0f))
        {
            if (NormalMapTextureName.GetLength() > 0)
                materialGenerator.Create("shaders/engine/simple_transparent_normalmap");
            else
                materialGenerator.Create("shaders/engine/simple_transparent");

            if (AlphaMapTextureName.GetLength() > 0)
            {
                materialGenerator.SetShaderStaticSwitchParameterByName("UseAlphaMap", true);
                materialGenerator.SetShaderTextureParameterStringByName("AlphaMap", AlphaMapTextureName.GetCharArray());
            }
            else
            {
                materialGenerator.SetShaderStaticSwitchParameterByName("UseAlphaMap", false);
                materialGenerator.SetShaderUniformParameterByName("AlphaCoefficient", SHADER_PARAMETER_TYPE_FLOAT, &pSourceMaterial->AlphaColor);
            }
        }
        else
        {
            if (NormalMapTextureName.GetLength() > 0)
                materialGenerator.Create("shaders/engine/simple_normalmap");
            else
                materialGenerator.Create("shaders/engine/simple");
        }

        // normalmaps
        if (NormalMapTextureName.GetLength() > 0)
            materialGenerator.SetShaderTextureParameterStringByName("NormalMap", NormalMapTextureName.GetCharArray());

        // diffusemaps
        if (DiffuseMapTextureName.GetLength() > 0)
        {
            materialGenerator.SetShaderStaticSwitchParameterByName("UseDiffuseMap", true);
            materialGenerator.SetShaderTextureParameterStringByName("DiffuseMap", DiffuseMapTextureName.GetCharArray());
        }
        else
        {
            materialGenerator.SetShaderStaticSwitchParameterByName("UseDiffuseMap", false);
            materialGenerator.SetShaderUniformParameterByName("DiffuseColor", SHADER_PARAMETER_TYPE_FLOAT3, &pSourceMaterial->DiffuseColor);
        }

        // specularmaps
        if (SpecularMapTextureName.GetLength() > 0)
        {
            materialGenerator.SetShaderStaticSwitchParameterByName("UseSpecularMap", true);
            materialGenerator.SetShaderTextureParameterStringByName("SpecularMap", SpecularMapTextureName.GetCharArray());
        }
        else
        {
            materialGenerator.SetShaderStaticSwitchParameterByName("UseSpecularMap", false);
            float sc = Max(pSourceMaterial->SpecularColor.x, Max(pSourceMaterial->SpecularColor.y, pSourceMaterial->SpecularColor.z));
            materialGenerator.SetShaderUniformParameterByName("SpecularCoefficient", SHADER_PARAMETER_TYPE_FLOAT, &sc);
        }
       
        // write material
        ByteStream *pStream = g_pVirtualFileSystem->OpenFile(outputMaterialFileName, BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE);
        if (pStream == NULL)
        {
            m_pProgressCallbacks->DisplayFormattedError("Could not open material file '%s' for writing.", outputMaterialFileName.GetCharArray());
            pSourceMaterial->OutputName = m_pOptions->DefaultMaterialName;
            continue;
        }

        // write it
        materialGenerator.SaveToXML(pStream);
        pStream->Release();

        // store name
        pSourceMaterial->OutputName = outputMaterialName;

        // log it
        m_pProgressCallbacks->DisplayFormattedInformation("Created OBJ material '%s' as '%s'", pSourceMaterial->Name.GetCharArray(), outputMaterialName.GetCharArray());
    }

    // bind to model
    for (uint32 i = 0; i < m_arrSourceMaterialNames.GetSize(); i++)
    {
        const SourceMaterial *pSourceMaterial = NULL;
        for (uint32 j = 0; j < m_arrSourceMaterials.GetSize(); j++)
        {
            if (m_arrSourceMaterials[j]->Name.CompareInsensitive(*m_arrSourceMaterialNames[i]))
            {
                pSourceMaterial = m_arrSourceMaterials[j];
                break;
            }
        }

        if (pSourceMaterial == NULL)
        {
            m_pProgressCallbacks->DisplayFormattedWarning("Could not find referenced material '%s'. Setting to default.", m_arrSourceMaterialNames[i]->GetCharArray());
            m_arrOutputMaterials.Add(m_pOptions->DefaultMaterialName);
            continue;
        }

        m_arrOutputMaterials.Add(pSourceMaterial->OutputName);
    }

    return true;
}

void OBJImporter::MergeGroups()
{
    DebugAssert(m_arrSourceGroups.GetSize() > 0);
    m_pProgressCallbacks->DisplayFormattedInformation("merging %u groups", m_arrSourceGroups.GetSize());

    SourceGroup *pMergedGroup = new SourceGroup();
    pMergedGroup->Name = "merged";

    for (uint32 i = 0; i < m_arrSourceGroups.GetSize(); i++)
    {
        m_pProgressCallbacks->DisplayFormattedInformation("  merging group '%s'", m_arrSourceGroups[i]->Name.GetCharArray());
        pMergedGroup->Faces.AddRange(m_arrSourceGroups[i]->Faces.GetBasePointer(), m_arrSourceGroups[i]->Faces.GetSize());
        delete m_arrSourceGroups[i];
    }

    m_arrSourceGroups.Clear();
    m_arrSourceGroups.Add(pMergedGroup);
}

bool OBJImporter::GenerateTriangles()
{
    uint32 totalVertexCount = 0;
    uint32 totalTriangleCount = 0;

    PODArray<uint32> GeneratedVertexSmoothingGroups;
    GeneratedVertexSmoothingGroups.Reserve(m_arrSourceVertices.GetSize());

    bool FlipWinding = false;
    float4x4 ConversionMatrix(float4x4::Identity);

    // coordinate system change
    if (m_pOptions->CoordinateSystem != ENGINE_COORDINATE_SYSTEM)
        ConversionMatrix = m_pOptions->TransformMatrix * float4x4::MakeCoordinateSystemConversionMatrix(m_pOptions->CoordinateSystem, ENGINE_COORDINATE_SYSTEM, &FlipWinding);
    else
        ConversionMatrix = m_pOptions->TransformMatrix;

    if (m_pOptions->FlipWinding)
        FlipWinding = !FlipWinding;

    m_pProgressCallbacks->SetProgressRange(m_arrSourceGroups.GetSize());
    m_pProgressCallbacks->SetProgressValue(0);

    for (uint32 i = 0; i < m_arrSourceGroups.GetSize(); i++)
    {
        const SourceGroup *pSourceGroup = m_arrSourceGroups[i];
        m_pProgressCallbacks->DisplayFormattedInformation("  processing group '%s'...", pSourceGroup->Name.GetCharArray());

        OutputMesh *pMesh = new OutputMesh();
        pMesh->Vertices.Reserve(m_arrSourceVertices.GetSize());
        pMesh->Triangles.Reserve(pMesh->Triangles.GetSize() + pSourceGroup->Faces.GetSize() * OBJIMPORTER_MAX_VERTICES_PER_FACE);
        pMesh->Translation.SetZero();
        m_arrOutputMeshes.Add(pMesh);
        GeneratedVertexSmoothingGroups.Clear();

        // generate mesh name
        pMesh->Name = pSourceGroup->Name;
        Resource::SanitizeResourceName(pMesh->Name);
        pMesh->Name.PrependFormattedString("%s/%s", m_pOptions->MeshDirectory.GetCharArray(), m_pOptions->MeshPrefix.GetCharArray());

        // add faces
        for (uint32 j = 0; j < pSourceGroup->Faces.GetSize(); j++)
        {
            const SourceFace *pSourceFace = &pSourceGroup->Faces[j];    

            // add vertices
            uint32 OutputVertexIndicies[OBJIMPORTER_MAX_VERTICES_PER_FACE];
            for (uint32 k = 0; k < pSourceFace->NumVertices; k++)
            {
                DebugAssert((uint32)pSourceFace->VertexIndicies[k] < m_arrSourceVertices.GetSize());
                float3 SourceVertex = m_arrSourceVertices[pSourceFace->VertexIndicies[k]];

                float2 SourceTexCoord = float2::Zero;
                if (pSourceFace->TexCoordIndices[k] >= 0)
                {
                    DebugAssert((uint32)pSourceFace->TexCoordIndices[k] < m_arrSourceTexCoords.GetSize());
                    SourceTexCoord = m_arrSourceTexCoords[pSourceFace->TexCoordIndices[k]];
                }

                // apply transform
                SourceVertex = (ConversionMatrix * float4(SourceVertex, 1.0f)).xyz();

                // if smoothing groups are enabled, search for a matching vertex, otherwise insert a new one
                uint32 l;
                if (m_pOptions->UseSmoothingGroups && pSourceFace->SmoothingGroups != 0)
                {
                    for (l = 0; l < pMesh->Vertices.GetSize(); l++)
                    {
                        if ((GeneratedVertexSmoothingGroups[l] & pSourceFace->SmoothingGroups) != 0 &&
                            pMesh->Vertices[l].Position == SourceVertex &&
                            pMesh->Vertices[l].TexCoord == SourceTexCoord)
                        {
                            GeneratedVertexSmoothingGroups[l] |= pSourceFace->SmoothingGroups;
                            break;
                        }
                    }
                }
                else
                {
                    l = pMesh->Vertices.GetSize();
                }

                if (l == pMesh->Vertices.GetSize())
                {
                    // add a new one
                    OutputVertex ov;
                    ov.Position = SourceVertex;
                    ov.TexCoord = SourceTexCoord;
                    pMesh->Vertices.Add(ov);
                    GeneratedVertexSmoothingGroups.Add(pSourceFace->SmoothingGroups);
                    totalVertexCount++;
                }

                // and store
                OutputVertexIndicies[k] = l;
            }

            // add indices
            for (uint32 k = 2; k < pSourceFace->NumVertices; k++)
            {
                OutputTriangle tri;

                if (FlipWinding)
                {
                    tri.Indices[0] = OutputVertexIndicies[0];
                    tri.Indices[1] = OutputVertexIndicies[k];
                    tri.Indices[2] = OutputVertexIndicies[k - 1];
                }
                else
                {
                    tri.Indices[0] = OutputVertexIndicies[0];
                    tri.Indices[1] = OutputVertexIndicies[k - 1];
                    tri.Indices[2] = OutputVertexIndicies[k];
                }

                tri.MaterialIndex = pSourceFace->MaterialIndex;

                pMesh->Triangles.Add(tri);
                totalTriangleCount++;
            }

            // reduce storage size
            pMesh->Vertices.Shrink();
            pMesh->Triangles.Shrink();
        }

        m_pProgressCallbacks->SetProgressValue(i);
    }

    m_pProgressCallbacks->DisplayFormattedInformation("Generated %u vertices, %u triangles.", totalVertexCount, totalTriangleCount);
    return true;
}

bool OBJImporter::WriteOutput()
{
    String outputMeshName;
    String outputMeshFileName;

    PODArray<int32> batchIndices;
    batchIndices.Reserve(m_arrOutputMaterials.GetSize());

    for (uint32 i = 0; i < m_arrOutputMeshes.GetSize(); i++)
    {
        OutputMesh *pOutputMesh = m_arrOutputMeshes[i];
        batchIndices.Clear();
        for (uint32 j = 0; j < m_arrOutputMaterials.GetSize(); j++)
            batchIndices.Add(-1);

        // create new mesh
        StaticMeshGenerator staticMeshGenerator;
        staticMeshGenerator.Create((m_arrSourceTexCoords.GetSize() > 0), false);
        uint32 lodIndex = staticMeshGenerator.AddLOD();

        // vertices
        const OutputVertex *pSourceVertex = pOutputMesh->Vertices.GetBasePointer();
        for (uint32 j = 0; j < pOutputMesh->Vertices.GetSize(); j++)
        {
            staticMeshGenerator.AddVertex(lodIndex, pSourceVertex->Position, float3::Zero, float3::Zero, float3::Zero, pSourceVertex->TexCoord, 0);
            pSourceVertex++;
        }

        // triangles
        const OutputTriangle *pSourceTriangle = pOutputMesh->Triangles.GetBasePointer();
        for (uint32 j = 0; j < pOutputMesh->Triangles.GetSize(); j++)
        {
            int32 batchIndex = batchIndices[pSourceTriangle->MaterialIndex];
            if (batchIndex < 0)
            {
                batchIndex = staticMeshGenerator.AddBatch(lodIndex, m_arrOutputMaterials[pSourceTriangle->MaterialIndex]);
                batchIndices[pSourceTriangle->MaterialIndex] = batchIndex;
            }

            staticMeshGenerator.AddTriangle(lodIndex, batchIndex, pSourceTriangle->Indices[0], pSourceTriangle->Indices[1], pSourceTriangle->Indices[2]);
            pSourceTriangle++;
        }

        // center mesh?
        if (m_pOptions->CenterMeshes != StaticMeshGenerator::CenterOrigin_Count)
            staticMeshGenerator.CenterMesh((StaticMeshGenerator::CenterOrigin)m_pOptions->CenterMeshes, &pOutputMesh->Translation);

        // calculate tangent space vectors, bounds and batches
        staticMeshGenerator.GenerateTangents(0);
        staticMeshGenerator.CalculateBounds();

        // build collision shape
        if (m_pOptions->BuildCollisionShapeType.CompareInsensitive("box"))
        {
            // build box
            staticMeshGenerator.BuildBoxCollisionShape();
        }
        else if (m_pOptions->BuildCollisionShapeType.CompareInsensitive("sphere"))
        {
            // build sphere
            staticMeshGenerator.BuildSphereCollisionShape();
        }
        else if (m_pOptions->BuildCollisionShapeType.CompareInsensitive("trianglemesh"))
        {
            // build triangle mesh
            staticMeshGenerator.BuildTriangleMeshCollisionShape();
        }
        else if (m_pOptions->BuildCollisionShapeType.CompareInsensitive("convexhull"))
        {
            // build convex hull
            staticMeshGenerator.BuildConvexHullCollisionShape();
        }
        else if (!m_pOptions->BuildCollisionShapeType.CompareInsensitive("none"))
        {
            m_pProgressCallbacks->DisplayFormattedWarning("invalid collision shape type: '%s', assuming none", m_pOptions->BuildCollisionShapeType.GetCharArray());
        }

        // write out
        outputMeshFileName.Format("%s.staticmesh.xml", pOutputMesh->Name.GetCharArray());
        ByteStream *pStream = g_pVirtualFileSystem->OpenFile(outputMeshFileName, BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE);
        if (pStream == NULL)
        {
            m_pProgressCallbacks->DisplayFormattedError("Could not open mesh output file '%s'", outputMeshFileName.GetCharArray());
            continue;
        }

        if (!staticMeshGenerator.SaveToXML(pStream))
        {
            m_pProgressCallbacks->DisplayFormattedError("Failed to save static mesh to XML file '%s'", outputMeshFileName.GetCharArray());
            pStream->Release();
            continue;
        }

        pStream->Release();
    }

    return true;
}

bool OBJImporter::WriteMap()
{
    // get template for StaticMeshBrush
    const ObjectTemplate *pStaticMeshTemplate = ObjectTemplateManager::GetInstance().GetObjectTemplate("StaticMeshBrush");
    if (pStaticMeshTemplate == nullptr)
    {
        m_pProgressCallbacks->DisplayError("Failed to get StaticMeshBrush template.");
        return false;
    }

    // create map
    MapSource *pMapSource = new MapSource();
    pMapSource->Create();

    // create objects for each mesh  
    for (uint32 i = 0; i < m_arrOutputMeshes.GetSize(); i++)
    {
        OutputMesh *pOutputMesh = m_arrOutputMeshes[i];

        // create entity
        MapSourceEntityData *pEntityData = pMapSource->CreateEntityFromTemplate(pStaticMeshTemplate);
        DebugAssert(pEntityData != nullptr);

        // set mesh name property
        pEntityData->SetPropertyValue("StaticMeshName", pOutputMesh->Name);

        // work out the transform
        float3 offsetPosition((pOutputMesh->Translation.SquaredLength() > 0.0f) ? -pOutputMesh->Translation : pOutputMesh->Translation);
        pEntityData->SetPropertyValueFloat3("Position", offsetPosition);
    }

    // write out the map
    bool result = pMapSource->SaveAs(m_pOptions->OutputMapName, m_pProgressCallbacks);
    delete pMapSource;
    return result;
}
