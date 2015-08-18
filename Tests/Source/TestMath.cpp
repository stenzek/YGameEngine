#include "MathLib/Common.h"
#include "MathLib/Vectorf.h"
#include "MathLib/Matrixf.h"
#include "YBaseLib/String.h"
#include "YBaseLib/Timer.h"
#include "MathLib/Frustum.h"
#include "YBaseLib/Log.h"
#include "YBaseLib/StringConverter.h"
#include "MathLib/StringConverters.h"
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <algorithm>
Log_SetChannel(TestMath);

#include "YBaseLib/Functor.h"
#include "MathLib/Quaternion.h"

#include "YBaseLib/MemArray.h"
#include "YBaseLib/AutoReleasePtr.h"
#include "YBaseLib/ByteStream.h"
#include "Core/RandomNumberGenerator.h"
#include "Core/Image.h"
#include "Core/ImageCodec.h"
#include "YBaseLib/FileSystem.h"
#include "Core/MeshUtilties.h"

float randfloat(float fMax = 9999.0f)
{
    float fv = rand() / (float)RAND_MAX;
    return fv * fMax;
}

#if Y_COMPILER_MSVC
__declspec(noinline)
#endif
void printvec(const char *name, const Vector4f &v)
{
    printf("%s %f %f %f %f\n", name, v.x, v.y, v.z, v.w);
}

#if Y_COMPILER_MSVC
__declspec(noinline)
#endif
void printmat(const char *name, const Matrix4x4f &m)
{
    printvec(name, m[0]);
    printvec(name, m[1]);
    printvec(name, m[2]);
    printvec(name, m[3]);
}

class test
{
public:
    void method() { printf("blah\n"); }
    void method1(uint32 n) { printf("blah %u\n", n); }
};

//__declspec(align(16)) struct aligned16
//{
//    float x[4];
//};

//int main(int argc, char *argv[])
//{
//     {
//         Vector4 v1 = Vector4(5, 6, 7, 8);
//         Vector4 v2 = Vector4(5, 6, 7, 8);
// 
//         Vector4 acc1 = Vector4::Zero;
//         Vector4 acc2 = Vector4(0, 0, 0, 0);
// 
//         Timer t;
//         uint32 i;
// 
//         t.Reset();
//         for (i = 0; i < 50000000; i++)
//             acc1 += v1.Normalize();
//         printf("%.4f %f %f %f %f\n", t.GetTimeMilliseconds(), acc1.x, acc1.y, acc1.z, acc1.w);
// 
//         t.Reset();
//         for (i = 0; i < 50000000; i++)
//             acc2 += v2.Normalize();
//         printf("%.4f %f %f %f %f\n", t.GetTimeMilliseconds(), acc2.x, acc2.y, acc2.z, acc2.w);
//     }

//     Vector4 test(1, 2, 3, 4);
//     Vector2 test3 = test.xy();
//     Vector3 test2 = test.xyz();

//     test *ptest = new test();
//     Functor *pf = MakeFunctor(ptest, &test::method);
//     Functor *pf1 = MakeFunctor(ptest, &test::method1, (uint32)1);
// 
//     pf->Invoke();
//     pf1->Invoke();
// 
//     pf1->Release();
//     pf->Release();
//     delete ptest;

    //Matrix4 m1 = Make4x4VerticalFoVPerspectiveProjectionMatrix(Math::DegreesToRadians(60.0f), 640, 480, 1.0f, 100.0f);
    //Matrix4 m2 = Make4x4VerticalFoVPerspectiveProjectionMatrix(Math::DegreesToRadians(60.0f), 640, 480, 1.0f + 0.001f, 100.0f + 0.001f);
    //Matrix4 m3 = Make4x4VerticalFoVPerspectiveProjectionMatrix(Math::DegreesToRadians(60.0f), 640, 480, 1.0f + 0.5f, 100.0f + 0.5f);

    //float t1 = m1.m22 + 0.001f * m1.m22;
    //float t2 =  0.001f * m1.m23;

//     float f1_12 = m2.m22 / m1.m22;
//     float f2_12 = m2.m23 / m1.m23;
//     float f1_13 = m3.m22 / m1.m22;
//     float f2_13 = m3.m23 / m1.m23;
// 
//     float np2 = -m2.m23 / m2.m22;
//     float fp2 = m2.m23 / (1.0f - m2.m22);
// 
//     Frustum f;
//     f.SetFromMatrix(m3);

    //Quaternion q = Quaternion::FromRotationYXZ(Math::DegreesToRadians(-90.0f), 0.0f, 0.0f);

//     Timer t;
// 
//     Vector3 temp(StringConverter::StringToVector3("1.0 2.0 3.0"));
//     Vector3 tempa(temp * temp);
//     Vector3 tempb((tempa * tempa).Normalize());
// 
//     for (uint32 i = 0; i < 100000000; i++)
//     {
//         tempb *= tempa;
//         tempa /= tempb;
//         tempb += tempa;
//     }
// 
//     float3 temp_f(tempb.GetFloat3());
//     printf("%f %f %f\n", temp_f.x, temp_f.y, temp_f.z);
//     printf("%.4fmsec\n", t.GetTimeMilliseconds());

//     float3 v(Math::DegreesToRadians(50),Math::DegreesToRadians(100), Math::DegreesToRadians(270));
// 
//     //Quaternion q(Quaternion::FromEulerAngles(v).Normalize());
//     //Quaternion q(-0.683013, -0.298836, 0.640856, -0.183013);
//     Quaternion q(-0.707106829, 0.0f, 0.0f, 0.707106829f);
//     q.NormalizeInPlace();
//     //float4x4 m1(float4x4::MakeRotationFromEulerAnglesMatrix(v));
//     float4x4 mq(q.GetFloat4x4());
//     //printmat("m1", m1);
//     printmat("mq", mq);
//     Quaternion mq_q(Quaternion::FromFloat4x4(mq));
// 
//     float3 src(1, 2, 3);
//     float3 r1(q * src);
//     float3 r1q(mq.TransformPoint(src));
//     float3 r2(mq_q * src);


    //Quaternion quat(Quaternion::FromEulerAngles(50.0f, 0.0f, 0.0f));
    //float3 euler(quat.GetEulerAngles());
//     euler.x = Math::RadiansToDegrees(euler.x);
//     euler.y = Math::RadiansToDegrees(euler.y);
//     euler.z = Math::RadiansToDegrees(euler.z);

//     int x = 5;
//     Quaternion temp(Quaternion::Identity);
//     aligned16 a16;
// 
//     auto func = [x, temp, a16]()
//     {
//         printf("%i", x);
//     };
// 
//     func();
//     int y = sizeof(func);
//     //void(*fptr)() = func;
//     std::function<void()> fptr = func;
//     int z = sizeof(fptr);
//     fptr();
// 
//     return 0;
// }

// class cb_base
// {
// public:
//     virtual ~cb_base() {}
//     virtual void execute() = 0;
// };
// 
// template<class T>
// class cb_impl : public cb_base
// {
//     T val;
// 
// public:
//     cb_impl(const T & val_) : val(val_) {}
// 
//     virtual void execute()
//     {
//         val();
//     }
// };
// 
// struct q_entry
// {
//     cb_base *ptr;
//     int size;
// };
// 
// MemArray<cb_base *> q_entries;
// 
// template<class T>
// void add_to_q(const T &val)
// {
//     //cb_impl<T> *vp = new cb_impl<T>(val);
//     cb_impl<T> *vp = (cb_impl<T> *)malloc(sizeof(cb_impl<T>));
//     new (vp)cb_impl<T>(val);
//     q_entries.Add(vp);
// }
// 
// void drain_q()
// {
//     printf("drain_q()\n");
//     while (q_entries.GetSize() > 0)
//     {
//         cb_base *vp;
//         q_entries.PopFront(&vp);
//         vp->execute();
//         //delete vp;
//         vp->~cb_base();
//         free(vp);
//     }
// }
// 
// void inner()
// {
//     Quaternion q(Quaternion::Identity);
//     float3 v(float3::One);
//     AutoReleasePtr<ByteStream> s = ByteStream_CreateGrowableMemoryStream();
// 
//     add_to_q([q, v, s]() {
//         printf("inside lambda, %f %f %f\n", v.x, v.y, v.z);
//         //bool b = s.IsNull();
//     });
// }
// 
// static std::initializer_list<int> i_x = { 5, 10, 15, 20 };
// 
// int main(int argc, char *argv[])
// {
//     //inner();
//     //drain_q();
// 
//     int c = 0;
//     for (size_t i = 0; i < i_x.size(); i++)
//         c += *(i_x.begin() + i);
// 
//     printf("%d", c);
//     return 0;
// }

// int main(int argc, char *argv[])
// {
//     RandomNumberGenerator rng;
//     /*for (uint32 i = 0; i < 32; i++)
//     {
//         float x = rng.NextRangeFloat(-1.0f, 1.0f);
//         float y = rng.NextRangeFloat(-1.0f, 1.0f);
//         float z = rng.NextRangeFloat(0.0f, 1.0f);
//         float3 val(x, y, z);
//         val.NormalizeInPlace();
//         printf("float3(%f, %f, %f),\n", val.x, val.y, val.z);
//     }*/
// 
//     Image img;
//     uint32 sz = 64;
//     img.Create(PIXEL_FORMAT_R8G8B8A8_UNORM, sz, sz, 1);
//     for (uint32 y = 0; y < sz; y++)
//     {
//         byte *pData = img.GetData() + (y * img.GetDataRowPitch());
//         for (uint32 x = 0; x < sz; x++)
//         {
//             float nx = rng.NextRangeFloat(-1.0f, 1.0f);
//             float ny = rng.NextRangeFloat(-1.0f, 1.0f);
//             float3 nrm(nx, ny, 0.0f);
//             nrm.NormalizeInPlace();
// 
//             nrm *= 0.5f;
//             nrm += 0.5f;
//             *pData++ = (byte)Math::Truncate(Math::Clamp(nrm.x * 255.0f, 0.0f, 255.0f));
//             *pData++ = (byte)Math::Truncate(Math::Clamp(nrm.y * 255.0f, 0.0f, 255.0f));
//             *pData++ = (byte)Math::Truncate(Math::Clamp(nrm.z * 255.0f, 0.0f, 255.0f));
//             *pData++ = 255;
//         }
//     }
// 
//     const char *filename = "D:\\random.png";
//     ImageCodec *codec = ImageCodec::GetImageCodecForFileName(filename);
//     ByteStream *pStream = FileSystem::OpenFile(filename, BYTESTREAM_OPEN_CREATE | BYTESTREAM_OPEN_WRITE | BYTESTREAM_OPEN_TRUNCATE | BYTESTREAM_OPEN_SEEKABLE);
//     codec->EncodeImage(filename, pStream, &img);
//     pStream->Release();
// 
//     return 0;
// }

int main__(int argc, char *argv[])
{
    Log::GetInstance().SetConsoleOutputParams(true);
    Log::GetInstance().SetDebugOutputParams(true);

    /*{
        float4x4 rotMat(float4x4::MakeRotationMatrixZ(90.0f));
        float4x4 preTransMat(float4x4::MakeTranslationMatrix(-0.5f, -0.5f, -0.5f));
        float4x4 postTransMat(float4x4::MakeTranslationMatrix(0.5f, 0.5f, 0.5f));
        float4x4 resMat(postTransMat * rotMat * preTransMat);
        Log_DevPrintf("{ { %.6ff, %.6ff, %.6ff, %.6ff }, { %.6ff, %.6ff, %.6ff, %.6ff }, { %.6ff, %.6ff, %.6ff, %.6ff } }", resMat.m00, resMat.m01, resMat.m02, resMat.m03, resMat.m10, resMat.m11, resMat.m12, resMat.m13, resMat.m20, resMat.m21, resMat.m22, resMat.m23);
    }

    {
        float4x4 rotMat(float4x4::MakeRotationMatrixZ(180.0f));
        float4x4 preTransMat(float4x4::MakeTranslationMatrix(-0.5f, -0.5f, -0.5f));
        float4x4 postTransMat(float4x4::MakeTranslationMatrix(0.5f, 0.5f, 0.5f));
        float4x4 resMat(postTransMat * rotMat * preTransMat);
        Log_DevPrintf("{ { %.6ff, %.6ff, %.6ff, %.6ff }, { %.6ff, %.6ff, %.6ff, %.6ff }, { %.6ff, %.6ff, %.6ff, %.6ff } }", resMat.m00, resMat.m01, resMat.m02, resMat.m03, resMat.m10, resMat.m11, resMat.m12, resMat.m13, resMat.m20, resMat.m21, resMat.m22, resMat.m23);
    }

    {
        float4x4 rotMat(float4x4::MakeRotationMatrixZ(270.0f));
        float4x4 preTransMat(float4x4::MakeTranslationMatrix(-0.5f, -0.5f, -0.5f));
        float4x4 postTransMat(float4x4::MakeTranslationMatrix(0.5f, 0.5f, 0.5f));
        float4x4 resMat(postTransMat * rotMat * preTransMat);
        Log_DevPrintf("{ { %.6ff, %.6ff, %.6ff, %.6ff }, { %.6ff, %.6ff, %.6ff, %.6ff }, { %.6ff, %.6ff, %.6ff, %.6ff } }", resMat.m00, resMat.m01, resMat.m02, resMat.m03, resMat.m10, resMat.m11, resMat.m12, resMat.m13, resMat.m20, resMat.m21, resMat.m22, resMat.m23);
    }*/

    static const float CUBE_FACE_TANGENTS[CUBE_FACE_COUNT][3] =
    {
        { 0, -1, 0 },       // RIGHT
        { 0, -1, 0 },       // LEFT
        { 1, 0, 0 },        // BACK
        { 1, 0, 0 },        // FRONT
        { 1, 0, 0 },        // TOP
        { 1, 0, 0 },        // BOTTOM
    };

    static const float CUBE_FACE_BINORMALS[CUBE_FACE_COUNT][3] =
    {
        { 0, 0, -1 },       // RIGHT
        { 0, 0, -1 },       // LEFT
        { 0, 0, -1 },       // BACK
        { 0, 0, -1 },       // FRONT
        { 0, -1, 0 },       // TOP
        { 0, -1, 0 },       // BOTTOM
    };

    static const float CUBE_FACE_NORMALS[CUBE_FACE_COUNT][3] =
    {
        { 1, 0, 0 },        // RIGHT
        { -1, 0, 0 },       // LEFT
        { 0, 1, 0 },        // BACK
        { 0, -1, 0 },       // FRONT
        { 0, 0, 1 },        // TOP
        { 0, 0, -1 },       // BOTTOM
    };

    for (uint32 i = 0; i < 6; i++)
    {
        Vector3f outTangent;
        float outBinormalSign;
        MeshUtilites::OrthogonalizeTangent(CUBE_FACE_TANGENTS[i], CUBE_FACE_BINORMALS[i], CUBE_FACE_NORMALS[i], outTangent, outBinormalSign);

        uint32 packedTangentAndSign, packedNormal;
        union
        {
            uint32 asUInt32;
            int8 asInt8[4];
        } converter;
        converter.asInt8[0] = (int8)Math::Clamp(outTangent.x * 127.0f, -127.0f, 127.0f);
        converter.asInt8[1] = (int8)Math::Clamp(outTangent.y * 127.0f, -127.0f, 127.0f);
        converter.asInt8[2] = (int8)Math::Clamp(outTangent.z * 127.0f, -127.0f, 127.0f);
        converter.asInt8[3] = (int8)Math::Clamp(outBinormalSign * 127.0f, -127.0f, 127.0f);
        packedTangentAndSign = converter.asUInt32;

        converter.asInt8[0] = (int8)Math::Clamp(CUBE_FACE_NORMALS[i][0] * 127.0f, -127.0f, 127.0f);
        converter.asInt8[1] = (int8)Math::Clamp(CUBE_FACE_NORMALS[i][1] * 127.0f, -127.0f, 127.0f);
        converter.asInt8[2] = (int8)Math::Clamp(CUBE_FACE_NORMALS[i][2] * 127.0f, -127.0f, 127.0f);
        converter.asInt8[3] = (int8)0;
        packedNormal = converter.asUInt32;

        Log_DevPrintf("{ 0x%08X, 0x%08X },", packedTangentAndSign, packedNormal);
    }


    return 0;
}
