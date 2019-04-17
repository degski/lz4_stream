
// MIT License
//
// Copyright (c) 2019 degski
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "lz4stream.hpp"

#include <cassert>

#include <exception>
#include <functional>

LZ4OutputStream::LZ4OutputBuffer::LZ4OutputBuffer(std::ostream &sink, const int compression_level_)
  : sink_(sink),
    src_buf_{},
    preferences_{},
    closed_(false)
{
  char* base = &src_buf_.front();
  setp(base, base + src_buf_.size() - 1);

  size_t ret = LZ4F_createCompressionContext(&ctx_, LZ4F_VERSION);
  if (LZ4F_isError(ret) != 0)
  {
    throw std::runtime_error(std::string("Failed to create LZ4 compression context: ")
                             + LZ4F_getErrorName(ret));
  }
  preferences_.compressionLevel = compression_level_;
  // TODO: No need to recalculate the dest_buf_ size on each construction
  dest_buf_.resize(LZ4F_compressBound(src_buf_.size(), &preferences_));
  writeHeader();
}

LZ4OutputStream::LZ4OutputBuffer::~LZ4OutputBuffer()
{
  close();
}

LZ4OutputStream::int_type LZ4OutputStream::LZ4OutputBuffer::overflow(int_type ch)
{
  assert(std::less_equal<char*>()(pptr(), epptr()));

  *pptr() = static_cast<LZ4OutputStream::char_type>(ch);
  pbump(1);

  compressAndWrite();

  return ch;
}

LZ4OutputStream::int_type LZ4OutputStream::LZ4OutputBuffer::sync()
{
  compressAndWrite();
  return 0;
}

void LZ4OutputStream::LZ4OutputBuffer::compressAndWrite()
{
  assert(!closed_);
  std::ptrdiff_t orig_size = pptr() - pbase();
  pbump(-orig_size);
  size_t comp_size = LZ4F_compressUpdate(ctx_, &dest_buf_.front(), dest_buf_.size(),
                                         pbase(), orig_size, nullptr);
  sink_.write(&dest_buf_.front(), comp_size);
}

void LZ4OutputStream::LZ4OutputBuffer::writeHeader()
{
  assert(!closed_);
  size_t ret = LZ4F_compressBegin(ctx_, &dest_buf_.front(), dest_buf_.size(), &preferences_);
  if (LZ4F_isError(ret) != 0)
  {
    throw std::runtime_error(std::string("Failed to start LZ4 compression: ")
                             + LZ4F_getErrorName(ret));
  }
  sink_.write(&dest_buf_.front(), ret);
}

void LZ4OutputStream::LZ4OutputBuffer::writeFooter()
{
  assert(!closed_);
  size_t ret = LZ4F_compressEnd(ctx_, &dest_buf_.front(), dest_buf_.size(), nullptr);
  if (LZ4F_isError(ret) != 0)
  {
    throw std::runtime_error(std::string("Failed to end LZ4 compression: ")
                             + LZ4F_getErrorName(ret));
  }
  sink_.write(&dest_buf_.front(), ret);
}

void LZ4OutputStream::LZ4OutputBuffer::close()
{
  if (closed_)
  {
    return;
  }
  sync();
  writeFooter();
  LZ4F_freeCompressionContext(ctx_);
  closed_ = true;
}
