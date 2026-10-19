# Third-party code

labelbee itself is licensed under the Apache License 2.0 (`LICENSE`). It embeds the following
third-party parts; their licence texts are next to them.

## Web interface (`www/vendor/`, `www/fonts/`)

| Library | Version | Licence | Used for |
|---|---|---|---|
| [qrcode-generator](https://github.com/kazuhikoarase/qrcode-generator) by Kazuhiko Arase | 1.4.4 | MIT (`www/vendor/LICENSE-qrcode-generator`) | QR codes |
| [JsBarcode](https://github.com/lindell/JsBarcode) by Johan Lindell | 3.11.6 | MIT (`www/vendor/LICENSE-jsbarcode`) | Code 128, Code 39, EAN-13 |
| [Material Design Icons](https://pictogrammers.com/library/mdi/) (`@mdi/font`, categories and aliases from `@mdi/svg` meta.json) | 7.4.47 | Pictogrammers Free License: icons and font Apache-2.0, code MIT (`www/vendor/LICENSE-mdi`) | icons on labels |
| Roboto, Roboto Condensed, Roboto Mono, Oswald, Barlow, Barlow Condensed (via `@fontsource`) | 5.3.0 | SIL OFL 1.1 (`www/fonts/LICENSE-*`) | label fonts; Barlow Condensed Bold for the wordmark |
| Roboto Slab (via `@fontsource`) | 5.3.0 | Apache-2.0 (`www/fonts/LICENSE-roboto-slab`) | label font |
| Comic Neue, Fredoka, Bangers, Kalam, Pacifico, Lobster, Silkscreen, Black Ops One, Bebas Neue (via `@fontsource`) | 5.3.0 | SIL OFL 1.1 (`www/fonts/LICENSE-*`) | label fonts |
| Luckiest Guy, Permanent Marker, Special Elite (via `@fontsource`) | 5.3.0 | Apache-2.0 (`www/fonts/LICENSE-*`) | label fonts |

"QR Code" is a registered trademark of DENSO WAVE INCORPORATED.

## Firmware

Built with [ESP-IDF](https://github.com/espressif/esp-idf) (Apache-2.0). Components from the ESP
component registry (`src/idf_component.yml`, versions in `dependencies.lock`, fetched at build time,
not part of this repository):

| Component | Licence |
|---|---|
| `espressif/usb`, `espressif/mdns` | Apache-2.0 |
| `espressif/cjson` (cJSON by Dave Gamble and contributors) | MIT |
| `joltwallet/littlefs` (by Brian Pugh), with littlefs by Arm Limited and the littlefs authors | MIT, littlefs BSD-3-Clause |

## Print protocol

`src/label.cpp` and `src/label.h` are derived from the following projects, both licensed under the
Apache License 2.0:

- [dymo-print](https://github.com/piobyte/dymo-print), Copyright (C) 2021 piobyte GmbH.
- [dymoprint](https://github.com/computerlyrik/dymoprint), Copyright (c) 2011 Sebastian J. Bronner,
  continued as [labelle](https://github.com/labelle-org/labelle).

## Case

The lid's wordmark in `case/labelbee-case.scad` (and the exported STL files) uses Barlow Condensed
Bold, SIL OFL 1.1.
