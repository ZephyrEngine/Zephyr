
#include "application.hpp"
#include "main_window.hpp"

int Application::Run(int argc, char** argv) {
  std::make_unique<zephyr::MainWindow>()->Run();
  return 0;
}