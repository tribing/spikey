# Spikey

Spikey is a Spinosaurus virtual pet for Arduino Uno R4 and a 320x240 ST7789 TFT display. It mixes classic virtual-pet care with a small runner mini-game, persistent save data, sound, animated feeding/drinking, and a bright prehistoric scene.

## Features

- Spinosaurus pet with hunger, joy, health, discipline, weight, age, sleep, poop, and death/victory states.
- Play mode inspired by dinosaur runner games, with rocks, cactus, pteranodons, ponds, score, and hi-score.
- Food, water, cleaning, doctor, training, sleep, stats, and sound controls.
- EEPROM save support for pet state, hi-score, and sound setting.
- ST7789 320x240 color TFT rendering with strip-buffered graphics.

## Hardware

- Arduino Uno R4 WiFi
- ST7789 320x240 TFT display
- 3 push buttons
- Piezo buzzer

Default pins are defined in [src/Spikey.ino](src/Spikey.ino):

| Part | Pin |
| --- | --- |
| TFT CS | 10 |
| TFT DC | 9 |
| TFT RST | 8 |
| Left button | 2 |
| Select button | 3 |
| Right button | 4 |
| Buzzer | 5 |

Buttons use `INPUT_PULLUP`, so wire each button between its pin and GND.

## Build

This project uses PlatformIO.

```sh
pio run
```

To upload:

```sh
pio run --target upload
```

## Controls

- Left button: open/cycle menu, jump in Play mode, retry after game over.
- Select button: select menu option, crouch in Play mode, retry after game over.
- Right button: back/exit, exit Play mode.

## Project Layout

- [src/Spikey.ino](src/Spikey.ino): main firmware.
- [platformio.ini](platformio.ini): PlatformIO board and library configuration.
- [diagram.json](diagram.json): wiring/project diagram metadata.
- [CONTRIBUTING.md](CONTRIBUTING.md): contribution rules for the main project.

## License

Spikey is shared under the Spikey Source-Available Community License v1.0.

You may read, study, build, flash, and privately modify Spikey for personal, non-commercial use. Contributions are welcome through the main project repository.

Forks, public mirrors, redistribution, rebranded versions, product versions, commercial use, paid services, manufacturing, kits, binaries, and firmware images are not allowed without prior written permission from the project owners.

This is a source-available community project, not an OSI-approved open-source project, because the license intentionally restricts forks, redistribution, and commercial use.
