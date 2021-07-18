#define ENABLE_GxEPD2_GFX 0

// No bluetooth locally to test right now.
// #include <BLEDevice.h>
// #include <BLEUtils.h>
// #include <BLEServer.h>

#include <GxEPD2_BW.h>
#include <U8g2_for_Adafruit_GFX.h>

// Holds the image for the screenblock
#include "screenblock.h"

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
#define BOTTOM_LEFT_BTN_PIN 26
#define TOP_LEFT_BTN_PIN 25
#define TOP_RIGHT_BTN_PIN 32
#define BOTTOM_RIGHT_BTN_PIN 4
#define BOTTOM_LEFT_BTN_MASK GPIO_SEL_26
#define TOP_LEFT_BTN_MASK GPIO_SEL_25
#define TOP_RIGHT_BTN_MASK GPIO_SEL_32
#define BOTTOM_RIGHT_BTN_MASK GPIO_SEL_4
#define ACC_INT_MASK GPIO_SEL_14
#define BTN_PIN_MASK BOTTOM_LEFT_BTN_MASK|TOP_LEFT_BTN_MASK|TOP_RIGHT_BTN_MASK|BOTTOM_RIGHT_BTN_MASK 

// Display
#define DISPLAY_WIDTH 200
#define DISPLAY_HEIGHT 200

U8G2_FOR_ADAFRUIT_GFX u8g2Fonts;

#define BG GxEPD_WHITE
#define FG GxEPD_BLACK

#define TOTAL_LINES 100
#define MAX_CHARS_PER_LINE 32

// This is font independent! Manually check.
#define LINES_PER_PAGE 14
#define HALF_PAGE LINES_PER_PAGE / 2

// Bluetooth definitions
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

enum ScrollDirection {
  ScrollForward = 1,
  ScrollBackward = -1,
};

struct TextBuffer {

  // During receiving, text width is calculated and is split up into lines.
  char lines[TOTAL_LINES][MAX_CHARS_PER_LINE];

  int16_t print_from_line;

  // Where to start inserting into the buffer, but also do not print past this
  // point, since it signals the end of the buffer / ring.
  uint16_t insert_from_line;

  // Our buffer has filled once and overflowed.
  uint8_t has_overflowed;
};

void text_buffer_default(TextBuffer *tb) {
  tb->print_from_line = 0;
  tb->insert_from_line = 0;
  tb->has_overflowed = 0;
}

void text_buffer_print_serial(TextBuffer* tb) {
  Serial.printf("tb->print_from_line = %i\n", tb->print_from_line);
  Serial.printf("tb->insert_from_line = %u\n", tb->insert_from_line);
  Serial.printf("\n");
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

  uint8_t last_char_width = 0;
    
  for (char* c = text; *c != '\0'; c += 1) {
    if (tb->insert_from_line >= TOTAL_LINES) {
      tb->insert_from_line = 0;
      tb->has_overflowed = 1;
    }

    if (x >= DISPLAY_WIDTH - last_char_width || *c == '\n') {
      tb->lines[tb->insert_from_line][at_char] = '\0';
      x = 0;
      at_char = 0;
      tb->insert_from_line += 1;

      if (*c == '\n') {
        continue;
      }
    }

    p[0] = *c;
    tb->lines[tb->insert_from_line][at_char] = *c;
    last_char_width = u8g2Fonts.getUTF8Width(p);
    x += last_char_width;
    at_char += 1;
  }

  tb->insert_from_line += 1;
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
  
  char p[2];
  p[1] = '\0';

  for(
    int16_t at_line = tb->print_from_line;
    y < DISPLAY_HEIGHT
    && at_line != tb->insert_from_line;
    at_line += 1,
    y += th
  ) {
    if (at_line >= TOTAL_LINES) {
      at_line = 0;
    }
    Serial.println(&tb->lines[at_line][0]);
    x = 0;
    for (char *c = &tb->lines[at_line][0]; *c != '\0'; c += 1) {
      u8g2Fonts.setCursor(x, y);
      p[0] = *c;
      u8g2Fonts.print(p);
      x += u8g2Fonts.getUTF8Width(p);
    }
  }

  display.display();
}

void text_buffer_top(TextBuffer* tb) {
  tb->print_from_line = tb->has_overflowed ? tb->insert_from_line + 1 : 0;
}

void text_buffer_bottom(TextBuffer* tb) {
  tb->print_from_line = tb->insert_from_line - 1;
}

void text_buffer_scroll_half_page(TextBuffer* tb, ScrollDirection direction) {
  int16_t next_line = tb->print_from_line;
  for(
    uint8_t i = 0;
    i < HALF_PAGE;
    i += 1
  ) {
    next_line += direction;
    if (next_line == tb->insert_from_line) {
      return;
    }
    if (tb->has_overflowed == 0) {
      if (next_line <= 0) {
        text_buffer_top(tb);
        return;
      }
    }
  }
 
  tb->print_from_line = next_line;
}

// RTC_DATA_ATTR is a macro to use "RTC Slow RAM". There is only about 4kb of it.
RTC_DATA_ATTR TextBuffer text_buffer;
RTC_DATA_ATTR uint8_t has_booted = 0;
RTC_DATA_ATTR uint8_t is_listening = 0;

//void init_bluetooth_server() {
//  BLEDevice::init("WatchyTerm");
//  BLEServer *pServer = BLEDevice::createServer();
//  BLEService *pService = pServer->createService(SERVICE_UUID);
//  BLECharacteristic *pCharacteristic =
//    pService->createCharacteristic(
//      CHARACTERISTIC_UUID,
//      BLECharacteristic::PROPERTY_WRITE
//    );
//
//  pCharacteristic->setValue("Hello World says Neil");
//  pService->start();
//  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
//  pAdvertising->addServiceUUID(SERVICE_UUID);
//  pAdvertising->setScanResponse(true);
//
//  // Taken from https://github.com/nkolban/ESP32_BLE_Arduino/blob/master/examples/BLE_server/BLE_server.ino
//  // functions that help with iPhone connections issue   
//  pAdvertising->setMinPreferred(0x06);
//  pAdvertising->setMinPreferred(0x12);
//  
//  BLEDevice::startAdvertising();
//}

void screenblock() {
  display.fillScreen(FG);
  display.drawBitmap(0, 0, epd_bitmap_screenblock, DISPLAY_WIDTH, DISPLAY_HEIGHT, BG);
  display.display();
}

uint8_t handle_button_interrupt() {
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  if (wakeup_reason != ESP_SLEEP_WAKEUP_EXT1) { return 1; }

  uint64_t button_bits = esp_sleep_get_ext1_wakeup_status();
  if (button_bits & TOP_LEFT_BTN_MASK) {
    text_buffer_scroll_half_page(&text_buffer, ScrollBackward);
    text_buffer_print(&text_buffer);
    return 1;
  } else if (button_bits & TOP_RIGHT_BTN_MASK) {
    if (is_listening == 0) {
      //init_bluetooth_server();
      is_listening = 1;
      return 1;
    } else {
      // Just put the device back into sleep mode
      is_listening = 0;
      return 1;
    }
  } else if (button_bits & BOTTOM_RIGHT_BTN_MASK) {
    screenblock();
    return 1;
  } else if (button_bits & BOTTOM_LEFT_BTN_MASK) {
    text_buffer_scroll_half_page(&text_buffer, ScrollForward);
    text_buffer_print(&text_buffer);
    return 1;
  }

  return 1;
}

void setup()
{
  Serial.begin(9600);

  display.init();

  u8g2Fonts.begin(display);

  u8g2Fonts.setFontMode(1);
  u8g2Fonts.setFontDirection(0);
  u8g2Fonts.setForegroundColor(FG);
  u8g2Fonts.setBackgroundColor(BG);
  u8g2Fonts.setFont(u8g2_font_ncenB10_tf);

  uint8_t should_go_back_to_sleep = 1;

  if (has_booted == 0) {
    text_buffer_default(&text_buffer);
    
    char text[400];
    sprintf(text, 
      "0 | s, soft c, z, x (xylophone)\n"
      "1 | t, d , th (this)\n"
      "2 | n\n"
      "3 | m\n"
      "4 | r\n"
      "5 | l\n"
      "6 | ch (cheese, chef), j, sh\n"
      "7 | k, q, hard g, ch (loch)\n"
      "8 | f, ph (phone), v, gh (laugh)\n"
      "9 | p, b\n"
    );
    
    text_buffer_insert(&text_buffer, text);
    text_buffer_print(&text_buffer);
      
    has_booted = 1;
  } else {
    should_go_back_to_sleep = handle_button_interrupt();
  }

  if (should_go_back_to_sleep) { 
    esp_sleep_enable_ext1_wakeup(BTN_PIN_MASK, ESP_EXT1_WAKEUP_ANY_HIGH);
    esp_deep_sleep_start();
  }
}

// Not used at all.
void loop() { }
