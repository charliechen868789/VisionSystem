#pragma once
#include <zmq.hpp>
#include <cstring>

// cppzmq < 4.7.0 compat (Yocto Dunfell SDK ships older cppzmq)
// CPPZMQ_VERSION may not exist at all in very old versions — treat as old API
#if !defined(CPPZMQ_VERSION) || CPPZMQ_VERSION < ZMQ_MAKE_VERSION(4, 7, 0)

inline void zmq_set_rcvtimeo(zmq::socket_t& s, int ms) {
    s.setsockopt(ZMQ_RCVTIMEO, &ms, sizeof(ms));
}
inline void zmq_set_sndtimeo(zmq::socket_t& s, int ms) {
    s.setsockopt(ZMQ_SNDTIMEO, &ms, sizeof(ms));
}
inline void zmq_set_subscribe(zmq::socket_t& s, const char* filter) {
    s.setsockopt(ZMQ_SUBSCRIBE, filter, strlen(filter));
}
inline void zmq_set_sndhwm(zmq::socket_t& s, int hwm) {
    s.setsockopt(ZMQ_SNDHWM, &hwm, sizeof(hwm));
}
inline void zmq_set_linger(zmq::socket_t& s, int ms) {
    s.setsockopt(ZMQ_LINGER, &ms, sizeof(ms));
}

#else

inline void zmq_set_rcvtimeo(zmq::socket_t& s, int ms)          { s.set(zmq::sockopt::rcvtimeo,  ms); }
inline void zmq_set_sndtimeo(zmq::socket_t& s, int ms)          { s.set(zmq::sockopt::sndtimeo,  ms); }
inline void zmq_set_subscribe(zmq::socket_t& s, const char* f)  { s.set(zmq::sockopt::subscribe, f);  }
inline void zmq_set_sndhwm(zmq::socket_t& s, int hwm)           { s.set(zmq::sockopt::sndhwm,    hwm);}
inline void zmq_set_linger(zmq::socket_t& s, int ms)            { s.set(zmq::sockopt::linger,    ms); }

#endif