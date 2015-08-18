#include "Engine/PrecompiledHeader.h"
#include "Engine/BulletDebugDraw.h"
Log_SetChannel(BulletDebugDraw);

using namespace Physics;

BulletDebugDraw::BulletDebugDraw()
    : m_debugMode(0)
{

}

BulletDebugDraw::~BulletDebugDraw()
{

}

void BulletDebugDraw::SetViewportDimensions(const uint32 width, const uint32 height)
{
    m_guiContext.SetViewportDimensions(width, height);
}

void BulletDebugDraw::BeginDraw()
{
    m_guiContext.PushManualFlush();
    m_guiContext.SetDepthTestingEnabled(true);
    m_guiContext.SetAlphaBlendingEnabled(false);
}

void BulletDebugDraw::EndDraw()
{
    m_guiContext.Flush();
    m_guiContext.PopManualFlush();
}

void BulletDebugDraw::drawLine(const btVector3& from,const btVector3& to,const btVector3& color)
{
    float3 fromVec(BulletVector3ToFloat3(from));
    float3 toVec(BulletVector3ToFloat3(to));
    float3 colorVec(BulletVector3ToFloat3(color));
    uint8 r = (uint8)Math::Clamp(colorVec.r * 255.0f, 0.0f, 255.0f);
    uint8 g = (uint8)Math::Clamp(colorVec.g * 255.0f, 0.0f, 255.0f);
    uint8 b = (uint8)Math::Clamp(colorVec.b * 255.0f, 0.0f, 255.0f);

    m_guiContext.Draw3DLine(fromVec, toVec, MAKE_COLOR_R8G8B8A8_UNORM(r, g, b, 255));
}

void BulletDebugDraw::drawLine(const btVector3& from,const btVector3& to, const btVector3& fromColor, const btVector3& toColor)
{
    float3 fromVec(BulletVector3ToFloat3(from));
    float3 fromColorVec(BulletVector3ToFloat3(fromColor));
    float3 toVec(BulletVector3ToFloat3(to));
    float3 toColorVec(BulletVector3ToFloat3(toColor));
    
    uint8 fromR = (uint8)Math::Clamp(fromColorVec.r * 255.0f, 0.0f, 255.0f);
    uint8 fromG = (uint8)Math::Clamp(fromColorVec.g * 255.0f, 0.0f, 255.0f);
    uint8 fromB = (uint8)Math::Clamp(fromColorVec.b * 255.0f, 0.0f, 255.0f);
    uint8 toR = (uint8)Math::Clamp(toColorVec.r * 255.0f, 0.0f, 255.0f);
    uint8 toG = (uint8)Math::Clamp(toColorVec.g * 255.0f, 0.0f, 255.0f);
    uint8 toB = (uint8)Math::Clamp(toColorVec.b * 255.0f, 0.0f, 255.0f);

    m_guiContext.Draw3DLine(fromVec, MAKE_COLOR_R8G8B8A8_UNORM(fromR, fromG, fromB, 255), toVec, MAKE_COLOR_R8G8B8A8_UNORM(toR, toG, toB, 255));
}

void BulletDebugDraw::drawContactPoint(const btVector3& PointOnB,const btVector3& normalOnB,btScalar distance,int lifeTime,const btVector3& color)
{
    //Log_DevPrintf("BulletDebugDraw::drawContactPoint: %f %f %f", PointOnB.x(), PointOnB.y(), PointOnB.z());

    btVector3 to= PointOnB+normalOnB*1;//distance;
    const btVector3&from = PointOnB;

    drawLine(from, to, color);
}

void BulletDebugDraw::reportErrorWarning(const char* warningString)
{
    Log_WarningPrintf("BulletDebugDraw::reportErrorWarning: '%s'", warningString);
}

void BulletDebugDraw::draw3dText(const btVector3& location,const char* textString)
{
    Log_DevPrintf("BulletDebugDraw::draw3dText: '%s' @ %f %f %f", textString, location.x(), location.y(), location.z());
}

void BulletDebugDraw::setDebugMode(int debugMode)
{
    m_debugMode = debugMode;
}

int BulletDebugDraw::getDebugMode() const
{
    return m_debugMode;
}
