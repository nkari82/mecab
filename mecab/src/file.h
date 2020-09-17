#ifndef MECAB_FILE_H_
#define MECAB_FILE_H_

#include <unordered_map>
#include <iosfwd>
#include <streambuf>
#include <memory>

namespace MeCab {

	macab_io_file_t* default_io();
	const char* default_io_what();

	class iobuf : public std::streambuf
	{
	private:
		macab_io_file_t* io_;
		file_handle_t  handle_;
		char buffer_[8192];

	public:
		iobuf(const char* file, macab_io_file_t *io);
		~iobuf();
		int underflow();
	};

	class IMMap
	{
	public:
		using Ptr = std::shared_ptr<IMMap>;

		virtual ~IMMap() = default;

		virtual char* data() = 0;
		virtual char* data(size_t size) = 0;
		template<class T = char> T* operator[](int index) { return (T*)op_index(index, sizeof(T)); }
		template<class T = char> T* operator+(int offset) { return (T*)op_index(offset, sizeof(T)); }
		template<class T = char> void operator+=(int offset) { op_add(offset, sizeof(T)); }
		virtual void read(void* val, size_t size) = 0;
		virtual void read(int offset, void** val, size_t size) = 0;	// no copy
		virtual void read(int offset, char** str) = 0; // no copy string
		virtual Ptr clone() = 0;

	protected:
		virtual char* op_index(int offset, size_t stride) { return nullptr; }
		virtual void op_add(int offset, size_t stride) {}
	};

	class MMap : public IMMap
	{
	private:
		char* ptr_;
		char* orig_;
		size_t size_;

	public:
		MMap(void* ptr, size_t size) : ptr_((char*)ptr), orig_((char*)ptr), size_(size)
		{}

		char* data() override { return ptr_; }
		char* data(size_t size) override { return ptr_; }
		void read(void* val, size_t size) override { std::memcpy(val, ptr_, size); ptr_ += size; }
		void read(int offset, void** val, size_t size) override { *val = (ptr_ + offset); }
		void read(int offset, char** str) override { *str = (ptr_ + offset); }
		Ptr clone() override { return std::make_shared<MMap>(ptr_, size_ - (ptr_ - orig_)); }

	private:
		char* op_index(int offset, size_t stride) override { return ptr_ + (offset * stride); }
		void op_add(int offset, size_t stride) override { ptr_ += (offset * stride); }
	};

	class FileMap : public IMMap
	{
	private:
		std::unordered_map<size_t, char*> mmap_;
		macab_io_file_t* io_;
		file_handle_t handle_;
		int pos_;
		size_t size_;

	public:
		FileMap(macab_io_file_t* io, file_handle_t handle, size_t size, int pos = 0);

		~FileMap();

		char* data() override;
		char* data(size_t size) override;
		void read(void* val, size_t size) override;
		void read(int offset, void** val, size_t size) override;
		void read(int offset, char** str) override;
		Ptr clone() override;

	private:
		char* op_index(int offset, size_t stride) override;
		void op_add(int offset, size_t stride) override;
	};
}

#endif