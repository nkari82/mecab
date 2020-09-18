#include <unordered_map>
#include <cassert>
#include "mecab.h"
#include "common.h"
#include "file.h"
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
		file_handle_t handle;
#if defined(_WIN32) && !defined(__CYGWIN__)
		HANDLE native;
		HANDLE map;
#else
		int native;
#endif
		int	mode;
		void* view;
		size_t length;
		std::string path;
	};

	static file_t* s_current(nullptr);

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
		file.handle = handle;
		file.path = path;
#if defined(_WIN32) && !defined(__CYGWIN__)
		file.mode = GENERIC_READ | (write ? GENERIC_WRITE : 0);
		file.native = ::CreateFileW(WPATH_FORCE(path), file.mode, !mapped ? 0 : FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		CHECK_FALSE(file.native != INVALID_HANDLE_VALUE) << "CreateFile() failed: " << path;

		file.length = ::GetFileSize(file.native, 0);
		if (mapped != nullptr)
		{
			file.map = ::CreateFileMapping(file.native, 0, write ? PAGE_READWRITE : PAGE_READONLY, 0, 0, 0);
			CHECK_FALSE(file.map) << "CreateFileMapping() failed: " << path;
			*mapped = file.view = ::MapViewOfFile(file.map, write ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ, 0, 0, 0);
			CHECK_FALSE(file.view) << "MapViewOfFile() failed: " << path;
		}
#else
		file.mode = (write ? O_RDWR : O_RDONLY) | O_BINARY;
		CHECK_FALSE((file.native = ::open(path, file.mode)) >= 0) << "open failed: " << path;

		struct stat st;
		::fstat(file.native, &st);
		file.length = st.st_size;

		if (mapped != nullptr)
		{
#ifdef HAVE_MMAP
			CHECK_FALSE((file.view = ::mmap(0, file.length, PROT_READ | (write ? PROT_WRITE : 0), MAP_SHARED, file.native, 0)) != MAP_FAILED)
				<< "mmap() failed: " << path;
#else
			file.view = malloc(file.length);
			CHECK_FALSE(::read(file.native, file.view, file.length) >= 0) << "read() failed: " << path;
			*mapped = file.view;
#endif
		}
#endif
		if (length != nullptr)
			*length = file.length;

		s_files.insert(s_files.end(), std::make_pair(handle, file));
		s_current = &file;
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
		::CloseHandle(file.native);
#else
		if (file.view != nullptr)
		{
#ifdef HAVE_MMAP
			::munmap(file.view, file.length);
#else
			if (file.mode & O_RDWR) {
				::write(file.native, file.view, file.length);
			}
			free(file.view);
#endif
		}
		::close(file.handle);
#endif
		s_files.erase(it);
		if (!s_files.empty() && s_current->handle == handle)
			s_current = &s_files.begin()->second;
	}

	static size_t read(file_handle_t handle, char *buffer, size_t size)
	{
		if (s_current->handle != handle)
		{
			auto it = s_files.find(handle);
			if (it == s_files.end())
				return 0;

			s_current = &it->second;
		}
		
		size_t read(0);
#if defined(_WIN32) && !defined(__CYGWIN__)
		ReadFile(s_current->native, buffer, (DWORD)size, (DWORD*)&read, nullptr);
#else
		read = ::read(s_current->native, buffer, size);
#endif
		return read;
	}

	static void seek(file_handle_t handle, int offset)
	{
		if (s_current->handle != handle)
		{
			auto it = s_files.find(handle);
			if (it == s_files.end())
				return;

			s_current = &it->second;
		}
		
#if defined(_WIN32) && !defined(__CYGWIN__)
		SetFilePointer(s_current->native, offset, nullptr, FILE_BEGIN);
#else
		::lseek(s_current->native, offset, 0);
#endif
	}

	macab_io_file_t* default_io() {
		static macab_io_file_t io{ open, close, read, seek };
		return &io;
	}

	const char* default_io_what() {
		return what_.str();
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

	FileMap::FileMap(macab_io_file_t* io, file_handle_t handle, size_t size, int pos)
		: io_(io)
		, handle_(handle)
		, size_(size)
		, relative_(0)
		, orig_(pos)
	{}

	FileMap::~FileMap()
	{
		for (auto& pair : mmap_)
			delete pair.second;
	}

	char* FileMap::data(size_t size)
	{
		char* val(nullptr);
		read(0, (void**)&val, size);
		return val;
	}

	char* FileMap::data()
	{
		char* val(nullptr);
		read(0, (void**)&val, size_ - relative_);
		return val;
	}

	void FileMap::read(void* val, size_t size)
	{
		io_->read(handle_, (char*)val, size);
		relative_ += (int)size;
	}

	void FileMap::read(int offset, void** val, size_t size)
	{
		auto pos = relative_ + offset;
		auto it = mmap_.find(pos);
		if (it == mmap_.end())
		{
			*val = new char[size];
			io_->seek(handle_, orig_ + pos);
			io_->read(handle_, (char*)*val, size);
			mmap_.emplace(std::make_pair(offset, (char*)*val));
		}
		else
		{
			*val = it->second;
		}
	}

	void FileMap::read(int offset, char** str)
	{
		auto pos = relative_ + offset;
		auto it = mmap_.find(pos);
		if (it == mmap_.end())
		{
			io_->seek(handle_, orig_ + pos);

			char temp[512];
			size_t len(0);
			do 
			{
				io_->read(handle_, &temp[len], 1);
			} while (temp[len++] != '\0');

			*str = new char[len];
			std::memcpy(*str, temp, len);
			mmap_.emplace(std::make_pair(offset, (char*)*str));
		}
		else
		{
			*str = it->second;
		}
	}

	IMMap::Ptr FileMap::clone()
	{
		return std::make_shared<FileMap>(io_, handle_, size_ - relative_, orig_ + relative_);
	}

	char* FileMap::op_index(int offset, size_t stride)
	{
		char* val(nullptr);
		read(orig_ + int(offset * stride), (void**)&val, stride);
		return val;
	}

	void FileMap::op_add(int offset, size_t stride)
	{
		relative_ += int(offset * stride);
	}

	MMap::MMap(void* ptr, size_t size) 
		: relative_(0)
		, orig_((char*)ptr)
		, size_(size)
	{}

	char* MMap::data()
	{
#if defined(_DEBUG)
		auto remain = size_ - relative_;
		(void)remain; // unused
#endif
		return orig_ + relative_;
	}

	char* MMap::data(size_t size)
	{
#if defined(_DEBUG)
		auto remain = size_ - relative_;
		(void)remain; // unused
#endif
		return orig_ + relative_;
	}

	void MMap::read(void* val, size_t size)
	{
		std::memcpy(val, orig_ + relative_, size);
		relative_ += (int)size;
	}

	void MMap::read(int offset, void** val, size_t size)
	{
		*val = (orig_ + relative_ + offset);
	}

	void MMap::read(int offset, char** str)
	{
		*str = (orig_ + relative_ + offset);
	}

	IMMap::Ptr MMap::clone()
	{
		return std::make_shared<MMap>(orig_ + relative_, size_ - relative_);
	}

	char* MMap::op_index(int offset, size_t stride)
	{
		return orig_ + relative_ + (offset * stride);
	}

	void MMap::op_add(int offset, size_t stride)
	{
		relative_ += int(offset * stride);
	}
}