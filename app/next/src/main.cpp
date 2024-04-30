
#include "main_window.hpp"

int main() {
  zephyr::get_logger().InstallSink(std::make_unique<zephyr::LoggerConsoleSink>());
  zephyr::MainWindow{}.Run();
  return 0;
}
