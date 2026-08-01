/**
 * @file utils.h
 * @author djsquiddy
 * @date April 2026
 */

#ifndef INCLUDE_AUTOINPUT_UTILS_H
#define INCLUDE_AUTOINPUT_UTILS_H
#pragma once

#include <cstdint>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>
#include <gsl/gsl>

namespace autoinput
{
    enum class ErrorCode : int32_t
    {
        SUCCESS = EXIT_SUCCESS,
        INVALID_PARAM = 101,
        FAILED_TO_INSTALL_HOOKS = 102,
        UNHANDLED_EXCEPTION = 103,
        FAILED_TO_LOAD_CONFIG = 104
    };

    std::string toLowerCase(std::string_view sv);

    std::string join(gsl::span<const std::string> vec, const std::string& delim);
    template <typename T>
    bool contains(std::vector<T> vec, const T& element)
    {
        for (const auto& item : vec)
        {
            if (item == element)
            {
                return true;
            }
        }
        return false;
    }

    class NonCopyable
    {
    protected:
        NonCopyable() = default;
    public:
        virtual ~NonCopyable() = default;
        NonCopyable(const NonCopyable&) = delete;
        NonCopyable& operator=(const NonCopyable&) = delete;
    };

    class NonMoveable
    {
    protected:
        NonMoveable() = default;
    public:
        virtual ~NonMoveable() = default;
        NonMoveable(NonMoveable&&) = delete;
        NonMoveable& operator=(NonMoveable&&) = delete;
    };

    class NonCopyableMoveable
    {
    protected:
        NonCopyableMoveable() = default;
    public:
        virtual ~NonCopyableMoveable() = default;
        NonCopyableMoveable(const NonCopyableMoveable&) = delete;
        NonCopyableMoveable& operator=(const NonCopyableMoveable&) = delete;
        NonCopyableMoveable(NonCopyableMoveable&&) = delete;
        NonCopyableMoveable& operator=(NonCopyableMoveable&&) = delete;
    };

}

#endif // INCLUDE_AUTOINPUT_UTILS_H
