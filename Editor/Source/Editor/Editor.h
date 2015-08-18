#pragma once
#include "Editor/Common.h"
#include "Editor/EditorIconLibrary.h"

class Image;
class Font;
class ObjectTemplate;
class EditorVisualDefinition;
class EditorMapWindow;

class Editor : public QApplication
{
    Q_OBJECT

public:
    Editor(int &argc, char **argv);
    ~Editor();

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // State Management
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // initialization/exit
    int Execute();
    void Exit();

    // frame execution blocking
    void BlockFrameExecution() { m_blockFrameExecution++; }
    void UnblockFrameExecution() { DebugAssert(m_blockFrameExecution > 0); m_blockFrameExecution--; }

    // trigger a frame execution event, to avoid waiting for the timer
    void QueueFrameExecution();

    // repaint ui without running any frames
    void ProcessBackgroundEvents();

    // framerate, assuming we don't sleep
    float GetEditorFPS() const { return m_editorFPS; }

    // change theme
    const EDITOR_THEME GetEditorTheme() const { return m_editorTheme; }
    void SetEditorTheme(EDITOR_THEME theme);

    // new map/open map
    EditorMapWindow *CreateMap();
    EditorMapWindow *OpenMap(const char *mapFileName);

    // managed map windows
    const EditorMapWindow *GetMapWindow(uint32 index) const { return m_mapWindows[index]; }
    EditorMapWindow *GetMapWindow(uint32 index) { return m_mapWindows[index]; }
    uint32 GetMapWindowCount() const { return m_mapWindows.GetSize(); }
    void AddMapWindow(EditorMapWindow *pMapWindow);
    void RemoveMapWindow(EditorMapWindow *pMapWindow);
    void CloseAllMapWindows();

    // other managed windows
    const QMainWindow *GetMainWindow(uint32 index) const { return m_mainWindows[index]; }
    QMainWindow *GetMainWindow(uint32 index) { return m_mainWindows[index]; }
    uint32 GetMainWindowCount() const { return m_mainWindows.GetSize(); }
    void AddMainWindow(QMainWindow *pMainWindow);
    void RemoveMainWindow(QMainWindow *pMainWindow);

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Resource Management
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // icon library
    const EditorIconLibrary *GetIconLibrary() const { return &m_iconLibrary; }

    // viewport overlay font
    const Font *GetViewportOverlayFont() const { return m_pViewportOverlayFont; }

    // visual definitions
    const EditorVisualDefinition *GetVisualDefinitionByName(const char *typeName) const;
    const EditorVisualDefinition *GetVisualDefinitionForObjectTemplate(const ObjectTemplate *pTemplate) const;
    const uint32 GetVisualDefinitionCount() const { return m_visualDefinitions.GetMemberCount(); }
    
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Signals
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Q_SIGNALS:
    // frame event
    void OnFrameExecution(float timeSinceLastFrame);

private:
    void RegisterEditorTypes();
    bool LoadObjectTemplates();
    bool LoadVisualDefinitions();
    bool Initialize();
    bool Startup();
    void Shutdown();

    // app arguments
    const char **m_pCommandLineArguments;
    uint32 m_nCommandLineArguments;

    // state
    bool m_running;
    QTimer *m_pFrameExecuteTimer;
    uint32 m_blockFrameExecution;
    bool m_frameExecutionQueued;
    Timer m_lastFrameTime;
    uint32 m_editorFPSAccumulator;
    float m_editorFPSTimeAccumulator;
    float m_editorFPS;

    // theme
    EDITOR_THEME m_editorTheme;

    // resources
    EditorIconLibrary m_iconLibrary;
    typedef CIStringHashTable<EditorVisualDefinition *> VisualDefinitionHashTable;
    VisualDefinitionHashTable m_visualDefinitions;
    const Font *m_pViewportOverlayFont;
    PODArray<EditorMapWindow *> m_mapWindows;
    PODArray<QMainWindow *> m_mainWindows;

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Slots
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private Q_SLOTS:
    void OnPendingCloseTriggered();
    void OnFrameExecuteTimerTriggered();
};

extern Editor *g_pEditor;
