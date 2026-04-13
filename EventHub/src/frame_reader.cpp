#include "frame_reader.h"
#include <cstring>

void FrameReader::feed(const uint8_t *data, size_t len)
{
    m_buf.insert(m_buf.end(), data, data + len);

    while (true) {
        if (m_state == State::ReadLen) {
            if (m_buf.size() < 4) break;
            // parse 4-byte LE length
            m_expected = uint32_t(m_buf[0])
                       | uint32_t(m_buf[1]) << 8
                       | uint32_t(m_buf[2]) << 16
                       | uint32_t(m_buf[3]) << 24;
            m_buf.erase(m_buf.begin(), m_buf.begin() + 4);
            m_state = State::ReadBody;
        }

        if (m_state == State::ReadBody) {
            if (m_buf.size() < m_expected) break;
            m_cb(m_buf.data(), m_expected);
            m_buf.erase(m_buf.begin(), m_buf.begin() + m_expected);
            m_state = State::ReadLen;
        }
    }
}