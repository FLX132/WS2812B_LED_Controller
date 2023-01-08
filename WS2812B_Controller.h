#ifndef WS2812B_CONTROLLER
#define WS2812B_CONTROLLER
#include<stdlib.h>
#include<iostream>
#include<iterator>
#include<array>
#include<map>
#include <Arduino.h>

/**
 * WS2812B uses GRB instead of RGB
 * Offset of G needs to be 0 and Offset of R 1
*/
#define R_OFFSET 1
#define G_OFFSET 0
#define B_OFFSET 2

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


    private:
        uint8_t curr_pin_out;
        uint32_t curr_GPIO;
        uint8_t length;
        bool first_GPIO_registers;
        uint8_t *curr_led;
        std::map<uint8_t, uint32_t> pin_to_GPIO_index {
            {32, 0x00000080}, //GPIO32
            {33, 0x00000040}, //GPIO33
            {25, 0x00000040}, //GPIO25
            {26, 0x00000020}, //GPIO26
            {27, 0x00000010}, //GPIO27
            {14, 0x00020000}, //GPIO14
            {12, 0x00080000}, //GPIO12
            {13, 0x00040000}, //GPIO13
            {15, 0x00010000}, //GPIO15
            {2, 0x20000000}, //GPIO2
            {4, 0x08000000}, //GPIO4
            {5, 0x04000000}, //GPIO5
            {18, 0x00002000}, //GPIO18
            {19, 0x00001000}, //GPIO19
            {21, 0x00000400}, //GPIO21
            {22, 0x00000200}, //GPIO22
            {23, 0x00000100}, //GPIO23
        };
};
#endif