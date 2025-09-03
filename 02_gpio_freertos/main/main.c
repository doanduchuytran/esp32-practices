#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <esp_intr_types.h>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "hal/gpio_types.h"
#include "portmacro.h"
#include "soc/gpio_num.h"

#define GPIO_INPUT_PIN    0    // Example: Button connected to GPIO0
#define GPIO_OUTPUT_PIN   2    // Example: LED connected to GPIO2
#define ESP_INTR_FLAG_DEFAULT 0

// Queue handle for passing GPIO events
static QueueHandle_t gpio_evt_queue = NULL;

// Interrupt handler
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    // Send GPIO number to the queue
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

// Task to process the GPIO event
static void gpio_task(void* arg)
{
    uint32_t io_num;
    while(1) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            uint32_t level = gpio_get_level(GPIO_OUTPUT_PIN);
            printf("level: %d\n", gpio_get_level(GPIO_OUTPUT_PIN));
            gpio_set_level(GPIO_OUTPUT_PIN, !level);
            printf("level: %d\n", gpio_get_level(GPIO_OUTPUT_PIN));
        }
    }
}

void app_main(void)
{
    // Configure output pin (LED)
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE; // no interrupt for LED
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL<<GPIO_OUTPUT_PIN);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    // Configure input pin (Button)
    io_conf.intr_type = GPIO_INTR_NEGEDGE;  // falling edge interrupt
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL<<GPIO_INPUT_PIN);
    io_conf.pull_up_en = 1;   // enable pull-up (button to GND)
    gpio_config(&io_conf);

    // Create a queue to handle GPIO events
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    
    // Create a task to handle interrupts
    xTaskCreate(gpio_task, "gpio_task", 2048, NULL, 10, NULL);

    // Install GPIO ISR service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

    // Attach the interrupt handler
    gpio_isr_handler_add(GPIO_INPUT_PIN, gpio_isr_handler, (void*) GPIO_INPUT_PIN);
}