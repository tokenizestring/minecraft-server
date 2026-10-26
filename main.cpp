
//=====================================================================================

#include "stdafx.hpp"

//=====================================================================================

static BOOL WINAPI on_console(DWORD type)
{
	mc::network.stop();
	return (type == CTRL_C_EVENT || type == CTRL_BREAK_EVENT);
}
/*
//=====================================================================================
*/
std::int32_t main()
{
	auto code{ 1 };

	utils::log.open(mc::log_file_name);
	SetConsoleTitleA("minecraft server");
	SetConsoleCtrlHandler(on_console, TRUE);

	utils::log.write(mc::color_server, "[server](start) '%s'", mc::server_versions);
	mc::config.load();
	mc::terrain.load();

	if (mc::java_registry.load() && mc::network.start())
	{
		mc::java_chunk.build();
		mc::java_command.build();
		mc::java_chat.build();
		mc::bedrock_chunk.build();
		mc::bedrock_player.build();
		mc::bedrock_debug.load();
		mc::network.run();
		code = 0;
	}

	utils::log.write(mc::color_server, "[server](stop) exit %d", code);
	utils::log.close();

	return code;
}

//=====================================================================================
