
#include "nvs_flash.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "string.h"
#include "freertos\task.h"
#include "esp_mac.h"
#include "esp_event.h"

#define ESP_WIFI_SSID "Esp32C3"
#define ESP_WIFI_PASS "12345678"
static const char *TAG = "HANDLER";

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{ // event_data 数据是event loop 传入,可以根据event id 类型解析 event_data
    if (event_base == WIFI_EVENT  && event_id == WIFI_EVENT_AP_STACONNECTED )
    {    
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_AP_STAIPASSIGNED)
    {
        ip_event_ap_staipassigned_t *event = (ip_event_ap_staipassigned_t *)event_data;
        ESP_LOGI(TAG, "ip:" IPSTR "", IP2STR(&event->ip));
    }
}

void app_main(void)
{
    esp_netif_t *p_esp_netif = NULL;

    ESP_LOGI("nvs", "nvs初始化");
    ESP_ERROR_CHECK(nvs_flash_init());
    {
        ESP_LOGI("loop", "event_loop初始化");
        ESP_ERROR_CHECK(esp_event_loop_create_default()); // task add

        ESP_LOGI("loop", "创建事件实例的回调函数");
        /*监听 wifi的所有事件*/
        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                            ESP_EVENT_ANY_ID /* WIFI_EVENT_AP_START */,
                                                            &wifi_event_handler,
                                                            NULL,
                                                            NULL));

        /*监听 sta被成功分配ip的事件*/
        ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                            IP_EVENT_AP_STAIPASSIGNED,
                                                            &wifi_event_handler,
                                                            NULL,
                                                            NULL));
        // esp_event_post_to();
        ip_event_ap_staipassigned_t a;
        /* WIFI_EVENT_AP_START_T */
        // esp_event_handler_register_with(esp_event_loop_handle_t event_loop,
        //  esp_event_base_t event_base, int32_t event_id,
        //   esp_event_handler_t event_handler, void *event_handler_arg)
    }
    ESP_LOGI("netif", "netif初始化");
    ESP_ERROR_CHECK(esp_netif_init()); // task add

    ESP_LOGI("netif", "netif配置wif ap");
    p_esp_netif = esp_netif_create_default_wifi_ap();
    assert(p_esp_netif);

    ESP_LOGI("wifi", "wifi初始化");
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&config)); // task add

    ESP_LOGI("wifi", "wifi_模式为ap");
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

    ESP_LOGI("wifi", "ap参数设置");
    wifi_config_t wifi_config_ap = {
        .ap = {
            .ssid = "ESP32_C3",
            .ssid_len = strlen("ESP32_C3"),
            .channel = 1,
            .password = "123456789",
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .max_connection = 4,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config_ap));

    ESP_LOGI("wifi", "启动wifi");
    ESP_ERROR_CHECK(esp_wifi_start());

    while (1)
    {

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}