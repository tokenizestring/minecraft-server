
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
	mc::blocks.load();
	mc::items.load();
	mc::generator.set_seed(mc::config.smp_seed);
	mc::world.set_spawn(mc::structures::world_spawn, { mc::spawn_x, mc::spawn_y, mc::spawn_z, mc::spawn_yaw });
	mc::world.set_spawn(mc::structures::world_smp, mc::generator.get_spawn());
	mc::edits.load();

	if (mc::java_registry.load() && mc::network.start())
	{
		mc::java_chunk.build();
		mc::java_command.build();
		mc::java_chat.build();
		mc::npc.load();
		mc::java_menu.build();
		mc::bedrock_menu.build();
		mc::bedrock_chunk.build();
		mc::bedrock_player.build();
		mc::bedrock_debug.load();
		mc::profile.build();
		mc::market.load();
		mc::drops.load();
		mc::network.run();
		mc::profile.save_all();
		mc::edits.shutdown();
		mc::drops.save();
		mc::java_chunk.shutdown();
		code = 0;
	}

	utils::log.write(mc::color_server, "[server](stop) exit %d", code);
	utils::log.close();

	return code;
}

//=====================================================================================
