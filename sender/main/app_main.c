/* SPI Slave example, sender (uses SPI master driver)

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "driver/spi_master.h"
// #include "esp_spi_flash.h"

#include "soc/gpio_reg.h"
#include "driver/gpio.h"
// #include "esp_intr_alloc.h"


/*
Pins in use. The SPI Master can use the GPIO mux, so feel free to change these if needed.
*/
// HSPI
#define GPIO_MOSI GPIO_NUM_13
#define GPIO_MISO GPIO_NUM_5
#define GPIO_SCLK GPIO_NUM_14
#define GPIO_CS GPIO_NUM_15
#define GPIO_HANDSHAKE GPIO_NUM_32

static spi_device_handle_t handle;

int8_t read(spi_device_handle_t handler, uint8_t *data, uint16_t len) {
	static struct spi_transaction_t trans;

	trans.flags = 0;
	trans.length = len * 8;
	trans.rxlength = trans.length;
	trans.tx_buffer = heap_caps_malloc(1, MALLOC_CAP_DMA);
	trans.rx_buffer = heap_caps_malloc(len, MALLOC_CAP_DMA);

	memset(trans.rx_buffer, 0, len*sizeof(uint8_t));

	/* debug */
	// ESP_LOG_LEVEL(ESP_LOG_DEBUG, "main", "length = %d", len);
	// ESP_LOG_LEVEL(ESP_LOG_DEBUG, "main", "reg_addr = %x", reg_addr);

	int ret = ESP_OK;
	ret = spi_device_transmit(handler, &trans);
	if (ret < 0) {
		ESP_LOGE("main", "Error: transaction transmission failled");
        return ret;
	}

	memcpy(data, trans.rx_buffer, len);

    free(trans.tx_buffer);
    free(trans.rx_buffer);

	/* debug */
	// for (int i = 0; i < len; i++)
	// 	ESP_LOG_LEVEL(ESP_LOG_DEBUG, "main", "rx_buffer[%d] = %d %x", i, ((uint8_t *) trans.rx_buffer)[i], ((uint8_t *) trans.rx_buffer)[i]);

	return ret;
}

int8_t write(spi_device_handle_t handler, uint8_t *data, uint16_t len) {
	static struct spi_transaction_t trans;

	/* debug */
	// ESP_LOG_LEVEL(ESP_LOG_DEBUG, "main", "length = %d", len);
	// ESP_LOG_LEVEL(ESP_LOG_DEBUG, "main", "reg_addr = %x", reg_addr);
	
	trans.flags = 0;
	trans.length = len * 8;
	trans.tx_buffer = heap_caps_malloc(len, MALLOC_CAP_DMA);

	memcpy(trans.tx_buffer, data, len);

	int8_t ret;
	ret = spi_device_transmit(handler, &trans);
	if (ret < 0) {
		ESP_LOGE("main", "Error: transaction transmission failled");
        return ret;
	}

    free(trans.tx_buffer);

	/* debug */
	// for (int i = 0; i < len; i++)
	// 	ESP_LOG_LEVEL(ESP_LOG_DEBUG, "main", "tx_buffer[%d] = [%x] %d", i, ((uint8_t *) trans.tx_buffer)[i], ((uint8_t *) trans.tx_buffer)[i]);

	return ret;
}


//Main application
void app_main()
{
    // Create the semaphore.
    // rdyToReadSem=xSemaphoreCreateBinary();

    //GPIO config for the handshake line.
    gpio_config_t io_conf={
        .intr_type=GPIO_PIN_INTR_DISABLE,
        .mode=GPIO_MODE_INPUT,
        .pull_up_en=1,
        .pin_bit_mask=(1ULL<<GPIO_HANDSHAKE)
    };

    //Set up handshake line interrupt.
    gpio_config(&io_conf);
 
    esp_err_t ret;

    //Configuration for the SPI bus
    spi_bus_config_t buscfg={
        .mosi_io_num=GPIO_MOSI,
        .miso_io_num=GPIO_MISO,
        .sclk_io_num=GPIO_SCLK,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1
    };

    //Configuration for the SPI device on the other side of the bus
    spi_device_interface_config_t devcfg={
        .command_bits=0,
        .address_bits=0,
        .dummy_bits=0,
        .clock_speed_hz= 1*  1000 *1000,
        .duty_cycle_pos= 128,        //50% duty cycle
        .mode=0,
        .spics_io_num=GPIO_CS,
        .cs_ena_posttrans=3,        //Keep the CS low 3 cycles after transaction, to stop slave from missing the last bit when CS has less propagation delay than CLK
        .queue_size=3
    };

    //Initialize the SPI bus and add the device we want to send stuff to.
    ret=spi_bus_initialize(HSPI_HOST, &buscfg, 1);
    assert(ret==ESP_OK);
    ret=spi_bus_add_device(HSPI_HOST, &devcfg, &handle);
    assert(ret==ESP_OK);

    int n=0;
    // char sendbuf[128] = {0};
    // char recvbuf[128] = {0};
    // spi_transaction_t t;
    // memset(&t, 0, sizeof(t));

    // //Assume the slave is ready for the first transmission: if the slave started up before us, we will not detect 
    // //positive edge on the handshake line.
    // // xSemaphoreGive(rdyToReadSem);

    while(1) {
        // int res = snprintf(sendbuf, sizeof(sendbuf),
        //         "Sender, transmission no. %04i. Last time, I received: \"%s\"", n, recvbuf);
        // if (res >= sizeof(sendbuf)) {
        //     printf("Data truncated\n");
        // }
        // t.length=sizeof(sendbuf)*8;
        // t.tx_buffer=sendbuf;
        // t.rx_buffer=recvbuf;
        // //Wait for slave to be ready for next byte before sending
        // // xSemaphoreTake(rdyToReadSem, portMAX_DELAY); //Wait until slave is ready
        // ret=spi_device_transmit(handle, &t);
        // printf("Received: %s\n", recvbuf);
        // n++;

        char sendbuf[128] = {0};
        snprintf(sendbuf, 128, "Sender, transmission no. %04i.", n);
        if(gpio_get_level(GPIO_HANDSHAKE) == 1){
            printf("READ\n");
            char recvbuf[128] = {0};
            read(handle, (uint8_t*) &recvbuf, 128);
            printf("Received: %s\n", recvbuf);
        } else {
            printf("WRITE\n");
            printf("->%s\n",sendbuf);
            write(handle, (uint8_t*) &sendbuf, 128);
            
        }
        n++;
        vTaskDelay(2500 / portTICK_PERIOD_MS);
    }

    //Never reached.
    ret=spi_bus_remove_device(handle);
    assert(ret==ESP_OK);
}
