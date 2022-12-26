#ifndef WS2812B_CONTROLLER
#define WS2812B_CONTROLLER
#include<stdlib.h>
#include<iostream>
#include<iterator>
#include<array>

struct RGB {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

class WS2812B_Controller {
    public:
        WS2812B_CONTROLLER();
        ~WS2812B_CONTROLLER();
        void start_light();
#define WS2812B_LENGTH 60
        void init_strip(std::array<RGB,WS2812B_LENGTH> temp);

    private:
        static struct Signal_Nibbles {
            static unsigned int big_endian : 4;
            static unsigned int little_endian : 4;
        };   
        std::array<WS2812B_Controller::RGB,WS2812B_LENGTH> LED_strip;
}
#endif