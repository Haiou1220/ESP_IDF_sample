
#include "nvs_flash.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "string.h"
#include "freertos\task.h"
#include "esp_mac.h"
#include "esp_event.h"
#include "freertos\event_groups.h"
#include "esp_netif_ip_addr.h"
#include "lwip\sockets.h"
#include "stdio.h"


#define ESP_WIFI_AP_SSID "Esp32C3"
#define ESP_WIFI__AP_PASS "12345678"
#define ESP_WIFI_STA_SSID "CMCC-EYUP"
#define ESP_WIFI__STA_PASS "3bykmt2u"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
static const char *TAG = "HANDLER";
EventGroupHandle_t g_WIFI_EventHandle;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    static uint8_t connectTryCount;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG, "esp_wifi_connect() ing");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (connectTryCount < 5)
        {
            ESP_LOGI(TAG, "esp_wifi_connect() ing:%u", connectTryCount + 1);
            esp_wifi_connect();
            connectTryCount++;
        }
        else
        {
            xEventGroupSetBits(g_WIFI_EventHandle, WIFI_FAIL_BIT);
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event_Ip_Msg = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "已成功连接到AP,获取到的的IP地址为:" IPSTR "", IP2STR(&event_Ip_Msg->ip_info.ip));
        ESP_LOGI(TAG, "网关的IP地址为:" IPSTR "", IP2STR(&event_Ip_Msg->ip_info.gw));
        xEventGroupSetBits(g_WIFI_EventHandle, WIFI_CONNECTED_BIT);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED)
    {
        wifi_event_sta_connected_t *event_wifi_Msg = (wifi_event_sta_connected_t *)event_data;
        ESP_LOGI(TAG, "STA已成功连接到AP :");
    }
}

int wifi_sta_init(void)
{
    int ret = -1;
    esp_netif_t *p_esp_netif = NULL;

    ESP_LOGI("loop", "创建事件实例的回调函数");
    /*监听 wifi的所有事件*/
    /*WIFI_EVENT_STA_CONNECTED ,WIFI_EVENT_AP_START */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    /*监听 sta被成功分配ip的事件*/
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));
    // esp_event_post_to();
    // ip_event_ap_staipassigned_t a;
    /* WIFI_EVENT_AP_START_T */
    // esp_event_handler_register_with(esp_event_loop_handle_t event_loop,
    //  esp_event_base_t event_base, int32_t event_id,
    //   esp_event_handler_t event_handler, void *event_handler_arg)

    ESP_LOGI("netif", "netif初始化");
    ESP_ERROR_CHECK(esp_netif_init()); // task add

    ESP_LOGI("netif", "netif配置wif sta");
    p_esp_netif = esp_netif_create_default_wifi_sta();
    assert(p_esp_netif);

    ESP_LOGI("netif", "sta 停止dhcp 客户端");
    ESP_ERROR_CHECK(esp_netif_dhcpc_stop(p_esp_netif));

    ESP_LOGI("netif", "分配 dhcp");
    esp_netif_ip_info_t ip_info;
    ip_info.ip.addr = ipaddr_addr("192.168.1.11");
    ip_info.netmask.addr = ipaddr_addr("255.255.255.0");
    ip_info.gw.addr = ipaddr_addr("192.168.1.1");
    esp_netif_set_ip_info(p_esp_netif, &ip_info);

    ESP_LOGI("wifi", "wifi初始化");
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&config)); // task add

    esp_wifi_set_ps(WIFI_PS_MIN_MODEM); // power save 配置

    ESP_LOGI("wifi", "wifi_模式为 sta");
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    ESP_LOGI("wifi", "sta 参数设置");
    wifi_config_t wifi_config_sta = {
        .sta = {
            .ssid = ESP_WIFI_STA_SSID,
            .password = ESP_WIFI__STA_PASS,

        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config_sta));

    ESP_LOGI("wifi", "启动wifi");
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI("wifi", "等待事件:STA_START, 再连接wifi");

    { // 等待事件组 WIFI_CONNECTED_BIT | WIFI_FAIL_BIT
        EventBits_t event_Bits = xEventGroupWaitBits(g_WIFI_EventHandle, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT, pdTRUE, pdFALSE, portMAX_DELAY);
        if (event_Bits & WIFI_FAIL_BIT)
        {
            ESP_LOGI(TAG, "WIFI连接失败\n");
            // 错误处理
            ret = -1;
        }
        else if (event_Bits & WIFI_CONNECTED_BIT)
        {
            ESP_LOGI(TAG, "WIFI连接成功");
            ret = 0;
        }
    }
    return ret;
}

void app_main(void)
{

    /*1.创建事件组*/
    g_WIFI_EventHandle = xEventGroupCreate();
    assert(g_WIFI_EventHandle);

    ESP_LOGI("nvs", "nvs初始化,程序第一步要求初始化,因为涉及到wifi的存储要求");
    ESP_ERROR_CHECK(nvs_flash_init());

    ESP_LOGI("loop", "创建event_loop默认事件循环");
    ESP_ERROR_CHECK(esp_event_loop_create_default()); // task add

    if (ESP_OK == wifi_sta_init())
    {
    }
    else
    {
        return;
    }

    // tcp 客户端配置
    /*
    客户端流程
    1-socket对象
    2-连接(内部完成绑定)
    3-数据收发
    4-关闭

    服务器流程
    1-socket对象
    2-绑定监听端口
    3-监听
    4-循环等待 accept
    5-收发数据
    6-异常处理
    7-关闭

    */
    // tcp 客户端配置

    int socket_obj, connect_ret;
    char send_str[20];
    char recv_str[50];
    static u32_t count;

    ESP_LOGI("tcp", "创建socket对象");
    socket_obj = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ESP_ERROR_CHECK((-1 == socket_obj) ? ESP_FAIL : ESP_OK);

    ESP_LOGI("tcp", "套接字绑定");

    struct sockaddr_in i_sockaddr_in;
    i_sockaddr_in.sin_family = AF_INET;
    i_sockaddr_in.sin_port = htons(30000);
    i_sockaddr_in.sin_addr.s_addr = inet_addr("192.168.1.3");

    for (int i = 0; i < 5; i++)
    {
        connect_ret = lwip_connect(socket_obj, (const struct sockaddr *)&i_sockaddr_in, sizeof(i_sockaddr_in));
        if (-1 == connect_ret)
        {
            ESP_LOGI("tcp", "lwip_connect nok:%d\n", i + 1);
            fflush(stdout);           
        }
        else
        {
            ESP_LOGI("tcp", "lwip_connect ok");
            break;
        }
    }
    if (-1 == connect_ret)
    {
        vTaskDelay(pdMS_TO_TICKS(3000));
        ESP_ERROR_CHECK((-1 == connect_ret) ? ESP_FAIL : ESP_OK);
    }

    while (1)
    {
        memset(send_str, 0, sizeof(send_str));
        sprintf(send_str, "hello world:%lu", count++);
        lwip_send(socket_obj, send_str, strlen(send_str) + 1, 0); // lwip_sendmsg();

        memset(recv_str, 0, sizeof(recv_str));
        ssize_t recv_len = lwip_recv(socket_obj, recv_str, sizeof(recv_str), 0); // 阻塞
        ESP_ERROR_CHECK((-1 == recv_len) ? ESP_FAIL : ESP_OK);
        if (recv_len > 0)
        {
            ESP_LOGI("tcp", "收到服务器的消息:%s", recv_str);
        }
        else if (0 == recv_len)
        {
            ESP_LOGI("tcp", "服务器下线了!");
            return;
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}