/**
 * @file localizationTest.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "autoinput_ui/core/localization.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <filesystem>
#include <fstream>

using namespace autoinput::ui;
using namespace autoinput;

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
        enFile << "[app]\n";
        enFile << "name = \"AutoInputApp\"\n";
        enFile << "[labels]\n";
        enFile << "sequenceStepsCount = \"Sequence Steps ({}):\"\n";
        enFile << "[buttons]\n";
        enFile << "save = \"Save\"\n";
        enFile << "cancel = \"Cancel\"\n";
        enFile.close();
        
        m_dePath = m_tempDir / "de-DE.toml";
        std::ofstream deFile(m_dePath);
        deFile << "[test]\n";
        deFile << "key1 = \"wert1\"\n";
        deFile << "[buttons]\n";
        deFile << "save = \"Speichern\"\n";
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
    // Verify that the localization file loads successfully
    EXPECT_TRUE(loc.loadFromFile(m_enPath));
    // Verify that looking up "test.key1" returns the expected translated value
    EXPECT_EQ(loc.text("test.key1"), "value1");
    // Verify that looking up "test.key2" returns the expected translated value
    EXPECT_EQ(loc.text("test.key2"), "value2");
}

TEST_F(LocalizationTest, FallbackToKey)
{
    Localization loc;
    // Verify that the localization file loads successfully
    EXPECT_TRUE(loc.loadFromFile(m_enPath));
    // Verify that querying a non-existent key falls back to returning the key name itself
    EXPECT_EQ(loc.text("non.existent"), "non.existent");
}

TEST_F(LocalizationTest, TextOr)
{
    Localization loc;
    // Verify that the localization file loads successfully
    EXPECT_TRUE(loc.loadFromFile(m_enPath));
    // Verify that textOr returns the translated value when the key exists
    EXPECT_EQ(loc.textOr("test.key1", "fallback"), "value1");
    // Verify that textOr returns the provided fallback string when the key does not exist
    EXPECT_EQ(loc.textOr("non.existent", "fallback"), "fallback");
}

TEST_F(LocalizationTest, LayeredLoading)
{
    Localization loc;
    // Load en as base
    // Verify that loading the base localization file succeeds
    EXPECT_TRUE(loc.loadFromFile(m_enPath, true));
    // Verify initial base translations before loading overlay
    EXPECT_EQ(loc.text("test.key1"), "value1");
    EXPECT_EQ(loc.text("test.key2"), "value2");
    
    // Load de as override
    // Verify that loading the overlay localization file succeeds
    EXPECT_TRUE(loc.loadFromFile(m_dePath, false));
    // Verify that the overlay file overrides matching existing keys
    EXPECT_EQ(loc.text("test.key1"), "wert1"); // Overridden
    // Verify that keys not present in the overlay retain their base translation
    EXPECT_EQ(loc.text("test.key2"), "value2"); // Kept from en
}

TEST_F(LocalizationTest, MissingFile)
{
    Localization loc;
    // Verify that attempting to load a non-existent file returns false
    EXPECT_FALSE(loc.loadFromFile(m_tempDir / "nonexistent.toml"));
}

TEST_F(LocalizationTest, LocalizationIdLookups)
{
    Localization loc;
    // Verify that the localization file loads successfully
    EXPECT_TRUE(loc.loadFromFile(m_enPath));

    // Verify lookup by Localization ID retrieves the expected text strings
    EXPECT_EQ(loc.text(LocIds::APP_NAME_ID), "AutoInputApp");
    EXPECT_EQ(loc.text(LocIds::BUTTONS_SAVE_ID), "Save");
    EXPECT_EQ(loc.text(LocIds::BUTTONS_CANCEL_ID), "Cancel");

    // Verify has() returns true for present IDs and false for missing IDs
    EXPECT_TRUE(loc.has(LocIds::APP_NAME_ID));
    EXPECT_TRUE(loc.has(LocIds::BUTTONS_SAVE_ID));
    EXPECT_FALSE(loc.has(LocIds::BUTTONS_ADD_ID)); // Not in temp file

    // Verify textOr() with Localization IDs returns translated string when present or fallback when missing
    EXPECT_EQ(loc.textOr(LocIds::APP_NAME_ID, "Fallback"), "AutoInputApp");
    EXPECT_EQ(loc.textOr(LocIds::BUTTONS_ADD_ID, "Add Button"), "Add Button");

    // Verify format() properly interpolates argument values into localized format strings
    EXPECT_EQ(loc.format(LocIds::LABELS_SEQUENCE_STEPS_COUNT_ID, 5), "Sequence Steps (5):");
}

TEST_F(LocalizationTest, LocalizationIdLayeredLoading)
{
    Localization loc;
    // Verify that base localization file loads successfully
    EXPECT_TRUE(loc.loadFromFile(m_enPath, true));
    // Verify base translations prior to applying override
    EXPECT_EQ(loc.text(LocIds::BUTTONS_SAVE_ID), "Save");
    EXPECT_EQ(loc.text(LocIds::BUTTONS_CANCEL_ID), "Cancel");

    // Verify that overlay localization file loads successfully
    EXPECT_TRUE(loc.loadFromFile(m_dePath, false));
    // Verify that overridden ID receives the new translation
    EXPECT_EQ(loc.text(LocIds::BUTTONS_SAVE_ID), "Speichern"); // Overridden
    // Verify that non-overridden ID retains original base translation
    EXPECT_EQ(loc.text(LocIds::BUTTONS_CANCEL_ID), "Cancel");   // Retained from en
}

TEST_F(LocalizationTest, MatchingFunctions)
{
    // Verify idToKey correctly converts valid Localization IDs to key strings
    EXPECT_EQ(LocalizationIds::idToKey(LocIds::APP_NAME_KEY.index), "app.name");
    EXPECT_EQ(LocalizationIds::idToKey(LocIds::BUTTONS_SAVE_ID), "buttons.save");
    // Verify idToKey returns empty string for out-of-range IDs
    EXPECT_EQ(LocalizationIds::idToKey(-1), "");
    EXPECT_EQ(LocalizationIds::idToKey(99999), "");

    // Verify keyToId correctly converts key strings to Localization IDs
    EXPECT_EQ(LocalizationIds::keyToId("app.name"), LocIds::APP_NAME_ID);
    EXPECT_EQ(LocalizationIds::keyToId("buttons.save"), LocIds::BUTTONS_SAVE_ID);
    // Verify keyToId returns INVALID_ID for unknown key string
    EXPECT_EQ(LocalizationIds::keyToId("non.existent.key"), LocalizationIds::INVALID_ID);

    // Verify round-trip consistency across all generated keys
    for (LocId id = 0; id < LocalizationIds::KEY_COUNT; ++id)
    {
        std::string_view key = LocalizationIds::idToKey(id);
        // Verify key string generated for valid ID is non-empty
        EXPECT_FALSE(key.empty());
        // Verify round-trip mapping of key string back to its original ID
        EXPECT_EQ(LocalizationIds::keyToId(key), id);
    }
}
