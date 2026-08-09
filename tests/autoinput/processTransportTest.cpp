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
        EXPECT_EQ(StdioProcessTransport::quoteWindowsArgument("simple"), "simple");
        EXPECT_EQ(StdioProcessTransport::quoteWindowsArgument("with spaces"), "\"with spaces\"");
        
        // Internal quotes
        EXPECT_EQ(StdioProcessTransport::quoteWindowsArgument("with\"quote"), "\"with\\\"quote\"");
        
        // Trailing backslashes - only quoted if spaces or other special chars are present
        EXPECT_EQ(StdioProcessTransport::quoteWindowsArgument("C:\\path\\"), "C:\\path\\");
        EXPECT_EQ(StdioProcessTransport::quoteWindowsArgument("C:\\path with spaces\\"), "\"C:\\path with spaces\\\\\"");
        
        // Backslashes before quotes
        EXPECT_EQ(StdioProcessTransport::quoteWindowsArgument("a\\\"b"), "\"a\\\\\\\"b\"");
        
        // Empty
        EXPECT_EQ(StdioProcessTransport::quoteWindowsArgument(""), "\"\"");
#else
        // On non-windows, it currently just returns the string
        EXPECT_EQ(StdioProcessTransport::quoteWindowsArgument("any"), "any");
#endif
    }
}
