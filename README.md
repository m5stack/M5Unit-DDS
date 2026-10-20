# M5Unit - DDS

## Overview

Library for Unit DDS using [M5UnitUnified](https://github.com/m5stack/M5UnitUnified).  
M5UnitUnified is a library for unified handling of various M5 units products.

### SKU:U105

DDS is a signal source Unit. It uses the AD9833 programmable waveform generator + STM32F0 micro controller. Based on I2C communication interface (addr:0x31) It can easily control the signal source to output multiple waveforms (sine wave, triangle wave, square wave output, sawtooth wave, signal output amplitude 0-0.6V) and adjust the frequency and phase.

It supports deep sleep mode, which can reduce the power consumption in the idle state. The Unit is suitable for the electronic circuit prototype design of various test instruments as a signal source.

## Related Link

- [Unit DDS - Document & Datasheet](https://docs.m5stack.com/en/unit/dds)

## Required Libraries

- [M5UnitUnified](https://github.com/m5stack/M5UnitUnified)
- [M5Utility](https://github.com/m5stack/M5Utility)
- [M5HAL](https://github.com/m5stack/M5HAL)

## License

- [M5Unit-DDS - MIT](LICENSE)

## Examples
See also [examples/UnitUnified](examples/UnitUnified)

- [UnitDDS/Output](examples/UnitUnified/UnitDDS/Output): Outputs a waveform. BtnA click changes the waveform, BtnA hold switches the frequency and phase bank
- [UnitDDS/ViaPaHub](examples/UnitUnified/UnitDDS/ViaPaHub): The same through PaHub (ch:0).
  Requires [M5Unit-HUB](https://github.com/m5stack/M5Unit-HUB).

### For ESP-IDF settings

> **NOTE:** The ESP-IDF native build (`idf.py`) targets ESP-IDF **5.1 or later** (5.x and 6.x).
> Only the M5UnitUnified driver (`M5UnitUnifiedDDS.h`) is available. The legacy driver (`Unit_DDS.h`) is for Arduino only.

Each example covers a single unit, so the examples need no Kconfig choice. Build them directly:

```sh
cd examples/UnitUnified/UnitDDS/Output
idf.py set-target esp32s3               # or esp32 / esp32c5 / esp32c6 / esp32h2 / esp32p4 / ...
idf.py build flash monitor
```

## Support via [PaHub](https://docs.m5stack.com/en/unit/Unit-PaHub%20v2.1)

|Unit|Support|Note|
|---|---|---|
|UnitDDS|OK||

See also the [ViaPaHub](examples/UnitUnified/UnitDDS/ViaPaHub) example and [M5Unit-HUB](https://github.com/m5stack/M5Unit-HUB)

## Doxygen document
[GitHub Pages](https://m5stack.github.io/M5Unit-DDS/)

If you want to generate documents on your local machine, execute the following command

```
bash docs/doxy.sh
```

It will output it under docs/html  
If you want to output Git commit hashes to html, do it for the git cloned folder.

### Required
- [Doxygen](https://www.doxygen.nl/)
- [pcregrep](https://formulae.brew.sh/formula/pcre2)
- [Git](https://git-scm.com/) (Output commit hash to html)
