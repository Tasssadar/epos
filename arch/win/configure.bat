
rem         Run this script before opening the Visual C++ project file

if exist cpconv.exe call convert.bat

cd ..\..\src

move agent.cc agent.cpp
if not exist client.cpp move client.cc client.cpp
move daemon.cc daemon.cpp
move encoding.cc encoding.cpp
move function.cc function.cpp
move daemon.cc daemon.cpp
move unit.cc unit.cpp
move hash.cc hash.cpp
move hashd.cc hashd.cpp
move hashi.cc hashi.cpp
move interf.cc interf.cpp
move lpcsyn.cc lpcsyn.cpp
move mbrsyn.cc mbrsyn.cpp
move monolith.cc monolith.cpp
move nonblock.cc nonblock.cpp
move options.cc options.cpp
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

move nnet\enumstring.cc nnet\enumstring.cpp
move nnet\map.cc nnet\map.cpp
move nnet\matrix.cc nnet\matrix.cpp
move nnet\neural.cc nnet\neural.cpp
move nnet\neural_parse.cc nnet\neural_parse.cpp
move nnet\perceptron.cc nnet\perceptron.cpp
move nnet\percinit.cc nnet\percinit.cpp
move nnet\percstruct.cc nnet\percstruct.cpp
move nnet\set.cc nnet\set.cpp
move nnet\stream.cc nnet\stream.cpp
move nnet\traindata.cc nnet\traindata.cpp
move nnet\utils.cc nnet\utils.cpp
move nnet\xml.cc nnet\xml.cpp
move nnet\xml_parse.cc nnet\xml_parse.cpp
move nnet\xmltempl.cc nnet\xmltempl.cpp
move nnet\xmlutils.cc nnet\xmlutils.cpp

rem        Do not rename exc.cc, block.cc, hashtmpl.cc etc. which are #included by other files
rem        ...but you need both client.cc and client.cpp as it is invoked
rem           both directly and indirectly, the same for hash.cc and hash.cpp

echo #include "..\epos.h" > nnet\epos.h
if not exist client.cc echo #include "client.cpp" > client.cc

del config.h
copy ..\arch\win\config.in .\config.h
copy ..\arch\win\epos.dsw .
copy ..\arch\win\epos.dsp .
copy ..\arch\win\say.dsp .
copy ..\arch\win\instserv.dsp .
copy ..\arch\win\eposm.dsp .

copy ..\arch\win\service\*.* .

cd ..\cfg\cfg

if not exist ..\..\arch\unix\epos.ini move .\epos.ini ..\..\arch\unix\epos.ini
copy ..\..\arch\win\epos.ini .\epos.ini

cd ..\..\arch\win

