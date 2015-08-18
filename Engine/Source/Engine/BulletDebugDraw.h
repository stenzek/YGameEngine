#pragma once
#include "Engine/Common.h"
#include "Renderer/MiniGUIContext.h"
#include "Engine/Physics/BulletHeaders.h"

class BulletDebugDraw : public btIDebugDraw
{
public:
    BulletDebugDraw();
    ~BulletDebugDraw();

    const MiniGUIContext &GetGUIContext() const { return m_guiContext; }

    void SetViewportDimensions(const uint32 width, const uint32 height);

    void BeginDraw();
    void EndDraw();

    virtual void drawLine(const btVector3& from,const btVector3& to,const btVector3& color);
    virtual void drawLine(const btVector3& from,const btVector3& to, const btVector3& fromColor, const btVector3& toColor);
    virtual void drawContactPoint(const btVector3& PointOnB,const btVector3& normalOnB,btScalar distance,int lifeTime,const btVector3& color);
    virtual void reportErrorWarning(const char* warningString);
    virtual void draw3dText(const btVector3& location,const char* textString);
    virtual void setDebugMode(int debugMode);
    virtual int getDebugMode() const;

private:
    MiniGUIContext m_guiContext;
    int m_debugMode;
};

