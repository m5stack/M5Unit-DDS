/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
  Example using M5UnitUnified for UnitDDS
*/
#include <cstdio>

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

// Show the current output. Frequency and phase settings are ignored for Mode::Sawtooth and Mode::DC
void show_output()
{
    char freq[16]{};
    const Mode m{mode_table[mode_index]};
    if (m == Mode::Sawtooth) {
        snprintf(freq, sizeof(freq), "fixed");
    } else if (m == Mode::DC) {
        snprintf(freq, sizeof(freq), "-");
    } else {
        snprintf(freq, sizeof(freq), "%u", static_cast<unsigned>(cur_bank ? FREQ_BANK_1 : FREQ_BANK_0));
    }
    M5.Log.printf("Output:%s Freq:%s\n", mode_str[mode_index], freq);
    lcd.fillRect(0, 0, lcd.width(), 16 * 2, TFT_BLACK);
    lcd.setCursor(0, 0);
    lcd.printf("Output:%s\nFreq:%s", mode_str[mode_index], freq);
}

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

    // Set freq and phase to BANK 1, then set freq and phase to Bank 0 and use BANK 0
    if (!unit.writeFrequencyAndPhase(true, FREQ_BANK_1, true, 180) ||
        !unit.writeOutput(mode_table[mode_index], cur_bank, FREQ_BANK_0, 0)) {
        M5_LOGE("Failed to write output");
        m5::unit::wiring::failStop();
    }

    lcd.setFont(&fonts::AsciiFont8x16);
    show_output();
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

        show_output();
    }

    // Change using bank for freq/phase
    if (M5.BtnA.wasHold()) {
        M5.Speaker.tone(1500, 20);
        cur_bank = !cur_bank;
        unit.writeCurrent(cur_bank, cur_bank);

        show_output();
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
