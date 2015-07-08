
rem         Run this script before opening the Visual C++ project file

rem	The script configures Epos to crosscompile to a palmtop executable

copy ..\win\cpconv2.exe .
copy ..\win\8859-2.enc .
copy ..\win\accent.* .
copy ..\win\convert.bat .
call convert.bat
del cpconv2.exe

cd ..\..\src

move agent.cc agent.cpp
if not exist client.cpp move client.cc client.cpp
move daemon.cc daemon.cpp
move encoding.cc encoding.cpp
move function.cc function.cpp
move unit.cc unit.cpp
if not exist hash.cpp move hash.cc hash.cpp
move hashd.cc hashd.cpp
move interf.cc interf.cpp
move options.cc options.cpp
move ktdsyn.cc ktdsyn.cpp
move ptdsyn.cc ptdsyn.cpp
move lpcsyn.cc lpcsyn.cpp
move monolith.cc monolith.cpp
move parser.cc parser.cpp
move rule.cc rule.cpp
move say-epos.cc say.cpp
move synth.cc synth.cpp
move tcpsyn.cc tcpsyn.cpp
move tdpsyn.cc tdpsyn.cpp
move text.cc text.cpp
move ttscp.cc ttscp.cpp
move voice.cc voice.cpp
move waveform.cc waveform.cpp

rem        Do not rename exc.cc, block.cc, hashtmpl.cc etc. which are #included by other files
rem        ...but you need both client.cc and client.cpp as it is invoked
rem           both directly and indirectly, the same for hash.cc and hash.cpp

if not exist client.cc echo #include "client.cpp" > client.cc
if not exist hash.cc echo #include "hash.cpp" > hash.cc

del config.h
copy ..\arch\win-ce\config.in .\config.h
copy ..\arch\win-ce\epos.vcp .
rem copy ..\arch\win-ce\eposm.vcp .
mkdir say
copy ..\arch\win-ce\say.vcp say
copy ..\arch\win-ce\epos.vcw .

copy ..\arch\win-ce\stdlib.cpp .

cd ..\cfg\cfg

if not exist ..\..\arch\unix\epos.ini move .\epos.ini ..\..\arch\unix\epos.ini
copy ..\..\arch\win\epos.ini .\epos.ini

cd ..\..\arch\win-ce

