/**
 * @file utils.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */

#ifndef INCLUDE_AUTOINPUT_SUPPORT_UTILS_H
#define INCLUDE_AUTOINPUT_SUPPORT_UTILS_H
#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <sstream>
#include <gsl/gsl>

namespace autoinput
{
    /**
     * @brief Converts a string to lower case.
     * @param sv The string view to convert.
     * @return The lower-case string.
     */
    std::string toLowerCase(std::string_view sv);

    /**
     * @brief Joins elements of a range into a single string with a delimiter.
     * @tparam Range The type of the range.
     * @param range The range of elements.
     * @param delim The delimiter string.
     * @return The joined string.
     */
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

    /**
     * @brief Checks if a vector contains an element.
     * @tparam TElement The type of the element.
     * @param vec The vector to search.
     * @param element The element to look for.
     * @return True if found.
     */
    template<typename TElement>
    bool contains(const std::vector<TElement>& vec, const TElement& element)
    {
        return std::find( std::begin(vec), std::end(vec), element) != std::end(vec);
    }

    /**
     * @brief Checks if a span of strings contains an element.
     * @param span The span of strings.
     * @param element The string to look for.
     * @return True if found.
     */
    bool contains(gsl::span<const std::string> span, const std::string& element);

    /**
     * @brief Checks if a span of char* contains an element.
     * @param span The span of char pointers.
     * @param element The string to look for.
     * @return True if found.
     */
    bool contains(gsl::span<char*> span, const std::string& element);

    /**
     * @brief Joins elements of a string span into a single string with a delimiter.
     * @param span The span of strings.
     * @param delim The delimiter string.
     * @return The joined string.
     */
    std::string join(gsl::span<const std::string> span, const std::string& delim);

    /**
     * @brief Escapes a string for use in a JSON value.
     * @param sv The string to escape.
     * @return The escaped string.
     */
    std::string escapeJsonString(std::string_view sv);


    /**
     * @brief Converts the first character of a string to upper case.
     * @param value The string view to capitalize.
     * @return The string with the first character converted to upper case.
     */
    std::string capitalize(std::string value);

#ifndef AUTOINPUT_MARK_NON_MOVABLE
    /**
     * @brief Disable both copying for the given class.
     */
#define AUTOINPUT_MARK_NON_MOVABLE(obj) \
    obj(obj&&) = delete; \
    obj& operator=(obj&&) = delete;
#endif // AUTOINPUT_MARK_NON_MOVABLE

#ifndef AUTOINPUT_MARK_NON_COPYABLE
/**
 * @brief Disable both copying for the given class.
 */
#define AUTOINPUT_MARK_NON_COPYABLE(obj) \
    obj(const obj&) = delete; \
    obj& operator=(const obj&) = delete;
#endif // AUTOINPUT_MARK_NON_COPYABLE


#ifndef AUTOINPUT_MARK_NON_COPYABLE_AND_NON_MOVABLE
/**
 * @brief Disable both copying and moving for the given class.
 */
#define AUTOINPUT_MARK_NON_COPYABLE_AND_NON_MOVABLE(obj) \
    AUTOINPUT_MARK_NON_COPYABLE(obj) \
    AUTOINPUT_MARK_NON_MOVABLE(obj)
#endif // AUTOINPUT_MARK_NON_COPYABLE_AND_NON_MOVABLE

}

#endif // INCLUDE_AUTOINPUT_SUPPORT_UTILS_H
