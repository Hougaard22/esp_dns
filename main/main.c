#include <esp_event.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <freertos/FreeRTOS.h>
#include <esp_wifi.h>
#include "dnssrv.h"

// WiFi configuration
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASS "your_wifi_password"

static void wifi_init_sta() {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
}

void app_main() {
    nvs_flash_init();

	esp_netif_init();
	esp_event_loop_create_default();

    wifi_init_sta();

    start_dns_server();
}