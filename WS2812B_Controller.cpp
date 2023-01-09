#include"WS2812B_Controller.h"

WS2812B_Controller::WS2812B_Controller(uint8_t pinnumber) : curr_led(NULL) {
    WS2812B_Controller::set_pin(pinnumber);
}

WS2812B_Controller::~WS2812B_Controller() {
    free(WS2812B_Controller::curr_led);
}

void WS2812B_Controller::set_pin(uint8_t pinnumber) {   
    if(WS2812B_Controller::pin_to_GPIO_index.find(pinnumber) != WS2812B_Controller::pin_to_GPIO_index.end()) { // Pin needs to be available for I/O operations to LED
        pinMode(WS2812B_Controller::curr_pin_out, INPUT); // INPUT is equivalent to 0x01 | 0000 0001b
        pinMode(pinnumber, OUTPUT); // OUTPUT is equivalent to 0x03 | 0000 0011b
        digitalWrite(pinnumber, LOW); // LOW is equivalent to 0x0 | 0000 0000b and HIGH to 0x1 | 0001 0000b
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
#define T0H_NOP 

    uint32_t output_reg_addr_set = ((6 < WS2812B_Controller::curr_pin_out < 9) ? GPIO_OUT1_W1TS_REG : GPIO_OUT_W1TS_REG);
    uint32_t output_reg_addr_clear = ((6 < WS2812B_Controller::curr_pin_out < 9) ? GPIO_OUT1_W1TC_REG : GPIO_OUT_W1TC_REG);
    uint32_t pin_hex = WS2812B_Controller::pin_to_GPIO_index.find(WS2812B_Controller::curr_pin_out)->second;

    // This Assembly section is used to load prerequierements to registers in the processor for faster access
    asm(
        "movi a3, 128;" // load comparing parameters into registers for later catching off bits in current_led rgb bytes
    );
    int temp_length = WS2812B_Controller::length*3;

    // This Assembly section is used for actually setting up lights
    for(int i = 0; i < temp_length; i++) {
        asm(
            "l8ui a11, %0, 0;" // %0 = curr_led
            "R:"
            "beqz a3, E;"
            
            "and a12, a11, a3;" // compare current_led first bit to 0x80 => 1 or 0
            "bnez a12, O;" // a12 equals zero than wait this time or this time
            //branch for 0 encoding
            "s32i %3, %1, 0;" // send to register output
            "nop;" // nop: 1; Nop x20
            "nop;" // nop: 2
            "nop;" // nop: 3
            "nop;" // nop: 4
            "nop;" // nop: 5
            "nop;" // nop: 6
            "nop;" // nop: 7
            "nop;" // nop: 8
            "nop;" // nop: 9
            "nop;" // nop: 10
            "nop;" // nop: 11
            "nop;" // nop: 12
            "nop;" // nop: 13
            "nop;" // nop: 14
            "nop;" // nop: 15
            "nop;" // nop: 16
            "nop;" // nop: 17
            "nop;" // nop: 18
            "nop;" // nop: 19
            "nop;" // nop: 20
            "s32i %2, %3, 0;" //send to register output negate
            "nop;" // nop: 1; Nop x56 - 9
            "nop;" // nop: 2
            "nop;" // nop: 3
            "nop;" // nop: 4
            "nop;" // nop: 5
            "nop;" // nop: 6
            "nop;" // nop: 7
            "nop;" // nop: 8
            "nop;" // nop: 9
            "nop;" // nop: 10
            "nop;" // nop: 11
            "nop;" // nop: 12
            "nop;" // nop: 13
            "nop;" // nop: 14
            "nop;" // nop: 15
            "nop;" // nop: 16
            "nop;" // nop: 17
            "nop;" // nop: 18
            "nop;" // nop: 19
            "nop;" // nop: 20
            "nop;" // nop: 21
            "nop;" // nop: 22
            "nop;" // nop: 23
            "nop;" // nop: 24
            "nop;" // nop: 25
            "nop;" // nop: 26
            "nop;" // nop: 27
            "nop;" // nop: 28
            "nop;" // nop: 29
            "nop;" // nop: 30
            "nop;" // nop: 31
            "nop;" // nop: 32
            "nop;" // nop: 33
            "nop;" // nop: 34
            "nop;" // nop: 35
            "nop;" // nop: 36
            "nop;" // nop: 37
            "nop;" // nop: 38
            "nop;" // nop: 39
            "nop;" // nop: 40
            "nop;" // nop: 41
            "nop;" // nop: 42
            "nop;" // nop: 43
            "nop;" // nop: 44
            "nop;" // nop: 45
            "nop;" // nop: 46
            "nop;" // nop: 47
            "srli a3, a3, 1;" // shift a3 >> 1
            "j R;" // Jump to end:
            "O:" //branch for 1 encoding
            "s32i %3, %1, 0;" // send to register output"
            "nop;" // nop: 1; Nop x52
            "nop;" // nop: 2
            "nop;" // nop: 3
            "nop;" // nop: 4
            "nop;" // nop: 5
            "nop;" // nop: 6
            "nop;" // nop: 7
            "nop;" // nop: 8
            "nop;" // nop: 9
            "nop;" // nop: 10
            "nop;" // nop: 11
            "nop;" // nop: 12
            "nop;" // nop: 13
            "nop;" // nop: 14
            "nop;" // nop: 15
            "nop;" // nop: 16
            "nop;" // nop: 17
            "nop;" // nop: 18
            "nop;" // nop: 19
            "nop;" // nop: 20
            "nop;" // nop: 21
            "nop;" // nop: 22
            "nop;" // nop: 23
            "nop;" // nop: 24
            "nop;" // nop: 25
            "nop;" // nop: 26
            "nop;" // nop: 27
            "nop;" // nop: 28
            "nop;" // nop: 29
            "nop;" // nop: 30
            "nop;" // nop: 31
            "nop;" // nop: 32
            "nop;" // nop: 33
            "nop;" // nop: 34
            "nop;" // nop: 35
            "nop;" // nop: 36
            "nop;" // nop: 37
            "nop;" // nop: 38
            "nop;" // nop: 39
            "nop;" // nop: 40
            "nop;" // nop: 41
            "nop;" // nop: 42
            "nop;" // nop: 43
            "nop;" // nop: 44
            "nop;" // nop: 45
            "nop;" // nop: 46
            "nop;" // nop: 47
            "nop;" // nop: 48
            "nop;" // nop: 49
            "nop;" // nop: 50
            "nop;" // nop: 51
            "nop;" // nop: 52
            "s32i %2, %3, 0;" //send to register output negate
            "nop;" // nop: 1; Nop x24 - 8
            "nop;" // nop: 2
            "nop;" // nop: 3
            "nop;" // nop: 4
            "nop;" // nop: 5
            "nop;" // nop: 6
            "nop;" // nop: 7
            "nop;" // nop: 8
            "nop;" // nop: 9
            "nop;" // nop: 10
            "nop;" // nop: 11
            "nop;" // nop: 12
            "nop;" // nop: 13
            "nop;" // nop: 14
            "nop;" // nop: 15
            "nop;" // nop: 16
            "srli a3, a3, 1;" // shift a3 >> 1
            "j R;"
            "E:"
            "movi a3, 128;"
            : "+r" (WS2812B_Controller::curr_led[0]), "+r" (output_reg_addr_set), "r+" (output_reg_addr_clear), "r+" (pin_hex)
        );
        WS2812B_Controller::curr_led++;
    }
    WS2812B_Controller::curr_led = WS2812B_Controller::curr_led - temp_length; // setting curr_led back to index 0

}

/*!
  @brief   Change length of the led strip
  @param   length  New length of the strip
*/
void WS2812B_Controller::init_strip_length(uint8_t length) {
    free(WS2812B_Controller::curr_led);

    WS2812B_Controller::curr_led = (uint8_t*)malloc(length * 3);
    WS2812B_Controller::length = length;
}

/*!
  @brief   Change led color of all lights on the strip.
  @param   r  Red color value for RGB usecase
  @param   g  Green color value for RGB usecase
  @param   b  Blue color value for RGB usecase
  @param   n  Used as entry point when needed
  @note    For changing only one single light refer to change_led_color.
           n is only used after initializing more lights on led strip to
           set them to standard light output. Otherwise the changing of a
           whole strip is started at 0.
*/
void WS2812B_Controller::change_led_color_all(uint8_t r, uint8_t g, uint8_t b, uint8_t n) {
    for(; n < WS2812B_Controller::length; n++) {
        WS2812B_Controller::change_led_color(n, r, b, g);
    }
}

/*!
  @brief   Change led color of one particular position on the strip.
  @param   n  position of light to be changed on the led strip. Needs to be
              formatted from 0 - length-1.
  @param   r  Red color value for RGB usecase
  @param   g  Green color value for RGB usecase
  @param   b  Blue color value for RGB usecase
  @note    For using only one single color refer to change_led_color_all
*/
void WS2812B_Controller::change_led_color(uint8_t n, uint8_t r, uint8_t g, uint8_t b) {
    if(n < WS2812B_Controller::length) {
        uint8_t *position_temp;
        position_temp = &WS2812B_Controller::curr_led[n*3];

        position_temp[R_OFFSET] = r;
        position_temp[G_OFFSET] = g;
        position_temp[B_OFFSET] = b;
    }
}
