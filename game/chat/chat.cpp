
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace mc
{
	chat_c chat;

	void chat_c::join(structures::player_s& player)
	{
		next_color = (next_color + 1u + static_cast<std::uint32_t>(functions::get_time_ms() % (std::size(chat_colors) - 1u))) % std::size(chat_colors);

		player.sidebar = false;
		player.chat_visible = true;
		player.color = chat_colors[next_color];
		open(player);
	}
	/*
	//=====================================================================================
	*/
	void chat_c::say(const structures::player_s& author, std::string_view message)
	{
		if (message.size())
		{
			utils::log.write(color_chat, "[chat](%s) <%s> %.*s", author.edition == structures::edition_bedrock ? "bedrock" : "java", author.name.c_str(), static_cast<std::int32_t>(message.size()), message.data());

			text.assign("<\xC2\xA7").append(1u, author.color).append(author.name).append("\xC2\xA7r> ").append(message);
			add_lines(text, functions::get_time_ms());
			refresh();
		}
	}
	/*
	//=====================================================================================
	*/
	void chat_c::tick(std::uint64_t now)
	{
		auto expired{ static_cast<std::size_t>(std::count_if(times.begin(), times.end(), [now](auto time) { return now - time >= config.chat_fade_ms; })) };

		if (expired)
		{
			lines.erase(lines.begin(), lines.begin() + expired);
			times.erase(times.begin(), times.begin() + expired);
			refresh();
		}
	}
	/*
	//=====================================================================================
	*/
	void chat_c::toggle(structures::player_s& player, std::string_view argument)
	{
		auto visible{ !player.chat_visible };

		if (argument == "on" || argument == "1")
		{
			visible = true;
		}

		else if (argument == "off" || argument == "0")
		{
			visible = false;
		}

		player.chat_visible = visible;
		show(player);
		notify(player, visible ? "chat on" : "chat off");
	}
	/*
	//=====================================================================================
	*/
	void chat_c::notify(structures::player_s& player, std::string_view message)
	{
		if (player.edition == structures::edition_bedrock)
		{
			bedrock_chat.notify(*static_cast<structures::session_s*>(player.link), message);
		}

		else
		{
			java_chat.notify(*static_cast<structures::client_s*>(player.link), message);
		}
	}
	/*
	//=====================================================================================
	*/
	void chat_c::add_lines(std::string_view message, std::uint64_t now)
	{
		auto start{ 0u };

		while (start < message.size())
		{
			auto end{ start };
			auto space{ 0u };

			for (auto points{ 0u }; end < message.size() && points < chat_wrap; ++points)
			{
				end += 1u;

				while (end < message.size() && (static_cast<std::uint8_t>(message[end]) >> 6u) == 0x02u)
				{
					end += 1u;
				}

				if (end < message.size() && message[end] == ' ')
				{
					space = end;
				}
			}

			if (end < message.size() && space > start)
			{
				end = space;
			}

			lines.emplace_back(message.substr(start, end - start));
			times.push_back(now);
			start = end;

			while (start < message.size() && message[start] == ' ')
			{
				start += 1u;
			}
		}

		if (lines.size() > config.chat_lines)
		{
			lines.erase(lines.begin(), lines.end() - config.chat_lines);
			times.erase(times.begin(), times.end() - config.chat_lines);
		}
	}
	/*
	//=====================================================================================
	*/
	void chat_c::refresh()
	{
		java_chat.prepare(lines);
		bedrock_chat.prepare(lines);

		for (auto viewer : world.get_players())
		{
			show(*viewer);
		}
	}
	/*
	//=====================================================================================
	*/
	void chat_c::open(structures::player_s& player)
	{
		if (player.edition == structures::edition_java)
		{
			java_chat.open(*static_cast<structures::client_s*>(player.link));
		}

		show(player);
	}
	/*
	//=====================================================================================
	*/
	void chat_c::show(structures::player_s& viewer)
	{
		if (viewer.edition == structures::edition_bedrock)
		{
			bedrock_chat.show(*static_cast<structures::session_s*>(viewer.link), viewer.chat_visible);
		}

		else
		{
			java_chat.show(*static_cast<structures::client_s*>(viewer.link), viewer.chat_visible);
		}
	}
}

//=====================================================================================
