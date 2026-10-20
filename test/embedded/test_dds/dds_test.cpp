/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  UnitTest for UnitDDS
*/
#include <gtest/gtest.h>
#include <Wire.h>
#include <M5Unified.h>
#include <M5UnitUnified.hpp>
#include <googletest/test_template.hpp>
#include <googletest/test_helper.hpp>
#include <unit/unit_DDS.hpp>
#include <esp_random.h>
#include <chrono>
#include <iostream>
#include <algorithm>

using namespace m5::unit::googletest;
using namespace m5::unit;
using namespace m5::unit::dds;
using namespace m5::unit::dds::command;
using m5::unit::types::elapsed_time_t;

class TestDDS : public I2CComponentTestBase<UnitDDS> {
protected:
    virtual UnitDDS* get_instance() override
    {
        auto ptr = new m5::unit::UnitDDS();
        return ptr;
    }
};

namespace {

constexpr uint32_t MINIMUM_FREQ{0};
constexpr uint32_t MAXIMUM_FREQ{1000000};

constexpr Mode mode_table[] = {
    Mode::Sin, Mode::Triangle, Mode::Square, Mode::Sawtooth, Mode::DC,
};

constexpr uint32_t valid_freq_table[] = {
    MINIMUM_FREQ,
    MAXIMUM_FREQ / 2,
    MAXIMUM_FREQ,
};
constexpr uint32_t invalid_freq_table[] = {MAXIMUM_FREQ + 1, std::numeric_limits<uint32_t>::max()};
constexpr uint16_t deg_table[]          = {0, 180, 360, 361, std::numeric_limits<uint16_t>::max()};
constexpr bool bank_table[]             = {false, true};

uint8_t read_control(UnitDDS* u)
{
    uint8_t v{};
    u->readRegister8(CONTROL_REG, v, 0);
    return v;
}

}  // namespace

TEST_F(TestDDS, Basic)
{
    SCOPED_TRACE(ustr);

    char desc[7]{};
    EXPECT_TRUE(unit->readDescription(desc));
    EXPECT_TRUE(strcmp(desc, "ad9833") == 0) << desc;
}

TEST_F(TestDDS, Mode)
{
    SCOPED_TRACE(ustr);

    for (auto&& mode : mode_table) {
        EXPECT_TRUE(unit->writeMode(mode));
        Mode m{};
        EXPECT_TRUE(unit->readMode(m));
        EXPECT_EQ(m, mode);
    }
}

TEST_F(TestDDS, Settings)
{
    SCOPED_TRACE(ustr);

    for (auto&& mode : mode_table) {
        EXPECT_TRUE(unit->writeMode(mode));
        Mode m{};
        EXPECT_TRUE(unit->readMode(m));
        EXPECT_EQ(m, mode);

        //
        for (auto&& f : valid_freq_table) {
            EXPECT_TRUE(unit->writeFrequency(false, f)) << f;
            EXPECT_TRUE(unit->writeFrequency(true, f)) << f;
        }
        for (auto&& f : invalid_freq_table) {
            EXPECT_FALSE(unit->writeFrequency(false, f)) << f;
            EXPECT_FALSE(unit->writeFrequency(true, f)) << f;
        }

        //
        for (auto&& d : deg_table) {
            EXPECT_TRUE(unit->writePhase(false, d)) << d;
            EXPECT_TRUE(unit->writePhase(true, d)) << d;
        }

        //
        for (auto&& fb : bank_table) {
            for (auto&& db : bank_table) {
                for (auto&& f : valid_freq_table) {
                    for (auto&& d : deg_table) {
                        auto s = m5::utility::formatString("%u:%u %u:%u", fb, static_cast<unsigned>(f), db, d);
                        SCOPED_TRACE(s);
                        EXPECT_TRUE(unit->writeFrequencyAndPhase(fb, f, db, d));
                    }
                }
            }
        }

        for (auto&& fb : bank_table) {
            for (auto&& db : bank_table) {
                for (auto&& f : invalid_freq_table) {
                    for (auto&& d : deg_table) {
                        auto s = m5::utility::formatString("%u:%u %u:%u", fb, static_cast<unsigned>(f), db, d);
                        SCOPED_TRACE(s);
                        EXPECT_FALSE(unit->writeFrequencyAndPhase(fb, f, db, d));
                    }
                }
            }
        }

        //
        for (auto&& fb : bank_table) {
            EXPECT_TRUE(unit->writeCurrentFrequency(fb));
            uint8_t c = read_control(unit.get());
            EXPECT_EQ((fb ? 0x40 : 0x00), c & 0x40) << static_cast<unsigned>(c);

            for (auto&& db : bank_table) {
                auto s = m5::utility::formatString("%u:%u", fb, db);
                SCOPED_TRACE(s);

                EXPECT_TRUE(unit->writeCurrent(fb, db));
                c = read_control(unit.get());
                EXPECT_EQ((fb ? 0x40 : 0x00), c & 0x40) << static_cast<unsigned>(c);
                EXPECT_EQ((db ? 0x20 : 0x00), c & 0x20) << static_cast<unsigned>(c);

                EXPECT_TRUE(unit->writeCurrentPhase(db));
                c = read_control(unit.get());
                EXPECT_EQ((db ? 0x20 : 0x00), c & 0x20) << static_cast<unsigned>(c);
            }
        }
    }
}

TEST_F(TestDDS, Output)
{
    SCOPED_TRACE(ustr);

    for (auto&& m : mode_table) {
        for (auto&& b : bank_table) {
            for (auto&& f : valid_freq_table) {
                for (auto&& d : deg_table) {
                    auto s = m5::utility::formatString("%u:%u:%u:%u", static_cast<unsigned>(m), b,
                                                       static_cast<unsigned>(f), d);
                    SCOPED_TRACE(s);
                    EXPECT_TRUE(unit->writeOutput(m, b, f, d));
                }
            }
        }
    }

    for (auto&& m : mode_table) {
        for (auto&& b : bank_table) {
            for (auto&& f : invalid_freq_table) {
                for (auto&& d : deg_table) {
                    auto s = m5::utility::formatString("%u:%u:%u:%u", static_cast<unsigned>(m), b,
                                                       static_cast<unsigned>(f), d);
                    SCOPED_TRACE(s);
                    EXPECT_FALSE(unit->writeOutput(m, b, f, d));
                }
            }
        }
    }
}

TEST_F(TestDDS, FrequencyCache)
{
    SCOPED_TRACE(ustr);

    // Verify frequency0()/frequency1() cache after writeFrequency
    for (auto&& f : valid_freq_table) {
        auto s = m5::utility::formatString("freq:%u", static_cast<unsigned>(f));
        SCOPED_TRACE(s);
        EXPECT_TRUE(unit->writeFrequency(false, f));
        EXPECT_EQ(unit->frequency0(), f);
        EXPECT_TRUE(unit->writeFrequency(true, f));
        EXPECT_EQ(unit->frequency1(), f);
    }

    // Verify cache after writeFrequencyAndPhase
    EXPECT_TRUE(unit->writeFrequencyAndPhase(false, 123456, true, 90));
    EXPECT_EQ(unit->frequency0(), 123456U);
    EXPECT_TRUE(unit->writeFrequencyAndPhase(true, 999999, false, 45));
    EXPECT_EQ(unit->frequency1(), 999999U);

    // Verify cache is not updated on failure
    uint32_t prev = unit->frequency0();
    EXPECT_FALSE(unit->writeFrequency(false, MAXIMUM_FREQ + 1));
    EXPECT_EQ(unit->frequency0(), prev);
}

TEST_F(TestDDS, BeginAppliesConfig)
{
    SCOPED_TRACE(ustr);

    // begin() with start_output=false does not write output
    EXPECT_TRUE(unit->writeMode(Mode::Triangle));
    auto cfg         = unit->config();
    cfg.start_output = false;
    cfg.mode         = Mode::Sin;
    unit->config(cfg);
    EXPECT_TRUE(unit->begin());

    Mode m{};
    EXPECT_TRUE(unit->readMode(m));
    EXPECT_EQ(m, Mode::Triangle);

    // begin() with start_output=true applies the config
    cfg.start_output = true;
    cfg.mode         = Mode::Square;
    cfg.select       = true;
    cfg.freq         = 500000;
    cfg.deg          = 180;  // There is no API to read the phase back, so deg is not verified
    unit->config(cfg);
    EXPECT_TRUE(unit->begin());

    EXPECT_TRUE(unit->readMode(m));
    EXPECT_EQ(m, Mode::Square);

    // select=true: bank 1 is used for both frequency and phase, and the output is woken up
    const uint8_t c{read_control(unit.get())};
    EXPECT_EQ(0x60, c & 0x60) << static_cast<unsigned>(c);
    EXPECT_EQ(0, c & 0x1C) << static_cast<unsigned>(c);
    EXPECT_EQ(unit->frequency1(), 500000U);
}

TEST_F(TestDDS, ModeTransitionFreqRestore)
{
    SCOPED_TRACE(ustr);

    // Set known frequencies
    constexpr uint32_t freq0 = 100000;
    constexpr uint32_t freq1 = 200000;
    EXPECT_TRUE(unit->writeFrequency(false, freq0));
    EXPECT_TRUE(unit->writeFrequency(true, freq1));

    // Switch to Sawtooth (firmware resets internal freq to 0)
    EXPECT_TRUE(unit->writeMode(Mode::Sawtooth));
    Mode m{};
    EXPECT_TRUE(unit->readMode(m));
    EXPECT_EQ(m, Mode::Sawtooth);

    // Switch back to Sin — should restore frequencies
    EXPECT_TRUE(unit->writeMode(Mode::Sin));
    EXPECT_TRUE(unit->readMode(m));
    EXPECT_EQ(m, Mode::Sin);

    // Cache should still hold the original frequencies
    // (only the driver cache is checked: there is no API to read the frequency back)
    EXPECT_EQ(unit->frequency0(), freq0);
    EXPECT_EQ(unit->frequency1(), freq1);

    // Same test with DC mode
    EXPECT_TRUE(unit->writeMode(Mode::DC));
    EXPECT_TRUE(unit->writeMode(Mode::Triangle));
    EXPECT_TRUE(unit->readMode(m));
    EXPECT_EQ(m, Mode::Triangle);
    EXPECT_EQ(unit->frequency0(), freq0);
    EXPECT_EQ(unit->frequency1(), freq1);
}

TEST_F(TestDDS, RandomFrequency)
{
    SCOPED_TRACE(ustr);

    EXPECT_TRUE(unit->writeMode(Mode::Sin));

    // Only the driver cache is checked: there is no API to read the frequency back
    for (int i = 0; i < 10; ++i) {
        uint32_t f = esp_random() % (MAXIMUM_FREQ + 1);
        auto s     = m5::utility::formatString("random freq:%u", static_cast<unsigned>(f));
        SCOPED_TRACE(s);
        EXPECT_TRUE(unit->writeFrequency(false, f));
        EXPECT_EQ(unit->frequency0(), f);
    }
}

TEST_F(TestDDS, ModeReserved)
{
    SCOPED_TRACE(ustr);

    // Mode::Reserved (value 0) — firmware behavior is undefined but should not crash
    EXPECT_TRUE(unit->writeMode(Mode::Reserved));
    Mode m{};
    EXPECT_TRUE(unit->readMode(m));
    EXPECT_EQ(m, Mode::Reserved);

    // Should be able to switch back to a normal mode
    EXPECT_TRUE(unit->writeMode(Mode::Sin));
    EXPECT_TRUE(unit->readMode(m));
    EXPECT_EQ(m, Mode::Sin);
}

TEST_F(TestDDS, Sleep)
{
    SCOPED_TRACE(ustr);

    EXPECT_FALSE(unit->sleep(false, false));

    EXPECT_TRUE(unit->sleep(true, false));
    uint8_t c = read_control(unit.get());
    EXPECT_EQ(0x10, c & 0x10) << static_cast<unsigned>(c);

    EXPECT_TRUE(unit->sleep(false, true));
    c = read_control(unit.get());
    EXPECT_EQ(0x08, c & 0x08) << static_cast<unsigned>(c);

    EXPECT_TRUE(unit->sleep());  // true,true
    c = read_control(unit.get());
    EXPECT_EQ(0x18, c & 0x18) << static_cast<unsigned>(c);

    EXPECT_TRUE(unit->reset());
    c = read_control(unit.get());
    EXPECT_EQ(0x04, c & 0x04) << static_cast<unsigned>(c);

    EXPECT_TRUE(unit->wakeup());
    c = read_control(unit.get());
    EXPECT_EQ(0, c & 0x1C);
}

TEST_F(TestDDS, SleepWriteSettings)
{
    SCOPED_TRACE(ustr);

    // Write initial state
    EXPECT_TRUE(unit->writeMode(Mode::Sin));
    EXPECT_TRUE(unit->writeFrequency(false, 1000));

    // Enter MCLK sleep
    EXPECT_TRUE(unit->sleep(true, false));
    uint8_t c{read_control(unit.get())};
    EXPECT_EQ(0x10, c & 0x18) << static_cast<unsigned>(c);

    // Write new frequency while sleeping — AD9833 accepts register writes during SLEEP1
    EXPECT_TRUE(unit->writeFrequency(false, 500000));
    EXPECT_EQ(unit->frequency0(), 500000U);

    // Write phase while sleeping
    EXPECT_TRUE(unit->writePhase(false, 90));

    // Wakeup and verify mode is still correct
    EXPECT_TRUE(unit->wakeup());
    c = read_control(unit.get());
    EXPECT_EQ(0, c & 0x1C) << static_cast<unsigned>(c);
    Mode m{};
    EXPECT_TRUE(unit->readMode(m));
    EXPECT_EQ(m, Mode::Sin);
}
