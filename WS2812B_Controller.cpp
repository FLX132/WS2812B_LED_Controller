#include"WS2812B_Controller.h"

WS2812B_Controller::WS2812B_Controller(uint8_t pinnumber) : curr_led(NULL) {
    WS2812B_Controller::set_pin(pinnumber);
}

WS2812B_Controller::~WS2812B_Controller() {
    
}

void WS2812B_Controller::set_pin(uint8_t pinnumber) {   
    if(WS2812B_Controller::pin_to_GPIO_index.find(pinnumber) != WS2812B_Controller::pin_to_GPIO_index.end()) { // Pin needs to be available for I/O operations to LED
        pinMode(WS2812B_Controller::curr_pin_out, INPUT); // INPUT is equivalent to 0x01 | 0000 0001b
        pinMode(pinnumber, OUTPUT); // OUTPUT is equivalent to 0x03 | 0000 0011b
        WS2812B_Controller::curr_pin_out = pinnumber;
        WS2812B_Controller::curr_GPIO = WS2812B_Controller::pin_to_GPIO_index.find(pinnumber)->second;
        (WS2812B_Controller::curr_pin_out > 0x1F) ? WS2812B_Controller::first_GPIO_registers == false : WS2812B_Controller::first_GPIO_registers == true;
    } else {
        auto nearest_member = WS2812B_Controller::pin_to_GPIO_index.lower_bound(pinnumber); // change to nearest available Pin
        WS2812B_Controller::curr_pin_out = nearest_member->first;
        WS2812B_Controller::curr_GPIO = nearest_member->second;
        (WS2812B_Controller::curr_pin_out > 0x1F) ? WS2812B_Controller::first_GPIO_registers == false : WS2812B_Controller::first_GPIO_registers == true;
    }
    
}

void WS2812B_Controller::start_light() {

    auto led_iterator = LED_strip.begin(); // start by first LED and iterate through the array

    /**
     * Init first Signal:
     *   T0H -> 0x14 -> T0L -> 0x38
     *   T1H -> 0x34 -> T1L -> 0x18
    */
    Signal_Nibbles::big_endian = 0x1;
    Signal_Nibbles::little_endian = 0x4;
    if(led_iterator->g & 0x80) {
        Signal_Nibbles::big_endian+=0x2;
    }

    /**
     * Preprocessor uses state-machine like transition:
     *   - T0H -> T0L
     *   - T0L -> T0H
     *   - T0L -> T1H
     *   - T1H -> T1L
     *   - T1l -> T1H
     *   - T1L -> T0H
     * 
     *  example for following explanation: T1L: 1. nibble -> 0x1 2. nibble -> 0x8 >> combined: 0x18 = 24 (minimum strokes to ensure correct signal)
     *  >--------------------------------------------------------------------<  >------------------------------------------------------------------>
     *                           sets first nibble                                                           sets second nibble
    */
#define T0HT0L (Signal_Nibbles::big_endian = Signal_Nibbles::big_endian | 0x2), (Signal_Nibbles::little_endian = Signal_Nibbles::little_endian << 1)
#define T0LT0H (Signal_Nibbles::big_endian = Signal_Nibbles::big_endian & 0x1), (Signal_Nibbles::little_endian = Signal_Nibbles::little_endian >> 1)
#define T0LT1H                                                                  (Signal_Nibbles::little_endian = Signal_Nibbles::little_endian << 1)
#define T1HT1L (Signal_Nibbles::big_endian = Signal_Nibbles::big_endian & 0x1), (Signal_Nibbles::little_endian = Signal_Nibbles::little_endian >> 1)
#define T1LT1H (Signal_Nibbles::big_endian = Signal_Nibbles::big_endian | 0x2), (Signal_Nibbles::little_endian = Signal_Nibbles::little_endian << 1)
#define T1LT0H                                                                  (Signal_Nibbles::little_endian = Signal_Nibbles::little_endian >> 1)

       // [31 ... 0] output field; address of GPIO 0-31 set and clear registers
#define GPIO_OUT_W1TS_REG 0x3FF44008
#define GPIO_OUT_W1TC_REG 0x3FF4400C 
       // [31 ... 8] unused; [7 ... 0] output field; address of GPIO 32-39 set and clear registers
#define GPIO_OUT1_W1TS_REG 0x3FF44014
#define GPIO_OUT1_W1TC_REG 0x3FF44018
        /**
         * Write HIGH to GPIO 15 -> out GPIO_OUT_W1TS_REG 0x00004000 (0000 0000 0000 0001 0000 0000 0000 0000)
         * 
         * Corresponding Bit in GPIO_OUT_REG will be changed:
         *     0x88108081 (1000 1000 0001 0000 1000 0000 1000 0001) -> 0x8810C081 (1000 1000 0001 0001 1000 0000 1000 0001)
         *     [GPIO-PIN set: 0, 4, 12, 16, 24, 31]                    [GPIO-PIN set: 0, 4, 12, 15, 16, 24, 31]
         * 
         * ----------------------------------------------
         * 
         * Write LOW to GPIO 15 -> out GPIO_OUT_W1TC_REG 0x00004000 (0000 0000 0000 0001 0000 0000 0000 0000)
         * 
         * Corresponding Bit in GPIO_OUT_REG will be changed:
         *     0x88108081 (1000 1000 0001 0001 1000 0000 1000 0001) -> 0x8810C081 (1000 1000 0001 0000 1000 0000 1000 0001)
         *     [GPIO-PIN set: 0, 4, 12, 15, 16, 24, 31]                    [GPIO-PIN set: 0, 4, 12, 16, 24, 31]
        */

    uint32_t output_reg_addr_set = ((6 < WS2812B_Controller::curr_pin_out < 9) ? GPIO_OUT1_W1TS_REG : GPIO_OUT_W1TS_REG);
    uint32_t output_reg_addr_clear = ((6 < WS2812B_Controller::curr_pin_out < 9) ? GPIO_OUT1_W1TC_REG : GPIO_OUT_W1TC_REG);
    uint32_t pin_hex = WS2812B_Controller::pin_to_GPIO_index.find(WS2812B_Controller::curr_pin_out);

    asm(
        // load pin_hex to ax register(s)
        // load 0x8000_0000 to ax register(s)
    );
    while(led_iterator != LED_strip.end()) {
        //execution with port etc.
        /*
        * 8 times read bit, save, shift left by 1
        * send bit with time code to correct pin on esp32
        * during wait (from 20 to 56 NOP) catch next bit and change big and little endian
        * same procedure with red and blue
        * loop starts again with led_iterator++
        */
       asm volatile (
        // loop 8 times
        //
        // load from iterator address to ax
        //
        // load green byte to ax
        // AND ax, 0x8000_0000
        // load from ax registers to GPIO high register
        // branch equal to 0x8000_0000 (+Jump)
        //    | NOPx?
        //    | jump back
        // NOPx?
        // load from ax registers to GPIO low register
        // branch equal to zero (+Jump)
        //    | NOPx?
        //    | jump back
        // NOPx?
        //
        // load red byte to ax
        // AND ax, 0x8000_0000
        // load from ax registers to GPIO high register
        // branch equal to 0x8000_0000 (+Jump)
        //    | NOPx?
        //    | jump back
        // NOPx?
        // load from ax registers to GPIO low register
        // branch equal to zero (+Jump)
        //    | NOPx?
        //    | jump back
        // NOPx?
        //
        // load blue byte to ax
        // AND ax, 0x8000_0000
        // load from ax registers to GPIO high register
        // branch equal to 0x8000_0000 (+Jump)
        //    | NOPx?
        //    | jump back
        // NOPx?
        // load from ax registers to GPIO low register
        // branch equal to zero (+Jump)
        //    | NOPx?
        //    | jump back
        // NOPx?
        //
        // load 0x8000_0000 from ax to shift register
        // shift ax register which stores 0x8000_0000 one to right
        // write back to ax
       );
       led_iterator++;
    }

}

void WS2812B_Controller::init_strip_length(uint8_t length) {
    free(WS2812B_Controller::curr_led);

    WS2812B_Controller::curr_led = (uint8_t*)malloc(length * 3);
    WS2812B_Controller::length = length;
}

void WS2812B_Controller::change_led_color_all(uint8_t r, uint8_t g, uint8_t b) {
}

void WS2812B_Controller::change_led_color(uint8_t n, uint8_t r, uint8_t g, uint8_t b) {}
