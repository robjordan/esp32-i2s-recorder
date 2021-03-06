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

### Real-Time Clock
The system requires a real-time clock, to be able to timestamp audio files correctly. This is not yet implemented.

## Software

### Tasks
The program comprises two tasks:
- The I<sup>2</sup>S task, which pulls data from the I<sup>2</sup>S bus, and writes it to large memory buffers in PSRAM, which are then enqueued to the SD card task. This overcomes a problem seen in the previous version, where writes to SD card would block for a long period, causes I<sup>2</sup>S data to lost.
- The SD card task, which waits on a queue for commands from the I<sup>2</sup>S task. Each queued command points to one memory buffer, containing one second of audio (192 KBytes). The queued command also includes a timestamp, to 1 minute resolution, to be used for generating a filename, and a sequence number, which happens to be the number of seconds within that minute.
