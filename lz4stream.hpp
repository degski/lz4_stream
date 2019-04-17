
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

#pragma once

// LZ4 Headers
#include <lz4frame.h>

// Standard headers
#include <cassert>
#include <streambuf>
#include <iostream>
#include <vector>
#include <memory>
#include <array>

/**
 * @brief An output stream that will LZ4 compress the input data.
 *
 * An output stream that will wrap another output stream and LZ4
 * compress its input data to that stream.
 *
 */
class LZ4OutputStream : public std::ostream
{
 public:
  /**
   * @brief Constructs an LZ4 compression output stream
   *
   * @param sink The stream to write compressed data to
   */
  LZ4OutputStream(std::ostream& sink)
    : std::ostream(new LZ4OutputBuffer(sink)),
      buffer_(dynamic_cast<LZ4OutputBuffer*>(rdbuf()))
  {
    assert(buffer_);
  }

  /**
   * @brief Destroys the LZ4 output stream. Calls close() if not already called.
   */
  ~LZ4OutputStream()
  {
    close();
    delete buffer_;
  }

  /**
   * @brief Flushes and writes LZ4 footer data to the LZ4 output stream.
   *
   * After calling this function no more data should be written to the stream.
   */
  void close()
  {
    buffer_->close();
  }

 private:
 class LZ4OutputBuffer : public std::streambuf {
     public:
     LZ4OutputBuffer ( std::ostream &sink, const int compression_level_ = 4 );
     ~LZ4OutputBuffer ( );

     LZ4OutputBuffer ( const LZ4OutputBuffer & ) = delete;
     LZ4OutputBuffer& operator= ( const LZ4OutputBuffer & ) = delete;
     void close ( );

     private:
     int_type overflow ( int_type ch ) override;
     int_type sync ( ) override;

     void compressAndWrite ( );
     void writeHeader ( );
     void writeFooter ( );

     std::ostream& sink_;
     std::array<char, 256> src_buf_;
     std::vector<char> dest_buf_;

     LZ4F_compressionContext_t ctx_;
     LZ4F_preferences_t preferences_;

     bool closed_;
  };

  LZ4OutputBuffer* buffer_;
};

/**
 * @brief An input stream that will LZ4 decompress output data.
 *
 * An input stream that will wrap another input stream and LZ4
 * decompress its output data to that stream.
 *
 */
class LZ4InputStream : public std::istream
{
 public:
  /**
   * @brief Constructs an LZ4 decompression input stream
   *
   * @param source The stream to read LZ4 compressed data from
   */
  LZ4InputStream(std::istream& source)
    : std::istream(new LZ4InputBuffer(source)),
      buffer_(dynamic_cast<LZ4InputBuffer*>(rdbuf()))
  {
    assert(buffer_);
  }

  /**
   * @brief Destroys the LZ4 output stream.
   */
  ~LZ4InputStream()
  {
    delete buffer_;
  }

 private:
  class LZ4InputBuffer : public std::streambuf
  {
  public:
    LZ4InputBuffer(std::istream &source);
    ~LZ4InputBuffer();
    int_type underflow() override;

    LZ4InputBuffer(const LZ4InputBuffer &) = delete;
    LZ4InputBuffer& operator= (const LZ4InputBuffer &) = delete;
  private:
    std::istream& source_;
    std::array<char, 64 * 1024> src_buf_;
    std::array<char, 64 * 1024> dest_buf_;
    size_t offset_;
    size_t src_buf_size_;
    LZ4F_decompressionContext_t ctx_;
  };

  LZ4InputBuffer* buffer_;
};
