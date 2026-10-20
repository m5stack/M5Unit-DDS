/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example of using UnitDDS via UnitPaHub

  Board ---> PaHub ---> ch:0 UnitDDS
  BtnA click: change the waveform / BtnA hold: switch the frequency and phase bank
*/
#include <algorithm>

#include <M5Unified.h>
#include <M5UnitUnified.h>
#include <M5UnitUnifiedDDS.h>
#include <M5Utility.h>
#include <M5UnitUnifiedHUB.h>                 // UnitPaHub
#include <wiring/m5_unit_unified_wiring.hpp>  // Board-aware connection helpers (include last)

using namespace m5::unit::dds;

namespace {
auto& lcd = M5.Display;
m5::unit::UnitUnified Units;
m5::unit::UnitPaHub hub;
m5::unit::UnitDDS unit;

constexpr uint8_t UNIT_CHANNEL{0};  // PaHub channel the unit is connected to

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

    if (!hub.add(unit, UNIT_CHANNEL)) {  // PaHub ch:0 -> UnitDDS
        M5_LOGE("Failed to add children");
        m5::unit::wiring::failStop();
    }
    // Run the PaHub at the lowest clock of the units behind it: its channel select is sent at that clock, and a
    // read on its own runs at the clock left by the last transfer
    auto hcfg  = hub.component_config();
    hcfg.clock = std::min(hcfg.clock, unit.component_config().clock);
    hub.component_config(hcfg);
    // Board-aware I2C for the PaHub: NessoN1 -> SoftwareI2C on GROVE, NanoC6/NanoH2 -> M5.Ex_I2C, others -> Wire.
    // The unit is reached through the hub (added above), so only the hub is added here.
    if (!m5::unit::wiring::addI2C(Units, hub) || !Units.begin()) {
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

#if !defined(ARDUINO)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>

#if CONFIG_FREERTOS_UNICORE
// Single-core SoCs: run loop() back-to-back, but every ~2 s yield a 5 ms slice so the IDLE task
// runs and feeds the task watchdog (default 5 s).
static inline void feedIdleTaskPeriodically(void)
{
    constexpr uint32_t FEED_INTERVAL_MS{2000};
    constexpr TickType_t FEED_SLEEP_TICKS{pdMS_TO_TICKS(5)};
    static uint32_t s_last_feed_ms{};
    const uint32_t now_ms{static_cast<uint32_t>(esp_timer_get_time() / 1000)};
    if (now_ms - s_last_feed_ms >= FEED_INTERVAL_MS) {
        s_last_feed_ms = now_ms;
        vTaskDelay(FEED_SLEEP_TICKS);
    }
}
#endif

extern "C" void app_main(void)
{
    setup();
    for (;;) {
#if CONFIG_FREERTOS_UNICORE
        feedIdleTaskPeriodically();
#endif
        loop();
    }
}
#endif
