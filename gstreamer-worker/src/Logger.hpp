// Logger.hpp
#pragma once

#include <string>

void log_message(const std::string& level, const std::string& fmt, ...);
void set_log_level(const std::string& level);