#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp32/rom/ets_sys.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "sdkconfig.h"
#include "mqtt_client.h"

#include "queue.h"
#include "randomdata.h"
#include "uart_lib.h"

static const char *TAG = "Gateway-station";

#define WIFI_SSID CONFIG_WIFI_SSID
#define WIFI_PASS CONFIG_WIFI_PASS

#define MQTT_BROKER_URL CONFIG_MQTT_BROKER_URL
#define MQTT_BROKER_USER CONFIG_MQTT_BROKER_USER
#define MQTT_BROKER_PASS CONFIG_MQTT_BROKER_PASS

#define MQTT_PUB_DEV1_TEMP CONFIG_MQTT_PUB_DEV1_TEMP
#define MQTT_PUB_DEV1_DO CONFIG_MQTT_PUB_DEV1_DO
#define MQTT_PUB_DEV1_PH CONFIG_MQTT_PUB_DEV1_PH
#define MQTT_PUB_DEV1_EC CONFIG_MQTT_PUB_DEV1_EC
#define MQTT_PUB_DEV2 CONFIG_MQTT_PUB_DEV2

#define M0 12
#define M1 13
#define MY_UART_NUM_1 UART_NUM_0
#define TXD_PIN_1 (GPIO_NUM_1)
#define RXD_PIN_1 (GPIO_NUM_3)

#define MY_UART_NUM_2 UART_NUM_2
#define TXD_PIN_2 (GPIO_NUM_17)
#define RXD_PIN_2 (GPIO_NUM_16)

Queue_ QueueMQTT;
SensorData sensor_data;

static esp_mqtt_client_handle_t mqtt_client;

/* esp netif object representing the WIFI station */
static esp_netif_t *sta_netif = NULL;

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t event_group_bits;

#define WIFI_CONNECTED_BIT BIT0
#define MQTT_CONNECTED_BIT BIT1
#define UART_CONNECTED_BIT BIT2

/**
 * WIFI and IP event handler using esp_event API.
 */
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "wifi disconnected.");
        esp_wifi_connect();
        xEventGroupClearBits(event_group_bits, WIFI_CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI(TAG, "wifi connected.");
        xEventGroupSetBits(event_group_bits, WIFI_CONNECTED_BIT);
    }
}

/**
 * MQTT event handler using esp_event API.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            xEventGroupSetBits(event_group_bits, MQTT_CONNECTED_BIT);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            xEventGroupClearBits(event_group_bits, MQTT_CONNECTED_BIT);
            xEventGroupWaitBits( // Wait for WIFI to reconnect
                event_group_bits,
                WIFI_CONNECTED_BIT,
                pdFALSE,
                pdFALSE,
                portMAX_DELAY
            );
            vTaskDelay( 2000 / portTICK_RATE_MS);
            ESP_ERROR_CHECK(esp_mqtt_client_reconnect(client));
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
            }
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

/**
 * WIFI initialization
 */
void wifi_init(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
}

/**
 * MQTT initialization
 */
static void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = MQTT_BROKER_URL,
	    .username = "admin",
        .password = "123"
    };
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, mqtt_client);
    esp_mqtt_client_start(mqtt_client);
}


/**
 * UART Init
*/
void init_uart()
{
    uart_config_t uart_config_1 = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    uart_config_t uart_config_2 = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };

    uart_param_config(MY_UART_NUM_1, &uart_config_1);
    uart_set_pin(MY_UART_NUM_1, TXD_PIN_1, RXD_PIN_1, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(MY_UART_NUM_1, 256, 0, 0, NULL, 0);

    uart_param_config(MY_UART_NUM_2, &uart_config_2);
    uart_set_pin(MY_UART_NUM_2, TXD_PIN_2, RXD_PIN_2, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(MY_UART_NUM_2, 256, 0, 0, NULL, 0);
}

/**
 * Task for receive Lora Data and enqueue to MQTT queue
*/

void LORA_task(void *pvParameter){
    sensor_data.id =0;
    sensor_data.pbgt =1;
    sensor_data.lenth = 16; 
    sensor_data.year = 2023;
    sensor_data.mon = 9;
    sensor_data.mday = 16;
    sensor_data.hour = 12;
    sensor_data.minz = 30;
    sensor_data.sec = 23;
    sensor_data.ec = 23.43;
    sensor_data.od = 1222.3;
    sensor_data.ph = 7.54;
    sensor_data.teamp = 30.21;
    char hex_data[101];   // 97 =  12 biến * 8(bytes) + 1 NULL

    encodeDataToHex( sensor_data.id, sensor_data.pbgt, sensor_data.lenth, sensor_data.ec, sensor_data.od, sensor_data.ph, sensor_data.teamp,sensor_data.year, sensor_data.mon,sensor_data.mday, sensor_data.hour, sensor_data.minz, sensor_data.sec , hex_data);

    while(1){

    uint8_t data_[128];

    int length_1 = uart_read_bytes(MY_UART_NUM_1, data_, sizeof(data_), 20 / portTICK_PERIOD_MS);
        if (length_1 > 0)
        {
            uart_write_bytes(MY_UART_NUM_2,  hex_data, strlen(hex_data) );
        }

    int length = uart_read_bytes(MY_UART_NUM_2,  data_, sizeof(data_), 20 / portTICK_PERIOD_MS );
        if (length > 0)
        {   
            xEventGroupSetBits(event_group_bits, UART_CONNECTED_BIT);
        } else {
            xEventGroupClearBits(event_group_bits, UART_CONNECTED_BIT);
        }

    xEventGroupWaitBits( // Wait for UART to reconnect
                event_group_bits,
                UART_CONNECTED_BIT,
                pdFALSE,
                pdFALSE,
                portMAX_DELAY
            );

    ESP_LOGI(TAG, "Init Lora Queue");
    Init(&QueueMQTT);
     /*
        nhận data rồi enqueue vào Queue MQTT
     */
    raw_package XData1;
    memcpy(XData1.data_raw, data_, sizeof(data_));
    XData1.data_len=length;

    enqueue(&QueueMQTT, XData1);
    ESP_LOGI(TAG, "Data enqueue successful");
    }

    vTaskDelay( 20 / portTICK_RATE_MS );
}

/**
 * FreeRTOS task for gettin DHT22 sensor readings and
 * sending them to MQTT broker
 */
void MQTT_task(void *pvParameter)
{   
    ESP_LOGI(TAG, "Starting MQTT Task");
    ESP_LOGI(TAG, "Waiting wifi for send MQTT ...\n");
    
    ESP_ERROR_CHECK(esp_wifi_start() );
    ESP_LOGI(TAG, "wifi_init_sta finished.");

    while(1) {  
        xEventGroupWaitBits( // Wait for MQTT connection
            event_group_bits,
            MQTT_CONNECTED_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY
        );
        
        ESP_LOGI(TAG, "Reading Data\n");

        raw_package temp = dequeue(&QueueMQTT);

        decodeHexToData((const char *)temp.data_raw, temp.data_len, &sensor_data);

        errorHandler(sensor_data.lenth);
    
        if (sensor_data.lenth){

            char tem[12];
            char DO[12];
            char pH[12];
            char EC[12];

            sprintf(tem, "%.2f", sensor_data.teamp);
            sprintf(DO, "%.2f", sensor_data.od);
            sprintf(pH, "%.2f", sensor_data.ph);
            sprintf(EC, "%.2f", sensor_data.ec);

            switch(sensor_data.pbgt) {
                case 1:
                    // ESP_LOGI(TAG, "Time: %s%s%s %s:%s:%s", sensor_data.mday, sensor_data.mon, sensor_data.year, sensor_data.hour, sensor_data.minz, sensor_data.sec);
                    ESP_LOGI(TAG, "Temp1 %s", tem);
                    ESP_LOGI(TAG, "DO %s", DO);
                    ESP_LOGI(TAG, "pH %s", pH);
                    ESP_LOGI(TAG, "EC %s", EC);

                    esp_mqtt_client_publish(mqtt_client, MQTT_PUB_DEV1_TEMP, tem, 0, 0, 0);
                    ESP_LOGI(TAG, "sent publish successful temp");
                    esp_mqtt_client_publish(mqtt_client, MQTT_PUB_DEV1_DO, DO, 0, 0, 0);
                    ESP_LOGI(TAG, "sent publish successful DO");
					esp_mqtt_client_publish(mqtt_client, MQTT_PUB_DEV1_PH, pH, 0, 0, 0);
					ESP_LOGI(TAG, "sent publish successful pH");
					esp_mqtt_client_publish(mqtt_client, MQTT_PUB_DEV1_EC, EC, 0, 0, 0);
					ESP_LOGI(TAG, "sent publish successful EC\n");

                    break;

                case 2:
                    // ESP_LOGI(TAG, "Time: %s%s%s %s:%s:%s", sensor_data.mday, sensor_data.mon, sensor_data.year, sensor_data.hour, sensor_data.minz, sensor_data.sec);
                    ESP_LOGI(TAG, "Temp2 %s", tem);

                    int msg_id2 = esp_mqtt_client_publish(mqtt_client, MQTT_PUB_DEV2, tem, 0, 0, 0);
                    ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id2);

                    break;
                default :
                    ESP_LOGE( TAG, "Unknown error\n" );
            }
        } else {
            esp_mqtt_client_disconnect(mqtt_client);
            esp_wifi_stop();
        }
        // -- wait at least 2 sec before reading again ------------
        // The interval of whole process must be beyond 2 seconds !! 
        vTaskDelay( 5000 / portTICK_RATE_MS );
    }
}

/**
 * Main function
 */
void app_main()
{
    nvs_flash_init();
    vTaskDelay( 1000 / portTICK_RATE_MS );
    event_group_bits = xEventGroupCreate();
    init_uart();
    gpio_set_direction(M0, GPIO_MODE_OUTPUT);
    gpio_set_direction(M1, GPIO_MODE_OUTPUT);
    gpio_set_level(M0, 0);
    gpio_set_level(M1, 0);
    wifi_init();
    mqtt_app_start();
    xTaskCreate( &LORA_task, "LORA_task", 2048, NULL, 1, NULL);
    xTaskCreate( &MQTT_task, "MQTT_task", 1024, NULL, 2, NULL);
}
