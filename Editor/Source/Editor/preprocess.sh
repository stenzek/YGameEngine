MOC=/usr/lib64/qt5/bin/moc
UIC=/usr/lib64/qt5/bin/uic
RCC=/usr/lib64/qt5/bin/rcc

echo Preprocessing...

$MOC -b Editor/PrecompiledHeader.h -o moc_Editor.cpp Editor.h
$MOC -b Editor/PrecompiledHeader.h -o moc_EditorCreateTerrainDialog.cpp EditorCreateTerrainDialog.h
$MOC -b Editor/PrecompiledHeader.h -o moc_EditorMapWindow.cpp EditorMapWindow.h
$MOC -b Editor/PrecompiledHeader.h -o moc_EditorProgressDialog.cpp EditorProgressDialog.h
$MOC -b Editor/PrecompiledHeader.h -o moc_EditorPropertyEditorWidget.cpp EditorPropertyEditorWidget.h
$MOC -b Editor/PrecompiledHeader.h -o moc_EditorRendererSwapChainWidget.cpp EditorRendererSwapChainWidget.h
$MOC -b Editor/PrecompiledHeader.h -o moc_EditorResourcePreviewWidget.cpp EditorResourcePreviewWidget.h
$MOC -b Editor/PrecompiledHeader.h -o moc_EditorResourceSaveDialog.cpp EditorResourceSaveDialog.h
$MOC -b Editor/PrecompiledHeader.h -o moc_EditorResourceSelectionDialog.cpp EditorResourceSelectionDialog.h
$MOC -b Editor/PrecompiledHeader.h -o moc_EditorResourceSelectionDialogModel.cpp EditorResourceSelectionDialogModel.h
$MOC -b Editor/PrecompiledHeader.h -o moc_EditorResourceSelectionWidget.cpp EditorResourceSelectionWidget.h
$MOC -b Editor/PrecompiledHeader.h -o moc_EditorBlockMeshEditor.cpp EditorStaticBlockMeshEditor.h
$MOC -b Editor/PrecompiledHeader.h -o moc_EditorViewport.cpp EditorViewport.h
$MOC -b Editor/PrecompiledHeader.h -o moc_EditorVirtualFileSystemModel.cpp EditorVirtualFileSystemModel.h

$UIC -o ui_EditorProgressDialog.h EditorProgressDialog.ui
$UIC -o ui_EditorCreateTerrainDialog.h EditorCreateTerrainDialog.ui
$RCC -o rcc_Editor.cpp Resources/editor.qrc

echo Done.

