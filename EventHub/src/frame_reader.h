#pragma once
#include <cstdint>
#include <string>
#include <functional>
#include <vector>

// Decodes the stream:  [ uint32 LE length ][ proto bytes ] ...
// Call feed() whenever new bytes arrive from libevent.
// Complete frames are delivered via the onFrame callback.
class FrameReader
{
public:
    using FrameCallback = std::function<void(const uint8_t*, size_t)>;

    explicit FrameReader(FrameCallback cb) : m_cb(std::move(cb)) {}

    // Feed raw bytes — may produce 0..N complete frames
    void feed(const uint8_t *data, size_t len);

private:
    FrameCallback       m_cb;
    std::vector<uint8_t> m_buf;       // accumulation buffer

    enum class State { ReadLen, ReadBody } m_state = State::ReadLen;
    uint32_t m_expected = 0;
};