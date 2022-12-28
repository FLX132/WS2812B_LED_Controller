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
        void init_strip(std::array<RGB,WS2812B_LENGTH> temp);

    private:
        static struct Signal_Nibbles {
            static unsigned int big_endian : 4;
            static unsigned int little_endian : 4;
        };   
        std::array<WS2812B_Controller::RGB,WS2812B_LENGTH> LED_strip;
        uint8_t curr_pin_out;
        uint32_t curr_GPIO;
        bool first_GPIO_registers;
        std::map<uint8_t, int> pin_to_GPIO_index {
            {5, 0x00000008},
            {8, 0x00000001},
            {10, 0x00000020},
            {11, 0x00000010},
            {12, 0x00000080},
            {13, 0x00000040},
            {14, 0x00000040},
            {15, 0x00000020},
            {16, 0x00000010},
            {17, 0x00020000},
            {18, 0x00080000},
            {20, 0x00040000},
            {21, 0x00010000},
            {22, 0x20000000},
            {23, 0x80000000},
            {24, 0x08000000},
            {25, 0x00008000},
            {27, 0x00004000},
            {28, 0x00400000},
            {29, 0x00200000},
            {30, 0x00100000},
            {31, 0x02000000},
            {32, 0x01000000},
            {33, 0x00800000},
            {34, 0x04000000},
            {35, 0x00002000},
            {36, 0x00000100},
            {38, 0x00001000},
            {39, 0x00000200},
            {40, 0x10000000},
            {41, 0x40000000},
            {42, 0x00000400},
        };
}
#endif