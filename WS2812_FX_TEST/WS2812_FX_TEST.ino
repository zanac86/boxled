/*
  Скетч создан на основе FASTSPI2 EFFECTS EXAMPLES автора teldredge (www.funkboxing.com)
  А также вот этой статьи https://www.tweaking4all.com/hardware/arduino/adruino-led-strip-effects/#cylon
  Доработан, переведён и разбит на файлы 2017 AlexGyver
  Смена выбранных режимов в случайном порядке через случайное время

  Переделан для нескольких вариантов
  можно подключить touch для переключения эффектов

*/


#include "FastLED.h"          // библиотека для работы с лентой

#define LED_COUNT 8          // число светодиодов в кольце/ленте 8 или 12

// ESP
//#define LED_DT 2
//#define BUTTON_PIN 4

// pro micro
#define LED_DT 10
#define BUTTON_PIN 2

#ifdef BUTTON_PIN
#include "GyverButton.h"
GButton touch(BUTTON_PIN, LOW_PULL, NORM_OPEN);
#endif

uint8_t bright_max_count = 3;
uint8_t brights[] = {10, 100, 250};
uint8_t bright_index = 0;

// эта яркость будет при включении
// если есть кнопка, то яркость переключается двойным касанием
// если нет кнопки, то лучше вписать побольше можно 150
// на максимальной яркости ток 0.06А на 16 светодиодов - 1А
uint8_t current_bright = brights[1];

uint8_t fav_modes[] = {
  2, 3, 4, 5, 6, 7, 8,
  11, 14, 15, 16,
  20, 23, 25, 27, 37
};

uint8_t num_modes = sizeof(fav_modes);       // получить количество "любимых" режимов (они все по 1 байту..)
unsigned long change_time = 0;
unsigned long last_change = 0;
unsigned long last_ticks_millis = 0;

uint8_t ledMode = 0;


uint8_t solid_colors[7][3] = {
  {255, 0, 0},
  {0, 255, 0},
  {0, 0, 255},
  {255, 255, 0},
  {0, 255, 255},
  {255, 0, 255},
  {255, 255, 255}
};

uint8_t solid_index = 0;


// цвета мячиков для режима
byte ballColors[3][3] = {
  {0xff, 0, 0},
  {0xff, 0xff, 0xff},
  {0   , 0   , 0xff}
};

// ---------------СЛУЖЕБНЫЕ ПЕРЕМЕННЫЕ-----------------
int BOTTOM_INDEX = 0;        // светодиод начала отсчёта
int TOP_INDEX = int(LED_COUNT / 2);
int EVENODD = LED_COUNT % 2;
struct CRGB leds[LED_COUNT];
int ledsX[LED_COUNT][3];     //-ARRAY FOR COPYING WHATS IN THE LED STRIP CURRENTLY (FOR CELL-AUTOMATA, MARCH, ETC)

int thisdelay = 20;          //-FX LOOPS DELAY VAR
int thisstep = 10;           //-FX LOOPS DELAY VAR
int thishue = 0;             //-FX LOOPS DELAY VAR
int thissat = 255;           //-FX LOOPS DELAY VAR

int idex = 0;                //-LED INDEX (0 to LED_COUNT-1
int ihue = 0;                //-HUE (0-255)
int ibright = 0;             //-BRIGHTNESS (0-255)
int isat = 0;                //-SATURATION (0-255)
int bouncedirection = 0;     //-SWITCH FOR COLOR BOUNCE (0-1)
float tcount = 0.0;          //-INC VAR FOR SIN LOOPS
int lcount = 0;              //-ANOTHER COUNTING VAR
uint16_t rainbow_index = 0;
// ---------------СЛУЖЕБНЫЕ ПЕРЕМЕННЫЕ-----------------

#include "LED_EFFECT_FUNCTIONS.h"
#include "UTILITY_FXNS.h"

void change_mode(int newmode);

void setup()
{
  Serial.begin(9600);              // открыть порт для связи
  // погасить светодиоды rx и tx на stm32
  //  pinMode(LED_BUILTIN_TX, INPUT);
  //  pinMode(LED_BUILTIN_RX, INPUT);
  //bitClear(DDRD,5);
  //bitClear(DDRB,0);

  // LEDS.addLeds<WS2813, LED_DT, GRB>(leds, LED_COUNT);  // настрйоки для нашей ленты (ленты на WS2811, WS2812, WS2812B)
  LEDS.addLeds<WS2812B, LED_DT, GRB>(leds, LED_COUNT);  // настрйоки для нашей ленты (ленты на WS2811, WS2812, WS2812B)

  LEDS.setBrightness(current_bright);  // ограничить максимальную яркость
  LEDS.setMaxPowerInVoltsAndMilliamps(5, 1000);

  setAll(0, 0, 0);          // погасить все светодиоды
  LEDS.show();                     // отослать команду

  Serial.print("Enable modes:");
  Serial.println(num_modes);

  randomSeed(analogRead(0));
}

void one_color_all(int cred, int cgrn, int cblu) {       //-SET ALL LEDS TO ONE COLOR
  for (int i = 0 ; i < LED_COUNT; i++ ) {
    leds[i].setRGB( cred, cgrn, cblu);
  }
}

uint8_t nextMode(uint8_t oldMode, bool useRandom)
{
  if (useRandom)
  {
    // случайный из списка любимых
    return fav_modes[random(0, num_modes - 1)];
  }
  // следующий из списка, если найду
  uint8_t m = 0;
  for (uint8_t i = 0; i < num_modes; i++)
  {
    if (fav_modes[i] == oldMode)
    {
      m = i;
      break;
    }
  }
  m = (m + 1) % num_modes;
  return fav_modes[m];
}

void loop() {
#ifdef BUTTON_PIN
  touch.tick();
#endif
  if (millis() - last_change > change_time) {
    change_time = random(10000, 30000);               // получаем новое случайное время до следующей смены режима
    //    change_time = 20000;
    ledMode = nextMode(ledMode, false);                // получаем новый случайный номер следующего режима
    change_mode(ledMode);                             // меняем режим через change_mode (там для каждого режима стоят цвета и задержки)
    last_change = millis();
    Serial.print("<");
    Serial.print(ledMode);
    Serial.println(">");
  } else
  {
#ifdef BUTTON_PIN
    if (touch.hasClicks()) {
      byte clicks = touch.getClicks();
      switch (clicks)
      {
        case 1:
          change_time = 60000;                // получаем новое случайное время до следующей смены режима
          ledMode = nextMode(ledMode, false);
          change_mode(ledMode);                             // меняем режим через change_mode (там для каждого режима стоят цвета и задержки)
          last_change = millis();
          Serial.print("<<");
          Serial.print(ledMode);
          Serial.println(">>");
          break;
        case 2:
          bright_index = (bright_index + 1) % bright_max_count;
          LEDS.setBrightness(brights[bright_index]);
          LEDS.show();
          break;
        case 3:
          change_time = 60000;
          ledMode = 100;
          one_color_all(solid_colors[solid_index][0], solid_colors[solid_index][1], solid_colors[solid_index][2]);
          solid_index = (solid_index + 1) % 7;
          break;
        case 4:
          change_time = 60000;
          ledMode = 37;
          change_mode(ledMode);                             // меняем режим через change_mode (там для каждого режима стоят цвета и задержки)
          break;
      }
    }
#endif
  }
  switch (ledMode) {
    case 999: break;                           // пазуа
    case  2: rainbow_fade(); break;            // плавная смена цветов всей ленты
    case  3: rainbow_loop(); break;            // крутящаяся радуга
    case  4: random_burst(); break;            // случайная смена цветов
    case  5: color_bounce(); break;            // бегающий светодиод
    case  6: color_bounceFADE(); break;        // бегающий паровозик светодиодов
    case  7: ems_lightsONE(); break;           // вращаются красный и синий
    case  8: ems_lightsALL(); break;           // вращается половина красных и половина синих
    case  9: flicker(); break;                 // случайный стробоскоп
    case 11: pulse_one_color_all_rev(); break; // пульсация со сменой цветов
    case 14: random_march(); break;            // безумие случайных цветов
    case 15: rwb_march(); break;               // белый синий красный бегут по кругу (ПАТРИОТИЗМ!)
    case 16: radiation(); break;               // пульсирует значок радиации
    case 20: pop_horizontal(); break;          // красные вспышки спускаются вниз
    case 23: rainbow_vertical(); break;        // радуга в вертикаьной плоскости (кольцо)
    case 25: random_color_pop(); break;        // безумие случайных вспышек
    case 26: ems_lightsSTROBE(); break;        // полицейская мигалка
    case 27: rgb_propeller(); break;           // RGB пропеллер
    case 29: matrix(); break;                  // зелёненькие бегают по кругу случайно
    case 30: new_rainbow_loop(); break;        // крутая плавная вращающаяся радуга
    case 37: rainbowCycle(thisdelay); break;                                        // очень плавная вращающаяся радуга
    case 38: TwinkleRandom(20, thisdelay, 1); break;                                // случайные разноцветные включения (1 - танцуют все, 0 - случайный 1 диод)
    case 39: RunningLights(0xff, 0xff, 0x00, thisdelay); break;                     // бегущие огни
    case 40: Sparkle(0xff, 0xff, 0xff, thisdelay); break;                           // случайные вспышки белого цвета
    case 41: SnowSparkle(0x10, 0x10, 0x10, thisdelay, random(100, 1000)); break;    // случайные вспышки белого цвета на белом фоне
    case 42: theaterChase(0xff, 0, 0, thisdelay); break;                            // бегущие каждые 3 (ЧИСЛО СВЕТОДИОДОВ ДОЛЖНО БЫТЬ КРАТНО 3)
    case 43: theaterChaseRainbow(thisdelay); break;                                 // бегущие каждые 3 радуга (ЧИСЛО СВЕТОДИОДОВ ДОЛЖНО БЫТЬ КРАТНО 3)
    case 44: Strobe(0xff, 0xff, 0xff, 10, thisdelay, 1000); break;                  // стробоскоп
    case 46: BouncingColoredBalls(3, ballColors); break;                            // прыгающие мячики цветные

  }
}

void change_mode(int newmode) {
  thissat = 255;
  switch (newmode) {
    case 0: one_color_all(0, 0, 0); LEDS.show(); break; //---ALL OFF
    case 1: one_color_all(255, 255, 255); LEDS.show(); break; //---ALL ON
    case 2: thisdelay = 90; break;                      //---STRIP RAINBOW FADE
    case 3: thisdelay = 80; thisstep = 10; break;       //---RAINBOW LOOP
    case 4: thisdelay = 50; break;                      //---RANDOM BURST
    case 5: thisdelay = 20; thishue = 0; break;         //---CYLON v1
    case 6: thisdelay = 80; thishue = 0; break;         //---CYLON v2
    case 7: thisdelay = 100; thishue = 0; break;         //---POLICE LIGHTS SINGLE
    case 8: thisdelay = 40; thishue = 0; break;         //---POLICE LIGHTS SOLID
    case 9: thishue = 120; thissat = 50; break;         //---STRIP FLICKER
    case 10: thisdelay = 15; thishue = 0; break;        //---PULSE COLOR BRIGHTNESS
    case 11: thisdelay = 30; thishue = 0; break;        //---PULSE COLOR SATURATION
    case 12: thisdelay = 60; thishue = 180; break;      //---VERTICAL SOMETHING
    case 13: thisdelay = 100; break;                    //---CELL AUTO - RULE 30 (RED)
    case 14: thisdelay = 80; break;                     //---MARCH RANDOM COLORS
    case 15: thisdelay = 80; break;                     //---MARCH RWB COLORS
    case 16: thisdelay = 60; thishue = 95; break;       //---RADIATION SYMBOL
    //---PLACEHOLDER FOR COLOR LOOP VAR DELAY VARS
    case 19: thisdelay = 35; thishue = 180; break;      //---SIN WAVE BRIGHTNESS
    case 20: thisdelay = 100; thishue = 0; break;       //---POP LEFT/RIGHT
    case 21: thisdelay = 100; thishue = 180; break;     //---QUADRATIC BRIGHTNESS CURVE
    //---PLACEHOLDER FOR FLAME VARS
    case 23: thisdelay = 50; thisstep = 15; break;      //---VERITCAL RAINBOW
    case 24: thisdelay = 50; break;                     //---PACMAN
    case 25: thisdelay = 35; break;                     //---RANDOM COLOR POP
    case 26: thisdelay = 25; thishue = 0; break;        //---EMERGECNY STROBE
    case 27: thisdelay = 120; thishue = 0; break;        //---RGB PROPELLER
    case 28: thisdelay = 100; thishue = 0; break;       //---KITT
    case 29: thisdelay = 100; thishue = 95; break;       //---MATRIX RAIN
    case 30: thisdelay = 50; break;                      //---NEW RAINBOW LOOP
    case 31: thisdelay = 100; break;                    //---MARCH STRIP NOW CCW
    case 32: thisdelay = 100; break;                    //---MARCH STRIP NOW CCW
    case 33: thisdelay = 50; break;                     // colorWipe
    case 34: thisdelay = 50; break;                     // CylonBounce
    case 35: thisdelay = 15; break;                     // Fire
    case 36: thisdelay = 50; break;                     // NewKITT
    case 37: thisdelay = 20; break;                     // rainbowCycle
    case 38: thisdelay = 10; break;                     // rainbowTwinkle
    case 39: thisdelay = 50; break;                     // RunningLights
    case 40: thisdelay = 0; break;                      // Sparkle
    case 41: thisdelay = 30; break;                     // SnowSparkle
    case 42: thisdelay = 50; break;                     // theaterChase
    case 43: thisdelay = 50; break;                     // theaterChaseRainbow
    case 44: thisdelay = 100; break;                    // Strobe

    case 101: one_color_all(255, 0, 0); LEDS.show(); break; //---ALL RED
    case 102: one_color_all(0, 255, 0); LEDS.show(); break; //---ALL GREEN
    case 103: one_color_all(0, 0, 255); LEDS.show(); break; //---ALL BLUE
    case 104: one_color_all(255, 255, 0); LEDS.show(); break; //---ALL COLOR X
    case 105: one_color_all(0, 255, 255); LEDS.show(); break; //---ALL COLOR Y
    case 106: one_color_all(255, 0, 255); LEDS.show(); break; //---ALL COLOR Z
  }
  bouncedirection = 0;
  one_color_all(0, 0, 0);
}
