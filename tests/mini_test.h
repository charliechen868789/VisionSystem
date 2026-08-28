#pragma once
// Minimal header-only test harness — no external test framework dependency
// so unit tests can build offline with only the same toolchain the app uses.
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace mini_test {

struct TestCase {
    std::string name;
    std::function<void()> fn;
};

inline std::vector<TestCase> &registry() {
    static std::vector<TestCase> cases;
    return cases;
}

struct Registrar {
    Registrar(const std::string &name, std::function<void()> fn) {
        registry().push_back({name, std::move(fn)});
    }
};

struct AssertionFailure {
    std::string message;
};

inline int run_all() {
    int failed = 0;
    for (auto &tc : registry()) {
        try {
            tc.fn();
            std::printf("[ PASS ] %s\n", tc.name.c_str());
        } catch (const AssertionFailure &e) {
            std::printf("[ FAIL ] %s — %s\n", tc.name.c_str(), e.message.c_str());
            ++failed;
        } catch (const std::exception &e) {
            std::printf("[ FAIL ] %s — unexpected exception: %s\n", tc.name.c_str(), e.what());
            ++failed;
        }
    }
    std::printf("---\n%zu tests, %d failed\n", registry().size(), failed);
    return failed == 0 ? 0 : 1;
}

} // namespace mini_test

#define TEST_CASE(name) \
    static void name(); \
    static mini_test::Registrar registrar_##name(#name, name); \
    static void name()

#define CHECK(cond) \
    do { \
        if (!(cond)) { \
            throw mini_test::AssertionFailure{std::string(__FILE__) + ":" + std::to_string(__LINE__) + \
                ": CHECK failed: " #cond}; \
        } \
    } while (0)

#define CHECK_EQ(a, b) \
    do { \
        if (!((a) == (b))) { \
            throw mini_test::AssertionFailure{std::string(__FILE__) + ":" + std::to_string(__LINE__) + \
                ": CHECK_EQ failed: " #a " == " #b}; \
        } \
    } while (0)
