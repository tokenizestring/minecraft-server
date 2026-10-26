
//=====================================================================================

#include "../../stdafx.hpp"

//=====================================================================================

namespace utils
{
	log_c log;

	bool log_c::open(const char* file_name)
	{
		if (auto console{ GetStdHandle(STD_OUTPUT_HANDLE) }; console != INVALID_HANDLE_VALUE)
		{
			DWORD mode{ 0u };
			GetConsoleMode(console, &mode);
			SetConsoleMode(console, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
			SetConsoleOutputCP(CP_UTF8);
		}

		file = std::fopen(mc::functions::get_local_path(file_name).c_str(), "a");

		return (file != nullptr);
	}
	/*
	//=====================================================================================
	*/
	void log_c::write(std::uint32_t color, const char* format, ...)
	{
		std::array<char, 1024u> text{};

		va_list arguments;
		va_start(arguments, format);
		std::vsnprintf(text.data(), text.size(), format, arguments);
		va_end(arguments);

		auto now{ std::time(nullptr) };
		std::tm local{};
		localtime_s(&local, &now);

		std::printf("\x1b[38;2;110;110;110m[%02d:%02d:%02d]\x1b[0m \x1b[38;2;%u;%u;%um%s\x1b[0m\n", local.tm_hour, local.tm_min, local.tm_sec, (color >> 16u) & 0xFFu, (color >> 8u) & 0xFFu, color & 0xFFu, text.data());

		if (file)
		{
			std::fprintf(file, "[%04d-%02d-%02d %02d:%02d:%02d] %s\n", local.tm_year + 1900, local.tm_mon + 1, local.tm_mday, local.tm_hour, local.tm_min, local.tm_sec, text.data());
			std::fflush(file);
		}
	}
	/*
	//=====================================================================================
	*/
	void log_c::close()
	{
		if (file)
		{
			std::fclose(file);
			file = nullptr;
		}
	}
}

//=====================================================================================
