cmake_minimum_required(VERSION 3.10.2)
project(mecab)

cmake_host_system_information(RESULT _os QUERY OS_NAME)
cmake_host_system_information(RESULT _platform QUERY OS_PLATFORM)
	
add_library(libmecab STATIC
	src/feature_index.cpp
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
	src/libmecab.cpp)
	
add_definitions( -DMECAB_STATIC
					 -D_CRT_SECURE_NO_DEPRECATE
					 -DMECAB_USE_THREAD 
					 -DHAVE_GETENV 
					 -DHAVE_WINDOWS_H 
					 -D_UNICODE
					 -DDIC_VERSION=102
					 -DVERSION="0.996"
					 -DPACKAGE="mecab"
					 -DMECAB_DEFAULT_RC="c:/mecab/etc/mecabrc")
target_include_directories(libmecab PRIVATE ./src ./)

add_executable (mecab src/mecab.cpp)
target_include_directories(mecab PRIVATE ./src)
target_link_libraries (mecab LINK_PUBLIC libmecab)

add_executable (mecab-dict-index src/mecab-dict-index.cpp)
target_include_directories(mecab-dict-index PRIVATE ./src)
target_link_libraries (mecab-dict-index LINK_PUBLIC libmecab)

add_executable (mecab-dict-gen src/mecab-dict-gen.cpp)
target_include_directories(mecab-dict-gen PRIVATE ./src)
target_link_libraries (mecab-dict-gen LINK_PUBLIC libmecab)

add_executable (mecab-cost-train src/mecab-cost-train.cpp)
target_include_directories(mecab-cost-train PRIVATE ./src)
target_link_libraries (mecab-cost-train LINK_PUBLIC libmecab)

add_executable (mecab-system-eval src/mecab-system-eval.cpp)
target_include_directories(mecab-system-eval PRIVATE ./src)
target_link_libraries (mecab-system-eval LINK_PUBLIC libmecab)

add_executable (mecab-test-gen src/mecab-dict-index.cpp)
target_include_directories(mecab-test-gen PRIVATE ./src)
target_link_libraries (mecab-test-gen LINK_PUBLIC libmecab)