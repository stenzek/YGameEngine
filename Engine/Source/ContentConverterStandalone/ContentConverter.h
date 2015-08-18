#include "ContentConverter/Common.h"
#include "ContentConverter/OBJImporter.h"
#include "ContentConverter/TextureImporter.h"
#include "ContentConverter/FontImporter.h"
#include "ContentConverter/AssimpSkeletonImporter.h"
#include "ContentConverter/AssimpSkeletalMeshImporter.h"
#include "ContentConverter/AssimpSkeletalAnimationImporter.h"
#include "ContentConverter/AssimpStaticMeshImporter.h"
#include "ContentConverter/AssimpSceneImporter.h"

int RunOBJImporter(int argc, char *argv[]);
bool ParseOBJConverterOption(OBJImporterOptions &Options, int &i, int argc, char *argv[]);

int RunTextureImporter(int argc, char *argv[]);
bool ParseTextureImporterOption(TextureImporterOptions &Options, int &i, int argc, char *argv[]);

int RunFontImporter(int argc, char *argv[]);
bool ParseFontImporterOption(FontImporterOptions &Options, int &i, int argc, char *argv[]);

int RunAssimpSkeletonImporter(int argc, char *argv[]);
bool ParseAssimpSkeletonImporterOption(AssimpSkeletonImporter::Options &Options, int &i, int argc, char *argv[]);

int RunAssimpSkeletalMeshImporter(int argc, char *argv[]);
bool ParseAssimpSkeletalMeshImporterOption(AssimpSkeletalMeshImporter::Options &Options, int &i, int argc, char *argv[]);

int RunAssimpSkeletalAnimationImporter(int argc, char *argv[]);
bool ParseAssimpSkeletalAnimationImporterOption(AssimpSkeletalAnimationImporter::Options &Options, int &i, int argc, char *argv[]);

int RunAssimpStaticMeshImporter(int argc, char *argv[]);
bool ParseAssimpStaticMeshImporterOption(AssimpStaticMeshImporter::Options &Options, int &i, int argc, char *argv[]);

int RunAssimpSceneImporter(int argc, char *argv[]);
bool ParseAssimpSceneImporterOption(AssimpStaticMeshImporter::Options &Options, int &i, int argc, char *argv[]);
