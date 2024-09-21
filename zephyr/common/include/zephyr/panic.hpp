
#pragma once

#include <fmt/format.h>
#include <string>
#include <utility>

namespace zephyr {

namespace detail {

[[noreturn]] void call_panic_handler(const char* file, int line, const char* message);

template<typename... Args>
[[noreturn]] void panic(const char* file, int line, const char* format, Args&&... args) {
  std::string message = fmt::format(fmt::runtime(format), std::forward<Args>(args)...);

  call_panic_handler(file, line, message.c_str());
}

} // namespace zephyr::detail

typedef void (*PanicHandlerFn)(const char* file, int line, const char* message);

void set_panic_handler(PanicHandlerFn handler);

} // namespace zephyr

#define ZEPHYR_PANIC(format, ...) zephyr::detail::panic(__FILE__, __LINE__, format, ## __VA_ARGS__);

#define ZEPHYR_UNREACHABLE() ZEPHYR_PANIC("Reached supposedly unreachable code")