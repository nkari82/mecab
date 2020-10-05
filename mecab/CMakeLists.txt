cmake_minimum_required(VERSION 3.11.4)
project(mecab)

cmake_host_system_information(RESULT _os QUERY OS_NAME)
cmake_host_system_information(RESULT _platform QUERY OS_PLATFORM)
	
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

mark_as_advanced(ROBIN_HOOD_INCLUDE_DIR)
find_path(ROBIN_HOOD_INCLUDE_DIR NAMES "robin_hood.h" DOC "robin_hood include directory")
find_path(ICONV_INCLUDE_DIRS NAMES iconv.h paths {proper-location})

configure_file(src/config.h.cmake ${CMAKE_CURRENT_SOURCE_DIR}/src/config.h)
include (CheckIncludeFiles)
include (CheckSymbolExists)
include (CheckFunctionExists)
include (CheckCXXCompilerFlag)
include (CheckLibraryExists)

check_include_files ( ctype.h HAVE_CTYPE_H )
check_include_files ( dirent.h HAVE_DIRENT_H )
check_include_files ( dlfcn.h HAVE_DLFCN_H )
check_include_files ( fcntl.h HAVE_FCNTL_H )
check_include_files ( inttypes.h HAVE_INTTYPES_H )
check_include_files ( io.h HAVE_IO_H )
check_include_files ( memory.h HAVE_MEMORY_H )
check_include_files ( pthread.h HAVE_PTHREAD_H )
check_include_files ( stdint.h HAVE_STDINT_H )
check_include_files ( stdlib.h HAVE_STDLIB_H )
check_include_files ( strings.h HAVE_STRING_H )
check_include_files ( sys/mman.h HAVE_SYS_MMAN_H )
check_include_files ( sys/param.h HAVE_SYS_PARAM_H )
check_include_files ( sys/stat.h HAVE_SYS_STAT_H )
check_include_files ( sys/times.h HAVE_SYS_TIMES_H )
check_include_files ( sys/types.h HAVE_SYS_TYPES_H )
check_include_files ( sys/unistd.h HAVE_UNISTD_H )
check_include_files ( windows.h HAVE_WINDOWS_H )
check_include_files ( ${ICONV_INCLUDE_DIRS}/iconv.h HAVE_ICONV )

check_function_exists ( getenv HAVE_GETENV )
check_function_exists ( mmap HAVE_MMAP )

check_cxx_source_compiles( "
#include <sched.h>
int main()
{
  int a = 10;
  __sync_fetch_and_add(&a, 10);
  __sync_val_compare_and_swap(&a, 0, 10);
  sched_yield();
  return 0;
}" HAVE_GCC_ATOMIC_OPS)

check_cxx_source_compiles( "
#include <libkern/OSAtomic.h>
int main()
{
  int a = 10;
  OSAtomicAdd32(10, &a);
  OSAtomicCompareAndSwapInt(10, 0, &a);
  return 0;
}" HAVE_OSX_ATOMIC_OPS)

check_cxx_source_compiles( "
int main()
{
  __thread int a = 0;
  a = 10;
  return 0;
}" HAVE_TLS_KEYWORD)

option(BUILD_SHARED_LIBS "Build shared library" OFF)

set(SOURCES src/feature_index.cpp
	src/param.cpp
	src/learner.cpp
	src/string_buffer.cpp
	src/char_property.cpp
	src/learner_tagger.cpp
	src/tagger.cpp
	src/connector.cpp
	src/tokenizer.cpp
	src/context_id.cpp
	src/dictionary.cpp
	src/utils.cpp
	src/dictionary_compiler.cpp
	src/viterbi.cpp
	src/dictionary_generator.cpp
	src/writer.cpp
	src/iconv_utils.cpp
	src/dictionary_rewriter.cpp
	src/lbfgs.cpp
	src/eval.cpp
	src/nbest_generator.cpp
	src/libmecab.cpp
	src/file.cpp)
	
add_definitions( -DHAVE_CONFIG_H 
					 -DMECAB_USE_THREAD 
					 -DHAVE_GETENV 
					 -D_UNICODE
					 -DDIC_VERSION=102
					 -DVERSION="0.996"
					 -DPACKAGE="mecab"
					 -DMECAB_DEFAULT_RC="mecabrc")	
if(MSVC)
add_definitions (-D_CRT_SECURE_NO_DEPRECATE)
endif()

set(ADDITIONAL_LIBRARIES libmecab)
if (BUILD_SHARED_LIBS)
	add_library(libmecab SHARED ${SOURCES})
else()
	add_definitions( -DMECAB_STATIC )
	add_library(libmecab STATIC ${SOURCES})
endif()

target_include_directories(libmecab PRIVATE src / ${ICONV_INCLUDE_DIRS})

add_executable (mecab src/mecab.cpp)
target_include_directories(mecab PRIVATE src)
target_link_libraries (mecab LINK_PUBLIC ${ADDITIONAL_LIBRARIES})

add_executable (mecab-dict-index src/mecab-dict-index.cpp)
target_include_directories(mecab-dict-index PRIVATE src)
target_link_libraries (mecab-dict-index LINK_PUBLIC ${ADDITIONAL_LIBRARIES})

add_executable (mecab-dict-gen src/mecab-dict-gen.cpp)
target_include_directories(mecab-dict-gen PRIVATE src)
target_link_libraries (mecab-dict-gen LINK_PUBLIC ${ADDITIONAL_LIBRARIES})

add_executable (mecab-cost-train src/mecab-cost-train.cpp)
target_include_directories(mecab-cost-train PRIVATE src)
target_link_libraries (mecab-cost-train LINK_PUBLIC ${ADDITIONAL_LIBRARIES})

add_executable (mecab-system-eval src/mecab-system-eval.cpp)
target_include_directories(mecab-system-eval PRIVATE src)
target_link_libraries (mecab-system-eval LINK_PUBLIC ${ADDITIONAL_LIBRARIES})

add_executable (mecab-test-gen src/mecab-dict-index.cpp)
target_include_directories(mecab-test-gen PRIVATE src)
target_link_libraries (mecab-test-gen LINK_PUBLIC ${ADDITIONAL_LIBRARIES})