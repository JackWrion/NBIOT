/* UART asynchronous example, that uses separate RX and TX tasks

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "string.h"
#include "driver/gpio.h"

static const int RX_BUF_SIZE = 1024;
int count = -3;

#define TXD_PIN (GPIO_NUM_17)				// TODO config UART here
#define RXD_PIN (GPIO_NUM_16)
#define POWER GPIO_NUM_4


// TODO init power
static void power_init(void){
	gpio_config_t io_conf;
	io_conf.pin_bit_mask = 1<< POWER;							//*** config GPIO
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pull_up_en = 1;
	io_conf.pull_down_en = 0;
	io_conf.intr_type = GPIO_INTR_DISABLE;							// vo hieu hoa interrupt
	gpio_config(&io_conf);
}







void init(void) {
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}





int sendData(const char* logName, const char* data)
{
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(UART_NUM_1, data, len);
    ESP_LOGI(logName, "Wrote %d bytes", txBytes);
    ESP_LOGI(logName, "write %s\n",data);
    return txBytes;
}



static void tx_task(void *arg)
{
    static const char *TX_TASK_TAG = "TX_TASK";
    esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
    while (1) {
    	printf("Step:.. %d\n",count);

    	//TODO Turn off anyway at the first state
    	if (count == -3){
    		sendData(TX_TASK_TAG, "at+cpowd=0\r");
    		count = -2;
    		vTaskDelay(3000 / portTICK_PERIOD_MS);
    	}

    	// Turn on 1 ---> hold 1 in 5s
    	else if (count == -2){
    		//sendData(TX_TASK_TAG, "at+cpowd=0\r");
    		gpio_set_level(POWER,0);
    		count = -1;
    		vTaskDelay(5000 / portTICK_PERIOD_MS);
    	}
    	// release 0 and wait 5s for completely setup
    	else if (count == -1){
    		gpio_set_level(POWER,1);
    		count = 0;
    		gpio_set_level(17,1);
    		vTaskDelay(10000 / portTICK_PERIOD_MS);
    	}

    	// Re-call turn off echo until SIM accepting UART
    	else if (count == 0){
    		//sendData(TX_TASK_TAG, "\r\n");
    		sendData(TX_TASK_TAG, "ate0\r");
    		vTaskDelay(500 / portTICK_PERIOD_MS);
    	}
    	else if (count == 1){
    	    //sendData(TX_TASK_TAG, "\r\n");
    		sendData(TX_TASK_TAG, "at\r\n");
    	    vTaskDelay(5000 / portTICK_PERIOD_MS);
    	}
    	else if (count > 1 && count < 6){
    		sendData(TX_TASK_TAG, "at\r");
    		vTaskDelay(5000 / portTICK_PERIOD_MS);
    	}


    	// Out of step , turn off
    	else if (count >= 6){
    		sendData(TX_TASK_TAG, "at+cpowd=0\r");
    		count = -2;
    		vTaskDelay(5000 / portTICK_PERIOD_MS);
    	}

    }
}

static void rx_task(void *arg)
{
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE+1);


    while (1) {
        const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 1000 / portTICK_PERIOD_MS);
        if (rxBytes > 0) {
            data[rxBytes] = 0;

            char* temp = (char*)data;

            // Process ERROR , EXCEPTION
            if (strstr(temp,"OK")) {
            	count += 1;
            	printf("Count: %d\n",count);
            }


            printf("data: %s\n",data);

            //////



            ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);
            ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
        }
    }
    free(data);
}





static void mqtt_task(void *arg)
{
    static const char *MQTT_TASK_TAG = "TX_TASK";
    esp_log_level_set(MQTT_TASK_TAG, ESP_LOG_INFO);
    while (1) {
    	printf("Step:.. %d\n",count);

    	//TODO Turn off anyway at the first state
    	if (count == -3){
    		sendData(MQTT_TASK_TAG, "at+cpowd=0\r");
    		count = -2;
    		vTaskDelay(3000 / portTICK_PERIOD_MS);
    	}

    	// Turn on 1 ---> hold 1 in 5s
    	else if (count == -2){
    		//sendData(TX_TASK_TAG, "at+cpowd=0\r");
    		gpio_set_level(POWER,0);
    		count = -1;
    		vTaskDelay(5000 / portTICK_PERIOD_MS);
    	}
    	// release 0 and wait 5s for completely setup
    	else if (count == -1){
    		gpio_set_level(POWER,1);
    		count = 0;
    		gpio_set_level(17,1);
    		vTaskDelay(10000 / portTICK_PERIOD_MS);
    	}

    	// Re-call turn off echo until SIM accepting UART
    	else if (count == 0){
    		//sendData(TX_TASK_TAG, "\r\n");
    		sendData(MQTT_TASK_TAG, "ate0\r");
    		vTaskDelay(500 / portTICK_PERIOD_MS);
    	}
    	else if (count == 1){
    	    //sendData(TX_TASK_TAG, "\r\n");
    		sendData(MQTT_TASK_TAG, "at\r\n");
    	    vTaskDelay(5000 / portTICK_PERIOD_MS);
    	}
    	else if (count > 1 && count < 6){
    		sendData(MQTT_TASK_TAG, "at\r");
    		vTaskDelay(5000 / portTICK_PERIOD_MS);
    	}


    	// Out of step , turn off
    	else if (count >= 6){
    		sendData(MQTT_TASK_TAG, "at+cpowd=0\r");
    		count = -2;
    		vTaskDelay(5000 / portTICK_PERIOD_MS);
    	}

    }
}


















void app_main(void)
{
    init();
    power_init();
    xTaskCreate(rx_task, "uart_rx_task", 1024*2, NULL, configMAX_PRIORITIES-1, NULL);
    xTaskCreate(tx_task, "uart_tx_task", 1024*2, NULL, configMAX_PRIORITIES, NULL);
}
