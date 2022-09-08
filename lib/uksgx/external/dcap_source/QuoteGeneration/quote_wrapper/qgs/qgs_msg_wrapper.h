/*
 * Copyright (C) 2011-2021 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef QGS_MSG_WRAPPER_H
#define QGS_MSG_WRAPPER_H

#include <string>
#include <cassert>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>  //for uint8_t


using data_buffer = std::vector<boost::uint8_t>;

const unsigned HEADER_SIZE = 4;

template <class MessageType>
class QgsMsgWrapper
{
public:
    typedef boost::shared_ptr<MessageType> MessagePointer;

    explicit QgsMsgWrapper(MessagePointer msg = MessagePointer())
        : m_msg(msg)
    {}

    void set_msg(MessagePointer msg) {
        m_msg = msg;
    }

    MessagePointer get_msg() {
        return m_msg;
    }

    bool pack(data_buffer& buf) const {
        if (!m_msg) {
            return false;
        }

        unsigned msg_size = m_msg->ByteSize();
        buf.resize(HEADER_SIZE + msg_size);
        encode_header(buf, msg_size);
        return m_msg->SerializeToArray(&buf[HEADER_SIZE], msg_size);
    }

    unsigned decode_header(const data_buffer& buf) const {
        if (buf.size() < HEADER_SIZE) {
            return 0;
        }
        unsigned msg_size = 0;
        for (unsigned i = 0; i < HEADER_SIZE; ++i) {
            msg_size = msg_size * 256 + (static_cast<unsigned>(buf[i]) & 0xFF);
        }
        return msg_size;
    }

    bool unpack(const data_buffer& buf) {
        return m_msg->ParseFromArray(&buf[HEADER_SIZE],
                                     static_cast<int>(buf.size() - HEADER_SIZE));
    }
private:
    void encode_header(data_buffer& buf, unsigned size) const {
        assert(buf.size() >= HEADER_SIZE);
        buf[0] = static_cast<boost::uint8_t>((size >> 24) & 0xFF);
        buf[1] = static_cast<boost::uint8_t>((size >> 16) & 0xFF);
        buf[2] = static_cast<boost::uint8_t>((size >> 8) & 0xFF);
        buf[3] = static_cast<boost::uint8_t>(size & 0xFF);
    }

    MessagePointer m_msg;
};

#endif /* QGS_MSG_WRAPPER_H */
