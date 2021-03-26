# esp32-i2s-recorder
Record audio received over I<sup>2</sup>S from a separate ADC module.

## Hardware

### [Lusya I<sup>2</sup>S ADC](https://www.aliexpress.com/item/1005001689599237.html)

Alternative, similar, product from Europe: [Audiophonics](https://www.audiophonics.fr/en/devices-hifi-audio-adc/adc-analog-to-digital-converter-akm5720-i2s-24bit-96khz-p-13351.html)

This provides an audio interface for a stereo pair of RCA jacks, or a 3.5mm TRS jack. It uses a good-quality AKM5720 ADC module to convert the stereo signal to 48kHz 24-bit audio and puts this onto the I<sup>2</sup>S bus. (96kHz sampling should also be possible).

Requires a stable Vcc of 4.2 - 5V.

### [ESP32-WROVER-B](https://www.espressif.com/sites/default/files/documentation/esp32-wrover-b_datasheet_en.pdf)

This variant of the ESP32 has 8MB of PSRAM. The ESP32 provides a master clock to the I<sup>2</sup>S bus, and reads from the the digital audio provided by the ADC module. It then writes the audio data to SD card.

Connections:

| ESP32  | ADC   |
|--------|-------|
| GPIO00 | MCLK  |
| GPIO32 | BICK |
| GPIO25 | DATA |
| GPIO27 | LRCK |
| GND    | GND  |
| 5v (via cap) | VCC |

### [SD card adapter](https://www.aliexpress.com/item/32867572635.html)

Provides interface to persist audio files to Micro SD card. 

Connections:

| ESP32  | SD card   |
|--------|-------|
| 3V3 | 3V3  |
| GPIO5 | CS |
| GPIO23 | MOSI |
| GPIO18 | CLK |
| GPIO19   | MISO  |
| 5v (via cap) | GND |
