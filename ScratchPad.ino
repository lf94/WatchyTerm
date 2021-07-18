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

#define BG GxEPD_WHITE
#define FG GxEPD_BLACK

#define TOTAL_LINES 500
#define MAX_CHARS_PER_LINE 30

// This is font independent! Manually check.
#define LINES_PER_PAGE 14

struct TextBuffer {

  // During receiving, text width is calculated and is split up into lines.
  char lines[TOTAL_LINES][MAX_CHARS_PER_LINE];

  uint16_t print_from_line;

  // Where to start inserting into the buffer, but also do not print past this
  // point, since it signals the end of the buffer / ring.
  uint16_t insert_from_line;
};

void text_buffer_default(TextBuffer *tb) {
  tb->print_from_line = 0;
  tb->insert_from_line = 0;
}

void text_buffer_insert(TextBuffer* tb, char* text) {
  // Since we print character-by-character, we prepare this small array to hold
  // a single character for u8g2.
  char p[2];
  p[1] = '\0';

  // Screen is 200x200, small enough to address using 8 bits.
  uint8_t x = 0;

  // Track where we are in a line.
  uint8_t at_char = 0;
    
  for (char* c = text; c != '\0'; c += 1) {
    if (x >= DISPLAY_WIDTH) {
      tb->lines[tb->insert_from_line][at_char] = '\0';
      x = 0;
      at_char = 0;
      tb->insert_from_line += 1;
    }
    if (*c == '\n') {
      tb->lines[tb->insert_from_line][at_char] = '\0';
      x = 0;
      at_char = 0;
      tb->insert_from_line += 1;
    }
    
    p[0] = *c;
    tb->lines[tb->insert_from_line][at_char] = *c;
    x += u8g2Fonts.getUTF8Width(p);
    at_char += 1;
  }
}

void text_buffer_print(TextBuffer* tb)
{
  // int16_t tw; // The glyph width is calculated below, so this is not necessary.
  int16_t ta = u8g2Fonts.getFontAscent();
  int16_t td = u8g2Fonts.getFontDescent();
  int16_t th = ta + abs(td);

  display.fillScreen(BG);

  uint16_t x = 0;
  uint16_t y = th;

  for(
    uint16_t at_line = tb->print_from_line;
    y < DISPLAY_HEIGHT
    && at_line != tb->insert_from_line;
    at_line += 1,
    y += th
  ) {
    u8g2Fonts.setCursor(x, y);
    u8g2Fonts.print(&tb->lines[at_line][0]);
    x += u8g2Fonts.getUTF8Width(&tb->lines[at_line][0]);
  }

  display.display();
}



TextBuffer text_buffer;

void setup()
{
  display.init();

  u8g2Fonts.begin(display);

  u8g2Fonts.setFontMode(1);
  u8g2Fonts.setFontDirection(0);
  u8g2Fonts.setForegroundColor(FG);
  u8g2Fonts.setBackgroundColor(BG);
  u8g2Fonts.setFont(u8g2_font_ncenB10_tf);

  text_buffer_default(&text_buffer);

  char* text = NULL;
  sprintf(text, "1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14eeeeeeeeeeeeeeaoteuaoehusaoheischaosrcuhasoetuh\n15\n16\n17\n18\n19\20");
  text_buffer_insert(&text_buffer, text);
  text_buffer_print(&text_buffer);
  
  // Enable deep sleep wake on any button press
  esp_sleep_enable_ext1_wakeup(BTN_PIN_MASK, ESP_EXT1_WAKEUP_ANY_HIGH);
  esp_deep_sleep_start();
}


void loop()
{

}
