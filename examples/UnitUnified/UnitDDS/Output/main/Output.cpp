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
#include <M5HAL.hpp>

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
// constexpr uint32_t FREQ_BANK_0{10000};
// constexpr uint32_t FREQ_BANK_1{80000};
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

    auto board = M5.getBoard();

    // NessoN1: Arduino Wire (I2C_NUM_0) cannot be used for GROVE port.
    //   Wire is used by M5Unified In_I2C for internal devices (IOExpander etc.).
    //   Wire1 exists but is reserved for HatPort — cannot be used for GROVE.
    //   Reconfiguring Wire to GROVE pins breaks In_I2C, causing ESP_ERR_INVALID_STATE in M5.update().
    //   Solution: Use SoftwareI2C via M5HAL (bit-banging) for the GROVE port.
    // NanoC6: Wire.begin() on GROVE pins conflicts with m5::I2C_Class registered by Ex_I2C.setPort()
    //   on the same I2C_NUM_0, causing sporadic NACK errors.
    //   Solution: Use M5.Ex_I2C (m5::I2C_Class) directly instead of Arduino Wire.
    bool unit_ready{};
    if (board == m5::board_t::board_ArduinoNessoN1) {
        // NessoN1: GROVE is on port_b (GPIO 5/4), not port_a (which maps to Wire pins 8/10)
        auto pin_num_sda = M5.getPin(m5::pin_name_t::port_b_out);
        auto pin_num_scl = M5.getPin(m5::pin_name_t::port_b_in);
        M5_LOGI("getPin(M5HAL): SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
        m5::hal::bus::I2CBusConfig i2c_cfg;
        i2c_cfg.pin_sda = m5::hal::gpio::getPin(pin_num_sda);
        i2c_cfg.pin_scl = m5::hal::gpio::getPin(pin_num_scl);
        auto i2c_bus    = m5::hal::bus::i2c::getBus(i2c_cfg);
        M5_LOGI("Bus:%d", i2c_bus.has_value());
        unit_ready = Units.add(unit, i2c_bus ? i2c_bus.value() : nullptr) && Units.begin();
    } else if (board == m5::board_t::board_M5NanoC6) {
        // NanoC6: Use M5.Ex_I2C (m5::I2C_Class, not Arduino Wire)
        M5_LOGI("Using M5.Ex_I2C");
        unit_ready = Units.add(unit, M5.Ex_I2C) && Units.begin();
    } else {
        auto pin_num_sda = M5.getPin(m5::pin_name_t::port_a_sda);
        auto pin_num_scl = M5.getPin(m5::pin_name_t::port_a_scl);
        M5_LOGI("getPin: SDA:%u SCL:%u", pin_num_sda, pin_num_scl);
        Wire.end();
        Wire.begin(pin_num_sda, pin_num_scl, 400 * 1000U);
        unit_ready = Units.add(unit, Wire) && Units.begin();
    }
    if (!unit_ready) {
        M5_LOGE("Failed to begin");
        lcd.fillScreen(TFT_RED);
        while (true) {
            m5::utility::delay(10000);
        }
    }

    M5_LOGI("M5UnitUnified has been begun");
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
