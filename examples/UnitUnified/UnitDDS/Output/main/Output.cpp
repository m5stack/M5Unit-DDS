/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example using M5UnitUnified for UnitDDS
*/
#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedDDS.h>
#include <M5Utility.h>
#include <wiring/m5_unit_unified_wiring.hpp>  // Board-aware connection helpers (include last)

using namespace m5::unit::dds;

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;
m5::unit::UnitDDS unit;

constexpr Mode mode_table[] = {
    Mode::Sin, Mode::Triangle, Mode::Square, Mode::Sawtooth, Mode::DC,
};
const char* mode_str[] = {
    "Sin", "Triangle", "Square", "Sawtooth", "DC",
};
uint8_t mode_index{};
bool cur_bank{};
constexpr uint32_t FREQ_BANK_0{10};
constexpr uint32_t FREQ_BANK_1{80};

}  // namespace

void setup()
{
    M5.begin();
    M5.setTouchButtonHeightByRatio(100);
    // The screen shall be in landscape mode
    if (lcd.height() > lcd.width()) {
        lcd.setRotation(1);
    }

    // Board-aware connection: NessoN1 -> SoftwareI2C on GROVE, NanoC6/NanoH2 -> M5.Ex_I2C, others -> Wire
    if (!m5::unit::wiring::addI2C(Units, unit) || !Units.begin()) {
        M5_LOGE("Failed to begin");
        m5::unit::wiring::failStop();
    }

    M5_LOGI("M5UnitUnified initialized");
    M5_LOGI("%s", Units.debugInfo().c_str());

    lcd.fillScreen(TFT_DARKGREEN);

    unit.writeFrequencyAndPhase(true, FREQ_BANK_1, true, 180);           // Set freq and phase to BANK 1
    unit.writeOutput(mode_table[mode_index], cur_bank, FREQ_BANK_0, 0);  // Set freq and phase to Bank 0 and use BANK 0

    lcd.setFont(&fonts::AsciiFont8x16);
    M5.Log.printf("Output:%s\n", mode_str[mode_index]);
    lcd.fillRect(0, 0, lcd.width(), 16 * 2, TFT_BLACK);
    lcd.setCursor(0, 0);
    lcd.printf("Output:%s\nFreq:%u", mode_str[mode_index], (unsigned int)(cur_bank ? FREQ_BANK_1 : FREQ_BANK_0));
}

void loop()
{
    M5.update();
    Units.update();

    // Change mode
    // To reduce glitches on mode change, enclose it in sleep(true,false) and wakeup()
    if (M5.BtnA.wasClicked()) {
        M5.Speaker.tone(3000, 20);

        unit.sleep(true, false);
        if (++mode_index >= m5::stl::size(mode_table)) {
            mode_index = 0;
        }
        unit.writeMode(mode_table[mode_index]);
        unit.wakeup();

        // Frequency and phase settings are ignored for Mode::Sawtooth and Mode::DC
        M5.Log.printf("Output:%s Freq:%u\n", mode_str[mode_index], cur_bank ? FREQ_BANK_1 : FREQ_BANK_0);
        lcd.fillRect(0, 0, lcd.width(), 16 * 2, TFT_BLACK);
        lcd.setCursor(0, 0);
        lcd.printf("Output:%s\nFreq:%u", mode_str[mode_index], (unsigned int)(cur_bank ? FREQ_BANK_1 : FREQ_BANK_0));
    }

    // Change using bank for freq/phase
    if (M5.BtnA.wasHold()) {
        M5.Speaker.tone(1500, 20);
        cur_bank = !cur_bank;
        unit.writeCurrent(cur_bank, cur_bank);

        M5.Log.printf("Output:%s Freq:%u\n", mode_str[mode_index], cur_bank ? FREQ_BANK_1 : FREQ_BANK_0);
        lcd.fillRect(0, 0, lcd.width(), 16 * 2, TFT_BLACK);
        lcd.setCursor(0, 0);
        lcd.printf("Output:%s\nFreq:%u", mode_str[mode_index], (unsigned int)(cur_bank ? FREQ_BANK_1 : FREQ_BANK_0));
    }
}
