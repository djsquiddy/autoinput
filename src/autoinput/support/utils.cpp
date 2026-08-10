/**
 * @file utils.cpp
 * @author djsquiddy
 * @date April 2026
 */

#include "autoinput/support/utils.h"
#include <algorithm>
#include <ranges>
#include <cctype>
#include <sstream>
#include <iomanip>

std::string autoinput::toLowerCase(std::string_view sv)
{
    std::string result(sv);
    std::ranges::transform(result, result.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
    return result;
}

bool autoinput::contains(gsl::span<const std::string> span, const std::string& element)
{
    return std::find(std::begin(span), std::end(span), element) != std::end(span);
}

bool autoinput::contains(gsl::span<char*> span, const std::string& element)
{
    return std::find_if(std::begin(span), std::end(span), [&element](const char* str)
    {
        return str != nullptr && element == str;
    }) != std::end(span);
}

std::string autoinput::join(gsl::span<const std::string> span, const std::string& delim)
{
    std::ostringstream oss;
    for (size_t i = 0; i < span.size(); ++i)
    {
        oss << span[i];
        if (i != span.size() - 1)
        {
            oss << delim;
        }
    }
    return oss.str();
}

std::string autoinput::escapeJsonString(std::string_view sv)
{
    std::ostringstream oss;
    for (const char c : sv)
    {
        switch (c)
        {
        case '\"': oss << "\\\""; break;
        case '\\': oss << "\\\\"; break;
        case '\b': oss << "\\b"; break;
        case '\f': oss << "\\f"; break;
        case '\n': oss << "\\n"; break;
        case '\r': oss << "\\r"; break;
        case '\t': oss << "\\t"; break;
        default:
            if (static_cast<unsigned char>(c) < 0x20)
            {
                oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
            }
            else
            {
                oss << c;
            }
            break;
        }
    }
    return oss.str();
}
