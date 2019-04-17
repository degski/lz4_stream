
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

LZ4InputStream::LZ4InputBuffer::LZ4InputBuffer ( std::istream & source ) :
    source_ ( source ), src_buf_ ( ), dest_buf_ ( ), offset_ ( 0 ), src_buf_size_ ( 0 ), ctx_ ( nullptr ) {
    size_t ret = LZ4F_createDecompressionContext ( &ctx_, LZ4F_VERSION );
    if ( LZ4F_isError ( ret ) != 0 )
        throw std::runtime_error ( std::string ( "Failed to create LZ4 decompression context: " ) + LZ4F_getErrorName ( ret ) );
    setg ( &src_buf_.front ( ), &src_buf_.front ( ), &src_buf_.front ( ) );
}

LZ4InputStream::int_type LZ4InputStream::LZ4InputBuffer::underflow ( ) {
    if ( offset_ == src_buf_size_ ) {
        source_.read ( &src_buf_.front ( ), src_buf_.size ( ) );
        src_buf_size_ = static_cast<size_t> ( source_.gcount ( ) );
        offset_       = 0;
    }
    if ( src_buf_size_ == 0 )
        return traits_type::eof ( );
    size_t src_size  = src_buf_size_ - offset_;
    size_t dest_size = dest_buf_.size ( );
    size_t ret = LZ4F_decompress ( ctx_, &dest_buf_.front ( ), &dest_size, &src_buf_.front ( ) + offset_, &src_size, nullptr );
    offset_ += src_size;
    if ( LZ4F_isError ( ret ) != 0 )
        throw std::runtime_error ( std::string ( "LZ4 decompression failed: " ) + LZ4F_getErrorName ( ret ) );
    if ( dest_size == 0 )
        return traits_type::eof ( );
    setg ( &dest_buf_.front ( ), &dest_buf_.front ( ), &dest_buf_.front ( ) + dest_size );
    return traits_type::to_int_type ( *gptr ( ) );
}

LZ4InputStream::LZ4InputBuffer::~LZ4InputBuffer ( ) { LZ4F_freeDecompressionContext ( ctx_ ); }
