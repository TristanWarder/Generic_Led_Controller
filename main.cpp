#include <pico/time.h>
#include <stdio.h>
#include <cstdint>
#include "PicoLedController.hpp"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

#include <PicoLed.hpp>
#include <Effects/Marquee.hpp>
#include <Effects/Stars.hpp>
#include <Effects/Comet.hpp>
#include <Effects/Bounce.hpp>
#include <Effects/Particles.hpp>

#include <functional>
#include <vector>

#define LED_PIN 0
#define LED_LENGTH 63

#define BUTTON_PIN 1
#define KNOB_PIN 26

PicoLed::Color previous[LED_LENGTH];

void scrollStrip(PicoLed::PicoLedController* strip, bool flip = false) {
  PicoLed::Color previous[LED_LENGTH];
  auto num = strip->getNumLeds();
  for(int i = 0; i < num; i++) {
    previous[i] = strip->getPixelColor(i);
  }
  if(flip) {
    for(int i = num - 1; i > 0; i--) {
      if(i - 1 < 0) {
        // Wrap to end
        strip->setPixelColor(num - 1, previous[i]);
      } else {
        if(i + 1 >= num - 1) {
          // Wrap to beginning
          strip->setPixelColor(i, previous[0]);
        } else {
          // Move pixel back
          strip->setPixelColor(i, previous[i + 1]);
        }
        // Move next pixel back
        strip->setPixelColor(i - 1, previous[i]);
      }
    }
    return;
  }
  for(int i = 0; i < num; i++) {
    if(i + 1 >= num) {
      // Wrap to beginning
      strip->setPixelColor(0 + i - num, previous[i]);
    } else {
      if(i - 1 < 0) {
        // Wrap to end
        strip->setPixelColor(i, previous[num + (i - 1)]);
      } else {
        // Move pixel forward
        strip->setPixelColor(i, previous[i - 1]);
      }
      // Move previous pixel forward
      strip->setPixelColor(i + 1, previous[i]);
    }
  }
}

void fillAlternating(PicoLed::PicoLedController* strip, PicoLed::Color first, PicoLed::Color second, int swaps, bool startFirst = true) {
  bool fillFirst = startFirst;
  int length = strip->getNumLeds();
  for(int i = 0; i < length; i += (length / swaps)) {
    if(fillFirst) {
      strip->fill(first, i, length / swaps);
    }
    else {
      strip->fill(second, i, length / swaps);
    }
    fillFirst = !fillFirst;
  }
}

struct Animation {
  std::function<void()> init = nullptr;
  std::function<void()> animate = nullptr;
  uint32_t delay = 10;
};

uint32_t lastAnimationTimestamp;

int main()
{
  stdio_init_all();
  sleep_ms(1000);

  gpio_set_dir(BUTTON_PIN, GPIO_IN);
  gpio_pull_up(BUTTON_PIN);

  adc_init();
  adc_gpio_init(KNOB_PIN);
  adc_select_input(0);

  bool buttonState = false;

  int32_t currentAnimation = -1;
  int32_t targetAnimation = 0;
  uint32_t currentAnimationDelay = 1;

  auto mainStrip = PicoLed::addLeds<PicoLed::WS2812B>(pio0, 0, LED_PIN, LED_LENGTH, PicoLed::FORMAT_GRB);

  std::vector<Animation> animations{};

  PicoLed::Comet heartbeat(mainStrip, {255, 0, 0}, 40.0, 40.0, 40.0);
  Animation heartbeatComet {
    nullptr,
    [&]() {
      heartbeat.animate();
    },
    1
  };

  Animation heartbeatScroll {
    [&]() {
      mainStrip.clear();
      fillAlternating(&mainStrip, {255, 0, 0}, {0, 0, 0}, 3);
    },
    [&]() {
      scrollStrip(&mainStrip);
    },
    40
  };

  animations.push_back(heartbeatComet);
  animations.push_back(heartbeatScroll);

  // 0. Initialize LED strip
  mainStrip.setBrightness(127);

  mainStrip.clear();

  mainStrip.show();

  lastAnimationTimestamp = to_ms_since_boot(get_absolute_time());
  while(true) {
    uint32_t currentTimestamp = to_ms_since_boot(get_absolute_time());
    Animation* current = nullptr;
    // If animation has been changed, move pointer and call animation init
    if(targetAnimation != currentAnimation) {
      current = &animations[targetAnimation];
      if(current->init != nullptr) current->init();
      currentAnimation = targetAnimation;
      mainStrip.show();
      lastAnimationTimestamp = currentTimestamp;
    } else {
      // Check if animation delay has elapsed, if so animate
      current = &animations[currentAnimation];
      if(currentTimestamp - lastAnimationTimestamp > current->delay) {
        if(current->animate != nullptr) current->animate();
        mainStrip.show();
        lastAnimationTimestamp = to_ms_since_boot(get_absolute_time());
      }

    }

    sleep_ms(10);
  }

  return 0;
}
