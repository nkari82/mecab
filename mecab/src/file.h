#ifndef MECAB_FILE_H_
#define MECAB_FILE_H_

namespace MeCab {

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
		int relative_;
		char* orig_;
		size_t size_;

	public:
		MMap(void* ptr, size_t size);

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

	class FileMap : public IMMap
	{
	private:
		robin_hood::unordered_map<size_t, char*> mmap_;
		macab_io_file_t* io_;
		file_handle_t handle_;
		int relative_;
		int orig_;
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