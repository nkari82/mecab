#include <unordered_map>
#include "mecab.h"
#include "common.h"

namespace MeCab {
	std::unordered_map<size_t, std::pair<FILE*,std::string>> s_files;

	static size_t open(const char *path, const char *mode)
	{
		if (!path)
			return 0;
		
		size_t handle = (int)std::hash<std::string>{}(path);
		if (s_files.find(handle) != s_files.end())
			return 0;

		FILE* file = fopen(path, mode);
		if (!file)
			return 0;

		s_files.insert(s_files.end(), std::make_pair(handle, std::make_pair(file, std::string(path))));
		return handle;
	}

	static void close(size_t handle)
	{
		auto it = s_files.find(handle);
		if (it == s_files.end())
			return;

		auto& pair = it->second;
		fclose(pair.first);
	}

	static size_t read(size_t handle, char *buffer, size_t size)
	{
		auto it = s_files.find(handle);
		if (it == s_files.end())
			return 0;

		auto& pair = it->second;
		return fread(buffer, 1, size, pair.first);
	}

	macab_io_file_t* default_io() {
		static macab_io_file_t io{ open, close, read };
		return &io;
	}


	iobuf::iobuf(const char* file, macab_io_file_t *io) : io_(io), handle_(0)
	{
		handle_ = io->open(file, "r");
	}

	iobuf::~iobuf()
	{
		io_->close(handle_);
	}

	int iobuf::underflow()
	{
		if (this->gptr() == this->egptr() && this->handle_)
		{
			size_t size = io_->read(this->handle_, buffer_, sizeof(buffer_));
			this->setg(this->buffer_, this->buffer_, this->buffer_ + size);
		}
		return this->gptr() == this->egptr()
			? traits_type::eof()
			: traits_type::to_int_type(*this->gptr());
	}
}