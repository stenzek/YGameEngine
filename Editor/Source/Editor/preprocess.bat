@echo off
set MOC="..\..\..\Dependancies\Windows\qt32\bin\moc.exe"
set UIC="..\..\..\Dependancies\Windows\qt32\bin\uic.exe"
set RCC="..\..\..\Dependancies\Windows\qt32\bin\rcc.exe"

echo Preprocessing...

rem --- Main ---

%MOC% -b Editor/PrecompiledHeader.h -o moc_Editor.cpp Editor.h
%MOC% -b Editor/PrecompiledHeader.h -o moc_EditorProgressDialog.cpp EditorProgressDialog.h
%MOC% -b Editor/PrecompiledHeader.h -o moc_EditorPropertyEditorDialog.cpp EditorPropertyEditorDialog.h
%MOC% -b Editor/PrecompiledHeader.h -o moc_EditorPropertyEditorWidget.cpp EditorPropertyEditorWidget.h
%MOC% -b Editor/PrecompiledHeader.h -o moc_EditorRendererSwapChainWidget.cpp EditorRendererSwapChainWidget.h
%MOC% -b Editor/PrecompiledHeader.h -o moc_EditorResourcePreviewWidget.cpp EditorResourcePreviewWidget.h
%MOC% -b Editor/PrecompiledHeader.h -o moc_EditorResourceSaveDialog.cpp EditorResourceSaveDialog.h
%MOC% -b Editor/PrecompiledHeader.h -o moc_EditorResourceSelectionDialog.cpp EditorResourceSelectionDialog.h
%MOC% -b Editor/PrecompiledHeader.h -o moc_EditorResourceSelectionDialogModel.cpp EditorResourceSelectionDialogModel.h
%MOC% -b Editor/PrecompiledHeader.h -o moc_EditorResourceSelectionWidget.cpp EditorResourceSelectionWidget.h
%MOC% -b Editor/PrecompiledHeader.h -o moc_EditorScriptEditor.cpp EditorScriptEditor.h
%MOC% -b Editor/PrecompiledHeader.h -o moc_EditorVectorEditWidget.cpp EditorVectorEditWidget.h
%MOC% -b Editor/PrecompiledHeader.h -o moc_EditorVirtualFileSystemModel.cpp EditorVirtualFileSystemModel.h
%MOC% -b Editor/PrecompiledHeader.h -o moc_SimpleTreeModel.cpp SimpleTreeModel.h
%MOC% -b Editor/PrecompiledHeader.h -o moc_ToolMenuWidget.cpp ToolMenuWidget.h

%UIC% -o ui_EditorProgressDialog.h EditorProgressDialog.ui
%RCC% -o rcc_Editor.cpp Resources/editor.qrc


rem --- MapEditor ---

%MOC% -b Editor/PrecompiledHeader.h -o MapEditor/moc_EditorCreateTerrainDialog.cpp MapEditor/EditorCreateTerrainDialog.h
%MOC% -b Editor/PrecompiledHeader.h -o MapEditor/moc_EditorEditMode.cpp MapEditor/EditorEditMode.h
%MOC% -b Editor/PrecompiledHeader.h -o MapEditor/moc_EditorEntityEditMode.cpp MapEditor/EditorEntityEditMode.h
%MOC% -b Editor/PrecompiledHeader.h -o MapEditor/moc_EditorGeometryEditMode.cpp MapEditor/EditorGeometryEditMode.h
%MOC% -b Editor/PrecompiledHeader.h -o MapEditor/moc_EditorMapEditMode.cpp MapEditor/EditorMapEditMode.h
%MOC% -b Editor/PrecompiledHeader.h -o MapEditor/moc_EditorMapWindow.cpp MapEditor/EditorMapWindow.h
%MOC% -b Editor/PrecompiledHeader.h -o MapEditor/moc_EditorMapViewport.cpp MapEditor/EditorMapViewport.h
%MOC% -b Editor/PrecompiledHeader.h -o MapEditor/moc_EditorTerrainEditMode.cpp MapEditor/EditorTerrainEditMode.h
%MOC% -b Editor/PrecompiledHeader.h -o MapEditor/moc_EditorWorldOutlinerWidget.cpp MapEditor/EditorWorldOutlinerWidget.h
%UIC% -o MapEditor/ui_EditorCreateTerrainDialog.h MapEditor/EditorCreateTerrainDialog.ui

rem --- BlockMeshEditor ---

%MOC% -b Editor/PrecompiledHeader.h -o BlockMeshEditor/moc_EditorBlockMeshEditor.cpp BlockMeshEditor/EditorBlockMeshEditor.h

rem --- StaticMeshEditor ---

%MOC% -b Editor/PrecompiledHeader.h -o StaticMeshEditor/moc_EditorStaticMeshEditor.cpp StaticMeshEditor/EditorStaticMeshEditor.h

rem --- SkeletalAnimationEditor ---

%MOC% -b Editor/PrecompiledHeader.h -o SkeletalAnimationEditor/moc_EditorSkeletalAnimationEditor.cpp SkeletalAnimationEditor/EditorSkeletalAnimationEditor.h

rem --- SkeletalMeshEditor ---

%MOC% -b Editor/PrecompiledHeader.h -o SkeletalMeshEditor/moc_EditorSkeletalMeshEditor.cpp SkeletalMeshEditor/EditorSkeletalMeshEditor.h

rem --- ResourceBrowser ---

%MOC% -b Editor/PrecompiledHeader.h -o ResourceBrowser/moc_EditorResourceBrowserWidget.cpp ResourceBrowser/EditorResourceBrowserWidget.h
%MOC% -b Editor/PrecompiledHeader.h -o ResourceBrowser/moc_EditorStaticMeshImportDialog.cpp ResourceBrowser/EditorStaticMeshImportDialog.h
%UIC% -o ResourceBrowser/ui_EditorStaticMeshImportDialog.h ResourceBrowser/EditorStaticMeshImportDialog.ui

rem -----------------

echo Done.
pause
