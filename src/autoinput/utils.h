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
#include <sstream>
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

    template <typename Range>
    std::string join(const Range& range, const std::string& delim)
    {
        std::ostringstream oss;
        auto it = std::begin(range);
        auto end = std::end(range);
        if (it != end)
        {
            oss << *it;
            ++it;
        }
        while (it != end)
        {
            oss << delim << *it;
            ++it;
        }
        return oss.str();
    }

    template<typename TElement>
    bool contains(const std::vector<TElement>& vec, const TElement& element)
    {
        return std::find( std::begin(vec), std::end(vec), element) != std::end(vec);
    }

    bool contains(gsl::span<const std::string> span, const std::string& element);
    bool contains(gsl::span<char*> span, const std::string& element);

    std::string join(gsl::span<const std::string> span, const std::string& delim);
    std::string escapeJsonString(std::string_view sv);

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
