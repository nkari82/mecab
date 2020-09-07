#include <unordered_map>
#include "mecab.h"
#include "common.h"

namespace MeCab {
	std::unordered_map<int, std::string> s_files;

	static int open(const char *path)
	{
		return 0;
	}

	static void close(int handle)
	{

	}

	macab_io_file_t* default_io() {
		static macab_io_file_t io{ open, close };
		return &io;
	}
}