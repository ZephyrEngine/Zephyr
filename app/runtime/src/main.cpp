
#include <zephyr/logger/sink/console.hpp>
#include <zephyr/logger/logger.hpp>
#include <zephyr/panic.hpp>
#include <fmt/format.h>

#ifdef WIN32

#include <windows.h>

void win32_panic_handler(const char* file, int line, const char* message) {
  std::string prompt = fmt::format(
    "The program encountered a fatal error and had to quit as a consequence:\n\n"
    "{}:{}: {}",
    file, line, message);

  MessageBoxA(nullptr, prompt.c_str(), "Zephyr Runtime", MB_OK | MB_ICONERROR);
}

#endif

int main() {

#ifdef WIN32
  zephyr::set_panic_handler(&win32_panic_handler);
#endif

  zephyr::get_logger().InstallSink(std::make_unique<zephyr::LoggerConsoleSink>());

  return 0;
}