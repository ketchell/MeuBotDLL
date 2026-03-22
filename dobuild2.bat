@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x86 >nul 2>&1
cmake --build "C:\EasyBot-main\out\build\x86-Release" --target EasyBotDLL --clean-first > C:\easybot_build.log 2>&1
echo BUILD_EXIT_CODE=%ERRORLEVEL% >> C:\easybot_build.log
