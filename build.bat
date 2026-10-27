@echo off

cd /d "%~dp0"

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"

for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set "VS_PATH=%%i"
)

if not defined VS_PATH (
    echo could not find visual studio
    exit /b 1
)

call "%VS_PATH%\VC\Auxiliary\Build\vcvars64.bat" > nul 2>&1

if not exist "build\obj" mkdir "build\obj"
if not exist "build\bin\data" mkdir "build\bin\data"

set "DEFS=/DNDEBUG /D_CONSOLE /D_CRT_SECURE_NO_WARNINGS"

set "CFLAGS=/nologo /MP /O2 /Oi /Ot /GL /Gy /GS- /std:c++20 /EHsc /MT /W3 %DEFS% /I."

echo.
echo [server]

cl.exe %CFLAGS% ^
    main.cpp ^
    utils\log\log.cpp ^
    core\config\config.cpp ^
    core\zlib\zlib.cpp ^
    core\network\network.cpp ^
    core\deflate\deflate.cpp ^
    game\world\world.cpp ^
    game\terrain\terrain.cpp ^
    game\blocks\blocks.cpp ^
    game\light\light.cpp ^
    game\caverns\caverns.cpp ^
    game\features\features.cpp ^
    game\flora\flora.cpp ^
    game\generator\generator.cpp ^
    game\edits\edits.cpp ^
    game\lookup\lookup.cpp ^
    game\items\items.cpp ^
    game\inventory\inventory.cpp ^
    game\profile\profile.cpp ^
    game\market\market.cpp ^
    game\drops\drops.cpp ^
    game\chat\chat.cpp ^
    game\command\command.cpp ^
    game\npc\npc.cpp ^
    java\registry\java_registry.cpp ^
    java\protocol\java_protocol.cpp ^
    java\builder\java_builder.cpp ^
    java\chunk\java_chunk.cpp ^
    java\play\java_play.cpp ^
    java\player\java_player.cpp ^
    java\chat\java_chat.cpp ^
    java\command\java_command.cpp ^
    java\block\java_block.cpp ^
    java\menu\java_menu.cpp ^
    java\inventory\java_inventory.cpp ^
    java\market\java_market.cpp ^
    java\drops\java_drops.cpp ^
    java\glow\java_glow.cpp ^
    bedrock\raknet\raknet.cpp ^
    bedrock\packet\bedrock_packet.cpp ^
    bedrock\login\bedrock_login.cpp ^
    bedrock\chunk\bedrock_chunk.cpp ^
    bedrock\play\bedrock_play.cpp ^
    bedrock\player\bedrock_player.cpp ^
    bedrock\chat\bedrock_chat.cpp ^
    bedrock\menu\bedrock_menu.cpp ^
    bedrock\market\bedrock_market.cpp ^
    bedrock\debug\bedrock_debug.cpp ^
    /Fo"build\obj\\" ^
    /Fe"build\bin\server.exe" ^
    /link /LTCG /INCREMENTAL:NO /OPT:REF /OPT:ICF /SUBSYSTEM:CONSOLE ^
    ws2_32.lib kernel32.lib

if errorlevel 1 (
    echo server build failed
    exit /b 1
)

xcopy /e /i /y /q "data" "build\bin\data" > nul

echo.
echo [done] build\bin\server.exe

exit /b 0
