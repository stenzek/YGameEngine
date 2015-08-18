mkdir Binaries
cd Binaries

mkdir bin32
mkdir bin32-debug
mkdir bin64
mkdir bin64-debug

rem ---------------------------------------------------------------------------------------------------------

copy /Y ..\Dependancies\Windows\bin32-debug\* bin32-debug\

copy /Y ..\Dependancies\Windows\qt32\bin\Qt5Cored.dll bin32-debug\
copy /Y ..\Dependancies\Windows\qt32\bin\Qt5Cored.pdb bin32-debug\
copy /Y ..\Dependancies\Windows\qt32\bin\Qt5Guid.dll bin32-debug\
copy /Y ..\Dependancies\Windows\qt32\bin\Qt5Guid.pdb bin32-debug\
copy /Y ..\Dependancies\Windows\qt32\bin\Qt5Widgetsd.dll bin32-debug\
copy /Y ..\Dependancies\Windows\qt32\bin\Qt5Widgetsd.pdb bin32-debug\
copy /Y ..\Dependancies\Windows\bin32\QtPropertyManagerd.dll bin32-debug\
copy /Y ..\Dependancies\Windows\bin32\QtPropertyManagerd.pdb bin32-debug\

mkdir bin32-debug\platforms
copy /Y ..\Dependancies\Windows\qt32\plugins\platforms\qminimald.dll bin32-debug\platforms
copy /Y ..\Dependancies\Windows\qt32\plugins\platforms\qminimald.pdb bin32-debug\platforms
copy /Y ..\Dependancies\Windows\qt32\plugins\platforms\qoffscreend.dll bin32-debug\platforms
copy /Y ..\Dependancies\Windows\qt32\plugins\platforms\qoffscreend.pdb bin32-debug\platforms
copy /Y ..\Dependancies\Windows\qt32\plugins\platforms\qwindowsd.dll bin32-debug\platforms
copy /Y ..\Dependancies\Windows\qt32\plugins\platforms\qwindowsd.pdb bin32-debug\platforms

mkdir bin32-debug\plugins
copy /Y ..\Dependancies\Windows\qt32\plugins\imageformats\qgifd.dll bin32-debug\plugins
copy /Y ..\Dependancies\Windows\qt32\plugins\imageformats\qgifd.pdb bin32-debug\plugins
copy /Y ..\Dependancies\Windows\qt32\plugins\imageformats\qicod.dll bin32-debug\plugins
copy /Y ..\Dependancies\Windows\qt32\plugins\imageformats\qicod.pdb bin32-debug\plugins
copy /Y ..\Dependancies\Windows\qt32\plugins\imageformats\qjpegd.dll bin32-debug\plugins
copy /Y ..\Dependancies\Windows\qt32\plugins\imageformats\qjpegd.pdb bin32-debug\plugins
copy /Y ..\Dependancies\Windows\qt32\plugins\imageformats\qmngd.dll bin32-debug\plugins
copy /Y ..\Dependancies\Windows\qt32\plugins\imageformats\qmngd.pdb bin32-debug\plugins
copy /Y ..\Dependancies\Windows\qt32\plugins\imageformats\qsvgd.dll bin32-debug\plugins
copy /Y ..\Dependancies\Windows\qt32\plugins\imageformats\qsvgd.pdb bin32-debug\plugins
copy /Y ..\Dependancies\Windows\qt32\plugins\imageformats\qtgad.dll bin32-debug\plugins
copy /Y ..\Dependancies\Windows\qt32\plugins\imageformats\qtgad.pdb bin32-debug\plugins
copy /Y ..\Dependancies\Windows\qt32\plugins\imageformats\qtiffd.dll bin32-debug\plugins
copy /Y ..\Dependancies\Windows\qt32\plugins\imageformats\qtiffd.pdb bin32-debug\plugins
copy /Y ..\Dependancies\Windows\qt32\plugins\imageformats\qwbmpd.dll bin32-debug\plugins
copy /Y ..\Dependancies\Windows\qt32\plugins\imageformats\qwbmpd.pdb bin32-debug\plugins

rem ---------------------------------------------------------------------------------------------------------

copy /Y ..\Dependancies\Windows\bin32\* bin32\

copy /Y ..\Dependancies\Windows\qt32\bin\Qt5Core.dll bin32\
copy /Y ..\Dependancies\Windows\qt32\bin\Qt5Gui.dll bin32\
copy /Y ..\Dependancies\Windows\qt32\bin\Qt5Widgets.dll bin32\
copy /Y ..\Dependancies\Windows\bin32\QtPropertyManager.dll bin32\

mkdir bin32\platforms
copy /Y ..\Dependancies\Windows\qt32\plugins\platforms\qminimal.dll bin32\platforms
copy /Y ..\Dependancies\Windows\qt32\plugins\platforms\qoffscreen.dll bin32\platforms
copy /Y ..\Dependancies\Windows\qt32\plugins\platforms\qwindows.dll bin32\platforms

mkdir bin32\plugins
copy /Y ..\Dependancies\Windows\qt32\plugins\imageformats\qgif.dll bin32\plugins
copy /Y ..\Dependancies\Windows\qt32\plugins\imageformats\qico.dll bin32\plugins
copy /Y ..\Dependancies\Windows\qt32\plugins\imageformats\qjpeg.dll bin32\plugins
copy /Y ..\Dependancies\Windows\qt32\plugins\imageformats\qmng.dll bin32\plugins
copy /Y ..\Dependancies\Windows\qt32\plugins\imageformats\qsvg.dll bin32\plugins
copy /Y ..\Dependancies\Windows\qt32\plugins\imageformats\qtga.dll bin32\plugins
copy /Y ..\Dependancies\Windows\qt32\plugins\imageformats\qtiff.dll bin32\plugins
copy /Y ..\Dependancies\Windows\qt32\plugins\imageformats\qwbmp.dll bin32\plugins

rem ---------------------------------------------------------------------------------------------------------

copy /Y ..\Dependancies\Windows\bin64-debug\* bin64-debug\

copy /Y ..\Dependancies\Windows\qt64\bin\Qt5Cored.dll bin64-debug\
copy /Y ..\Dependancies\Windows\qt64\bin\Qt5Cored.pdb bin64-debug\
copy /Y ..\Dependancies\Windows\qt64\bin\Qt5Guid.dll bin64-debug\
copy /Y ..\Dependancies\Windows\qt64\bin\Qt5Guid.pdb bin64-debug\
copy /Y ..\Dependancies\Windows\qt64\bin\Qt5Widgetsd.dll bin64-debug\
copy /Y ..\Dependancies\Windows\qt64\bin\Qt5Widgetsd.pdb bin64-debug\
copy /Y ..\Dependancies\Windows\bin64\QtPropertyManagerd.dll bin64-debug\
copy /Y ..\Dependancies\Windows\bin64\QtPropertyManagerd.pdb bin64-debug\

mkdir bin64-debug\platforms
copy /Y ..\Dependancies\Windows\qt64\plugins\platforms\qminimald.dll bin64-debug\platforms
copy /Y ..\Dependancies\Windows\qt64\plugins\platforms\qminimald.pdb bin64-debug\platforms
copy /Y ..\Dependancies\Windows\qt64\plugins\platforms\qoffscreend.dll bin64-debug\platforms
copy /Y ..\Dependancies\Windows\qt64\plugins\platforms\qoffscreend.pdb bin64-debug\platforms
copy /Y ..\Dependancies\Windows\qt64\plugins\platforms\qwindowsd.dll bin64-debug\platforms
copy /Y ..\Dependancies\Windows\qt64\plugins\platforms\qwindowsd.pdb bin64-debug\platforms

mkdir bin64-debug\plugins
copy /Y ..\Dependancies\Windows\qt64\plugins\imageformats\qgifd.dll bin64-debug\plugins
copy /Y ..\Dependancies\Windows\qt64\plugins\imageformats\qgifd.pdb bin64-debug\plugins
copy /Y ..\Dependancies\Windows\qt64\plugins\imageformats\qicod.dll bin64-debug\plugins
copy /Y ..\Dependancies\Windows\qt64\plugins\imageformats\qicod.pdb bin64-debug\plugins
copy /Y ..\Dependancies\Windows\qt64\plugins\imageformats\qjpegd.dll bin64-debug\plugins
copy /Y ..\Dependancies\Windows\qt64\plugins\imageformats\qjpegd.pdb bin64-debug\plugins
copy /Y ..\Dependancies\Windows\qt64\plugins\imageformats\qmngd.dll bin64-debug\plugins
copy /Y ..\Dependancies\Windows\qt64\plugins\imageformats\qmngd.pdb bin64-debug\plugins
copy /Y ..\Dependancies\Windows\qt64\plugins\imageformats\qsvgd.dll bin64-debug\plugins
copy /Y ..\Dependancies\Windows\qt64\plugins\imageformats\qsvgd.pdb bin64-debug\plugins
copy /Y ..\Dependancies\Windows\qt64\plugins\imageformats\qtgad.dll bin64-debug\plugins
copy /Y ..\Dependancies\Windows\qt64\plugins\imageformats\qtgad.pdb bin64-debug\plugins
copy /Y ..\Dependancies\Windows\qt64\plugins\imageformats\qtiffd.dll bin64-debug\plugins
copy /Y ..\Dependancies\Windows\qt64\plugins\imageformats\qtiffd.pdb bin64-debug\plugins
copy /Y ..\Dependancies\Windows\qt64\plugins\imageformats\qwbmpd.dll bin64-debug\plugins
copy /Y ..\Dependancies\Windows\qt64\plugins\imageformats\qwbmpd.pdb bin64-debug\plugins

rem ---------------------------------------------------------------------------------------------------------

copy /Y ..\Dependancies\Windows\bin64\* bin64\

copy /Y ..\Dependancies\Windows\qt64\bin\Qt5Core.dll bin64\
copy /Y ..\Dependancies\Windows\qt64\bin\Qt5Gui.dll bin64\
copy /Y ..\Dependancies\Windows\qt64\bin\Qt5Widgets.dll bin64\
copy /Y ..\Dependancies\Windows\bin64\QtPropertyManager.dll bin64\

mkdir bin64\platforms
copy /Y ..\Dependancies\Windows\qt64\plugins\platforms\qminimal.dll bin64\platforms
copy /Y ..\Dependancies\Windows\qt64\plugins\platforms\qoffscreen.dll bin64\platforms
copy /Y ..\Dependancies\Windows\qt64\plugins\platforms\qwindows.dll bin64\platforms

mkdir bin64\plugins
copy /Y ..\Dependancies\Windows\qt64\plugins\imageformats\qgif.dll bin64\plugins
copy /Y ..\Dependancies\Windows\qt64\plugins\imageformats\qico.dll bin64\plugins
copy /Y ..\Dependancies\Windows\qt64\plugins\imageformats\qjpeg.dll bin64\plugins
copy /Y ..\Dependancies\Windows\qt64\plugins\imageformats\qmng.dll bin64\plugins
copy /Y ..\Dependancies\Windows\qt64\plugins\imageformats\qsvg.dll bin64\plugins
copy /Y ..\Dependancies\Windows\qt64\plugins\imageformats\qtga.dll bin64\plugins
copy /Y ..\Dependancies\Windows\qt64\plugins\imageformats\qtiff.dll bin64\plugins
copy /Y ..\Dependancies\Windows\qt64\plugins\imageformats\qwbmp.dll bin64\plugins
