/**
 * @file utils.cpp
 * @author djsquiddy
 * @date April 2026
 */

#include "utils.h"

std::string autoinput::toLowerCase(std::string_view sv)
{
    std::string result(sv);
    std::ranges::transform(result, result.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
    return result;
}

std::string autoinput::join(const std::vector<std::string>& vec, const std::string& delim)
{
    std::ostringstream oss;
    for (size_t i = 0; i < vec.size(); ++i)
    {
        oss << vec[i];
        if (i != vec.size() - 1)
        {
            oss << delim;
        }
    }
    return oss.str();
}
