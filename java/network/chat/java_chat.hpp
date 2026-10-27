
//=====================================================================================

#pragma once

#include "../../../core/engine/engine.hpp"

//=====================================================================================

namespace mc
{
	class java_chat_c
	{
	public:

		void build();
		void handle(structures::client_s& client, structures::reader_s& in);
		void handle_command(structures::client_s& client, structures::reader_s& in);
		void notify(structures::client_s& client, std::string_view message);
		void open(structures::client_s& client);
		void prepare(const std::vector<std::string>& lines);
		void show(structures::client_s& client, bool visible);

	private:

		void show_modern(structures::client_s& client, std::uint32_t shown);
		void show_1_8(structures::client_s& client, std::uint32_t shown);
		void send_display(structures::client_s& client, std::string_view objective);
		std::shared_ptr<const std::vector<std::uint8_t>> render_modern(structures::family_e family, const std::vector<std::string>& lines);
		std::shared_ptr<const std::vector<std::uint8_t>> render_1_8(const std::vector<std::string>& lines);

		static std::string_view get_1_8_text(std::string_view text);

		std::array<std::shared_ptr<const std::vector<std::uint8_t>>, structures::family_count> boards;
		std::array<std::shared_ptr<const std::vector<std::uint8_t>>, structures::family_count> clears;
		std::uint32_t count;
		std::string line;
		std::vector<std::uint8_t> packet;
	};

	extern java_chat_c java_chat;
}

//=====================================================================================
