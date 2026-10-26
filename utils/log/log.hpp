
//=====================================================================================

#pragma once

#include "../../stdafx.hpp"

//=====================================================================================

namespace utils
{
	class log_c
	{
	public:

		bool open(const char* file_name);
		void write(std::uint32_t color, const char* format, ...);
		void close();

	private:

		std::FILE* file;
	};

	extern log_c log;
}

//=====================================================================================
