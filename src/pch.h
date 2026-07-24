/**
 * @file pch.h
 * @author djsquiddy
 * @date July 2026
 */

#ifndef INCLUDE_AUTOINPUT_PCH_H
#define INCLUDE_AUTOINPUT_PCH_H
#pragma once

#include <algorithm>
#include <atomic>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iosfwd>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <ranges>
#include <source_location>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif // defined(_WIN32)

#endif // INCLUDE_AUTOINPUT_PCH_H
