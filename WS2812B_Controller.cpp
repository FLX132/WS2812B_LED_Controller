#include"WS2812B_Controller.h"

WS2812B_Controller::WS2812B_Controller() {}

WS2812B_Controller::~WS2812B_Controller() {}

void WS2812B_Controller::start_light() {

    auto led_iterator = LED_strip.begin();
    Signal_Nibbles::big_endian = 0x1;
    Signal_Nibbles::little_endian = 0x4;
    if(led_iterator->g & 0x80) {
        Signal_Nibbles::big_endian+=0x2;
    }
    
#define T0HT0L (Signal_Nibbles::big_endian = Signal_Nibbles::big_endian | 0x2), (Signal_Nibbles::little_endian = Signal_Nibbles::little_endian << 1)
#define T0LT0H (Signal_Nibbles::big_endian = Signal_Nibbles::big_endian & 0x1), (Signal_Nibbles::little_endian = Signal_Nibbles::little_endian >> 1)
#define T0LT1H                                                                  (Signal_Nibbles::little_endian = Signal_Nibbles::little_endian << 1)
#define T1HT1L (Signal_Nibbles::big_endian = Signal_Nibbles::big_endian & 0x1), (Signal_Nibbles::little_endian = Signal_Nibbles::little_endian >> 1)
#define T1LT1H (Signal_Nibbles::big_endian = Signal_Nibbles::big_endian | 0x2), (Signal_Nibbles::little_endian = Signal_Nibbles::little_endian << 1)
#define T1LT0H                                                                  (Signal_Nibbles::little_endian = Signal_Nibbles::little_endian >> 1)

#ifdef PIN
    while(led_iterator != LED_strip.end()) {
        //execution with port etc.
        /*
        * 8 times read bit, save, shift left by 1
        * send bit with time code to correct pin on esp32
        * during wait (from 20 to 56 NOP) catch next bit and change big and little endian
        * same procedure with red and blue
        * loop starts again
        */
    }
#endif
}

void WS2812B_Controller::init_strip(std::array<RGB,WS2812B_LENGTH> temp) {
    LED_strip = temp;
}

/**
 * class WS2812B_Controller {
    public:
        WS2812B_CONTROLLER();
        ~WS2812B_CONTROLLER();
        void start_light();
#define WS2812B_LENGTH 60
        void init_strip(std::array<RGB,WS2812B_LENGTH> temp);

    private:
        static struct Signal_Nibbles {
            unsigned int big_endian : 4;
            unsigned int little_endian : 4;
        };   
        struct RGB {
            uint8_t r;
            uint8_t g;
            uint8_t b;
        };
        std::array<RGB,WS2812B_LENGTH> LED_strip;
}
*/