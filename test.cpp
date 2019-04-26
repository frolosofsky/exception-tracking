#include "exception-tracker.h"

#include <iostream>

using exception_tracker::track;
using exception_tracker::contextual_track;
using exception_tracker::contextual_exception;
using exception_tracker::trace_exception;

void check_contextual_exception() {
    struct A : public std::exception {
        char const *what() const noexcept override { return "abc"; }
    };
    contextual_exception<A> e("file", 1);
    assert(std::string(e.what()) == "file:1: abc");
}

void check_trace_exception() {
    struct A : public std::exception {
        char const *what() const noexcept override { return "A"; }
    };
    struct B : public std::exception {
        char const *what() const noexcept override { return "B"; }
    };
    struct C : public std::runtime_error {
        C(std::string const &a, std::string const &b) : std::runtime_error("C(" + a + "," + b + ")") {}
    };
    {
        auto root = A();
        auto level1 = trace_exception<B>(root);
        auto level2 = trace_exception<A>(level1);
        assert(std::string(level2.what()) == "A\nB\nA");
    }
    {
        auto root = B();
        auto level1 = track<C>(root, "12", "34");
        auto level2 = track<A>(level1);
        auto level3 = contextual_track<C>(level2, "file", 42, "abc", "def");
        assert(std::string(level3.what()) == "file:42: C(abc,def)\nA\nC(12,34)\nB");
    }
}

namespace example {
struct file_not_found : public std::runtime_error {
    file_not_found(std::string const &f) : std::runtime_error(std::string("File not found -- " + f)) {}
};

struct length_exceeded : public std::runtime_error {
    length_exceeded(std::string const &data, size_t const limit)
        : std::runtime_error("Data length exceeded (" + std::to_string(data.size()) + " > " + std::to_string(limit) + ") -- " + data) {}
};

void open_file(std::string const &filename) {
    throw contextual_exception<file_not_found>(__FILE__, __LINE__, filename);
}

void big_brother(std::string const &, std::string const &, std::string const &) {
    try {
        open_file("observation.log");
    } catch (std::exception const &e) {
        throw contextual_track<std::runtime_error>(e, __FILE__, __LINE__, "big brother sucks");
    }
}

void send_packet(std::string const &data) {
    if (data.size() > 20) {
        throw contextual_exception<length_exceeded>(__FILE__, __LINE__, data, 20);
    }
}

void send_email(std::string const &dst, std::string const &subject, std::string const &body) {
    try {
        if (dst == "foo@bar.baz") {
            big_brother(dst, subject, body);
        }
        send_packet(dst + subject + body);
    } catch (std::exception const &e) {
        throw contextual_track<std::runtime_error>(e, __FILE__, __LINE__, "send_email failed");
    }
}

void example() {
    try {
        send_email("foo@bar.baz", "Hello there", "How are you doing bro?");
    } catch (std::exception const &e) {
        std::cout << "Operation failed: " << std::endl << e.what() << std::endl;
    }
    try {
        send_email("bar@foo.baz", "Hello there", "How are you doing bro?");
    } catch (std::exception const &e) {
        std::cout << "Operation failed: " << std::endl << e.what() << std::endl;
    }
}
}

int main() {
    check_contextual_exception();
    check_trace_exception();
    example::example();
    return 0;
}
