#define ENABLE_GxEPD2_GFX 0

#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>

// (Instructed by a demonstration, which has "old" and "new" styles:)
// Select the display class and display driver class in the following file (new style):
#include "GxEPD2_display_selection_new_style.h"

// Taken from somewhere
// Pins
#define SDA 21
#define SCL 22
#define ADC_PIN 33
#define RTC_PIN GPIO_NUM_27
#define CS 5
#define DC 10
#define RESET 9
#define BUSY 19
#define VIB_MOTOR_PIN 13
#define MENU_BTN_PIN 26
#define BACK_BTN_PIN 25
#define UP_BTN_PIN 32
#define DOWN_BTN_PIN 4
#define MENU_BTN_MASK GPIO_SEL_26
#define BACK_BTN_MASK GPIO_SEL_25
#define UP_BTN_MASK GPIO_SEL_32
#define DOWN_BTN_MASK GPIO_SEL_4
#define ACC_INT_MASK GPIO_SEL_14
#define BTN_PIN_MASK MENU_BTN_MASK|BACK_BTN_MASK|UP_BTN_MASK|DOWN_BTN_MASK 

// Display
#define DISPLAY_WIDTH 200
#define DISPLAY_HEIGHT 200

U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;

#define BLEN (1024 * 100)
#define BG GxEPD_WHITE
#define FG GxEPD_BLACK

struct TextBuffer {

  // A circular buffer to hold all the incoming text.
  char ring[BLEN];

  // The line pointer always points towards the beginning of a line which is
  // where printing will start from.
  char *line;

  // Points toward the beginning of the next page, which is always the beginning
  // of a line. Set to NULL when there is no next page.
  char *nextPage;

  // Essentially the same behavior, just in reverse, as the above.
  char *prevPage;
};

TextBuffer text;

void text_buffer_default(TextBuffer* t) {
  t->line = t->ring;
  t->prevPage = t->line;
  t->nextPage = NULL;
}

void text_buffer_page_next(TextBuffer *t) {
  t->prevPage = t->line;
  t->line = t->nextPage;
}

void text_buffer_page_prev(TextBuffer *t) {
  t->nextPage = t->line;
  t->line = t->prevPage;
}

void setup()
{
  display.init();

  u8g2Fonts.begin(display);

  u8g2Fonts.setFontMode(1);
  u8g2Fonts.setFontDirection(0);
  u8g2Fonts.setForegroundColor(FG);
  u8g2Fonts.setBackgroundColor(BG);
  u8g2Fonts.setFont(u8g2_font_ncenB10_tf);

  text_buffer_default(&text);

  sprintf(text.ring, "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14eeeeeeeeeeeeeeaoteuaoehusaoheischaosrcuhasoetuh\n15\n16\n17\n18\n19\20");
  text.nextPage = printt(&text);
  text_buffer_page_next(&text);
  text.nextPage = printt(&text);
  text_buffer_page_prev(&text);
  text.nextPage = printt(&text);
  text_buffer_page_next(&text);
  text.nextPage = printt(&text);
  
  // Enable deep sleep wake on any button press
  esp_sleep_enable_ext1_wakeup(BTN_PIN_MASK, ESP_EXT1_WAKEUP_ANY_HIGH);
  esp_deep_sleep_start();
}

void loop()
{

}

// Returns a pointer to the next page if there is one.
char* printt(TextBuffer *text)
{
  // int16_t tw; // The glyph width is calculated below, so this is not necessary.
  int16_t ta = u8g2Fonts.getFontAscent();
  int16_t td = u8g2Fonts.getFontDescent();
  int16_t th = ta + abs(td);

  uint16_t x = 0;
  uint16_t y = th;
  char p[2];
  p[1] = '\0';

  char *nextPage = NULL;

  display.fillScreen(BG);
     
  for(char *c = text->line; *c != '\0'; c++) {
    if (x >= DISPLAY_WIDTH) {
      x = 0;
      y += th;
    }
    if (*c == '\n') {
      x = 0;
      y += th;
    }
    
    u8g2Fonts.setCursor(x, y);
    p[0] = *c;
    u8g2Fonts.print(p);
    x += u8g2Fonts.getUTF8Width(p);

    if (y >= DISPLAY_HEIGHT) {
      if (*c == '\n') {
        c = c + 1;
      }
      nextPage = c;
      break;
    }
  }

  display.display();

  return nextPage;
}
