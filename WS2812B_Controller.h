#ifndef WS2812B_CONTROLLER
#define WS2812B_CONTROLLER
#include<stdlib.h>
#include<iostream>
#include<iterator>
#include<array>
#include<map>
#include <Arduino.h>

struct RGB {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

class WS2812B_Controller {
    public:
        WS2812B_CONTROLLER() = default;
        WS2812B_CONTROLLER(uint8_t pinnumber);
        ~WS2812B_CONTROLLER();
        void start_light();
        void set_pin(uint8_t pinnumber);
#define WS2812B_LENGTH 60
        void init_strip(int length);
        void set_strip_LED(RGB light);
        void shamble_LED_bits();
        void set_strip(RGB light);
        void set_strip_diff(std::array<WS2812B_Controller::RGB,WS2812B_LENGTH> strip);


    private:
        static struct Signal_Nibbles {
            static unsigned int big_endian : 4;
            static unsigned int little_endian : 4;
        };   
        std::array<WS2812B_Controller::RGB,WS2812B_LENGTH> LED_strip;
        uint8_t curr_pin_out;
        uint32_t curr_GPIO;
        bool first_GPIO_registers;
        std::map<uint8_t, uint32_t> pin_to_GPIO_index {
            {7, 0x00000080}, //GPIO32
            {8, 0x00000040}, //GPIO33
            {9, 0x00000040}, //GPIO25
            {10, 0x00000020}, //GPIO26
            {11, 0x00000010}, //GPIO27
            {12, 0x00020000}, //GPIO14
            {13, 0x00080000}, //GPIO12
            {15, 0x00040000}, //GPIO13
            {23, 0x00010000}, //GPIO15
            {21, 0x20000000}, //GPIO2
            {24, 0x08000000}, //GPIO4
            {26, 0x04000000}, //GPIO5
            {27, 0x00002000}, //GPIO18
            {28, 0x00001000}, //GPIO19
            {30, 0x00000400}, //GPIO21
            {33, 0x00000200}, //GPIO22
            {34, 0x00000100}, //GPIO23
        };
}
#endif