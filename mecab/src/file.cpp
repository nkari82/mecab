#include <unordered_map>
#include <cassert>
#include "mecab.h"
#include "common.h"
#include "utils.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(_WIN32) && !defined(__CYGWIN__)
#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif
#else
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#endif

namespace MeCab {
	struct file_t
	{
#if defined(_WIN32) && !defined(__CYGWIN__)
		HANDLE handle;
		HANDLE map;
#else
		int handle;
#endif
		int	mode;
		void* view;
		size_t length;
		std::string path;
	};

	static std::unordered_map<file_handle_t, file_t> s_files;
	static whatlog what_;

	static file_handle_t open(const char *path, const char *mode, size_t* length, void **mapped)
	{
		if (!path) return 0;

		bool read = !strcmp(mode, "r");
		bool write = !strcmp(mode, "r+");
		if(!read && !write)
			CHECK_FALSE(false) << "unknown open mode:" << path;

		file_handle_t handle = (file_handle_t)std::hash<std::string>{}(path);
		if (s_files.find(handle) != s_files.end())
			return 0;

		file_t file;
		std::memset(&file, 0, sizeof(file));
		file.path = path;
#if defined(_WIN32) && !defined(__CYGWIN__)
		file.mode = GENERIC_READ | (write ? GENERIC_WRITE : 0);
		file.handle = ::CreateFileW(WPATH_FORCE(path), file.mode, !mapped ? 0 : FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if ( file.handle == INVALID_HANDLE_VALUE )
			return 0;

		file.length = ::GetFileSize(file.handle, 0);

		if (mapped != nullptr)
		{
			file.map = ::CreateFileMapping(file.handle, 0, write ? PAGE_READWRITE : PAGE_READONLY, 0, 0, 0);
			*mapped = file.view = ::MapViewOfFile(file.map, write ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ, 0, 0, 0);
		}
#else
		file.mode = (write ? O_RDWR : O_RDONLY) | O_BINARY;
		file.handle = ::open(path, file.mode);
		if (file.handle == 0)
			return 0;

		struct stat st;
		::fstat(file.handle, &st);
		file.length = st.st_size;

		if (mapped != nullptr)
		{
#ifdef HAVE_MMAP
			file.view = ::mmap(0, file.length, PROT_READ | (write ? PROT_WRITE : 0), MAP_SHARED, file.handle, 0);
#else
			file.view = malloc(file.length);
			::read(file.handle, file.view, file.length);
			*mapped = file.view;
#endif
		}
#endif
		if (length != nullptr)
			*length = file.length;

		s_files.insert(s_files.end(), std::make_pair(handle, file));
		return handle;
	}

	static void close(file_handle_t handle)
	{
		auto it = s_files.find(handle);
		if (it == s_files.end())
			return;

		auto& file = it->second;
#if defined(_WIN32) && !defined(__CYGWIN__)
		if( file.view != nullptr) ::UnmapViewOfFile(file.view);
		if( file.map != 0 ) ::CloseHandle(file.map);
		::CloseHandle(file.handle);
#else
		if (file.view != nullptr)
		{
#ifdef HAVE_MMAP
			::munmap(file.view, file.length);
#else
			if (file.mode & O_RDWR) {
				::write(file.handle, file.view, file.length);
			}
			free(file.view);
#endif
		}
		::close(file.handle);
#endif
		s_files.erase(it);
	}

	static size_t read(file_handle_t handle, char *buffer, size_t size)
	{
		auto it = s_files.find(handle);
		if (it == s_files.end())
			return 0;

		auto& file = it->second;
		
		size_t read(0);
#if defined(_WIN32) && !defined(__CYGWIN__)
		ReadFile(file.handle, buffer, (DWORD)size, (DWORD*)&read, nullptr);
#else
		read = ::read(file.handle, buffer, size);
#endif
		return read;
	}

	macab_io_file_t* default_io() {
		static macab_io_file_t io{ open, close, read };
		return &io;
	}

	iobuf::iobuf(const char* file, macab_io_file_t *io) : io_(io ? io : default_io()), handle_(0)
	{
		handle_ = io->open(file, "r", nullptr, nullptr);
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
		return this->gptr() == this->egptr() ? traits_type::eof() : traits_type::to_int_type(*this->gptr());
	}
}