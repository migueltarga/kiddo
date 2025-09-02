#include <Arduino.h>

#include <lvgl.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <Preferences.h>
#include "config.h"
#include "i18n.h"
#include "file_system.h"
#include "async_manager.h"
#include "story_engine.h"
#include "inventory_manager.h"
#include "ui/screens/ui_screens.h"
#include "ui/app_ui.h"
#include "ui/fonts.h"
#include "ui/router.h"
#include "audio.h"

#if __has_include(<WiFi.h>)
#include <WiFi.h>
#endif

SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);
uint32_t draw_buf[DRAW_BUF_SIZE / 4];
TFT_eSPI tft;
Preferences prefs;
uint8_t brightness = 200;
uint8_t story_font_scale = 1;
bool online_mode = false;
bool wifi_connected = false;

static void touchscreen_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    if (touchscreen.tirqTouched() && touchscreen.touched())
    {
        TS_Point p = touchscreen.getPoint();
        int16_t rx = p.x;
        int16_t ry = p.y;
#ifdef TOUCH_SWAP_XY
        int16_t tmp = rx;
        rx = ry;
        ry = tmp;
#endif
        int16_t px = map(rx, TOUCH_X_MIN, TOUCH_X_MAX, 1, SCREEN_WIDTH);
        int16_t py = map(ry, TOUCH_Y_MIN, TOUCH_Y_MAX, 1, SCREEN_HEIGHT);
#ifdef TOUCH_INVERT_X
        px = (SCREEN_WIDTH + 1) - px;
#endif
#ifdef TOUCH_INVERT_Y
        py = (SCREEN_HEIGHT + 1) - py;
#endif
        if (px < 1)
            px = 1;
        if (px > SCREEN_WIDTH)
            px = SCREEN_WIDTH;
        if (py < 1)
            py = 1;
        if (py > SCREEN_HEIGHT)
            py = SCREEN_HEIGHT;
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = px;
        data->point.y = py;
    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void setup()
{
    Serial.begin(115200);
    delay(50);
    
    SPIFFS.begin(true);
    
    tft.init();
    tft.setRotation(0);
    backlight_init();
    lv_init();
    
    FileSystem::init();
    
    touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    touchscreen.begin(touchscreenSPI);
    touchscreen.setRotation(TOUCH_ROTATION);
    pinMode(XPT2046_IRQ, INPUT_PULLUP);
    lv_display_t *disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf, sizeof(draw_buf));
    lv_display_set_default(disp);
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_display(indev, disp);
    lv_indev_set_read_cb(indev, touchscreen_read);

    audio::init();
    prefs.begin(PNS, false);
    brightness = prefs.getUChar(PK_BRIGHTNESS, 200);
    current_language = (Language)prefs.getUInt(PK_LANG, LANG_EN);
    story_font_scale = prefs.getUChar(PK_STORY_FONT, 1);
    online_mode = prefs.getBool(PK_ONLINE_MODE, false);
    backlight_write(brightness);

    ui_router::show_splash(S()->loading);
    for (int i = 0; i < 10; i++) {
        lv_timer_handler();
        delay(10);
    }
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(); 
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 6) {
        delay(500);
        attempts++;
        for (int i = 0; i < 10; i++) {
            lv_timer_handler();
            delay(10);
        }
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        wifi_connected = true;
    } else {
        wifi_connected = false;
        WiFi.mode(WIFI_OFF);
    }

    for (int i = 0; i < 5; i++) {
        lv_timer_handler();
        delay(10);
    }
    
    AsyncManager::init();
    
    InventoryManager::initialize();
    
    story::loadFromFS();
    ui_story_set_home_cb([]() { ui_router::show_home(); });
    ui_router::show_home();
}

void loop()
{
    lv_timer_handler();
    audio::update();
    
    AsyncManager::process();
    
    lv_tick_inc(5);
    delay(5);
}
