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

#define RX_BUF_SIZE  512
#define TXD_PIN (GPIO_NUM_17)				// TODO config UART here
#define RXD_PIN (GPIO_NUM_16)
#define POWER GPIO_NUM_4

#define PASSWORD "4CndXRTNmiaqsORnNHd7"
#define BROKER "demo.thingsboard.io"


static int ACK=0;
static int Err_count = 0;


//# INIT EVERYTHING..........//////////
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


static void uart_init(void) {
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


static void led_init(void){
	gpio_config_t io_conf;
	io_conf.pin_bit_mask = 1<< GPIO_NUM_2;							//*** config GPIO
	io_conf.mode = GPIO_MODE_OUTPUT;
	io_conf.pull_up_en = 0;
	io_conf.pull_down_en = 1;
	io_conf.intr_type = GPIO_INTR_DISABLE;							// vo hieu hoa interrupt
	gpio_config(&io_conf);
}




















// SEND DATA via UART
int sendData(const char* logName, const char* data)
{
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(UART_NUM_1, data, len);
    ESP_LOGI(logName, "Wrote %d bytes", txBytes);
    ESP_LOGI(logName, "Write:  %s\n",data);
    return txBytes;
}




char ceng_data[128]="";
char *psi,*rsrp,*rsrq,*sinr,*cellID;
int count2 = 2;
static int ceng_len = 0;
void getDataCENG(char* rawData){
    char* data = strstr(rawData,"\"");
    //printf("%s",b);
    char arrData[strlen(data)];
    for(int i = 0;i < strlen(data);i++){
        arrData[i] = data[i];
    }
    char* presentData = strtok(arrData," \",");
    while(presentData != NULL && count2 <9){
        presentData = strtok(NULL, " ,");

        if(count2 == 2){
            psi = presentData;
        }
        else if(count2 == 3){
            rsrp = presentData;
        }
        else if(count2 == 5){
            rsrq = presentData;
        }
        else if(count2 ==6){
            sinr = presentData;
        }
        else if(count2 ==8){
            cellID = presentData;
        }
        count2++;
    }
    count2 = 2;
    //printf("{\"psi\": &s,\"rsrp\": &s,\"rsrq\": &s,\"sinr\": &s,\"cellID\": &s,}", psi, rsrp, rsrq, sinr, cellID);
    sprintf(ceng_data,"{\"psi\":%s,\"rsrp\":%s,\"rsrq\":%s,\"sinr\":%s,\"cellID\":%s}", psi, rsrp, rsrq, sinr, cellID);
    ceng_len = strlen(ceng_data);
}







static int status_mqtt = 0;
static void mqtt_task(void *arg)
{

    static const char *MQTT_TASK_TAG = "MQTT_TASK";
    esp_log_level_set(MQTT_TASK_TAG, ESP_LOG_INFO);
    while (1) {

    	//TODO Check connect
    	if (status_mqtt == 0){
    		sendData(MQTT_TASK_TAG, "AT+CNACT=0,1\r");
    		status_mqtt = 1;
    		vTaskDelay(5000 / portTICK_PERIOD_MS);
    	}

    	// config Host and client ID
    	else if (status_mqtt == 1){
    		sendData(MQTT_TASK_TAG, "AT+SMCONF=\"URL\",\"demo.thingsboard.io\"\r");
    		vTaskDelay(500 / portTICK_PERIOD_MS);
    		sendData(MQTT_TASK_TAG, "AT+SMCONF=\"CLIENTID\",\"135e5672-8c35-40e7-9854-ecd81a344ee7\"\r");
    		status_mqtt = 2;
    		vTaskDelay(1000 / portTICK_PERIOD_MS);
    	}

    	//config Username and Password
    	else if (status_mqtt == 2){
    		sendData(MQTT_TASK_TAG, "AT+SMCONF=\"USERNAME\",\"4CndXRTNmiaqsORnNHd7\"\r");
    		vTaskDelay(100 / portTICK_PERIOD_MS);
    		sendData(MQTT_TASK_TAG, "AT+SMCONF=\"PASSWORD\",\"4CndXRTNmiaqsORnNHd7\"\r");
    		//sendData(MQTT_TASK_TAG, "AT+SMCONF=\"PASSWORD\",\"C3ZBwHndbwMkOXIz4HmJWRs9OrddkTfU\"\r");	// PASSWORD is very IMPORTANT
    		vTaskDelay(100 / portTICK_PERIOD_MS);
    		sendData(MQTT_TASK_TAG, "AT+SMCONF?\r");
    		status_mqtt = 3;
    	    vTaskDelay(2000 / portTICK_PERIOD_MS);
    	}

    	// CONNECTING.....
    	else if (status_mqtt == 3){
    		sendData(MQTT_TASK_TAG, "AT+SMCONN\r");
    	    vTaskDelay(20000 / portTICK_PERIOD_MS);
    	    if (ACK) status_mqtt = 4;
    	}


    	//Subscribe
    	else if (status_mqtt == 4){
    		//sendData(TX_TASK_TAG, "at+cpowd=0\r");
    		//sendData(MQTT_TASK_TAG, "AT+SMSUB=\"messages/d86dabaa-d818-4e30-b7ee-fa649f772bda/status\",0\r");
    		sendData(MQTT_TASK_TAG, "AT+SMSUB=\"v1/devices/me/rpc/request/+\",0\r");
    		status_mqtt = 5;
    		vTaskDelay(5000 / portTICK_PERIOD_MS);
    	}


    	//Get CENG DATA
    	else if (status_mqtt == 5){
    		sendData(MQTT_TASK_TAG, "AT+CENG?\r");		// remember delete malloc
    		vTaskDelay(5000 / portTICK_PERIOD_MS);
    		if (ACK) status_mqtt = 6;
    	}








    	// Publishing.......
    	// Format-------------------
    	// AT+SMPUB="messages/d86dabaa-d818-4e30-b7ee-fa649f772bda/update",64,0,1
    	// -------------------------
        else if (status_mqtt == 6){
        	char msg[100];
        	//sprintf(msg,"AT+SMPUB=\"messages/d86dabaa-d818-4e30-b7ee-fa649f772bda/update\",%d,0,1\r",ceng_len);
        	sprintf(msg,"AT+SMPUB=\"v1/devices/me/telemetry\",%d,0,1\r",ceng_len);
   	   		//sendData(MQTT_TASK_TAG, "AT+SMPUB=\"messages/d86dabaa-d818-4e30-b7ee-fa649f772bda/update\",64,0,0\r");
        	sendData(MQTT_TASK_TAG, msg);
   	   		vTaskDelay(1000 / portTICK_PERIOD_MS);
   	   		status_mqtt = 7;
   	   	}


    	//add data publish.....
        else if (status_mqtt == 7){
        	 //strcpy(ceng_data,"{\"psi\":367,\"rsrp\":-70,\"rsrq\":-10,\"sinr\":8,\"cellID\":151089173}   ");
        	 //printf("Publish at step 7: %s\n",ceng_data);
           	 sendData(MQTT_TASK_TAG, ceng_data);
           	 status_mqtt = 5;
           	 vTaskDelay(10000 / portTICK_PERIOD_MS);
        }
    }
}










static int count = -3;
static void tx_task(void *arg)
{
    static const char *TX_TASK_TAG = "TX_TASK";
    esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
    while (1) {
    	printf("At preparing step:.. %d\n",count);

    	//TODO Turn off anyway at the first state
    	if (count == -3){
    		sendData(TX_TASK_TAG, "at+cpowd=0\r");
    		vTaskDelay(1000 / portTICK_PERIOD_MS);
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
    		vTaskDelay(10000 / portTICK_PERIOD_MS);
    	}

    	// Re-call turn off echo until SIM accepting UART
    	else if (count == 0){
    		gpio_set_level(17,1);
    		sendData(TX_TASK_TAG, "ate0\r");
    		vTaskDelay(1000 / portTICK_PERIOD_MS);
    	}
    	else if (count == 1){
    	    //sendData(TX_TASK_TAG, "\r\n");
    		sendData(TX_TASK_TAG, "at\r\n");
    	    vTaskDelay(5000 / portTICK_PERIOD_MS);
    	}
    	else if (count > 1 && count < 6){
    		sendData(TX_TASK_TAG, "at\r");
    		vTaskDelay(2000 / portTICK_PERIOD_MS);
    	}


    	// Out of step , turn off
    	else if (count >= 6){
    		xTaskCreate(mqtt_task, "mqtt_task", 1024*2, NULL, configMAX_PRIORITIES-1, NULL);
    		vTaskDelete(NULL);

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
            ACK = 1;


            if (strstr(temp,"+CENG")){
               //printf("At step RX 6: Get CENG...\n");

               if (rxBytes < 55) {
            	   ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_ERROR);
            	   ACK = 0;
               }

               else{
            	   ACK = 1;
            	   strcpy(ceng_data,(char*)data);
            	   getDataCENG(ceng_data);
            	   count += 1;
            	   printf("Count: %d\n",count);
               }

            }

            // Process ERROR , EXCEPTION
            else if (strstr(temp,"OK")) {
            	ACK = 1;
            	count += 1;
            	printf("Count: %d\n",count);
            }
            //
            else if (strstr(temp,"ERROR")){

            	ACK =0;
            	ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_ERROR);
            }


            // handler when Error too much
            if (ACK == 0){
            	Err_count += 1;
            	if (Err_count == 10) esp_restart();
            }
            else Err_count = 0;



            ESP_LOGI(RX_TASK_TAG, "Read %d bytes:...\n", rxBytes);
            ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rxBytes, ESP_LOG_WARN);
        }
    }
    free(data);
}









static void led_task(void * arg){
	int toggle = 0;
	while (1){
		toggle = 1-toggle;
		gpio_set_level(2,toggle);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}




//static void GNSS(){
//	AT+CGNSPWR=1
//	AT+CGNSINF
//}









void app_main(void)
{
    uart_init();
    power_init();
    led_init();

    xTaskCreate(rx_task, "uart_rx_task", 1024*2, NULL, configMAX_PRIORITIES, NULL);
    xTaskCreate(tx_task, "uart_tx_task", 1024*2, NULL, configMAX_PRIORITIES-1, NULL);
    xTaskCreate(led_task, "LED_task", 1024*2, NULL, configMAX_PRIORITIES-2, NULL);
}
