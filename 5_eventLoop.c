#if 1
#include <stdio.h>
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include <string.h>

esp_event_loop_handle_t g_event_loop;
TaskHandle_t g_event_source_task_hdl;
TaskHandle_t g_application_task_hdl;

ESP_EVENT_DEFINE_BASE(MY_EVENT_BASE);

typedef enum
{
    EVENT_ID_0 = 0,
    EVENT_ID_1,
    EVENT_ID_2,
    EVENT_ID_3,
    EVENT_ID_4,
    EVENT_ID_5,
} my_event_id_t;

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

void my_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ESP_LOGE("my_event_handler", "%s收到:BASE:%s,ID:%ld,msg:%s\n",(char*)event_handler_arg, (char *)event_base, event_id, (char *)event_data);
}

void event_source_task(void *par)
{
    static my_event_id_t my_event_id = EVENT_ID_0;
    char str0[20];

    ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
    ESP_LOGE("MAIN", "进入 event_source_task() ");


    while (1)
    {   
        memset(str0,0,sizeof(str0));
        sprintf (str0,"hello,Processor_%d",my_event_id);

        /* 字符串长度包含结尾 strlen(str)+1 */
        esp_event_post_to(g_event_loop, MY_EVENT_BASE, (int32_t)my_event_id,
                          (void *)str0, strlen(str0) + 1, portMAX_DELAY);
        ESP_LOGE("event_source_task", "post id:%d", my_event_id);

        my_event_id++;
        if (my_event_id > EVENT_ID_5)
        {
            my_event_id = EVENT_ID_0;
        }

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void application_task(void *par)
{
    ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
    ESP_LOGE("MAIN", "进入 application_task() ");

    ESP_ERROR_CHECK(esp_event_handler_register_with(g_event_loop, MY_EVENT_BASE, (int32_t )EVENT_ID_0, my_event_handler,
                                                    "Iam a Processor0"));
    ESP_ERROR_CHECK(esp_event_handler_register_with(g_event_loop, MY_EVENT_BASE, (int32_t )EVENT_ID_5, my_event_handler,
                                                    "Iam a Processor5"));
    ESP_LOGE("APP_TASK", "绑定 事件环 处理函数");

    ESP_LOGE("MAIN", "通知 event_source_task 运行");
    xTaskNotifyGive(g_event_source_task_hdl);

    while (1)
    {

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void)
{

    ESP_LOGE("MAIN", "创建 Event Loop");

    esp_event_loop_args_t event_loop_args = {
        .queue_size = 10,
        .task_name = "myEvtLpTsk",
        .task_priority = uxTaskPriorityGet(NULL) + 1,
        .task_stack_size = configMINIMAL_STACK_SIZE * 4,
        .task_core_id = tskNO_AFFINITY,
    };

    ESP_ERROR_CHECK(esp_event_loop_create(&event_loop_args, &g_event_loop));

    ESP_LOGE("MAIN", "创建 任务 Event Loop source task");
    xTaskCreate(event_source_task, "event_source_task", configMINIMAL_STACK_SIZE * 8, NULL, uxTaskPriorityGet(NULL), &g_event_source_task_hdl);
    xTaskCreate(application_task, "application_task", configMINIMAL_STACK_SIZE * 8, NULL, uxTaskPriorityGet(NULL), &g_application_task_hdl);

    ESP_LOGE("MAIN", "通知 application_task 运行");
    xTaskNotifyGive(g_application_task_hdl);

    ESP_LOGE("MAIN", "删除任务 app_main()");
    // vTaskDelete(NULL);
    vTaskDelay(pdMS_TO_TICKS(5000));
    task_list();

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

#endif

#if 0 
/* esp_event (event loop library) basic example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//#include "event_source.h"
#include "esp_event_base.h"
#include "esp_event.h"
#include "esp_timer.h"


// Declarations for the event source
#define TASK_ITERATIONS_COUNT 10 // number of times the task iterates
#define TASK_PERIOD 1000         // period of the task loop in milliseconds
ESP_EVENT_DECLARE_BASE(TASK_EVENTS);         // declaration of the task events family
enum {
    TASK_ITERATION_EVENT                     // raised during an iteration of the loop within the task
};



static const char* TAG = "user_event_loops";

// Event loops
esp_event_loop_handle_t loop_with_task;
esp_event_loop_handle_t loop_without_task;

static void application_task(void* args)
{
    // Wait to be started by the main task
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    while(1) {
        ESP_LOGI(TAG, "application_task: running application task");
         esp_event_loop_run(loop_without_task, 100);
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

/* Event source task related definitions */
ESP_EVENT_DEFINE_BASE(TASK_EVENTS);

TaskHandle_t g_task;

static void task_iteration_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    // Two types of data can be passed in to the event handler: the handler specific data and the event-specific data.
    //
    // The handler specific data (handler_args) is a pointer to the original data, therefore, the user should ensure that
    // the memory location it points to is still valid when the handler executes.
    //
    // The event-specific data (event_data) is a pointer to a deep copy of the original data, and is managed automatically.
    int iteration = *((int*) event_data);

    char* loop;

    if (handler_args == loop_with_task) {
        loop = "loop_with_task";
    } else {
        loop = "loop_without_task";
    }

    ESP_LOGI(TAG, "handling %s:%s from %s, iteration %d", base, "TASK_ITERATION_EVENT", loop, iteration);
}

static void task_event_source(void* args)
{
    // Wait to be started by the main task
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    for(int iteration = 1; iteration <= TASK_ITERATIONS_COUNT; iteration++) {
        esp_event_loop_handle_t loop_to_post_to;

        if (iteration % 2 == 0) {
            // if even, post to the event loop with dedicated task
            loop_to_post_to = loop_with_task;
        } else {
            // if odd, post to the event loop without a dedicated task
            loop_to_post_to = loop_without_task;
        }

        ESP_LOGI(TAG, "posting %s:%s to %s, iteration %d out of %d", TASK_EVENTS, "TASK_ITERATION_EVENT",
                loop_to_post_to == loop_with_task ? "loop_with_task" : "loop_without_task",
                iteration, TASK_ITERATIONS_COUNT);

        ESP_ERROR_CHECK(esp_event_post_to(loop_to_post_to, TASK_EVENTS, TASK_ITERATION_EVENT, &iteration, sizeof(iteration),\
        (loop_to_post_to == loop_with_task ? pdMS_TO_TICKS(200) : pdMS_TO_TICKS(200))));

        vTaskDelay(pdMS_TO_TICKS(500));
    }

    vTaskDelay(pdMS_TO_TICKS(TASK_PERIOD));

    ESP_LOGI(TAG, "deleting task event source");

    vTaskDelete(NULL);
}

/* Example main */
void app_main(void)
{
    ESP_LOGI(TAG, "setting up");

    esp_event_loop_args_t loop_with_task_args = {
        .queue_size = 10,
        .task_name = "loop_task", // task will be created
        .task_priority = uxTaskPriorityGet(NULL),
        .task_stack_size = 3072,
        .task_core_id = tskNO_AFFINITY
    };

    esp_event_loop_args_t loop_without_task_args = {
        .queue_size = 10,
        .task_name = NULL // no task will be created
    };

    // Create the event loops
    ESP_ERROR_CHECK(esp_event_loop_create(&loop_with_task_args, &loop_with_task));
    ESP_ERROR_CHECK(esp_event_loop_create(&loop_without_task_args, &loop_without_task));

    // Register the handler for task iteration event. Notice that the same handler is used for handling event on different loops.
    // The loop handle is provided as an argument in order for this example to display the loop the handler is being run on.
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(loop_with_task, TASK_EVENTS, TASK_ITERATION_EVENT, task_iteration_handler, loop_with_task, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register_with(loop_without_task, TASK_EVENTS, TASK_ITERATION_EVENT, task_iteration_handler, loop_without_task, NULL));

    // Create the event source task
    TaskHandle_t task_event_source_hdl;
    ESP_LOGI(TAG, "starting event source");
    xTaskCreate(task_event_source, "task_event_source", 3072, NULL, uxTaskPriorityGet(NULL) + 1, &task_event_source_hdl);

    // Create the application task
    TaskHandle_t application_task_hdl;
    ESP_LOGI(TAG, "starting application task");
    xTaskCreate(application_task, "application_task", 3072, NULL, uxTaskPriorityGet(NULL) + 1, &application_task_hdl);

    // Start the event source task first to post an event
    xTaskNotifyGive(task_event_source_hdl);

    // Start the application task to run the event handlers
    xTaskNotifyGive(application_task_hdl);
}
#endif