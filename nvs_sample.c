#include <stdio.h>

#include "nvs_flash.h"
#include "esp_log.h"
#include "freertos\FreeRTOS.h"
#include "freertos\task.h"
#include "driver\gpio.h"
#include "esp_chip_info.h"
#include "esp_mac.h"
#include "memory.h"
#include "esp_wifi.h"

typedef struct
{
  char ip[10];
  char addr[10];
} AP_t;

esp_chip_info_t esp_chip_info_data;

static char *level1 = "level1";
static char *level2 = "level2";
static uint8_t led_status_switch = 0;
// esp_flash_t  chip ;
uint8_t mac;
uint8_t arry_mac[6];
uint8_t *pmac;
uint8_t arry_mac1[6];
uint8_t *pmac1;
// nvs
nvs_handle_t restarCount_nvs_hdl;
nvs_handle_t bobl_nvs_hdl;

uint32_t restarCountValue = 0;

const char *my_pt = "my_nvs";

const char *pnamespace_name = "SP_restarCount"; // NVS_KEY_NAME_MAX_SIZE 16
const char *pnamespace_bobl = "SP_bobl";        // NVS_KEY_NAME_MAX_SIZE 16
const char *p_key = "restart_key";

const char *p_bobl_key = "mac_ip_addr";
uint8_t get_blob_lengh;

// bobl 复杂数据

AP_t my_ap_arry[2];

void step(void)
{
  esp_err_t nvn_init_rusule = ESP_OK;
  esp_err_t esp_err_rusule = ESP_OK;
  { // led 指示灯
    gpio_reset_pin(GPIO_NUM_2);
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_2, 0);
  }
  // nvs

  // nvn_init_rusule += nvs_flash_init();
  //  nvn_init_rusule += nvs_open(pnamespace_name,NVS_READWRITE, &restarCount_nvs_hdl);//使用nvs分区
  //  nvn_init_rusule += nvs_open(pnamespace_bobl,NVS_READWRITE, &bobl_nvs_hdl);//使用nvs分区
  // ESP_LOGI("NVS_init","NVS_init:%s\n",(nvn_init_rusule == ESP_OK)?"OK":"NO");

  ESP_ERROR_CHECK(nvs_flash_init_partition("nvs")); // ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(nvs_flash_init_partition(my_pt)); // 使用创建的分区表,1M容量的分区
  ESP_ERROR_CHECK(nvs_open_from_partition(my_pt, pnamespace_name, NVS_READWRITE, &restarCount_nvs_hdl));
  ESP_ERROR_CHECK(nvs_open_from_partition(my_pt, pnamespace_bobl, NVS_READWRITE, &bobl_nvs_hdl));

  ESP_LOGI("nvs_open", "exe:nvs_open:ok\n");

  /*
  命名空间的名称在调用 nvs_open() 或 nvs_open_from_partition 中指定，调用后将返回一个不透明句柄，用于后续调用 nvs_get_*、nvs_set_* 和 nvs_commit 函数。
  */

  // read
  nvs_get_u32(restarCount_nvs_hdl, p_key, &restarCountValue);
  // show
  ESP_LOGI("NVS_show", "restart_key:count:%lu\n", restarCountValue);

  // bolb
  memset(&my_ap_arry, 0, sizeof(my_ap_arry));
  get_blob_lengh = sizeof(my_ap_arry);
  esp_err_rusule = nvs_get_blob(bobl_nvs_hdl, p_bobl_key, (void *)my_ap_arry, (size_t *)&get_blob_lengh);
  // 打印 bobl
  ESP_LOGI("NVS_bobl", "nvs_get_blob:%s\n", (esp_err_rusule == ESP_OK) ? "OK" : "NO");

  for (int i = 0; i < 2; i++)
  {
    ESP_LOGI("NVS_bobl", "nvs_get_blob:ip:%s\n", my_ap_arry[i].ip);

    ESP_LOGI("NVS_bobl", "nvs_get_blob:addr:%s\n", my_ap_arry[i].addr);
  }
}

void loop(void)
{
  {
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

    // 模拟restart
    // write
    nvs_set_u32(restarCount_nvs_hdl, p_key, ++restarCountValue);
    nvs_commit(restarCount_nvs_hdl); // 立刻写入
  }
}

void end(void)
{
  esp_err_t nvn_end_rusule = ESP_OK;
  // show

  ESP_LOGI("NVS_show", "last,restart_key:count:%lu\n", restarCountValue);
  ESP_LOGI("NVS_bolo", "bolo last write:%lu\n", restarCountValue % 0xff);

  // 关闭nvs句柄
  nvs_close(restarCount_nvs_hdl);
  nvs_close(bobl_nvs_hdl);
  // 反初始化操作flash
  // nvn_end_rusule = nvs_flash_deinit();//nva分区
  ESP_ERROR_CHECK(nvs_flash_deinit_partition("nvs")); // nvs_flash_deinit();//"nvs" 分区
  nvn_end_rusule = nvs_flash_deinit_partition(my_pt); // my_partitions 分区
  ESP_LOGI("NVS_end", "nvs_flash_deinit:%s\n", (nvn_end_rusule == ESP_OK) ? "OK" : "NO");

 // wifi_init_config_t a = {.ampdu_rx_enable=1,.ampdu_tx_enable=0};
}

void iterator(void)
{
  // Example of listing all the key-value pairs of any type under specified partition and namespace
  nvs_iterator_t it = NULL;
  esp_err_t res = nvs_entry_find(my_pt, pnamespace_name, NVS_TYPE_ANY, &it);
  while (res == ESP_OK)
  {
    nvs_entry_info_t info;
    nvs_entry_info(it, &info);                             // Can omit error check if parameters are guaranteed to be non-NULL
    printf("key '%s', type '%d' \n", info.key, info.type); // 获取指定 分区+命名空间 的key
    res = nvs_entry_next(&it);
  }
  nvs_release_iterator(it);

  {

    // Example of nvs_get_stats() to get the number of used entries and free entries:
    nvs_stats_t nvs_stats;
    nvs_get_stats(NULL, &nvs_stats);
    printf("Count: UsedEntries = (%d), FreeEntries = (%d), AllEntries = (%d)\n",
           nvs_stats.used_entries, nvs_stats.free_entries, nvs_stats.total_entries);
    nvs_get_stats("nvs", &nvs_stats);
    printf("nvs:Count: UsedEntries = (%d), FreeEntries = (%d), AllEntries = (%d)\n",
           nvs_stats.used_entries, nvs_stats.free_entries, nvs_stats.total_entries);
    nvs_get_stats(my_pt, &nvs_stats);
    printf("my_pt:Count: UsedEntries = (%d), FreeEntries = (%d), AllEntries = (%d)\n",
           nvs_stats.used_entries, nvs_stats.free_entries, nvs_stats.total_entries);
    /*
    entry :32byte
    输出nvs分区:allEntry=756,那么24192byte = 23.6kbyte
    设置了my_nvs = 1M, 输出32256,32256*32/1024=1008kbyte  1008/1024=0.98Mbyte
    */
  }
}

void app_main(void)
{
  assert(1);

  vTaskDelay(2000 / portTICK_PERIOD_MS);

  step();

  for (int i = 0; i < 5; i++) // loop
  {
    loop();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  {
    // set bobl
    for (int i = 0; i < sizeof(my_ap_arry) / sizeof(AP_t); i++)
    {
      {
        memcpy(my_ap_arry[i].ip, "123456789", sizeof(my_ap_arry->ip));
        memcpy(my_ap_arry[i].addr, "abcdefghi", sizeof(my_ap_arry->addr));

        memset((void *)my_ap_arry[i].ip, (int)(restarCountValue % 0xff), (size_t)1);
        memset((void *)my_ap_arry[i].addr, (int)(restarCountValue % 0xff), (size_t)1);
      }
    }

    nvs_set_blob(bobl_nvs_hdl, p_bobl_key, my_ap_arry, sizeof(my_ap_arry));
    nvs_commit(bobl_nvs_hdl);
  }
  iterator();
  end();

  vTaskSuspend(NULL);
}
