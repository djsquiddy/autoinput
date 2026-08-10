#include <gtest/gtest.h>
#include "autoinput/input/waitDelay.h"

using namespace autoinput;

TEST(WaitDelayTest, ParseSingleValues)
{
    auto v1 = parseWaitDelayInput("100ms");
    ASSERT_TRUE(v1.has_value());
    EXPECT_DOUBLE_EQ(v1->minValue, 100.0);
    EXPECT_EQ(v1->durationType, "ms");
    EXPECT_FALSE(v1->useRange);

    auto v2 = parseWaitDelayInput("2s");
    ASSERT_TRUE(v2.has_value());
    EXPECT_DOUBLE_EQ(v2->minValue, 2.0);
    EXPECT_EQ(v2->durationType, "s");

    auto v3 = parseWaitDelayInput("1m");
    ASSERT_TRUE(v3.has_value());
    EXPECT_DOUBLE_EQ(v3->minValue, 1.0);
    EXPECT_EQ(v3->durationType, "m");

    auto v4 = parseWaitDelayInput("0.5s");
    ASSERT_TRUE(v4.has_value());
    EXPECT_DOUBLE_EQ(v4->minValue, 0.5);
    EXPECT_EQ(v4->durationType, "s");

    auto v5 = parseWaitDelayInput("1.25s");
    ASSERT_TRUE(v5.has_value());
    EXPECT_DOUBLE_EQ(v5->minValue, 1.25);
}

TEST(WaitDelayTest, ParseRanges)
{
    auto v1 = parseWaitDelayInput("100ms..250ms");
    ASSERT_TRUE(v1.has_value());
    EXPECT_DOUBLE_EQ(v1->minValue, 100.0);
    EXPECT_DOUBLE_EQ(v1->maxValue, 250.0);
    EXPECT_TRUE(v1->useRange);
    EXPECT_EQ(v1->durationType, "ms");

    auto v2 = parseWaitDelayInput("1s..2s");
    ASSERT_TRUE(v2.has_value());
    EXPECT_DOUBLE_EQ(v2->minValue, 1.0);
    EXPECT_DOUBLE_EQ(v2->maxValue, 2.0);
    EXPECT_EQ(v2->durationType, "s");

    auto v3 = parseWaitDelayInput("0.5s..1.25s");
    ASSERT_TRUE(v3.has_value());
    EXPECT_DOUBLE_EQ(v3->minValue, 0.5);
    EXPECT_DOUBLE_EQ(v3->maxValue, 1.25);
    EXPECT_EQ(v3->durationType, "s");
}

TEST(WaitDelayTest, ParseMixedUnitRanges)
{
    auto v1 = parseWaitDelayInput("500ms..1s");
    ASSERT_TRUE(v1.has_value());
    // Normalized to "s" (second unit)
    EXPECT_DOUBLE_EQ(v1->minValue, 0.5);
    EXPECT_DOUBLE_EQ(v1->maxValue, 1.0);
    EXPECT_EQ(v1->durationType, "s");
}

TEST(WaitDelayTest, ParseInvalidInputs)
{
    EXPECT_FALSE(parseWaitDelayInput("").has_value());
    EXPECT_FALSE(parseWaitDelayInput("abc").has_value());
    EXPECT_FALSE(parseWaitDelayInput("ms").has_value());
    EXPECT_FALSE(parseWaitDelayInput("1x").has_value());
    EXPECT_FALSE(parseWaitDelayInput("1..2..3").has_value());
    EXPECT_FALSE(parseWaitDelayInput("..100ms").has_value());
    EXPECT_FALSE(parseWaitDelayInput("100ms..").has_value());
}

TEST(WaitDelayTest, MillisecondsConversion)
{
    EXPECT_EQ(waitDelayInputToMilliseconds(500.0, "ms").count(), 500);
    EXPECT_EQ(waitDelayInputToMilliseconds(2.0, "s").count(), 2000);
    EXPECT_EQ(waitDelayInputToMilliseconds(1.0, "m").count(), 60000);
    EXPECT_EQ(waitDelayInputToMilliseconds(0.5, "s").count(), 500);
}

TEST(WaitDelayTest, Formatting)
{
    WaitDelayInput input;
    input.hasValue = true;
    input.minValue = 100.0;
    input.durationType = "ms";
    input.useRange = false;
    EXPECT_EQ(formatWaitDelayInput(input), "100ms");

    input.useRange = true;
    input.maxValue = 250.0;
    EXPECT_EQ(formatWaitDelayInput(input), "100..250ms");

    input.minValue = 0.5;
    input.maxValue = 1.25;
    input.durationType = "s";
    EXPECT_EQ(formatWaitDelayInput(input), "0.5..1.25s");
}

TEST(WaitDelayTest, WaitDelayDataRegression)
{
    WaitDelayData data;
    EXPECT_TRUE(data.parseWaitTimeDelay("100ms", true));
    EXPECT_TRUE(data.hasPress);
    EXPECT_FALSE(data.usePressRange);
    EXPECT_EQ(data.minWaitPressDelay.count(), 100);

    EXPECT_TRUE(data.parseWaitTimeDelay("100ms..200ms", false));
    EXPECT_TRUE(data.hasRelease);
    EXPECT_TRUE(data.useReleaseRange);
    EXPECT_EQ(data.minWaitReleaseDelay.count(), 100);
    EXPECT_EQ(data.maxWaitReleaseDelay.count(), 200);
}
