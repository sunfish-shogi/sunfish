# Microsoft Developer Studio Project File - Name="sikou" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** 編集しないでください **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=sikou - Win32 Debug
!MESSAGE これは有効なﾒｲｸﾌｧｲﾙではありません。 このﾌﾟﾛｼﾞｪｸﾄをﾋﾞﾙﾄﾞするためには NMAKE を使用してください。
!MESSAGE [ﾒｲｸﾌｧｲﾙのｴｸｽﾎﾟｰﾄ] ｺﾏﾝﾄﾞを使用して実行してください
!MESSAGE 
!MESSAGE NMAKE /f "sikou.mak".
!MESSAGE 
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "sikou.mak" CFG="sikou - Win32 Debug"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "sikou - Win32 Release" ("Win32 (x86) Dynamic-Link Library" 用)
!MESSAGE "sikou - Win32 Debug" ("Win32 (x86) Dynamic-Link Library" 用)
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "sikou - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "sikou___Win32_Release"
# PROP BASE Intermediate_Dir "sikou___Win32_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release6"
# PROP Intermediate_Dir "Release6\Sikou"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SIKOU_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SIKOU_EXPORTS" /D "RAND2" /D QUIES_SEARCH=1 /D "SIKOU_DLL" /D "NLEARN" /YX /FD /c
# SUBTRACT CPP /Fr
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "NDEBUG"
# ADD RSC /l 0x411 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /dll /machine:I386

!ELSEIF  "$(CFG)" == "sikou - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "sikou___Win32_Debug"
# PROP BASE Intermediate_Dir "sikou___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug6"
# PROP Intermediate_Dir "Debug6\Sikou"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SIKOU_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "SIKOU_EXPORTS" /D "DEBUG" /D "RAND2" /D QUIES_SEARCH=1 /D "SIKOU_DLL" /D "NLEARN" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x411 /d "_DEBUG"
# ADD RSC /l 0x411 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib winmm.lib /nologo /dll /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "sikou - Win32 Release"
# Name "sikou - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\book.cpp
# End Source File
# Begin Source File

SOURCE=.\data.cpp
# End Source File
# Begin Source File

SOURCE=.\debug.cpp
# End Source File
# Begin Source File

SOURCE=.\effect.cpp
# End Source File
# Begin Source File

SOURCE=.\evaluate.cpp
# End Source File
# Begin Source File

SOURCE=.\filelist.cpp
# End Source File
# Begin Source File

SOURCE=.\gencheck.cpp
# End Source File
# Begin Source File

SOURCE=.\generate.cpp
# End Source File
# Begin Source File

SOURCE=.\hash.cpp
# End Source File
# Begin Source File

SOURCE=.\kifu.cpp
# End Source File
# Begin Source File

SOURCE=.\limit.cpp
# End Source File
# Begin Source File

SOURCE=.\match.cpp
# End Source File
# Begin Source File

SOURCE=.\mate.cpp
# End Source File
# Begin Source File

SOURCE=.\param.cpp
# End Source File
# Begin Source File

SOURCE=.\SFMT\SFMT.c
# End Source File
# Begin Source File

SOURCE=.\shogi.cpp
# End Source File
# Begin Source File

SOURCE=.\shogi2.cpp
# End Source File
# Begin Source File

SOURCE=.\sikou.cpp
# End Source File
# Begin Source File

SOURCE=.\string.cpp
# End Source File
# Begin Source File

SOURCE=.\think.cpp
# End Source File
# Begin Source File

SOURCE=.\thread.cpp
# End Source File
# Begin Source File

SOURCE=.\tree.cpp
# End Source File
# Begin Source File

SOURCE=.\uint96.cpp
# End Source File
# Begin Source File

SOURCE=.\wconf.cpp
# End Source File
# Begin Source File

SOURCE=.\window.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\feature.h
# End Source File
# Begin Source File

SOURCE=.\match.h
# End Source File
# Begin Source File

SOURCE=.\rand.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\shogi.h
# End Source File
# Begin Source File

SOURCE=.\sikou.h
# End Source File
# Begin Source File

SOURCE=.\uint96.h
# End Source File
# Begin Source File

SOURCE=.\wconf.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\icon01.ico
# End Source File
# Begin Source File

SOURCE=.\sikou.rc
# End Source File
# End Group
# End Target
# End Project
