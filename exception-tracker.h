#pragma once

#include <stdexcept>
#include <string>

namespace exception_tracker {

// Injects contextual information, such as filename and line, to the exception.
template<typename Base>
class contextual_exception : public Base {
public:
    contextual_exception(contextual_exception const &other)
        : Base(other),
          file(other.file),
          line(other.line),
          cache(other.cache) {}

    template<typename... Args>
    contextual_exception(std::string const &file,
                         uint32_t const line,
                         Args&&... args)
        : Base(std::forward<Args>(args)...),
          file(file),
          line(line) {}

    char const *what() const noexcept override {
        if (cache.empty()) {
            cache = file + ":" + std::to_string(line) + ": " + std::string(Base::what());
        }
        return cache.c_str();
    }

    std::string file;
    uint32_t line;

private:
    mutable std::string cache;
};

// Tracks previously thrown exception.
template<typename Base, typename Prev>
class trace_exception : public Base {
public:
    trace_exception(trace_exception const &other)
        : Base(other),
          prev(other.prev),
          error_str(other.error_str) {}

    template<typename... Args>
    trace_exception(Prev const &p, Args&&... args)
        : Base(std::forward<Args>(args)...),
          prev(p),
          error_str(std::string(Base::what()) + "\n" + std::string(p.what())) {}

    char const *what() const noexcept override {
        return error_str.c_str();
    }

    const Prev prev;

private:
    const std::string error_str;
};

template<typename Base, typename Prev, typename... Args>
trace_exception<Base, Prev> track(Prev const &prev, Args&&... args) {
    return {prev, std::forward<Args>(args)...};
}

template<typename Base, typename Prev, typename... Args>
contextual_exception<trace_exception<Base, Prev>> contextual_track(
    Prev const &prev, std::string const &file, uint32_t const line, Args&&... args) {

    return {file, line, prev, std::forward<Args>(args)...};
}

}
