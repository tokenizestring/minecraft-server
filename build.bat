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
    game\world\world\world.cpp ^
    game\world\terrain\terrain.cpp ^
    game\world\blocks\blocks\blocks.cpp ^
    game\world\blocks\light\light.cpp ^
    game\world\generation\caverns\caverns.cpp ^
    game\world\generation\features\features.cpp ^
    game\world\generation\flora\flora.cpp ^
    game\world\generation\generator\generator.cpp ^
    game\world\storage\edits\edits.cpp ^
    game\world\storage\lookup\lookup.cpp ^
    game\world\storage\surface\surface.cpp ^
    game\world\entities\drops\drops.cpp ^
    game\world\entities\mobs\mobs.cpp ^
    game\world\farming\crops\crops.cpp ^
    game\world\farming\growth\growth.cpp ^
    game\player\items\items.cpp ^
    game\player\inventory\inventory.cpp ^
    game\player\profile\profile.cpp ^
    game\player\vitals\vitals.cpp ^
    game\economy\market\market.cpp ^
    game\social\chat\chat.cpp ^
    game\social\command\command.cpp ^
    game\social\friends\friends.cpp ^
    game\social\hub\hub.cpp ^
    java\network\registry\java_registry.cpp ^
    java\network\protocol\java_protocol.cpp ^
    java\network\play\java_play.cpp ^
    java\network\chat\java_chat.cpp ^
    java\network\command\java_command.cpp ^
    java\world\builder\java_builder.cpp ^
    java\world\chunk\java_chunk.cpp ^
    java\world\block\java_block.cpp ^
    java\world\glow\java_glow.cpp ^
    java\entities\player\java_player.cpp ^
    java\entities\drops\java_drops.cpp ^
    java\entities\mobs\java_mobs.cpp ^
    java\interface\menu\java_menu.cpp ^
    java\interface\inventory\java_inventory.cpp ^
    java\interface\market\java_market.cpp ^
    java\interface\social\java_social.cpp ^
    bedrock\network\raknet\raknet.cpp ^
    bedrock\network\packet\bedrock_packet.cpp ^
    bedrock\network\login\bedrock_login.cpp ^
    bedrock\network\play\bedrock_play.cpp ^
    bedrock\network\debug\bedrock_debug.cpp ^
    bedrock\world\chunk\bedrock_chunk.cpp ^
    bedrock\entities\player\bedrock_player.cpp ^
    bedrock\interface\chat\bedrock_chat.cpp ^
    bedrock\interface\menu\bedrock_menu.cpp ^
    bedrock\interface\market\bedrock_market.cpp ^
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
