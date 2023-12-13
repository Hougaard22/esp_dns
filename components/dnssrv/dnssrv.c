#include <string.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_wifi.h>
#include <lwip/sockets.h>

#define TAG "DNS_SERVER"

// DNS server configuration
#define DNS_SERVER_IP "8.8.8.8"
#define DNS_SERVER_PORT 53

// Rate limiting configuration
#define MAX_REQUESTS_PER_SECOND 5
#define TOKEN_BUCKET_SIZE 10

// Token bucket variables
static int tokens = TOKEN_BUCKET_SIZE;
static TickType_t last_token_time = 0;
static TickType_t token_interval;


static void dns_server_task(void *pvParameters) {
    token_interval = pdMS_TO_TICKS(1000 / MAX_REQUESTS_PER_SECOND);

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket");
        vTaskDelete(NULL);
    }

    struct sockaddr_in server_address = {
        .sin_family = AF_INET,
        .sin_port = htons(DNS_SERVER_PORT),
        .sin_addr.s_addr = INADDR_ANY,
    };

    if (bind(sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        ESP_LOGE(TAG, "Unable to bind socket");
        close(sock);
        vTaskDelete(NULL);
    }

    struct sockaddr_in dns_server_address = {
        .sin_family = AF_INET,
        .sin_port = htons(DNS_SERVER_PORT),
        .sin_addr.s_addr = inet_addr(DNS_SERVER_IP),
    };

    ESP_LOGI(TAG, "DNS server started on %s:%d", DNS_SERVER_IP, DNS_SERVER_PORT);

    while (1) {
        struct sockaddr_in client_address;
        socklen_t client_address_len = sizeof(client_address);
        uint8_t buf[512];

        int len = recvfrom(sock, buf, sizeof(buf) - 1, 0, (struct sockaddr *)&client_address, &client_address_len);
        if (len < 0) {
            ESP_LOGE(TAG, "recvfrom failed");
            continue;
        }

        buf[len] = '\0';

        // Check rate limit
        TickType_t current_time = xTaskGetTickCount();
        if (current_time - last_token_time > token_interval) {
            tokens = TOKEN_BUCKET_SIZE;
            last_token_time = current_time;
        }

        if (tokens <= 0) {
            ESP_LOGW(TAG, "Rate limit exceeded. Dropping DNS request.");
            continue;
        }

        tokens--;

        ESP_LOGI(TAG, "Received DNS request: %s", buf);

        // Forward the DNS query to the external DNS server
        int external_dns_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
        if (external_dns_sock < 0) {
            ESP_LOGE(TAG, "Unable to create external DNS socket");
            continue;
        }

        if (connect(external_dns_sock, (struct sockaddr *)&dns_server_address, sizeof(dns_server_address)) < 0) {
            ESP_LOGE(TAG, "Unable to connect to external DNS server");
            close(external_dns_sock);
            continue;
        }

        if (sendto(external_dns_sock, buf, len, 0, (struct sockaddr *)&dns_server_address, sizeof(dns_server_address)) < 0) {
            ESP_LOGE(TAG, "Unable to send DNS query to external DNS server");
            close(external_dns_sock);
            continue;
        }

        // Receive the DNS response from the external DNS server
        len = recvfrom(external_dns_sock, buf, sizeof(buf) - 1, 0, NULL, NULL);
        if (len < 0) {
            ESP_LOGE(TAG, "Unable to receive DNS response from external DNS server");
            close(external_dns_sock);
            continue;
        }

        buf[len] = '\0';

        ESP_LOGI(TAG, "Received DNS response from external DNS server: %s", buf);

        // Send the DNS response back to the client
        sendto(sock, buf, len, 0, (struct sockaddr *)&client_address, client_address_len);

        close(external_dns_sock);
    }
}

void start_dns_server() {
    xTaskCreate(dns_server_task, "dns_server_task", 4096, NULL, 5, NULL);
}