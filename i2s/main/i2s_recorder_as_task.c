/* I2S Example


*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/i2s.h"
#include "driver/gpio.h"
#include "driver/sdmmc_host.h"
#include "esp_system.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "esp_log.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include "sdkconfig.h"
#include "nvs.h"
#include "nvs_flash.h"


static const char *TAG = "i2s_recorder";

#define SAMPLE_RATE     (48000)
#define I2S_BITS_PER_SAMPLE I2S_BITS_PER_SAMPLE_32BIT
#define FILE_BITS_PER_SAMPLE (16)
#define RECBUF_SIZE     (SAMPLE_RATE*FILE_BITS_PER_SAMPLE*2)    // 1 second
#define NUM_RECBUFS     (8)
#define MAX_SAMPLES     (256)
#define I2S_NUM         (0)
#define I2S_BCK_IO      (GPIO_NUM_32)
#define I2S_WS_IO       (GPIO_NUM_27)
#define I2S_DO_IO       (I2S_PIN_NO_CHANGE)
#define I2S_DI_IO       (GPIO_NUM_25)
#define MOUNT_POINT     "/sdcard"


// DMA channel to be used by the SPI peripheral
#ifndef SPI_DMA_CHAN
#define SPI_DMA_CHAN    1
#endif //SPI_DMA_CHAN
// Pin mapping when using SPI mode.
// With this mapping, SD card can be used both in SPI and 1-line SD mode.
// Note that a pull-up on CS line is required in SD mode.
#define PIN_NUM_MISO 19
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   5

#define PRO_CPU	0
#define APP_CPU	1

xTaskHandle TaskHandle_Task1;

void i2s_init(void);
void sd_init(void);
void main_task(void * pvParameters);

int32_t buffer32[MAX_SAMPLES] = {0};
sdmmc_card_t* card;
const char mount_point[] = MOUNT_POINT;
QueueHandle_t queue;
void *buffer[NUM_RECBUFS];
typedef struct qe 
{
    /* data */
    char filename[256];
    uint32_t seqno;
    void *buffer;
    size_t buf_len;
} q_entry;



void app_main(void)
{
    ESP_LOGI(TAG, "..._as_task.c");

    // Allocate from PSRAM the buffer pages we will use to grab record data
    for (int i=0; i<NUM_RECBUFS; i++) {
        if ((buffer[i] = malloc(RECBUF_SIZE)) == NULL) {
            ESP_LOGE(TAG, "Failed to allocate a record buffer.");
        }
        ESP_LOGI(TAG, "Allocated %d bytes at 0x%08X", RECBUF_SIZE, (uint32_t)buffer[i]);
    }

    // Allocate a queue, with max depth corresponding to the number of buffers


    xTaskCreatePinnedToCore(main_task, "i2s_record", 8192, NULL, 1, NULL, APP_CPU);
}

void main_task(void * pvParameters) {
    sd_init();

    // First create a file.
    ESP_LOGI(TAG, "Opening file");
    FILE* f = fopen(MOUNT_POINT"/60s.raw", "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }

    // Initialise the I2S bus
    i2s_init();

    // Loop reading from I2S and writing to file
    size_t samplesWritten = 0;
    while (samplesWritten < 48000 * 2 * 180) {
        size_t bytesRead = 0;

        esp_err_t rc;

        rc = i2s_read(I2S_NUM, buffer32, sizeof(buffer32), &bytesRead, 100);
        int samplesRead = bytesRead / 4;
        if (rc != ESP_OK) {
            ESP_LOGE(TAG, "i2s_read(): rc=%d  bytes=%d\n", rc, bytesRead);
        }

        if (fwrite(buffer32, sizeof(int32_t), samplesRead, f) < samplesRead) {
            ESP_LOGE(TAG, "Failed to write samples.");
            return; 
        }
        
        samplesWritten += samplesRead;

    }


    fclose(f);
    ESP_LOGI(TAG, "File written");

    // All done, unmount partition and disable SDMMC or SPI peripheral
    esp_vfs_fat_sdcard_unmount(mount_point, card);
    ESP_LOGI(TAG, "Card unmounted");

    while (1)
        ;

    //deinitialize the bus after all devices are removed
    // spi_bus_free(host.slot);
}

void read_loop() {
    
}

void sd_init(void) {
    esp_err_t ret;
    
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    ESP_LOGI(TAG, "Initializing SD card");

    ESP_LOGI(TAG, "Using SPI peripheral");

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    ret = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CHAN);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return;
    }

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = PIN_NUM_CS;
    slot_config.host_id = host.slot;

    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);

        // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);

    // Use POSIX and C standard library functions to work with files.
    // First create a file.
    ESP_LOGI(TAG, "Opening file");
    FILE* f = fopen(MOUNT_POINT"/hello.txt", "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return;
    }
    fprintf(f, "Hello %s!\n", card->cid.name);
    fclose(f);
    ESP_LOGI(TAG, "File written");

    // Check if destination file exists before renaming
    struct stat st;
    if (stat(MOUNT_POINT"/foo.txt", &st) == 0) {
        // Delete it if it exists
        unlink(MOUNT_POINT"/foo.txt");
    }

    // Rename original file
    ESP_LOGI(TAG, "Renaming file");
    if (rename(MOUNT_POINT"/hello.txt", MOUNT_POINT"/foo.txt") != 0) {
        ESP_LOGE(TAG, "Rename failed");
        return;
    }

    // Open renamed file for reading
    ESP_LOGI(TAG, "Reading file");
    f = fopen(MOUNT_POINT"/foo.txt", "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return;
    }
    char line[64];
    fgets(line, sizeof(line), f);
    fclose(f);
    // strip newline
    char* pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0';
    }
    ESP_LOGI(TAG, "Read from file: '%s'", line);

}


void i2s_init () {

    // ESP32 as slave seems to be prone to frame alignment errors eg samples 
    // that should start 0xF start 0x7. 
    // Switching instead to ESP32 as master using code from:
    // https://github.com/YetAnotherElectronicsChannel/ESP32_DSP_I2S_SETUP/blob/master/code/main/main.c

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,                           //2-channels
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .dma_buf_count = 4,
        .dma_buf_len = MAX_SAMPLES,
        .use_apll = false,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,  //Interrupt level 1
        .tx_desc_auto_clear = true,
    };
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCK_IO,
        .ws_io_num = I2S_WS_IO,
        .data_out_num = I2S_DO_IO,
        .data_in_num = I2S_DI_IO                                               //Not used
    };
    if (ESP_OK != i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL)) {
        printf("i2s_driver_install: error");
    }
    if (ESP_OK != i2s_set_pin(I2S_NUM, &pin_config)) {
        printf("i2s_set_pin: error");
    }

    //enable MCLK on GPIO0
	REG_WRITE(PIN_CTRL, 0xFF0); 
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1);

    
}
