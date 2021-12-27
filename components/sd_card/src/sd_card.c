#include <freertos/FreeRTOS.h>
#include <sd_card.h>
#include <esp_vfs.h>
#include <esp_vfs_fat.h>
#include <sdmmc_cmd.h>

#define MOUNT_POINT "/sdcard"

#define PIN_MISO 26
#define PIN_MOSI 14
#define PIN_CLK  27
#define PIN_CS   12

sdmmc_host_t host = SDSPI_HOST_DEFAULT();
sdmmc_card_t* card = NULL;
esp_vfs_fat_sdmmc_mount_config_t mount_config = {
    .format_if_mount_failed = false,
    .max_files = 5,
    .allocation_unit_size = 16*1024
};
spi_bus_config_t bus_config = {
    .mosi_io_num = PIN_MOSI,
    .miso_io_num = PIN_MISO,
    .sclk_io_num = PIN_CLK,
    .quadhd_io_num = -1,
    .quadwp_io_num = -1,
    .max_transfer_sz = 4000
};
sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();

static const char* TAG = "sdcard";

void sd_card_init() {
    esp_err_t ret;

    ret = spi_bus_initialize(host.slot, &bus_config, SPI_DMA_CH2);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Unable to initialize SPI bus!");
        return;
    }
    
    slot_config.gpio_cs = PIN_CS;
    slot_config.host_id = host.slot;

    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Unable to create mount!");
        return;
    }

    ESP_LOGI(TAG, "SD Card mounted!");

    sdmmc_card_print_info(stdout, card);
}

void sd_card_deinit() {
    esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
    spi_bus_free(host.slot);
}

FILE* sd_card_open_file(char filename[]) {
    printf("Poopy");
    sdmmc_card_print_info(stdout, card);
    // sizeof(MOUNT_POINT) includes the null char, which technically makes it one longer
    // but we need to include the "/" for the directory, which would be a +1, but we are
    // using the extra from the sizeof() to account for that
    char file[sizeof(MOUNT_POINT) + strlen(filename) + 1];
    sprintf(file, MOUNT_POINT "/%s", filename);
    ESP_LOGI(TAG, "Opening file: %s", file);
    FILE* f_h = fopen(file, "w+");
    if (f_h != NULL) return f_h;

    ESP_LOGE(TAG, "Unable to open %s", file);
    return NULL;
}
