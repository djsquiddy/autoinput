/**
 * @file processTransportTest.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>
#include "autoinput/services/processTransport.h"
#include <filesystem>

namespace autoinput::services
{
    TEST(ProcessTransportTest, WindowsQuoting)
    {
#ifdef _WIN32
        // Basic
        // Verify that a simple string without spaces or quotes is returned unchanged
        EXPECT_EQ(StdioProcessTransport::quoteWindowsArgument("simple"), "simple");
        // Verify that an argument with spaces is wrapped in double quotes
        EXPECT_EQ(StdioProcessTransport::quoteWindowsArgument("with spaces"), "\"with spaces\"");
        
        // Internal quotes
        // Verify that internal double quotes are properly escaped with backslashes
        EXPECT_EQ(StdioProcessTransport::quoteWindowsArgument("with\"quote"), "\"with\\\"quote\"");
        
        // Trailing backslashes - only quoted if spaces or other special chars are present
        // Verify that trailing backslashes without spaces remain unquoted
        EXPECT_EQ(StdioProcessTransport::quoteWindowsArgument("C:\\path\\"), "C:\\path\\");
        // Verify that trailing backslashes with spaces are escaped properly when quoted
        EXPECT_EQ(StdioProcessTransport::quoteWindowsArgument("C:\\path with spaces\\"), "\"C:\\path with spaces\\\\\"");
        
        // Backslashes before quotes
        // Verify that backslashes preceding internal quotes are properly escaped
        EXPECT_EQ(StdioProcessTransport::quoteWindowsArgument("a\\\"b"), "\"a\\\\\\\"b\"");
        
        // Empty
        // Verify that an empty argument string is wrapped as empty double quotes
        EXPECT_EQ(StdioProcessTransport::quoteWindowsArgument(""), "\"\"");
#else
        // On non-windows, it currently just returns the string
        // Verify that argument quoting returns the string unchanged on non-Windows platforms
        EXPECT_EQ(StdioProcessTransport::quoteWindowsArgument("any"), "any");
#endif
    }
}
