/*=============================================================================

  Library: CppMicroServices

  Copyright (c) The CppMicroServices developers. See the COPYRIGHT
  file at the top-level directory of this distribution and at
  https://github.com/CppMicroServices/CppMicroServices/COPYRIGHT .

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

=============================================================================*/

#include "cppmicroservices/detail/BundleResourceBuffer.h"

#include <cassert>
#include <cstdint>
#include <limits>
#include <stdlib.h>

#ifdef US_PLATFORM_WINDOWS
#  define DATA_NEEDS_NEWLINE_CONVERSION 1
#  undef REMOVE_LAST_NEWLINE_IN_TEXT_MODE
#else
#  undef DATA_NEEDS_NEWLINE_CONVERSION
#  define REMOVE_LAST_NEWLINE_IN_TEXT_MODE 1
#endif

namespace cppmicroservices {

namespace detail {

class BundleResourceBufferPrivate
{
public:
  BundleResourceBufferPrivate(std::unique_ptr<void, void (*)(void*)> data,
                              std::size_t size,
                              const char* begin,
                              std::ios_base::openmode mode)
    : begin(begin)
    , end(begin + size)
    , current(begin)
    , mode(mode)
    , uncompressedData(reinterpret_cast<unsigned char*>(data.release()),
                       data.get_deleter())
#ifdef DATA_NEEDS_NEWLINE_CONVERSION
    , pos(0)
#endif
  {}

  const char* const begin;
  const char* const end;
  const char* current;

  const std::ios_base::openmode mode;

  std::unique_ptr<unsigned char, void (*)(void*)> uncompressedData;

#ifdef DATA_NEEDS_NEWLINE_CONVERSION
  // records the stream position ignoring CR characters
  std::streambuf::pos_type pos;
#endif
};

BundleResourceBuffer::BundleResourceBuffer(
  std::unique_ptr<void, void (*)(void*)> data,
  std::size_t _size,
  std::ios_base::openmode mode)
  : d(nullptr)
{
  assert(_size <
         static_cast<std::size_t>(std::numeric_limits<uint32_t>::max()));

  char* begin = reinterpret_cast<char*>(data.get());
  std::size_t size = begin ? _size : 0;

#ifdef DATA_NEEDS_NEWLINE_CONVERSION
  if (begin != nullptr && !(mode & std::ios_base::binary) && begin[0] == '\r') {
    ++begin;
    --size;
  }
#endif

#ifdef REMOVE_LAST_NEWLINE_IN_TEXT_MODE
  if (begin != nullptr && !(mode & std::ios_base::binary) &&
      begin[size - 1] == '\n') {
    --size;
  }
#endif

  d.reset(new BundleResourceBufferPrivate(std::move(data), size, begin, mode));
}

BundleResourceBuffer::~BundleResourceBuffer() {}

BundleResourceBuffer::int_type BundleResourceBuffer::underflow()
{
  if (d->current == d->end)
    return traits_type::eof();

#ifdef DATA_NEEDS_NEWLINE_CONVERSION
  char c = *d->current;
  if (!(d->mode & std::ios_base::binary)) {
    if (c == '\r') {
      if (d->current + 1 == d->end) {
        return traits_type::eof();
      }
      c = d->current[1];
    }
  }
  return traits_type::to_int_type(c);
#else
  return traits_type::to_int_type(*d->current);
#endif
}

BundleResourceBuffer::int_type BundleResourceBuffer::uflow()
{
  if (d->current == d->end)
    return traits_type::eof();

#ifdef DATA_NEEDS_NEWLINE_CONVERSION
  char c = *d->current++;
  if (!(d->mode & std::ios_base::binary)) {
    if (c == '\r') {
      if (d->current == d->end) {
        return traits_type::eof();
      }
      c = *d->current++;
    }
  }
  return traits_type::to_int_type(c);
#else
  return traits_type::to_int_type(*d->current++);
#endif
}

BundleResourceBuffer::int_type BundleResourceBuffer::pbackfail(int_type ch)
{
  int backOffset = -1;
#ifdef DATA_NEEDS_NEWLINE_CONVERSION
  if (!(d->mode & std::ios_base::binary)) {
    while ((d->current - backOffset) >= d->begin &&
           d->current[backOffset] == '\r') {
      --backOffset;
    }
    // d->begin always points to a character != '\r'
  }
#endif
  if (d->current == d->begin ||
      (ch != traits_type::eof() && ch != d->current[backOffset])) {
    return traits_type::eof();
  }

  d->current += backOffset;
  return traits_type::to_int_type(*d->current);
}

std::streamsize BundleResourceBuffer::showmanyc()
{
  assert(d->current <= d->end);

#ifdef DATA_NEEDS_NEWLINE_CONVERSION
  std::streamsize ssize = 0;
  std::size_t chunkSize = d->end - d->current;
  for (std::size_t i = 0; i < chunkSize; ++i) {
    if (d->current[i] != '\r') {
      ++ssize;
    }
  }
  return ssize;
#else
  return d->end - d->current;
#endif
}

std::streambuf::pos_type BundleResourceBuffer::seekoff(
  std::streambuf::off_type off,
  std::ios_base::seekdir way,
  std::ios_base::openmode /*which*/)
{
#ifdef DATA_NEEDS_NEWLINE_CONVERSION
  std::streambuf::off_type step = 1;
  if (way == std::ios_base::beg) {
    d->current = d->begin;
  } else if (way == std::ios_base::end) {
    d->current = d->end;
    step = -1;
  }

  if (!(d->mode & std::ios_base::binary)) {
    if (way == std::ios_base::beg) {
      d->pos = 0;
    } else if (way == std::ios_base::end) {
      d->current -= 1;
    }

    std::streambuf::off_type i = 0;
    // scan through off amount of characters excluding '\r'
    while (i != off) {
      if (*d->current != '\r') {
        i += step;
        d->pos += step;
      }
      d->current += step;
    }

    // adjust the position in case of a "backwards" seek
    if (way == std::ios_base::end) {
      // fix pointer from previous while loop
      d->current += 1;
      d->pos = 0;
      i = 0;
      const std::streampos currInternalPos = d->current - d->begin;
      while (i != currInternalPos) {
        if (d->begin[i] != '\r') {
          d->pos += 1;
        }
        ++i;
      }
    }
  } else {
    d->current += off;
    d->pos = d->current - d->begin;
  }
  return d->pos;
#else
  if (way == std::ios_base::beg) {
    d->current = d->begin + off;
    return off;
  } else if (way == std::ios_base::cur) {
    d->current += off;
    return d->current - d->begin;
  } else {
    d->current = d->end + off;
    return d->current - d->begin;
  }
#endif
}

std::streambuf::pos_type BundleResourceBuffer::seekpos(
  std::streambuf::pos_type sp,
  std::ios_base::openmode /*which*/)
{
  return this->seekoff(sp, std::ios_base::beg);
}

} // namespace detail

} // namespace cppmicroservices
