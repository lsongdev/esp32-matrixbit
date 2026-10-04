#pragma once
#include <stdint.h>
using gpio_num_t = int;
enum gpio_mode_t { GPIO_MODE_INPUT, GPIO_MODE_OUTPUT };
int gpio_set_level(gpio_num_t pin, uint32_t level);
int gpio_set_direction(gpio_num_t pin, gpio_mode_t mode);
