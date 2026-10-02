#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>
#include <REL/Relocation.h>

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <spdlog/sinks/basic_file_sink.h>

using namespace std::literals;
namespace logger = SKSE::log;
