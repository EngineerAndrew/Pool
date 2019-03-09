@echo off

SET cfiles=..\code\main.cpp
SET includes=/I C:\SDL2-2.0.7\include

pushd ..\build
cl /Zi /FC /nologo %cfiles% %includes% /link /LIBPATH:C:/SDL2-2.0.7/lib/x64 SDL2.lib SDL2main.lib SDL2_ttf.lib /SUBSYSTEM:WINDOWS /Entry:mainCRTStartup
popd