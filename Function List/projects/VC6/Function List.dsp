# Microsoft Developer Studio Project File - Name="Function List" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** NICHT BEARBEITEN **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=Function List - Win32 Debug
!MESSAGE Dies ist kein gültiges Makefile. Zum Erstellen dieses Projekts mit NMAKE
!MESSAGE verwenden Sie den Befehl "Makefile exportieren" und führen Sie den Befehl
!MESSAGE 
!MESSAGE NMAKE /f "Function List.mak".
!MESSAGE 
!MESSAGE Sie können beim Ausführen von NMAKE eine Konfiguration angeben
!MESSAGE durch Definieren des Makros CFG in der Befehlszeile. Zum Beispiel:
!MESSAGE 
!MESSAGE NMAKE /f "Function List.mak" CFG="Function List - Win32 Debug"
!MESSAGE 
!MESSAGE Für die Konfiguration stehen zur Auswahl:
!MESSAGE 
!MESSAGE "Function List - Win32 Release" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE "Function List - Win32 Debug" (basierend auf  "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "Function List - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "FUNCTIONLIST_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /I "..\..\src\\" /I "..\..\src\HelpDlg\\" /I "..\..\src\MISC\\" /I "..\..\src\UserDefineDlg\\" /I "..\..\src\Toolbar\\" /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "FUNCTIONLIST_EXPORTS" /FR /FD /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x407 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib shlwapi.lib comctl32.lib /nologo /dll /machine:I386 /out:"C:/Programme/Notepad++/Plugins/FunctionList.dll"

!ELSEIF  "$(CFG)" == "Function List - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "FUNCTIONLIST_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I ".\src\\" /I ".\src\HelpDlg\\" /I ".\src\MISC\\" /I ".\src\UserDefineDlg\\" /I ".\src\Toolbar\\" /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "FUNCTIONLIST_EXPORTS" /D "DIRECT_ACCESS" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x407 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib shlwapi.lib comctl32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /incremental:no

!ENDIF 

# Begin Target

# Name "Function List - Win32 Release"
# Name "Function List - Win32 Debug"
# Begin Group "Quellcodedateien"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\src\MISC\CommentList.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\FunctionListDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\FunctionListDialog.rc
# End Source File
# Begin Source File

SOURCE=..\..\src\HelpDlg\HelpDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\Toolbar\ImageListSet.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\PluginInterface.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\MISC\PosReminder.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\StaticDialog.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\stdafx.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\SysMsg.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\Toolbar\ToolBar.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\MISC\ToolTip.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\HelpDlg\URLCtrl.cpp
# End Source File
# Begin Source File

SOURCE=..\..\src\UserDefineDlg\UserDefineDialog.cpp
# End Source File
# End Group
# Begin Group "Header-Dateien"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\..\src\MISC\CommentList.h
# End Source File
# Begin Source File

SOURCE=..\..\src\Docking.h
# End Source File
# Begin Source File

SOURCE=..\..\src\DockingDlgInterface.h
# End Source File
# Begin Source File

SOURCE=..\..\src\MISC\FunctionList.h
# End Source File
# Begin Source File

SOURCE=..\..\src\FunctionListDialog.h
# End Source File
# Begin Source File

SOURCE=..\..\src\FunctionListResource.h
# End Source File
# Begin Source File

SOURCE=..\..\src\HelpDlg\HelpDialog.h
# End Source File
# Begin Source File

SOURCE=..\..\src\HelpDlg\HelpTxt.h
# End Source File
# Begin Source File

SOURCE=..\..\src\Toolbar\ImageListSet.h
# End Source File
# Begin Source File

SOURCE=..\..\src\PluginInterface.h
# End Source File
# Begin Source File

SOURCE=..\..\src\MISC\PosReminder.h
# End Source File
# Begin Source File

SOURCE=..\..\src\rcNotepad.h
# End Source File
# Begin Source File

SOURCE=..\..\src\rcUserDefine.h
# End Source File
# Begin Source File

SOURCE=..\..\src\resource.h
# End Source File
# Begin Source File

SOURCE=..\..\src\Scintilla.h
# End Source File
# Begin Source File

SOURCE=..\..\src\StaticDialog.h
# End Source File
# Begin Source File

SOURCE=..\..\src\stdafx.h
# End Source File
# Begin Source File

SOURCE=..\..\src\SysMsg.h
# End Source File
# Begin Source File

SOURCE=..\..\src\Toolbar\ToolBar.h
# End Source File
# Begin Source File

SOURCE=..\..\src\MISC\ToolTip.h
# End Source File
# Begin Source File

SOURCE=..\..\src\MISC\UndoRedoTB.h
# End Source File
# Begin Source File

SOURCE=..\..\src\HelpDlg\URLCtrl.h
# End Source File
# Begin Source File

SOURCE=..\..\src\UserDefineDlg\UserDefineDialog.h
# End Source File
# Begin Source File

SOURCE=..\..\src\Window.h
# End Source File
# End Group
# Begin Group "Ressourcendateien"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\..\res\Tab.ico
# End Source File
# End Group
# End Target
# End Project
