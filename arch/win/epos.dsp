# Microsoft Developer Studio Project File - Name="epos" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

CFG=epos - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "epos.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "epos.mak" CFG="epos - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "epos - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "epos - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "epos - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_MT" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "NDEBUG" /D "WIN32" /D "_MBCS" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386
# ADD LINK32 wsock32.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /machine:I386 /nodefaultlib:"libcd.lib" /nodefaultlib:"libc.lib"

!ELSEIF  "$(CFG)" == "epos - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_MT" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "_MT" /D "_DEBUG" /D "WIN32" /D "_MBCS" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /pdbtype:sept
# ADD LINK32 wsock32.lib winmm.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /debug /machine:I386 /nodefaultlib:"libcd" /pdbtype:sept
# SUBTRACT LINK32 /nodefaultlib

!ENDIF 

# Begin Target

# Name "epos - Win32 Release"
# Name "epos - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;cc;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\agent.cpp
# End Source File
# Begin Source File

SOURCE=.\client.cpp
# End Source File
# Begin Source File

SOURCE=.\daemon.cpp
# End Source File
# Begin Source File

SOURCE=.\encoding.cpp
# End Source File
# Begin Source File

SOURCE=.\function.cpp
# End Source File
# Begin Source File

SOURCE=.\hash.cpp
# End Source File
# Begin Source File

SOURCE=.\hashd.cpp
# End Source File
# Begin Source File

SOURCE=.\hashi.cpp
# End Source File
# Begin Source File

SOURCE=.\interf.cpp
# End Source File
# Begin Source File

SOURCE=.\mbrsyn.cpp
# End Source File
# Begin Source File

SOURCE=.\lpcsyn.cpp
# End Source File
# Begin Source File

SOURCE=.\nonblock.cpp
# End Source File
# Begin Source File

SOURCE=.\options.cpp
# End Source File
# Begin Source File

SOURCE=.\parser.cpp
# End Source File
# Begin Source File

SOURCE=.\rule.cpp
# End Source File
# Begin Source File

SOURCE=..\libs\regex\rx.c
# End Source File
# Begin Source File

SOURCE=.\service.cpp
# End Source File
# Begin Source File

SOURCE=.\synth.cpp
# End Source File
# Begin Source File

SOURCE=.\tcpsyn.cpp
# End Source File
# Begin Source File

SOURCE=.\tdpsyn.cpp
# End Source File
# Begin Source File

SOURCE=.\text.cpp
# End Source File
# Begin Source File

SOURCE=.\ttscp.cpp
# End Source File
# Begin Source File

SOURCE=.\unit.cpp
# End Source File
# Begin Source File

SOURCE=.\voice.cpp
# End Source File
# Begin Source File

SOURCE=.\waveform.cpp
# End Source File
# Begin Source File

SOURCE=.\nnet\enumstring.cpp
# End Source File
# Begin Source File

SOURCE=.\nnet\map.cpp
# End Source File
# Begin Source File

SOURCE=.\nnet\matrix.cpp
# End Source File
# Begin Source File

SOURCE=.\nnet\neural.cpp
# End Source File
# Begin Source File

SOURCE=.\nnet\neural_parse.cpp
# End Source File
# Begin Source File

SOURCE=.\nnet\perceptron.cpp
# End Source File
# Begin Source File

SOURCE=.\nnet\percinit.cpp
# End Source File
# Begin Source File

SOURCE=.\nnet\percstruct.cpp
# End Source File
# Begin Source File

SOURCE=.\nnet\set.cpp
# End Source File
# Begin Source File

SOURCE=.\nnet\stream.cpp
# End Source File
# Begin Source File

SOURCE=.\nnet\traindata.cpp
# End Source File
# Begin Source File

SOURCE=.\nnet\utils.cpp
# End Source File
# Begin Source File

SOURCE=.\nnet\xml.cpp
# End Source File
# Begin Source File

SOURCE=.\nnet\xml_parse.cpp
# End Source File
# Begin Source File

SOURCE=.\nnet\xmltempl.cpp
# End Source File
# Begin Source File

SOURCE=.\nnet\xmlutils.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\agent.h
# End Source File
# Begin Source File

SOURCE=.\client.h
# End Source File
# Begin Source File

SOURCE=.\common.h
# End Source File
# Begin Source File

SOURCE=.\config.h
# End Source File
# Begin Source File

SOURCE=.\defaults.h
# End Source File
# Begin Source File

SOURCE=.\encoding.h
# End Source File
# Begin Source File

SOURCE=.\epos.h
# End Source File
# Begin Source File

SOURCE=.\exc.h
# End Source File
# Begin Source File

SOURCE=.\function.h
# End Source File
# Begin Source File

SOURCE=.\hash.h
# End Source File
# Begin Source File

SOURCE=.\hashtmpl.h
# End Source File
# Begin Source File

SOURCE=.\interf.h
# End Source File
# Begin Source File

SOURCE=.\ktdsyn.h
# End Source File
# Begin Source File

SOURCE=.\lpcsyn.h
# End Source File
# Begin Source File

SOURCE=.\navel.h
# End Source File
# Begin Source File

SOURCE=.\options.h
# End Source File
# Begin Source File

SOURCE=.\parser.h
# End Source File
# Begin Source File

SOURCE=.\ptdsyn.h
# End Source File
# Begin Source File

SOURCE=.\rule.h
# End Source File
# Begin Source File

SOURCE=..\libs\regex\rx.h
# End Source File
# Begin Source File

SOURCE=.\service.h
# End Source File
# Begin Source File

SOURCE=.\slab.h
# End Source File
# Begin Source File

SOURCE=.\synth.h
# End Source File
# Begin Source File

SOURCE=.\tcpsyn.h
# End Source File
# Begin Source File

SOURCE=.\tdpsyn.h
# End Source File
# Begin Source File

SOURCE=.\text.h
# End Source File
# Begin Source File

SOURCE=.\unit.h
# End Source File
# Begin Source File

SOURCE=.\voice.h
# End Source File
# Begin Source File

SOURCE=.\waveform.h
# End Source File
# Begin Source File

SOURCE=.\winapp.h
# End Source File
# Begin Source File

SOURCE=.\nnet\base.h
# End Source File
# Begin Source File

SOURCE=.\nnet\enumstring.h
# End Source File
# Begin Source File

SOURCE=.\nnet\iterator.h
# End Source File
# Begin Source File

SOURCE=.\nnet\map.h
# End Source File
# Begin Source File

SOURCE=.\nnet\matrix.h
# End Source File
# Begin Source File

SOURCE=.\nnet\neural.cc.h
# End Source File
# Begin Source File

SOURCE=.\nnet\neural.h
# End Source File
# Begin Source File

SOURCE=.\nnet\neural_parse.h
# End Source File
# Begin Source File

SOURCE=.\nnet\nnettypes.h
# End Source File
# Begin Source File

SOURCE=.\nnet\pair.h
# End Source File
# Begin Source File

SOURCE=.\nnet\perceptron.h
# End Source File
# Begin Source File

SOURCE=.\nnet\percstruct.h
# End Source File
# Begin Source File

SOURCE=.\nnet\set.h
# End Source File
# Begin Source File

SOURCE=.\nnet\slowstring.h
# End Source File
# Begin Source File

SOURCE=.\nnet\stream.h
# End Source File
# Begin Source File

SOURCE=.\nnet\traindata.h
# End Source File
# Begin Source File

SOURCE=.\nnet\trainprocess.h
# End Source File
# Begin Source File

SOURCE=.\nnet\utils.h
# End Source File
# Begin Source File

SOURCE=.\nnet\vector.h
# End Source File
# Begin Source File

SOURCE=.\nnet\xml.h
# End Source File
# Begin Source File

SOURCE=.\nnet\xml_parse.h
# End Source File
# Begin Source File

SOURCE=.\nnet\xmlstream.h
# End Source File
# Begin Source File

SOURCE=.\nnet\xmltempl.h
# End Source File
# Begin Source File

SOURCE=.\nnet\xmlutils.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
