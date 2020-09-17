//  MeCab -- Yet Another Part-of-Speech and Morphological Analyzer
//
//
//  Copyright(C) 2001-2006 Taku Kudo <taku@chasen.org>
//  Copyright(C) 2004-2006 Nippon Telegraph and Telephone Corporation
#include <fstream>
#include <sstream>
#include <array>
#include <unordered_map>
#include "mecab.h"
#include "common.h"
#include "file.h"
#include "connector.h"
#include "param.h"
#include "utils.h"

namespace MeCab {

bool Connector::open(const Param &param, macab_io_file_t *io) {
  const std::string filename = create_filename
      (param.get<std::string>("dicdir"), MATRIX_FILE);
  return open(filename.c_str(), "r", io);
}

bool Connector::open(const char* filename, const char *mode, macab_io_file_t *io) {
  close();
  io_ = io ? *io : *default_io();
  const char *mapped(nullptr);
  size_t length(0);
  CHECK_FALSE(handle_ = io_.open(filename, mode, &length, (void**)&mapped)) << "cannot open: " << filename;

  std::shared_ptr<IMMap> ptr;
  ptr.reset(mapped ? new MMap((char*)mapped, length) : (IMMap*)(new FileMap(&io_, handle_, length)));

  ptr->read(&lsize_, sizeof(unsigned short));
  ptr->read(&rsize_, sizeof(unsigned short));

  matrix_ = (short*)ptr->data();

  CHECK_FALSE(matrix_) << "matrix is NULL";
  CHECK_FALSE((length/sizeof(short)) >= 2) << "file size is invalid: " << filename;

  CHECK_FALSE(static_cast<size_t>(lsize_ * rsize_ + 2) == (length / sizeof(short))) << "file size is invalid: " << filename;

  return true;
}

void Connector::close() {
  if (io_.close != nullptr)
	 io_.close(handle_);
}

bool Connector::openText(const char *filename) {
  std::ifstream ifs(WPATH(filename));
  if (!ifs) {
    WHAT << "no such file or directory: " << filename;
    return false;
  }
  char *column[2];
  std::array<char, BUF_SIZE> buf;
  ifs.getline(buf.data(), buf.size());
  CHECK_DIE(tokenize2(buf.data(), "\t ", column, 2) == 2)
      << "format error: " << buf.data();
  lsize_ = std::atoi(column[0]);
  rsize_ = std::atoi(column[1]);
  return true;
}

bool Connector::compile(const char *ifile, const char *ofile) {
  std::ifstream ifs(WPATH(ifile));
  std::istringstream iss(MATRIX_DEF_DEFAULT);
  std::istream *is = &ifs;

  if (!ifs) {
    std::cerr << ifile
              << " is not found. minimum setting is used." << std::endl;
    is = &iss;
  }


  char *column[4];
  std::array<char, BUF_SIZE> buf;

  is->getline(buf.data(), buf.size());

  CHECK_DIE(tokenize2(buf.data(), "\t ", column, 2) == 2) << "format error: " << buf.data();

  const unsigned short lsize = std::atoi(column[0]);
  const unsigned short rsize = std::atoi(column[1]);
  std::vector<short> matrix(lsize * rsize);
  std::fill(matrix.begin(), matrix.end(), 0);

  std::cout << "reading " << ifile << " ... " << lsize << "x" << rsize << std::endl;

  while (is->getline(buf.data(), buf.size())) {
    CHECK_DIE(tokenize2(buf.data(), "\t ", column, 3) == 3) << "format error: " << buf.data();
    const size_t l = std::atoi(column[0]);
    const size_t r = std::atoi(column[1]);
    const int    c = std::atoi(column[2]);
    CHECK_DIE(l < lsize && r < rsize) << "index values are out of range";
    progress_bar("emitting matrix      ", l + 1, lsize);
    matrix[(l + lsize * r)] = static_cast<short>(c);
  }

  std::ofstream ofs(WPATH(ofile), std::ios::binary|std::ios::out);
  CHECK_DIE(ofs) << "permission denied: " << ofile;
  ofs.write(reinterpret_cast<const char*>(&lsize), sizeof(unsigned short));
  ofs.write(reinterpret_cast<const char*>(&rsize), sizeof(unsigned short));
  ofs.write(reinterpret_cast<const char*>(&matrix[0]), lsize * rsize * sizeof(short));
  ofs.close();

  return true;
}
}
