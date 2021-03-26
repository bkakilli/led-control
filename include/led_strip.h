#ifndef LED_STRIP

#include <FastLED.h>
#include "cpt_city_palettes.h"

#define LED_TYPE WS2812B    /* I assume you have WS2812B leds, if not just change it to whatever you have */
#define BRIGHTNESS          255   /* Control the brightness of your leds */
#define SATURATION          255   /* Control the saturation of your leds */
#define COLOR_ORDER         GRB
#define MILLI_AMPS          200 // IMPORTANT: set the max milli-Amps of your power supply (4A = 4000mA)

enum AnimationType {SOLID_COLOR, PATTERN, MOVING_PATTERN};

class LedStrip {
public:
  virtual void update() = 0;
  // virtual void init(uint16_t num_leds) = 0;
  virtual void set_palette(CRGBPalette16 palette) = 0;
  virtual void set_solid_color(CRGB color) = 0;
  virtual uint16_t get_bpm() = 0;
  virtual void set_bpm(uint16_t bpm) = 0;
  virtual uint8_t get_brightness() = 0;
  virtual void set_brightness(uint8_t bpm) = 0;
};

template <uint8_t DATA_PIN>
class LedStripPort: public LedStrip {
public:
  LedStripPort(uint16_t num_leds) {
    _data_pin = DATA_PIN;
    _num_leds = num_leds;
    _stride = 255/num_leds;
    _motion_bpm = 10;
    _brightness = BRIGHTNESS;

    _animation_type = MOVING_PATTERN;
    _custom_palette = CPT_RAINBOW_GP;

    _leds = new CRGB[_num_leds];

    FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(_leds, _num_leds);         // for WS2812 (Neopixel)
    //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS); // for APA102 (Dotstar)
    FastLED.setDither(false);
    FastLED.setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(_brightness);
    FastLED.setMaxPowerInVoltsAndMilliamps(5, MILLI_AMPS);
    FastLED.clear();
    fill_solid(_leds, _num_leds, CRGB::Black);
  }
  ~LedStripPort() {
    delete _leds;
  }

  void set_palette(CRGBPalette16 palette) {
    _custom_palette = palette;
    _animation_type = MOVING_PATTERN;
  }
  
  void set_solid_color(CRGB color) {
    _solid_color = color;
    _animation_type = SOLID_COLOR;
    fill_solid(_leds, _num_leds, _solid_color);
  }

  /**
   * BPM getter and setter
   */
  void set_bpm(uint16_t bpm) {_motion_bpm = bpm;}
  uint16_t get_bpm() {return _motion_bpm;}

  /**
   * Brightness getter and setter
   */
  void set_brightness(uint8_t brightness) {
    _brightness = brightness;
    FastLED.setBrightness(_brightness);
  }
  uint8_t get_brightness() {return _brightness;}
  
  /**
   * Update the strip decoration
  */
  void update() {
    switch (_animation_type)
    {
    case SOLID_COLOR:
      fill_solid(_leds, _num_leds, _solid_color);
      break;
    case MOVING_PATTERN:
      fill_palette(_leds, _num_leds, beat8(_motion_bpm), _stride, _custom_palette, BRIGHTNESS, LINEARBLEND);
      break;
    
    default:
      break;
    }
  }

private:
  CRGB *_leds;
  uint8_t _data_pin;
  uint16_t _num_leds;
  uint8_t _stride;
  uint16_t _motion_bpm;
  uint8_t _brightness;
  CRGBPalette16 _custom_palette;
  CRGB _solid_color;

  int _animation_type;
};
#endif