#include"WS2812B_Controller.h"

WS2812B_Controller::WS2812B_Controller(uint8_t pinnumber) : curr_led(NULL) {
    WS2812B_Controller::set_pin(pinnumber);
    this->reserved_channels[RMT_CHANNEL_MAX];
    srand( (unsigned)time(NULL) );
}

WS2812B_Controller::~WS2812B_Controller() {
    free(WS2812B_Controller::curr_led);
}

void WS2812B_Controller::set_pin(uint8_t pinnumber) {   
        pinMode(WS2812B_Controller::curr_pin_out, INPUT); // INPUT is equivalent to 0x01 | 0000 0001b
        pinMode(pinnumber, OUTPUT); // OUTPUT is equivalent to 0x03 | 0000 0011b
        digitalWrite(pinnumber, LOW); // LOW is equivalent to 0x0 | 0000 0000b and HIGH to 0x1 | 0001 0000b
        WS2812B_Controller::curr_pin_out = pinnumber;
        WS2812B_Controller::curr_GPIO = pin_to_GPIO_index.find(pinnumber)->second;
        (WS2812B_Controller::curr_pin_out > 0x1F) ? WS2812B_Controller::first_GPIO_registers == false : WS2812B_Controller::first_GPIO_registers == true;
}

void WS2812B_Controller::start_light() {
  rmt_channel_t channel = RMT_CHANNEL_MAX;
    for (size_t i = 0; i < RMT_CHANNEL_MAX; i++) {
        if (!this->reserved_channels[i]) {
            this->reserved_channels[i] = true;
            channel = (rmt_channel_t)i;
            break;
        }
    }
  if(channel == RMT_CHANNEL_MAX) {
    return;
  }
  //rmt_config_t config = RMT_DEFAULT_CONFIG_TX((gpio_num_t)WS2812B_Controller::curr_pin_out, channel);
  rmt_config_t config = {
        .rmt_mode = RMT_MODE_TX,
        .channel = channel,
        .gpio_num = (gpio_num_t)WS2812B_Controller::curr_pin_out,
        .clk_div = 2,
        .mem_block_num = 1,
        .tx_config = {
            .carrier_freq_hz = 38000,
            .carrier_level = RMT_CARRIER_LEVEL_HIGH,
            .idle_level = RMT_IDLE_LEVEL_LOW,
            .carrier_duty_percent = 33,
            .carrier_en = false,
            .loop_en = false,
            .idle_output_en = true,
        }
    };
  rmt_config(&config);
  rmt_driver_install(config.channel, 0, 0);
  uint32_t counter_clk_hz = 0;
  rmt_get_counter_clock(channel, &counter_clk_hz);
  float ratio = (float)counter_clk_hz / 1e9;
#define T0H_NS (400)
#define T0L_NS (850)
#define T1H_NS (800)
#define T1L_NS (450)
  t0h_ticks = (uint32_t)(ratio * T0H_NS);
  t0l_ticks = (uint32_t)(ratio * T0L_NS);
  t1h_ticks = (uint32_t)(ratio * T1H_NS);
  t1l_ticks = (uint32_t)(ratio * T1L_NS);
  //translate and sending is missing
  rmt_translator_init(config.channel, WS2812B_Controller::ws2812b_rmt);
  rmt_write_sample(config.channel, this->curr_led, (size_t)WS2812B_Controller::length*3, true);
  rmt_wait_tx_done(config.channel, pdMS_TO_TICKS(100));
  rmt_driver_uninstall(config.channel);
  reserved_channels[channel] = false;
  gpio_set_direction(pin_to_GPIO_index.find(WS2812B_Controller::curr_pin_out)->second, GPIO_MODE_OUTPUT);
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

void WS2812B_Controller::change_led_color_random(uint8_t n) {
  if(n<WS2812B_Controller::length) {
    uint8_t r = (uint8_t)std::rand() % 255;
    uint8_t g = (uint8_t)std::rand() % 255;
    uint8_t b = (uint8_t)std::rand() % 255;

    uint8_t *position_temp = &WS2812B_Controller::curr_led[n*3];
    position_temp[R_OFFSET] = r;
    position_temp[G_OFFSET] = g;
    position_temp[B_OFFSET] = b;
  }
}
void WS2812B_Controller::change_led_color_random_all() {
  for(int n = 0; n < WS2812B_Controller::length; n++) {
    WS2812B_Controller::change_led_color_random(n);
  }
}
void WS2812B_Controller::change_led_color_queue(uint8_t r, uint8_t g, uint8_t b) {
  uint8_t *lastpos = &WS2812B_Controller::curr_led[(WS2812B_Controller::length-1)*3];
  uint8_t *newpos =  &WS2812B_Controller::curr_led[(WS2812B_Controller::length-1)*3 - 3];

  for(int i = (WS2812B_Controller::length-1)*3 - 3; i >= 0; i-=3) {
    lastpos[R_OFFSET] = newpos[R_OFFSET];
    lastpos[G_OFFSET] = newpos[G_OFFSET];
    lastpos[B_OFFSET] = newpos[B_OFFSET]; 

    lastpos = newpos;
    newpos-=3;
  }

   lastpos[R_OFFSET] = r;
   lastpos[G_OFFSET] = g;
   lastpos[B_OFFSET] = b; 
}

void WS2812B_Controller::change_led_brightness_all(uint8_t b) {}
void WS2812B_Controller::change_led_brightness(uint8_t n, uint8_t b) {}
