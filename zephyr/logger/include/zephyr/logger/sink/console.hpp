
#pragma once

#include <zephyr/logger/logger.hpp>

namespace zephyr {

/// Logs colored messages to the process's standard output handle
class LoggerConsoleSink final : public Logger::SinkBase {
  protected:
    void AppendImpl(Logger::Message const& message) override;
};

} // namespace zephyr