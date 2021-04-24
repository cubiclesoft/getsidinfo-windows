@echo off
cls

cl /Ox getsidinfo.cpp utf8/*.cpp json/*.cpp -D_USING_V110_SDK71_ -DSUBSYSTEM_CONSOLE /link /FILEALIGN:512 /OPT:REF /OPT:ICF /INCREMENTAL:NO /subsystem:console,5.01 user32.lib netapi32.lib advapi32.lib /out:getsidinfo.exe
cl /Ox getsidinfo.cpp utf8/*.cpp json/*.cpp -D_USING_V110_SDK71_ -DSUBSYSTEM_WINDOWS /link /FILEALIGN:512 /OPT:REF /OPT:ICF /INCREMENTAL:NO /subsystem:windows,5.01 user32.lib shell32.lib netapi32.lib advapi32.lib /out:getsidinfo-win.exe
