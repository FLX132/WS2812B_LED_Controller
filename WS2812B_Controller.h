#ifndef WS2812B_CONTROLLER
#define WS2812B_CONTROLLER
#include<stdlib.h>
#include<iostream>
#include<iterator>
#include<array>
#include<map>
#include <Arduino.h>
#include <driver/rmt.h>

/**
 * WS2812B uses GRB instead of RGB
 * Offset of G needs to be 0 and Offset of R 1
*/
#define R_OFFSET 1
#define G_OFFSET 0
#define B_OFFSET 2

static uint32_t t0h_ticks = 0;
static uint32_t t1h_ticks = 0;
static uint32_t t0l_ticks = 0;
static uint32_t t1l_ticks = 0;

class WS2812B_Controller {
    public:
        WS2812B_Controller(uint8_t pinnumber);
        // More accurate Version of constructor needed for the future
        ~WS2812B_Controller();
        void start_light();
        void set_pin(uint8_t pinnumber);
#define WS2812B_LENGTH 60
        void init_strip_length(uint8_t length);
        void change_led_color_all(uint8_t r, uint8_t g, uint8_t b, uint8_t n = 0);
        void change_led_color(uint8_t n, uint8_t r, uint8_t g, uint8_t b);
        void test_light();
        void output() {
          Serial.println((int)length);
        }


    private:
        bool reserved_channels[RMT_CHANNEL_MAX];
        uint8_t curr_pin_out;
        uint32_t curr_GPIO;
        uint8_t length;
        bool first_GPIO_registers;
        uint8_t *curr_led;
        std::map<uint8_t, gpio_num_t> pin_to_GPIO_index {
            {32, gpio_num_t::GPIO_NUM_32}, //GPIO32
            {33, gpio_num_t::GPIO_NUM_33}, //GPIO33
            {25, gpio_num_t::GPIO_NUM_25}, //GPIO25
            {26, gpio_num_t::GPIO_NUM_26}, //GPIO26
            {27, gpio_num_t::GPIO_NUM_27}, //GPIO27
            {14, gpio_num_t::GPIO_NUM_14}, //GPIO14
            {12, gpio_num_t::GPIO_NUM_12}, //GPIO12
            {13, gpio_num_t::GPIO_NUM_13}, //GPIO13
            {15, gpio_num_t::GPIO_NUM_15}, //GPIO15
            {2, gpio_num_t::GPIO_NUM_2}, //GPIO2
            {4, gpio_num_t::GPIO_NUM_4}, //GPIO4
            {5, gpio_num_t::GPIO_NUM_5}, //GPIO5
            {18, gpio_num_t::GPIO_NUM_18}, //GPIO18
            {19, gpio_num_t::GPIO_NUM_19}, //GPIO19
            {21, gpio_num_t::GPIO_NUM_21}, //GPIO21
            {22, gpio_num_t::GPIO_NUM_22}, //GPIO22
            {23, gpio_num_t::GPIO_NUM_23}, //GPIO23
        };
        static void ws2812b_rmt(const void *src, rmt_item32_t *dest, size_t src_size, size_t wanted_num, size_t *translated_size, size_t *item_num) {

          if(src == NULL || dest == NULL) {
            *translated_size = 0;
            *item_num = 0;
          }
          const rmt_item32_t bit0 = {{{ t0h_ticks, 1, t0l_ticks, 0 }}};
          const rmt_item32_t bit1 = {{{ t1h_ticks, 1, t1l_ticks, 0 }}};
          uint8_t* src_pointer = (uint8_t *)src;
          size_t size = 0;
          size_t num = 0;
          rmt_item32_t *pdest = dest;

          while(num < wanted_num && size < src_size) {
            for(int i = 0; i < 8; i++) {
              if(*src_pointer & (1 << (7-i))) {
                pdest->val = bit1.val;                
              } else {
                pdest->val = bit0.val;
              }
              pdest++;
              num++;
            }
            src_pointer++;
            size++;
          }
          *translated_size = size;
          *item_num = num;
        };
};
#endif