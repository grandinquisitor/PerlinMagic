#include <avr/power.h>
#include <avr/sleep.h>

#ifdef __AVR__
#include <avr/pgmspace.h>
#else
#define PROGMEM
#define pgm_read_byte(addr) (*(const unsigned char *)(addr))
#endif

#include <FastLED.h>
#include <Bounce2.h>
#include <jled.h>


#define NEOPIXEL_DATA_PIN 3
#define POWER_SWITCH_PIN 2
#define RED_LED_PIN 9


#define DEBUG_SERIAL_
#ifdef DEBUG_SERIAL
#define Sprintlns(a) (Serial.println(F(a)))
#define Sprints(a) (Serial.print(F(a)))
#define Sprintln(...) (Serial.println(__VA_ARGS__))
#define Sprint(...) (Serial.print(__VA_ARGS__))
#define Sflush() Serial.flush()
#else
#define Sprintln(...)
#define Sprint(...)
#define Sprintlns(a)
#define Sprints(a)
#define Sflush()
#endif


#define DEFAULT_BRIGHTNESS         180


// In my setup I had a few "extra" leds at the beginning that needed to be skipped.

#define NUM_LEDS_VIRTUAL    54
#define NUM_LEDS 60
#define LED_OFFSET (NUM_LEDS - NUM_LEDS_VIRTUAL)
CRGB leds[NUM_LEDS];
CRGB virtualLeds = leds + LED_OFFSET;



// http://www.color-hex.com/color-palette/13860

DEFINE_GRADIENT_PALETTE( twtr1_gp ) {
  0,    79, 52, 255,  // darker blue
  100,   154, 212, 255, // baby blue
  200,    16, 223, 253, // cyan
  225,   198, 254, 255, // light cyan
  255,   255, 255, 255  // white
}; //full white

const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM =
{
  CRGB::Red,
  CRGB::Gray, // 'white' is too bright compared to red and blue
  CRGB::Blue,
  CRGB::Black,

  CRGB::Red,
  CRGB::Gray,
  CRGB::Blue,
  CRGB::Black,

  CRGB::Red,
  CRGB::Red,
  CRGB::Gray,
  CRGB::Gray,
  CRGB::Blue,
  CRGB::Blue,
  CRGB::Black,
  CRGB::Black
};

const TProgmemPalette16 myBlackAndwhitePalette_p PROGMEM =
{
  CRGB::White,
  CRGB::Black,
  CRGB::Black,
  CRGB::Black,
  CRGB::White,
  CRGB::Black,
  CRGB::Black,
  CRGB::Black,
  CRGB::White,
  CRGB::Black,
  CRGB::Black,
  CRGB::Black,
  CRGB::White,
  CRGB::Black,
  CRGB::Black,
  CRGB::Black
};

const TProgmemPalette16 myBlueAndwhitePalette_p PROGMEM =
{
  CRGB::Blue,
  CRGB::Black,
  CRGB::Black,
  CRGB::Black,
  CRGB::Blue,
  CRGB::Black,
  CRGB::Black,
  CRGB::Black,
  CRGB::Blue,
  CRGB::Black,
  CRGB::Black,
  CRGB::Black,
  CRGB::Blue,
  CRGB::Black,
  CRGB::Black,
  CRGB::Black
};

const TProgmemPalette16 myGoldAndwhitePalette_p PROGMEM =
{
  CRGB::Gold,
  CRGB::Black,
  CRGB::Black,
  CRGB::Black,
  CRGB::Gold,
  CRGB::Black,
  CRGB::Black,
  CRGB::Black,
  CRGB::Gold,
  CRGB::Black,
  CRGB::Black,
  CRGB::Black,
  CRGB::Gold,
  CRGB::Black,
  CRGB::Black,
  CRGB::Black,
};

DEFINE_GRADIENT_PALETTE( goldSpectrum_p ) {
  0,    255, 215, 0,  // gold
  120,    255, 215, 0,  // gold
  130,   0, 0, 0, // black
  150,    255, 215, 0,  // gold
  200,    255, 215, 0,  // gold
  210,    255, 228, 225,  // mistyrose
  220,    255, 215, 0,  // gold
  255,    255, 215, 0,  // gold
};

const CRGBPalette16 fixed_palettes[] = {
  twtr1_gp,
  goldSpectrum_p,
  myRedWhiteBluePalette_p,
  RainbowColors_p,
  RainbowStripeColors_p,
  myBlackAndwhitePalette_p,
  myBlueAndwhitePalette_p,
  myGoldAndwhitePalette_p,
  LavaColors_p
};

#define NUM_PALETTES 9





// define parameter register.

#define BRIGHTNESS_CNTRL 0
#define COLOR_CNTRL 1
#define SCALE_CNTRL 2
#define SPEED_CNTRL 3
#define BLEND_CNTRL 4
#define BEAT_CNTRL  5
#define PALETTE_DISP_CNTRL 6
#define ROTATE_CNTRL 7
#define NUM_CNTRLS 8

const struct selector_def {
  const bool use_bounds; // saturate at max/min or allow overflow
  const uint8_t mn; // min (inclusive)
  const uint8_t mx; // max (inclusive)
  const uint8_t incr; // increment
  const uint8_t def; // default
} selector_defs[NUM_CNTRLS] = {
  {1, 0, 255, 16, DEFAULT_BRIGHTNESS}, // BRIGHTNESS_CNTRL
  {0, 0, NUM_PALETTES + 1, 1, 0}, // COLOR_CNTRL
  {1, 0, 127, 1, 29}, // SCALE_CNTRL
  {1, 0, 255, 5, 20}, // SPEED_CNTRL
  {1, 1, 128, 8, 24}, // BLEND_CNTRL
  {0, 0, 2, 1, 1}, // BEAT_CNTRL
  {0, 0, 3, 1, 1}, // PALETTE_DISP_CNTRL
  {0, 0, NUM_LEDS_VIRTUAL, (NUM_LEDS_VIRTUAL >> 8) | 1, 0} // ROTATE_CNTRL
};

uint8_t selector_values[NUM_CNTRLS];




JLed builtinLED = JLed(LED_BUILTIN);
JLed redLED = JLed(RED_LED_PIN);




// Palette definitions
CRGBPalette16 currentPalette = RainbowColors_p;
CRGBPalette16 targetPalette = RainbowColors_p;



// rotary encoder pin change interrupt handler
volatile byte seqA = 0;
volatile byte seqB = 0;
volatile byte cntLeft = 0;
volatile byte cntRight = 0;

ISR (PCINT2_vect) {

  // use raw port manipulation for speed
  boolean A_val = bit_is_set(PIND, PD6);
  boolean B_val = bit_is_set(PIND, PD7);

  // Record the A and B signals in seperate sequences
  seqA <<= 1;
  seqA |= A_val;

  seqB <<= 1;
  seqB |= B_val;

  // Mask the MSB four bits
  seqA &= 0b00001111;
  seqB &= 0b00001111;

  // Compare the recorded sequence with the expected sequence
  if (seqA == 0b00001001 && seqB == 0b00000011) {
    cntLeft++;
  }

  if (seqA == 0b00000011 && seqB == 0b00001001) {
    cntRight++;
  }
}


uint8_t saved_brightness_cntrl = selector_defs[BRIGHTNESS_CNTRL].def;

void resetDefaults() {
  saved_brightness_cntrl = selector_values[BRIGHTNESS_CNTRL];
  for (uint8_t i = 0; i < NUM_CNTRLS; ++i) {
    if (i != ROTATE_CNTRL) {
      selector_values[i] = selector_defs[i].def;
    }
  }
}

void randomize() {
  selector_values[BRIGHTNESS_CNTRL] = saved_brightness_cntrl;
  for (uint8_t i = 0; i < NUM_CNTRLS; ++i) {
    if (i != ROTATE_CNTRL && i != BRIGHTNESS_CNTRL) {
      selector_values[i] = random8(selector_defs[i].mn, selector_defs[i].mx);
    }
  }
}

void handlePowerBtn() {

}

void sleepNow(void)
{
  // Set pin 2 as interrupt and attach handler:
  attachInterrupt(digitalPinToInterrupt(POWER_SWITCH_PIN), handlePowerBtn, FALLING);
  delay(100);
  //
  // Choose our preferred sleep mode:
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  //
  // Set sleep enable (SE) bit:
  sleep_enable();
  //
  // Put the device to sleep:
  sleep_mode();
  //
  // Upon waking up, sketch continues from this point.
  sleep_disable();

  detachInterrupt(digitalPinToInterrupt(POWER_SWITCH_PIN));
}


void setup() {
  pinMode(POWER_SWITCH_PIN, INPUT_PULLUP);

  // rotary encoder setup:
  pinMode(6, INPUT_PULLUP);
  pinMode(7, INPUT_PULLUP);

  // http://gammon.com.au/interrupts
  PCMSK2 |= bit(PCINT22) | bit(PCINT23);
  PCIFR  |= bit(PCIF2);   // clear any outstanding interrupts
  PCICR  |= bit(PCIE2);   // enable pin change interrupts for A0 to A5

  // address selection switches and reset button
  pinMode(14, INPUT_PULLUP);
  pinMode(15, INPUT_PULLUP);
  pinMode(16, INPUT_PULLUP);
  pinMode(17, INPUT_PULLUP);

  resetDefaults();

#ifdef DEBUG_SERIAL
  Serial.begin(9600);

  delay(500); // wait for console opening, don't block

  Sprintlns("starting up");
#endif

  FastLED.addLeds<WS2812B, NEOPIXEL_DATA_PIN, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(DEFAULT_BRIGHTNESS);
  FastLED.setMaxPowerInVoltsAndMilliamps(5, 550);

  FastLED.clear();
  FastLED.show();

  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  // intro animation

  for (uint8_t i = 0; i < NUM_LEDS; ++i) {
    digitalWrite(LED_BUILTIN, i & 1);
    leds[i] = CRGB::Red;
    FastLED.show();
    FastLED.delay(10);
    FastLED.clear();
  }
  for (uint8_t i = 0; i < NUM_LEDS; ++i) {
    digitalWrite(LED_BUILTIN, !(i & 1));
    leds[NUM_LEDS - 1 - i] = CRGB::Red;
    FastLED.show();
    FastLED.delay(10);
    FastLED.clear();
  }

  digitalWrite(LED_BUILTIN, LOW);

  FastLED.clear();
  FastLED.show();

  if (digitalRead(POWER_SWITCH_PIN) == LOW) {
    delay(200);

    FastLED.setBrightness(0);

    for (uint8_t i = 0; i < 100; ++i) {
      run_fill();
      FastLED.setBrightness(scale8(ease8InOutApprox(map(i, 0, 100, 0, 255)), DEFAULT_BRIGHTNESS));
      FastLED.show();
      FastLED.delay(10);
    }
  }
  FastLED.setBrightness(DEFAULT_BRIGHTNESS);
}


uint32_t real_z = 0;

void loop() {

  static bool isOff = true;
  // off button
  if (digitalRead(POWER_SWITCH_PIN) == HIGH) {
    if (isOff) {
      FastLED.clear();
      FastLED.show();
      sleepNow();
      return;
    }

    isOff = true;

    Sprintlns("turning off");
    Sflush();

    // off animation
    const uint16_t duration = 600;
    const uint8_t fps = 40;

    // note: math below doesn't handle rounding, edge cases
    const uint8_t spacing = 1000 / fps;
    const uint8_t numberOfSteps = duration / spacing;
    const uint8_t &bright = selector_values[BRIGHTNESS_CNTRL];

    redLED.On().FadeOff(duration);
    for (uint8_t j = 0; j < numberOfSteps; ++j) {
      FastLED.setBrightness(map(ease8InOutApprox(map(j, 0, numberOfSteps, 0, 255)), 255, 0, 0, bright));

      FastLED.show();
      redLED.Update();
      FastLED.delay(spacing);
    }
    redLED.Off();
    redLED.Update();
    FastLED.setBrightness(0);
    FastLED.show();
    delay(1);
    // clear leds and sleep forever, set interrupt
    sleepNow();

    // wake animation
    Sprintlns("woke up");
    for (uint8_t j = 0; j < numberOfSteps; ++j) {
      FastLED.setBrightness(map(ease8InOutApprox(map(j, 0, numberOfSteps, 0, 255)), 0, 255, 0, bright));
      FastLED.show();
      FastLED.delay(spacing);
    }
    FastLED.setBrightness(bright);

    return;
  } else {
    isOff = false;
  }

  // read address selector switches
  // debounce technique: http://drmarty.blogspot.com/2009/05/best-switch-debounce-routine-ever.html
  static uint8_t PINC_T1 = 0xff;
  static uint8_t PINC_T2 = 0xff;
  static uint8_t PINC_DEB = 0xff;

  EVERY_N_MILLISECONDS(1) {
    uint8_t PINC_T0 = PINC;

    PINC_DEB = PINC_DEB & (PINC_T0 | PINC_T1 | PINC_T2) | (PINC_T0 & PINC_T1 & PINC_T2);
    PINC_T2 = PINC_T1;
    PINC_T1 = PINC_T0;
  }

  if (cntLeft > 0) {
    Sprintlns("got left");
  }

  if (cntRight > 0) {
    Sprintlns("got right");
  }

  static uint8_t selector;
  selector = PINC_DEB & (_BV(PC0) | _BV(PC1) | _BV(PC2));
  static int8_t lastSelector = -1;

  if (selector != lastSelector) {
    Serial.print("new selector ");
    Serial.println(selector);
    Serial.print("current value ");
    Serial.println(selector_values[selector]);
    lastSelector = selector;
  }

  const struct selector_def &selector_def = selector_defs[selector];
  uint8_t &val = selector_values[selector];
  const uint8_t &mx = selector_def.mx;
  const uint8_t &mn = selector_def.mn;
  const uint8_t &incr = selector_def.incr;

  bool gotUpdate = false;

  bool handledReset = false;
  static uint32_t lastReset = 0;
  static bool resetWasDown = false;

  if (!bit_is_set(PINC_DEB, PC3)) {
    if (!resetWasDown) {
      resetWasDown = true;
      uint32_t diff = millis() - lastReset;
      if (diff > 50 && diff < 500) {
        // this switch is REALLY bouncey.
        Sprintln(diff);
        // double tap!
        Sprintlns("randomizing");
        handledReset = true;
        redLED.Blink(100, 100).Repeat(6);
        randomize();
        lastReset = millis();
      } else if (diff > 500) {
        // if reset, then set everything to def
        // to debounce: check if every value doesn't change after n millis
        Sprintlns("resetting");
        redLED.Blink(200, 200).Repeat(3);
        resetDefaults();
        handledReset = true;
        lastReset = millis();
      }
    }
  } else {
    resetWasDown = false;
  }

  if (cntLeft || cntRight) {
    Sprintln(PINC_DEB, BIN);
    Sprintln(selector, BIN);

    gotUpdate = true;

    builtinLED.On().DelayAfter(100).Off();

    if (cntRight) {
      if (mx - incr < val) { // = val + incr > mx
        if (selector_def.use_bounds) {
          // saturate, don't overflow
          val = mx;
          redLED.Blink(100, 100).Repeat(2);
        } else {
          // overflow
          val = max(mx - val + (incr - 1), mn);
          redLED.Off().Breathe(400).Repeat(1);
        }
      } else {
        val += incr;
      }
      --cntRight;
    } else {
      if (mn + incr > val) { // = val - incr < mn
        if (selector_def.use_bounds) {
          // saturate, don't overflow
          val = mn;
          redLED.Blink(100, 100).Repeat(2);
        } else {
          // overflow
          val = mx - ((incr - 1) - (val - mn));
          redLED.Off().Breathe(400).Repeat(1);
        }
      } else {
        val -= incr;
      }
      --cntLeft;
    }

    Sprints("new value: ");
    Sprint(val);
    Sprint(' ');
    Sprintln(val, BIN);

    selector_values[selector] = val;
  }

  const uint8_t &z_speed = selector_values[SPEED_CNTRL];
  const uint8_t &paletteChoice = selector_values[COLOR_CNTRL];
  const uint8_t &blendSpeed = selector_values[BLEND_CNTRL];

  const bool useStaticPalette = (paletteChoice > 1);
  const bool useRandomPalette = (paletteChoice == 0);
  const bool useRandomPredefinedPalette = (paletteChoice == 1);
  const uint8_t paletteIndexOffset = 2;

  static uint8_t currentBlendSpeed = blendSpeed;

  if (gotUpdate || handledReset) {
    if (selector == BRIGHTNESS_CNTRL || handledReset) {
      FastLED.setBrightness(selector_values[BRIGHTNESS_CNTRL]);
    }
    if (selector == COLOR_CNTRL || handledReset) {
      currentBlendSpeed = 1;
      if (useStaticPalette) {
        targetPalette = fixed_palettes[selector_values[COLOR_CNTRL] - 2];
      }
    }
  }

  EVERY_N_MILLISECONDS(10) {
    nblendPaletteTowardPalette(currentPalette, targetPalette, currentBlendSpeed);
    run_fill();
    if (currentBlendSpeed < blendSpeed) {
      ++currentBlendSpeed;
    }
  }

  EVERY_N_MILLISECONDS(1) {
    real_z += z_speed;
  }

  EVERY_N_SECONDS(5) {
    if (useRandomPalette) {
      targetPalette = CRGBPalette16(CHSV(random8(), 255, random8(128, 255)), CHSV(random8(), 255, random8(128, 255)), CHSV(random8(), 192, random8(128, 255)), CHSV(random8(), 255, random8(128, 255)));
    } else if (useRandomPredefinedPalette) {
      targetPalette = fixed_palettes[random8(NUM_PALETTES)];
    }
  }

  redLED.Update();
  builtinLED.Update();
  FastLED.show();
}

// inspiration:
// https://gist.github.com/hsiboy/f9ef711418b40e259b06
// https://gist.github.com/StefanPetrick/5e853bea959e738bc6c2c2026683e3a4
// https://github.com/atuline/FastLED-Demos / inoise8_pal_demo

void run_fill() {
  static uint16_t dist;

  const uint8_t &zoom = selector_values[SCALE_CNTRL];
  const uint8_t &beatFade = selector_values[BEAT_CNTRL];
  const uint8_t &paletteDisp = selector_values[PALETTE_DISP_CNTRL];
  const uint8_t &rotate = selector_values[ROTATE_CNTRL];
  const uint8_t &paletteChoice = selector_values[COLOR_CNTRL];

  static uint16_t scale = zoom << 3;
  if (scale != zoom << 3) {
    scale = lerp16by8(scale, zoom << 3, 8);
  }

  static uint16_t shift_x = 0;
  static uint16_t shift_y = 0;

  static int8_t x_direction = 1;
  static int8_t y_direction = 1;
  static bool wasZero = false;
  uint8_t nextDist = 0;

  switch (beatFade) {
    case 0:
      break;
    case 1:
      // rotate, where speed = sine
      nextDist = beatsin8(10, 1, 4);
      if (nextDist == 1) {
        if (wasZero == false) {

          if (random8(0, 8) == 0) {
            x_direction *= -1;
          }
          if (random8(0, 8) == 0) {
            y_direction *= -1;
          }
          wasZero = true;
        }
      } else {
        wasZero = false;
      }
      if (x_direction == 1) {
        shift_x += nextDist;
      } else {
        shift_x -= nextDist;
      }
      if (y_direction == 1) {
        shift_y += nextDist;
      } else {
        shift_y -= nextDist;
      }
      break;
    case 2:
      // x sways, y slowly moves up
      shift_x = beatsin8(10, 1, 4);
      --shift_y;
      break;
  }

  const bool noiseBri = paletteDisp & 1;
  const TBlendType blendType = (paletteDisp & 2) ? NOBLEND : LINEARBLEND;

  for (uint8_t i = LED_OFFSET; i < NUM_LEDS; i++) {

    uint8_t theta = map(i - LED_OFFSET + rotate, 0, NUM_LEDS_VIRTUAL, 0, 255);
    uint32_t real_x = (cos8(theta) + shift_x) * scale;
    uint32_t real_y = (sin8(theta) + shift_y) * scale;

    uint8_t noise = inoise16(real_x, real_y, real_z) >> 8;

    uint8_t index = noise * 3;
    uint8_t bri   = noise;

    leds[i] = blend(leds[i], ColorFromPalette(currentPalette, index, noiseBri ? bri : 255, blendType), 200);
  }

  // extra offset leds in my setup
  for (uint8_t i = 0; i < LED_OFFSET; i++) {
    leds[i] = CRGB::Black;
  }

  FastLED.show();
}
