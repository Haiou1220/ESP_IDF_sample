#include <stdio.h>
 
#include "nvs_flash.h"
#include "esp_log.h"
#include "freertos\FreeRTOS.h"
#include "freertos\task.h"
#include "driver\gpio.h"
#include "esp_chip_info.h"
#include "esp_mac.h"
esp_chip_info_t esp_chip_info_data;
char list_str[40*10];
static char* level1 = "level1";
static char* level2 = "level2";
static uint8_t led_status_switch = 0;
//esp_flash_t  chip ;
uint8_t mac;
uint8_t arry_mac[6];
uint8_t *pmac;

uint8_t arry_mac1[6];
uint8_t *pmac1;


void app_main(void)
{
  esp_err_t  esp_err_get_mac,esp_err_get_mac1;
    pmac = arry_mac;
    nvs_flash_init();
    vTaskList(list_str);
    
    esp_log_level_set(level1,ESP_LOG_VERBOSE);   //esp_log_level_set(level2,ESP_LOG_DEBUG);
        //esp_log_level_set("交通灯",ESP_LOG_DEBUG);
    esp_log_level_set("conter",ESP_LOG_ERROR); 
         
         
    {
        gpio_reset_pin(GPIO_NUM_2);
        gpio_set_direction(GPIO_NUM_2,GPIO_MODE_OUTPUT);
         gpio_set_level(GPIO_NUM_2, 0);

               gpio_reset_pin(GPIO_NUM_22);
        gpio_set_direction(GPIO_NUM_22,GPIO_MODE_OUTPUT);
         gpio_set_level(GPIO_NUM_22, 0);

               gpio_reset_pin(GPIO_NUM_23);
        gpio_set_direction(GPIO_NUM_23,GPIO_MODE_OUTPUT);
         gpio_set_level(GPIO_NUM_23, 0);
    }

esp_err_t esp_flash_write(esp_flash_t *chip, const void *buffer, uint32_t address, uint32_t length);

     ESP_LOGI("任务信息","\n任务\t 状态\t 优先级\t 栈容量\t \n%s\n",list_str);

     printf("任务信息,log_level:%d\n",esp_log_level_get("任务信息"));//ESP_LOG_INFO 3
     printf("level1,log_level:%d\n",esp_log_level_get(level1));//ESP_LOG_DEBUG 4
        printf("conter,log_level:%d\n",esp_log_level_get("conter")); //conter

    for(int i = 0;i < 5;i++)
    {
    ESP_LOGE("交通灯","交通灯%d = NO",i);
    ESP_LOGE("conter","1tick=%lu ms  ",portTICK_PERIOD_MS);

    ESP_LOGV(level1,"this debug1_information \n");
    ESP_LOGE(level2,"this DEBUG1_information \n");

 

 if(led_status_switch)
 {
    //on TO OFF
    led_status_switch = 0;
  gpio_set_level(GPIO_NUM_2, 0);
   gpio_set_level(GPIO_NUM_22, 0);
     gpio_set_level(GPIO_NUM_23, 0);
 }else{ //off TO ON
    led_status_switch = 1;
      gpio_set_level(GPIO_NUM_2, 1);
  gpio_set_level(GPIO_NUM_22, 1);
gpio_set_level(GPIO_NUM_23, 1);
 }


      ESP_LOGI(level2,"this esp_cpu_get_core_id号:%d \n", esp_cpu_get_core_id());  
   ESP_LOGI(level2,"esp_get_idf_version:%s \n", esp_get_idf_version());  

   
// void esp_chip_info(esp_chip_info_t *out_info)
// /* Chip feature flags, used in esp_chip_info_t */
// #define CHIP_FEATURE_EMB_FLASH      BIT(0)      //!< Chip has embedded flash memory
// #define CHIP_FEATURE_WIFI_BGN       BIT(1)      //!< Chip has 2.4GHz WiFi
// #define CHIP_FEATURE_BLE            BIT(4)      //!< Chip has Bluetooth LE
// #define CHIP_FEATURE_BT             BIT(5)      //!< Chip has Bluetooth Classic
// #define CHIP_FEATURE_IEEE802154     BIT(6)      //!< Chip has IEEE 802.15.4
// #define CHIP_FEATURE_EMB_PSRAM      BIT(7)      //!< Chip has embedded psram
  esp_chip_info(&esp_chip_info_data);
   ESP_LOGI(level2,"esp_chip_info:%#lX \n", esp_chip_info_data.features);  
 
  esp_err_get_mac = esp_efuse_mac_get_default( pmac);
   ESP_LOGI(level2,"esp_efuse_mac_get_default:%s:%02d:%02d:%02d:%02d:%02d:%02d \n", (ESP_OK == esp_err_get_mac)?"OK":"NO",*pmac,*(pmac+1),*(pmac+2),*(pmac+3),*(pmac+4),*(pmac+5));
   ESP_LOGI(level2,"esp_efuse_mac_get_default:%s:%02d:%02d:%02d:%02d:%02d:%02d \n", (ESP_OK == esp_err_get_mac)?"OK":"NO",pmac[0],pmac[1],pmac[2],pmac[3],pmac[4],pmac[5]);


  esp_err_get_mac1 = esp_efuse_mac_get_default( arry_mac1);
   ESP_LOGI(level2,"esp_efuse_mac_get_default1:%s:%02d:%02d:%02d:%02d:%02d:%02d \n", (ESP_OK == esp_err_get_mac1)?"OK":"NO",arry_mac1[0],arry_mac1[1],arry_mac1[2],arry_mac1[3],arry_mac1[4],arry_mac1[5]);
   ESP_LOGI(level2,"esp_efuse_mac_get_default1:%s:%02d:%02d:%02d:%02d:%02d:%02d \n", (ESP_OK == esp_err_get_mac1)?"OK":"NO",*arry_mac1,*(arry_mac1+1),*(arry_mac1+2),*(arry_mac1+3),*(arry_mac1+4),*(arry_mac1+5));

    vTaskDelay(1000/portTICK_PERIOD_MS);

    }
   
     //xTaskCreate
    vTaskSuspend(NULL);
}

