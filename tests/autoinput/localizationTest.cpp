/**
 * @file localizationTest.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../src/autoinput_ui/core/localization.h"
#include <filesystem>
#include <fstream>

using namespace autoinput::ui;

class LocalizationTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_tempDir = std::filesystem::temp_directory_path() / "autoinput_loc_test";
        std::filesystem::create_directories(m_tempDir);
        
        m_enPath = m_tempDir / "en-US.toml";
        std::ofstream enFile(m_enPath);
        enFile << "[test]\n";
        enFile << "key1 = \"value1\"\n";
        enFile << "key2 = \"value2\"\n";
        enFile.close();
        
        m_dePath = m_tempDir / "de-DE.toml";
        std::ofstream deFile(m_dePath);
        deFile << "[test]\n";
        deFile << "key1 = \"wert1\"\n";
        deFile.close();
    }

    void TearDown() override
    {
        std::filesystem::remove_all(m_tempDir);
    }

    std::filesystem::path m_tempDir;
    std::filesystem::path m_enPath;
    std::filesystem::path m_dePath;
};

TEST_F(LocalizationTest, LoadAndText)
{
    Localization loc;
    EXPECT_TRUE(loc.loadFromFile(m_enPath));
    EXPECT_EQ(loc.text("test.key1"), "value1");
    EXPECT_EQ(loc.text("test.key2"), "value2");
}

TEST_F(LocalizationTest, FallbackToKey)
{
    Localization loc;
    EXPECT_TRUE(loc.loadFromFile(m_enPath));
    EXPECT_EQ(loc.text("non.existent"), "non.existent");
}

TEST_F(LocalizationTest, TextOr)
{
    Localization loc;
    EXPECT_TRUE(loc.loadFromFile(m_enPath));
    EXPECT_EQ(loc.textOr("test.key1", "fallback"), "value1");
    EXPECT_EQ(loc.textOr("non.existent", "fallback"), "fallback");
}

TEST_F(LocalizationTest, LayeredLoading)
{
    Localization loc;
    // Load en as base
    EXPECT_TRUE(loc.loadFromFile(m_enPath, true));
    EXPECT_EQ(loc.text("test.key1"), "value1");
    EXPECT_EQ(loc.text("test.key2"), "value2");
    
    // Load de as override
    EXPECT_TRUE(loc.loadFromFile(m_dePath, false));
    EXPECT_EQ(loc.text("test.key1"), "wert1"); // Overridden
    EXPECT_EQ(loc.text("test.key2"), "value2"); // Kept from en
}

TEST_F(LocalizationTest, MissingFile)
{
    Localization loc;
    EXPECT_FALSE(loc.loadFromFile(m_tempDir / "nonexistent.toml"));
}
