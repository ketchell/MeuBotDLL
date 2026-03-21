@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x86
cmake --build "C:\EasyBot-main\out\build\x86-Release" --target EasyBotDLL --clean-first
