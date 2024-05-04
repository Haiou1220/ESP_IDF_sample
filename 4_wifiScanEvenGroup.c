#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "esp_event.h"
#include "driver\gpio.h"
#include "freertos/event_groups.h"
#include "esp_bit_defs.h"

TaskHandle_t my_app_task_hl = NULL;

EventGroupHandle_t g_wifiEventGrpHle;
#define WIFI_EVENT_STA_START_DONE_BIT BIT0
#define WIFI_EVENT_SCAN_DONE_BIT BIT1

void wifi_scan_fun(void *par);

void task_list(void)
{
  /*Enable FreeRTOS trace facility
Enable FreeRTOS stats formatting functions*/
  char ptrTaskList[250];
  vTaskList(ptrTaskList);
  printf("*******************************************\n");
  printf("Task            State   Prio    Stack    Num\n");
  printf("*******************************************\n");
  printf(ptrTaskList);
  printf("*******************************************\n");
}

void app_task(void *par)
{
  int8_t static led_status_switch;

  ESP_LOGI("app_task", "apptask创建完成");

  // 初始化led灯
  gpio_reset_pin(GPIO_NUM_2);
  gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
  gpio_set_level(GPIO_NUM_2, 0);

  // 等待 WIFI_EVENT_STA_START_DONE_BIT 事件
  xEventGroupWaitBits(g_wifiEventGrpHle, WIFI_EVENT_STA_START_DONE_BIT, pdTRUE, pdTRUE, portMAX_DELAY);
  wifi_scan_fun(NULL); // 执行扫描fun

  printf("in app_task,分配了%d,消耗了%d, free stack size:%d\n", configMINIMAL_STACK_SIZE * 5, (configMINIMAL_STACK_SIZE * 5) - uxTaskGetStackHighWaterMark(NULL), uxTaskGetStackHighWaterMark(NULL));

  while (1)
  {
    ESP_LOGI("app_task", "app task ing");

    if (led_status_switch) // led 指示灯
    {
      // on TO OFF
      led_status_switch = 0;
      gpio_set_level(GPIO_NUM_2, 0);
    }
    else
    { // off TO ON
      led_status_switch = 1;
      gpio_set_level(GPIO_NUM_2, 1);
    }

    vTaskDelay(3000 / portTICK_PERIOD_MS);
  }
}

void run_on_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{

  printf("run_on_event_handler:event_base:%s,event_id:%ld\n", (char *)event_base, event_id);

  switch ((wifi_event_t)event_id)
  {
  case WIFI_EVENT_STA_START: // Station start, esp_wifi_start()返回完成事件:
    xEventGroupSetBits(g_wifiEventGrpHle, WIFI_EVENT_STA_START_DONE_BIT);
    break;

  case WIFI_EVENT_SCAN_DONE: // 扫描完成,
    xEventGroupSetBits(g_wifiEventGrpHle, WIFI_EVENT_SCAN_DONE_BIT);
    break;

  default:
    break;
  }
}

void wifi_scan_fun(void *par)
{
  uint16_t max_aps = 20;
  wifi_ap_record_t ap_records[max_aps];
  uint16_t aps_count = max_aps;
  uint16_t ap_num = 0;

  wifi_country_t wifi_country_config = {
      .cc = "CN",
      .schan = 1,
      .nchan = 13,
  };
  ESP_ERROR_CHECK(esp_wifi_set_country(&wifi_country_config));

  ESP_LOGI("WIFI", "4. Wi-Fi 启动扫描");
  ESP_ERROR_CHECK(esp_wifi_scan_start(NULL, false)); // false 非阻塞

  // 等待 WIFI_EVENT_SCAN_DONE 事件
  xEventGroupWaitBits(g_wifiEventGrpHle, WIFI_EVENT_SCAN_DONE_BIT, pdTRUE, pdTRUE, portMAX_DELAY);

  // 打印wifi扫描结果
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_num));
  ESP_LOGI("WIFI", "扫描结束,可用AP Count : %d", ap_num);

  memset(ap_records, 0, sizeof(ap_records));
  ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&aps_count, ap_records));
  ESP_LOGI("WIFI", "AP Count: %d", aps_count);
  printf("%30s %s %s %s\n", "SSID", "频道", "强度", "MAC地址");

  for (int i = 0; i < aps_count; i++)
  {
    printf("%30s  %3d  %3d  %02X-%02X-%02X-%02X-%02X-%02X\n", ap_records[i].ssid, ap_records[i].primary, ap_records[i].rssi, ap_records[i].bssid[0], ap_records[i].bssid[1], ap_records[i].bssid[2], ap_records[i].bssid[3], ap_records[i].bssid[4], ap_records[i].bssid[5]);
  }
}

void app_main(void)
{
  // 创建 wifi事件组 //队列事件等信号要提前刺激,避免任务等待事件句柄是空指针
  g_wifiEventGrpHle = xEventGroupCreate();
  // 创建app task;
  printf("in main_task free stack size:%d\n", uxTaskGetStackHighWaterMark(NULL));
  xTaskCreate(app_task, "myAppTask", configMINIMAL_STACK_SIZE * 5, NULL, 2, NULL); // 历程中堆栈深度是1024*12

  ESP_LOGI("WIFI", "0. 初始化NVS存储");
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_LOGI("WIFI", "1. Wi-Fi 初始化阶段");
  ESP_ERROR_CHECK(esp_netif_init());                ////tcpip task create :tiT
  ESP_ERROR_CHECK(esp_event_loop_create_default()); // event task  :sys_evt  esp_event_loop_create();
  esp_netif_create_default_wifi_sta();              // tcpip bind sta

  wifi_init_config_t wifi_config = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&wifi_config)); // wifi task create : wifi

  /*-----------------2. Wi-Fi 初始化阶段------------------------*/
  ESP_LOGI("WIFI", "2. Wi-Fi 初始化阶段");
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

  // 配置事件循环 注册
  // esp_event_handler_instance_register_with();
  esp_event_handler_register(ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, run_on_event_handler, NULL);

  ESP_LOGI("WIFI", "3. Wi-Fi 启动阶段");
  ESP_ERROR_CHECK(esp_wifi_start());

  printf("in main_task ,分配了%d,消耗了:%d,free stack size:%d\n", CONFIG_ESP_MAIN_TASK_STACK_SIZE, CONFIG_ESP_MAIN_TASK_STACK_SIZE - uxTaskGetStackHighWaterMark(NULL), uxTaskGetStackHighWaterMark(NULL));

  while (1)
  {
    ESP_LOGI("main_task", "main task ing");
    vTaskDelay(3000 / portTICK_PERIOD_MS);
  }
}