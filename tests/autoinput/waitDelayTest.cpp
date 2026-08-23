#include <gtest/gtest.h>
#include "autoinput/input/waitDelay.h"

using namespace autoinput;

TEST(WaitDelayTest, ParseSingleValues)
{
    auto v1 = parseWaitDelayInput("100ms");
    // Ensure millisecond delay value is parsed successfully
    ASSERT_TRUE(v1.has_value());
    // Verify minimum delay value is 100.0
    EXPECT_DOUBLE_EQ(v1->minValue, 100.0);
    // Verify duration type is milliseconds
    EXPECT_EQ(v1->durationType, "ms");
    // Verify single value does not enable range mode
    EXPECT_FALSE(v1->useRange);

    auto v2 = parseWaitDelayInput("2s");
    // Ensure second delay value is parsed successfully
    ASSERT_TRUE(v2.has_value());
    // Verify minimum delay value is 2.0
    EXPECT_DOUBLE_EQ(v2->minValue, 2.0);
    // Verify duration type is seconds
    EXPECT_EQ(v2->durationType, "s");

    auto v3 = parseWaitDelayInput("1m");
    // Ensure minute delay value is parsed successfully
    ASSERT_TRUE(v3.has_value());
    // Verify minimum delay value is 1.0
    EXPECT_DOUBLE_EQ(v3->minValue, 1.0);
    // Verify duration type is minutes
    EXPECT_EQ(v3->durationType, "m");

    auto v4 = parseWaitDelayInput("0.5s");
    // Ensure decimal second delay value is parsed successfully
    ASSERT_TRUE(v4.has_value());
    // Verify minimum delay value is 0.5
    EXPECT_DOUBLE_EQ(v4->minValue, 0.5);
    // Verify duration type is seconds
    EXPECT_EQ(v4->durationType, "s");

    auto v5 = parseWaitDelayInput("1.25s");
    // Ensure decimal delay value with multiple decimal places is parsed successfully
    ASSERT_TRUE(v5.has_value());
    // Verify minimum delay value is 1.25
    EXPECT_DOUBLE_EQ(v5->minValue, 1.25);
    // Verify duration type is seconds
    EXPECT_EQ(v5->durationType, "s");
}

TEST(WaitDelayTest, ParseRanges)
{
    auto v1 = parseWaitDelayInput("100ms..250ms");
    // Ensure millisecond range delay value is parsed successfully
    ASSERT_TRUE(v1.has_value());
    // Verify minimum range boundary value is 100.0
    EXPECT_DOUBLE_EQ(v1->minValue, 100.0);
    // Verify maximum range boundary value is 250.0
    EXPECT_DOUBLE_EQ(v1->maxValue, 250.0);
    // Verify range mode is enabled
    EXPECT_TRUE(v1->useRange);
    // Verify duration type is milliseconds
    EXPECT_EQ(v1->durationType, "ms");

    auto v2 = parseWaitDelayInput("1s..2s");
    // Ensure second range delay value is parsed successfully
    ASSERT_TRUE(v2.has_value());
    // Verify minimum range boundary value is 1.0
    EXPECT_DOUBLE_EQ(v2->minValue, 1.0);
    // Verify maximum range boundary value is 2.0
    EXPECT_DOUBLE_EQ(v2->maxValue, 2.0);
    // Verify duration type is seconds
    EXPECT_EQ(v2->durationType, "s");

    auto v3 = parseWaitDelayInput("0.5s..1.25s");
    // Ensure decimal second range delay value is parsed successfully
    ASSERT_TRUE(v3.has_value());
    // Verify minimum range boundary value is 0.5
    EXPECT_DOUBLE_EQ(v3->minValue, 0.5);
    // Verify maximum range boundary value is 1.25
    EXPECT_DOUBLE_EQ(v3->maxValue, 1.25);
    // Verify duration type is seconds
    EXPECT_EQ(v3->durationType, "s");
}

TEST(WaitDelayTest, ParseMixedUnitRanges)
{
    auto v1 = parseWaitDelayInput("500ms..1s");
    // Ensure mixed unit range delay value is parsed successfully
    ASSERT_TRUE(v1.has_value());
    // Normalized to "s" (second unit)
    // Verify minimum value is normalized to seconds
    EXPECT_DOUBLE_EQ(v1->minValue, 0.5);
    // Verify maximum value matches specified seconds
    EXPECT_DOUBLE_EQ(v1->maxValue, 1.0);
    // Verify normalized duration type is seconds
    EXPECT_EQ(v1->durationType, "s");
}

TEST(WaitDelayTest, ParseInvalidInputs)
{
    // Verify empty string input returns nullopt
    EXPECT_FALSE(parseWaitDelayInput("").has_value());
    // Verify non-numeric input returns nullopt
    EXPECT_FALSE(parseWaitDelayInput("abc").has_value());
    // Verify unit without number returns nullopt
    EXPECT_FALSE(parseWaitDelayInput("ms").has_value());
    // Verify number with invalid unit returns nullopt
    EXPECT_FALSE(parseWaitDelayInput("1x").has_value());
    // Verify invalid range syntax with multiple delimiters returns nullopt
    EXPECT_FALSE(parseWaitDelayInput("1..2..3").has_value());
    // Verify range missing start value returns nullopt
    EXPECT_FALSE(parseWaitDelayInput("..100ms").has_value());
    // Verify range missing end value returns nullopt
    EXPECT_FALSE(parseWaitDelayInput("100ms..").has_value());
}

TEST(WaitDelayTest, MillisecondsConversion)
{
    // Verify 500 milliseconds converts to 500ms duration
    EXPECT_EQ(waitDelayInputToMilliseconds(500.0, "ms").count(), 500);
    // Verify 2 seconds converts to 2000ms duration
    EXPECT_EQ(waitDelayInputToMilliseconds(2.0, "s").count(), 2000);
    // Verify 1 minute converts to 60000ms duration
    EXPECT_EQ(waitDelayInputToMilliseconds(1.0, "m").count(), 60000);
    // Verify 0.5 seconds converts to 500ms duration
    EXPECT_EQ(waitDelayInputToMilliseconds(0.5, "s").count(), 500);
}

TEST(WaitDelayTest, Formatting)
{
    WaitDelayInput input;
    input.hasValue = true;
    input.minValue = 100.0;
    input.durationType = "ms";
    input.useRange = false;
    // Verify formatting single value with millisecond unit
    EXPECT_EQ(formatWaitDelayInput(input), "100ms");

    input.useRange = true;
    input.maxValue = 250.0;
    // Verify formatting range with millisecond unit
    EXPECT_EQ(formatWaitDelayInput(input), "100..250ms");

    input.minValue = 0.5;
    input.maxValue = 1.25;
    input.durationType = "s";
    // Verify formatting range with decimal values and second unit
    EXPECT_EQ(formatWaitDelayInput(input), "0.5..1.25s");
}

TEST(WaitDelayTest, WaitDelayDataRegression)
{
    WaitDelayData data;
    // Verify press delay parsing succeeds for single value
    EXPECT_TRUE(data.parseWaitTimeDelay("100ms", true));
    // Verify hasPress flag is set
    EXPECT_TRUE(data.hasPress);
    // Verify range is not used for single value
    EXPECT_FALSE(data.usePressRange);
    // Verify minimum press delay is 100ms
    EXPECT_EQ(data.minWaitPressDelay.count(), 100);

    // Verify release delay parsing succeeds for range value
    EXPECT_TRUE(data.parseWaitTimeDelay("100ms..200ms", false));
    // Verify hasRelease flag is set
    EXPECT_TRUE(data.hasRelease);
    // Verify range is used for range value
    EXPECT_TRUE(data.useReleaseRange);
    // Verify minimum release delay is 100ms
    EXPECT_EQ(data.minWaitReleaseDelay.count(), 100);
    // Verify maximum release delay is 200ms
    EXPECT_EQ(data.maxWaitReleaseDelay.count(), 200);
}
