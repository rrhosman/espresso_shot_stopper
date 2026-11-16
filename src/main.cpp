// Brew-by-weight controller
//   CrowPanel 1.46" rotary display (ESP32-S3)  +  Bookoo Themis Ultra  +  Bambino Plus
//
// Knob  = target weight.   START on screen = run the shot (see shot.h).
// Settings are in config.h.

#define LGFX_USE_V1
#include <Arduino.h>
#include <LovyanGFX.hpp>
#include <NimBLEDevice.h>
#include <Preferences.h>
#include <Wire.h>
#include <lvgl.h>

#include "bookoo.h"
#include "config.h"
#include "shot.h"

// ============================================================================
//  Display (JD9855 panel used on CrowPanel 1.46" - Elecrow's custom init list)
// ============================================================================
struct Panel_JD9855 : public lgfx::Panel_LCD {
    Panel_JD9855() {
        _cfg.panel_width   = _cfg.memory_width  = 360;
        _cfg.panel_height  = _cfg.memory_height = 390;
        _cfg.dummy_read_pixel = 8;
    }

protected:
    const uint8_t* getInitCommands(uint8_t listno) const override {
        static constexpr uint8_t list0[] = {
            0xDE, 1, 0x00,
            0xDF, 2, 0x98, 0x55,
            0xB2, 1, 0x1F,
            0xB7, 4, 0x00, 0x1D, 0x00, 0x45,
            0xBB, 6, 0x1B, 0x64, 0xC4, 0x1E, 0x3E, 0xF5,
            0xBC, 4, 0x03, 0x20, 0xF3, 0xC0,
            0xC0, 2, 0x22, 0xA1,
            0xC3, 11, 0x00, 0x02, 0x2A, 0x0B, 0x08, 0x48, 0x08, 0x04, 0x62, 0x30, 0x30,
            0xC4, 11, 0x40, 0x00, 0xAD, 0x68, 0x43, 0x07, 0x04, 0x16, 0x43, 0x07, 0x04,
            0xC8, 32,
                0x3F, 0x36, 0x30, 0x2D, 0x2F, 0x31, 0x2B, 0x2A,
                0x27, 0x25, 0x1F, 0x14, 0x10, 0x09, 0x04, 0x00,
                0x3F, 0x36, 0x30, 0x2D, 0x2F, 0x31, 0x2B, 0x2A,
                0x27, 0x25, 0x1F, 0x14, 0x10, 0x09, 0x04, 0x00,
            0xD3, 2, 0x28, 0x13,
            0xD9, 6, 0x00, 0x00, 0xFF, 0x00, 0xF0, 0x00,
            0xDE, 1, 0x01,
            0xB7, 8, 0x13, 0xE7, 0x64, 0x39, 0x06, 0x36, 0x19, 0x1C,
            0xBE, 1, 0x00,
            0xC1, 3, 0x00, 0x4A, 0x80,
            0xC2, 4, 0x00, 0x16, 0xDA, 0xE7,
            0xC7, 8, 0x00, 0x00, 0x00, 0x38, 0x08, 0x08, 0x00, 0x01,
            0xC8, 6, 0x00, 0x00, 0x00, 0x00, 0x15, 0x3B,
            0xC9, 5, 0x00, 0x16, 0x06, 0x04, 0x0A,
            0xCA, 5, 0x08, 0x35, 0x16, 0x1F, 0x1F,
            0xCB, 5, 0x01, 0x16, 0x07, 0x05, 0x0B,
            0xCC, 5, 0x09, 0x35, 0x16, 0x1F, 0x1F,
            0xCD, 5, 0x01, 0x16, 0x09, 0x0B, 0x05,
            0xCE, 5, 0x07, 0x15, 0x1F, 0x16, 0x1F,
            0xCF, 5, 0x00, 0x16, 0x08, 0x0A, 0x04,
            0xD0, 5, 0x06, 0x15, 0x1F, 0x16, 0x1F,
            0xD1, 2, 0x02, 0x30,
            0xD2, 5, 0x02, 0x03, 0x52, 0xDF, 0xDD,
            0xD3, 3, 0x3B, 0x04, 0x48,
            0xD5, 7, 0x10, 0x10, 0x07, 0x07, 0x0F, 0x94, 0x26,
            0xD6, 3, 0x00, 0x00, 0x40,
            0xD7, 3, 0x00, 0x00, 0x20,
            0xDE, 1, 0x02,
            0xB6, 1, 0x1C,
            0xDE, 1, 0x00,
            0x4D, 1, 0x00,
            0x4E, 1, 0x00,
            0x4F, 1, 0x00,
            0x4C, 1 + CMD_INIT_DELAY, 0x01, 10,
            0x4C, 1, 0x00,
            0x35, 0,
            0x3A, 1, 0x05,
            0x2A, 4, 0x00, 0x00, 0x01, 0x67,
            0x2B, 4, 0x00, 0x00, 0x01, 0x67,
            CMD_SLPOUT, CMD_INIT_DELAY, 120,
            CMD_DISPON, 0,
            0xFF, 0xFF,
        };
        return (listno == 0) ? list0 : nullptr;
    }

    void setColorDepth_impl(lgfx::color_depth_t depth) override {
        _write_depth = ((int)depth & lgfx::color_depth_t::bit_mask) > 16
                           ? lgfx::rgb888_3Byte
                           : lgfx::rgb565_2Byte;
        _read_depth = _write_depth;
    }
};

class LGFX : public lgfx::LGFX_Device {
    Panel_JD9855  _panel;
    lgfx::Bus_SPI _bus;

public:
    LGFX() {
        {
            auto cfg        = _bus.config();
            cfg.spi_host    = SPI2_HOST;
            cfg.spi_mode    = 0;
            cfg.freq_write  = 80000000;
            cfg.freq_read   = 20000000;
            cfg.spi_3wire   = true;
            cfg.use_lock    = true;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            cfg.pin_sclk    = PIN_LCD_SCLK;
            cfg.pin_mosi    = PIN_LCD_MOSI;
            cfg.pin_miso    = -1;
            cfg.pin_dc      = PIN_LCD_DC;
            _bus.config(cfg);
            _panel.setBus(&_bus);
        }
        {
            auto cfg         = _panel.config();
            cfg.pin_cs       = PIN_LCD_CS;
            cfg.pin_rst      = PIN_LCD_RST;
            cfg.pin_busy     = -1;
            cfg.memory_width = cfg.panel_width  = 360;
            cfg.memory_height = cfg.panel_height = 360;
            cfg.offset_x = cfg.offset_y = 0;
            cfg.offset_rotation  = 0;
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits  = 1;
            cfg.readable   = false;
            cfg.invert     = false;
            cfg.rgb_order  = true;
            cfg.dlen_16bit = false;
            cfg.bus_shared = false;
            _panel.config(cfg);
        }
        setPanel(&_panel);
    }
};

static LGFX gfx;

// ============================================================================
//  Shared state (written by the BLE task, read by loop())
// ============================================================================
static volatile bool     gConnected   = false;
static volatile float    gWeight      = 0;
static volatile uint32_t gScaleTimer  = 0;
static volatile uint8_t  gBattery     = 0;
static volatile uint32_t gLastFrameMs = 0;

static QueueHandle_t gCmdQueue;   // bookoo::Frame, loop() -> BLE task

static void queueScaleCmd(uint8_t cmd, uint8_t v1 = 0, uint8_t v2 = 0) {
    bookoo::Frame f = bookoo::makeCmd(cmd, v1, v2);
    xQueueSend(gCmdQueue, &f, 0);
}

// ============================================================================
//  Bluetooth (own task, so connecting never freezes the screen)
// ============================================================================
static NimBLEAddress gScaleAddr;
static volatile bool gScaleFound = false;

static void onScaleData(NimBLERemoteCharacteristic*, uint8_t* data, size_t len, bool) {
    bookoo::Reading r;
    if (!bookoo::parseWeight(data, len, r)) return;
    gWeight      = r.grams;
    gScaleTimer  = r.timerMs;
    gBattery     = r.battery;
    gLastFrameMs = millis();
}

class ScanCallbacks : public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* dev) override {
        if (!dev->haveName()) return;
        String name = dev->getName().c_str();
        String upper = name;
        upper.toUpperCase();
        // "OOKOO" rather than "BOOKOO": one firmware drops the leading B.
        if (upper.indexOf("OOKOO_SC") < 0) return;
        if (strlen(SCALE_NAME_FILTER) && name.indexOf(SCALE_NAME_FILTER) < 0) return;
        Serial.printf("Found scale: %s\n", name.c_str());
        gScaleAddr  = dev->getAddress();
        gScaleFound = true;
        NimBLEDevice::getScan()->stop();
    }
};

class ClientCallbacks : public NimBLEClientCallbacks {
    void onDisconnect(NimBLEClient*) override {
        gConnected = false;
        Serial.println("Scale disconnected");
    }
};

static void bleTask(void*) {
    static ScanCallbacks   scanCb;
    static ClientCallbacks clientCb;

    NimBLEDevice::init("brew-knob");
    NimBLEScan* scan = NimBLEDevice::getScan();
    scan->setAdvertisedDeviceCallbacks(&scanCb, false);
    scan->setActiveScan(true);
    scan->setInterval(100);
    scan->setWindow(80);

    NimBLEClient* client = NimBLEDevice::createClient();
    client->setClientCallbacks(&clientCb, false);
    client->setConnectTimeout(5);

    NimBLERemoteCharacteristic* cmdChr = nullptr;

    for (;;) {
        if (!client->isConnected()) {
            gConnected = false;
            cmdChr     = nullptr;

            if (!gScaleFound) {
                scan->start(3, false);   // blocks up to 3 s
                scan->clearResults();
                continue;
            }
            gScaleFound = false;         // rescan next time if this attempt fails
            if (!client->connect(gScaleAddr)) continue;

            NimBLERemoteService* svc = client->getService(NimBLEUUID(bookoo::UUID_SERVICE));
            NimBLERemoteCharacteristic* dataChr =
                svc ? svc->getCharacteristic(NimBLEUUID(bookoo::UUID_DATA)) : nullptr;
            cmdChr = svc ? svc->getCharacteristic(NimBLEUUID(bookoo::UUID_COMMAND)) : nullptr;

            if (!dataChr || !cmdChr || !dataChr->subscribe(true, onScaleData)) {
                Serial.println("Scale is missing the expected service - disconnecting");
                client->disconnect();
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }
            xQueueReset(gCmdQueue);
            gLastFrameMs = millis();
            gConnected   = true;
            Serial.println("Scale connected");
#if FORCE_FLOW_MODE
            queueScaleCmd(bookoo::CMD_SET_MODE, 1, 1);
#endif
        }

        bookoo::Frame f;
        if (xQueueReceive(gCmdQueue, &f, pdMS_TO_TICKS(100)) == pdTRUE && cmdChr && gConnected) {
            bool ok = cmdChr->writeValue(f.b, sizeof(f.b), cmdChr->canWrite());
            Serial.printf("Scale cmd %02X %s\n", f.b[2], ok ? "ok" : "FAILED");
        }
    }
}

// ============================================================================
//  Bambino button + shot sequencer
// ============================================================================
static bool     gDemoMode    = false;
static uint32_t gDemoStartMs = 0;

static void setBrewButton(bool pressed) {
    digitalWrite(BREW_PIN, (pressed ^ (bool)BREW_PIN_ACTIVE_LOW) ? HIGH : LOW);
    Serial.printf("[%lu] button %s\n", millis(), pressed ? "DOWN" : "up");
}

static void shotScaleCmd(ShotController::ScaleCmd c) {
    switch (c) {
    case ShotController::SC_RESET_TIMER:    queueScaleCmd(bookoo::CMD_TIMER_RESET);    break;
    case ShotController::SC_START_TIMER:    queueScaleCmd(bookoo::CMD_TIMER_START);    break;
    case ShotController::SC_TARE_AND_START: queueScaleCmd(bookoo::CMD_TARE_AND_START); break;
    case ShotController::SC_STOP_TIMER:     queueScaleCmd(bookoo::CMD_TIMER_STOP);     break;
    }
}

static const ShotConfig kShotCfg = {
    START_HOLD_MS, STOP_PRESS_MS, ABORT_GAP_MS, MAX_SHOT_MS, STOP_OFFSET_G, (bool)TARE_ON_START,
};
static ShotController shot(kShotCfg, setBrewButton, shotScaleCmd);

static bool scaleOk() {
    if (gDemoMode) return true;
    // Signed difference: the BLE task may stamp a frame after we read millis().
    return gConnected && (int32_t)(millis() - gLastFrameMs) < SCALE_STALE_MS;
}

static void updateDemoScale() {
    if (!gDemoMode) return;
    if (shot.active()) {
        uint32_t elapsed = millis() - gDemoStartMs;
        gScaleTimer = elapsed;
        // Simulate ~3s pre-infusion delay, then ~1.8 g/s flow
        gWeight = (elapsed > 3000) ? ((elapsed - 3000) * 0.0018f) : 0.0f;
    }
}

// ============================================================================
//  Target weight (knob) - stored in tenths of a gram
// ============================================================================
static Preferences prefs;
static int32_t  targetDg      = (int32_t)(TARGET_DEFAULT_G * 10);
static bool     targetDirty   = false;
static uint32_t targetEditMs  = 0;
static inline float targetG() { return targetDg / 10.0f; }

static volatile int32_t encCount = 0;
static volatile uint8_t encPrev  = 0;
static const DRAM_ATTR int8_t kQuad[16] = {0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0};

static void IRAM_ATTR encoderISR() {
    uint8_t s = (digitalRead(PIN_ENC_A) << 1) | digitalRead(PIN_ENC_B);
    encCount += kQuad[(encPrev << 2) | s];
    encPrev = s;
}

static void handleKnob() {
    static int32_t  consumed    = 0;
    static uint32_t lastClickMs = 0;

    int32_t clicks = (encCount - consumed) / ENC_EDGES_PER_CLICK;
    if (!clicks) return;
    consumed += clicks * ENC_EDGES_PER_CLICK;
    if (shot.active()) return;   // target is locked while a shot is running
#if ENC_REVERSE
    clicks = -clicks;
#endif
    uint32_t now = millis();
    uint32_t dt  = now - lastClickMs;
    lastClickMs  = now;

    // Velocity acceleration: slow = 0.5g/click, medium = 1.0g/click, fast spin = 2.0g/click
    int32_t stepDg = (int32_t)(TARGET_STEP_G * 10);
    if (dt < 45) {
        stepDg *= 4;   // 2.0 g per detent when spinning quickly
    } else if (dt < 100) {
        stepDg *= 2;   // 1.0 g per detent at medium speed
    }

    targetDg = constrain(targetDg + clicks * stepDg,
                         (int32_t)(TARGET_MIN_G * 10), (int32_t)(TARGET_MAX_G * 10));
    targetDirty  = true;
    targetEditMs = now;
}

static void saveTargetIfIdle() {
    if (targetDirty && millis() - targetEditMs > 3000) {
        prefs.putInt("target", targetDg);
        targetDirty = false;
    }
}

static void showMessage(const char* m);

static void handleKnobSwitch() {
    static bool     lastPressed = false;
    static uint32_t pressStart  = 0;
    static bool     longHandled = false;

    bool pressed = (digitalRead(PIN_ENC_SW) == LOW);
    uint32_t now = millis();

    if (pressed && !lastPressed) {
        pressStart  = now;
        longHandled = false;
    } else if (pressed && !longHandled && (now - pressStart >= 800)) {
        longHandled = true;
        if (!shot.active()) {
            gDemoMode   = !gDemoMode;
            gWeight     = 0.0f;
            gScaleTimer = 0;
            gBattery    = 100;
            showMessage(gDemoMode ? "Demo scale ON" : "Demo scale OFF");
        }
    } else if (!pressed && lastPressed && !longHandled && (now - pressStart >= 30)) {
        // Short click on knob switch triggers START / STOP (same as screen button)
        if (shot.active()) {
            shot.stop(now);
        } else if (!scaleOk()) {
            showMessage("No scale (hold knob for demo)");
        } else if (!gDemoMode && !TARE_ON_START && gWeight > START_MAX_WEIGHT_G) {
            showMessage("Tare the scale first");
        } else {
            if (gDemoMode) {
                gWeight      = 0.0f;
                gScaleTimer  = 0;
                gDemoStartMs = now;
            }
            shot.start(now);
        }
    }
    lastPressed = pressed;
}

// ============================================================================
//  LVGL glue: display flush + CST816 touch
// ============================================================================
static lv_disp_draw_buf_t drawBuf;
static lv_color_t         drawPixels[360 * 40];

static void flushCb(lv_disp_drv_t* drv, const lv_area_t* a, lv_color_t* px) {
    uint32_t w = a->x2 - a->x1 + 1, h = a->y2 - a->y1 + 1;
    gfx.startWrite();
    gfx.setAddrWindow(a->x1, a->y1, w, h);
    gfx.writePixels((lgfx::rgb565_t*)&px->full, w * h);
    gfx.endWrite();
    lv_disp_flush_ready(drv);
}

static void touchCb(lv_indev_drv_t*, lv_indev_data_t* data) {
    // CST816: reg 0x02 = finger count, 0x03..0x06 = X hi/lo, Y hi/lo.
    // The chip sleeps (and NACKs) when nothing touches it - that just means "released".
    uint8_t b[5] = {0};
    Wire.beginTransmission(0x15);
    Wire.write(0x02);
    bool ok = Wire.endTransmission(false) == 0 && Wire.requestFrom(0x15, 5) == 5;
    if (ok) for (int i = 0; i < 5; i++) b[i] = Wire.read();

    if (ok && b[0] > 0) {
        data->state   = LV_INDEV_STATE_PR;
        data->point.x = ((b[1] & 0x0F) << 8) | b[2];
        data->point.y = ((b[3] & 0x0F) << 8) | b[4];
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

// ============================================================================
//  Screen
// ============================================================================
static lv_obj_t *uiArc, *uiStatus, *uiTimer, *uiWeight, *uiTarget, *uiBtn, *uiBtnLabel;
static char     uiMsg[32]  = "";
static uint32_t uiMsgUntil = 0;

static void showMessage(const char* m) {
    strlcpy(uiMsg, m, sizeof(uiMsg));
    uiMsgUntil = millis() + 3000;
}

static void onButton(lv_event_t*) {
    uint32_t now = millis();
    if (shot.active()) {
        shot.stop(now);
    } else if (!scaleOk()) {
        showMessage("No scale (hold knob for demo)");
    } else if (!gDemoMode && !TARE_ON_START && gWeight > START_MAX_WEIGHT_G) {
        showMessage("Tare the scale first");
    } else {
        if (gDemoMode) {
            gWeight      = 0.0f;
            gScaleTimer  = 0;
            gDemoStartMs = now;
        }
        shot.start(now);
    }
}

static lv_obj_t* makeLabel(const lv_font_t* font, lv_color_t color, int y) {
    lv_obj_t* l = lv_label_create(lv_scr_act());
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, color, 0);
    lv_obj_align(l, LV_ALIGN_CENTER, 0, y);
    return l;
}

static void buildUi() {
    lv_obj_t* scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    // Ring around the edge: weight as a fraction of target
    uiArc = lv_arc_create(scr);
    lv_obj_set_size(uiArc, 352, 352);
    lv_obj_center(uiArc);
    lv_arc_set_rotation(uiArc, 270);
    lv_arc_set_bg_angles(uiArc, 0, 360);
    lv_arc_set_range(uiArc, 0, 1000);
    lv_arc_set_value(uiArc, 0);
    lv_obj_remove_style(uiArc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(uiArc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_arc_width(uiArc, 10, LV_PART_MAIN);
    lv_obj_set_style_arc_width(uiArc, 10, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(uiArc, lv_color_hex(0x2A2A2A), LV_PART_MAIN);
    lv_obj_set_style_arc_color(uiArc, lv_color_hex(0xE8A33D), LV_PART_INDICATOR);

    uiStatus = makeLabel(&lv_font_montserrat_18, lv_color_hex(0x9A9A9A), -126);
    uiTimer  = makeLabel(&lv_font_montserrat_36, lv_color_white(), -82);
    uiWeight = makeLabel(&lv_font_montserrat_48, lv_color_white(), -26);
    uiTarget = makeLabel(&lv_font_montserrat_32, lv_color_hex(0xE8A33D), 30);

    uiBtn = lv_btn_create(scr);
    lv_obj_set_size(uiBtn, 160, 62);
    lv_obj_align(uiBtn, LV_ALIGN_CENTER, 0, 98);
    lv_obj_set_style_radius(uiBtn, 31, 0);
    lv_obj_add_event_cb(uiBtn, onButton, LV_EVENT_CLICKED, NULL);
    uiBtnLabel = lv_label_create(uiBtn);
    lv_obj_set_style_text_font(uiBtnLabel, &lv_font_montserrat_28, 0);
    lv_obj_center(uiBtnLabel);
}

static void updateUi() {
    bool ok     = scaleOk();
    bool active = shot.active();
    float w     = gWeight;

    // Status line
    if ((int32_t)(uiMsgUntil - millis()) > 0) {
        lv_label_set_text(uiStatus, uiMsg);
    } else if (!ok) {
        lv_label_set_text(uiStatus, "Searching for scale...");
    } else {
        switch (shot.state()) {
        case ShotController::HOLDING:    lv_label_set_text(uiStatus, "Starting (holding button)"); break;
        case ShotController::BREWING:    lv_label_set_text(uiStatus, "Brewing"); break;
        case ShotController::ABORT_GAP:
        case ShotController::STOP_PRESS: lv_label_set_text(uiStatus, "Stopping"); break;
        default:
            switch (shot.lastReason()) {
            case ShotController::TARGET_REACHED: lv_label_set_text(uiStatus, "Done"); break;
            case ShotController::USER_STOP:      lv_label_set_text(uiStatus, "Stopped"); break;
            case ShotController::TIMEOUT:        lv_label_set_text(uiStatus, "Stopped: time limit"); break;
            case ShotController::SCALE_LOST:     lv_label_set_text(uiStatus, "Stopped: scale lost"); break;
            default:
                if (gDemoMode) {
                    lv_label_set_text(uiStatus, "Demo scale ready");
                } else {
                    lv_label_set_text_fmt(uiStatus, "Scale ready  %d%%", gBattery);
                }
                break;
            }
        }
    }

    if (ok) {
        lv_label_set_text_fmt(uiTimer, "%lu.%lu s", (unsigned long)(gScaleTimer / 1000),
                              (unsigned long)((gScaleTimer % 1000) / 100));
        int dg = (int)lroundf(w * 10);
        lv_label_set_text_fmt(uiWeight, "%s%d.%d g", dg < 0 ? "-" : "", abs(dg) / 10, abs(dg) % 10);
        lv_arc_set_value(uiArc, constrain((int)(w / targetG() * 1000), 0, 1000));
    } else {
        lv_label_set_text(uiTimer, "--.- s");
        lv_label_set_text(uiWeight, "--.- g");
        lv_arc_set_value(uiArc, 0);
    }

    lv_label_set_text_fmt(uiTarget, "target %d.%d g", (int)(targetDg / 10), (int)(targetDg % 10));

    lv_label_set_text(uiBtnLabel, active ? "STOP" : "START");
    lv_obj_set_style_bg_color(uiBtn, active ? lv_color_hex(0xC0392B)
                                   : ok     ? lv_color_hex(0x2E8B57)
                                            : lv_color_hex(0x444444), 0);
}

// ============================================================================
//  setup / loop
// ============================================================================
void setup() {
    // Button output first, in its released state, before anything else can go wrong.
    digitalWrite(BREW_PIN, BREW_PIN_ACTIVE_LOW ? HIGH : LOW);
    pinMode(BREW_PIN, OUTPUT);
    digitalWrite(BREW_PIN, BREW_PIN_ACTIVE_LOW ? HIGH : LOW);

    Serial.begin(115200);

    pinMode(PIN_PWR_LED, OUTPUT); digitalWrite(PIN_PWR_LED, LOW);
    pinMode(PIN_PWR_1, OUTPUT);   digitalWrite(PIN_PWR_1, HIGH);
    pinMode(PIN_PWR_2, OUTPUT);   digitalWrite(PIN_PWR_2, HIGH);

    // Display reset, then init
    pinMode(PIN_LCD_RST, OUTPUT);
    digitalWrite(PIN_LCD_RST, HIGH); delay(10);
    digitalWrite(PIN_LCD_RST, LOW);  delay(10);
    digitalWrite(PIN_LCD_RST, HIGH); delay(120);
    gfx.init();
    gfx.fillScreen(TFT_BLACK);
    pinMode(PIN_LCD_BL, OUTPUT);
    digitalWrite(PIN_LCD_BL, HIGH);

    // Touch (reset both pins 5 and 13 so either wiki/factory wiring works)
    pinMode(PIN_TOUCH_RST, OUTPUT);
    digitalWrite(PIN_TOUCH_RST, LOW);  delay(10);
    digitalWrite(PIN_TOUCH_RST, HIGH); delay(50);
    pinMode(PIN_TOUCH_INT, INPUT_PULLUP);
    Wire.begin(PIN_TOUCH_SDA, PIN_TOUCH_SCL, 400000);

    // Knob
    pinMode(PIN_ENC_A, INPUT_PULLUP);
    pinMode(PIN_ENC_B, INPUT_PULLUP);
    pinMode(PIN_ENC_SW, INPUT_PULLUP);
    encPrev = (digitalRead(PIN_ENC_A) << 1) | digitalRead(PIN_ENC_B);
    attachInterrupt(PIN_ENC_A, encoderISR, CHANGE);
    attachInterrupt(PIN_ENC_B, encoderISR, CHANGE);

    prefs.begin("brew", false);
    targetDg = constrain(prefs.getInt("target", targetDg),
                         (int32_t)(TARGET_MIN_G * 10), (int32_t)(TARGET_MAX_G * 10));

    // LVGL
    lv_init();
    lv_disp_draw_buf_init(&drawBuf, drawPixels, NULL, sizeof(drawPixels) / sizeof(drawPixels[0]));
    static lv_disp_drv_t dispDrv;
    lv_disp_drv_init(&dispDrv);
    dispDrv.hor_res  = 360;
    dispDrv.ver_res  = 360;
    dispDrv.flush_cb = flushCb;
    dispDrv.draw_buf = &drawBuf;
    lv_disp_drv_register(&dispDrv);

    static lv_indev_drv_t indevDrv;
    lv_indev_drv_init(&indevDrv);
    indevDrv.type    = LV_INDEV_TYPE_POINTER;
    indevDrv.read_cb = touchCb;
    lv_indev_drv_register(&indevDrv);

    buildUi();
    updateUi();

    gCmdQueue = xQueueCreate(8, sizeof(bookoo::Frame));
    xTaskCreatePinnedToCore(bleTask, "ble", 8192, NULL, 2, NULL, 0);
}

void loop() {
    updateDemoScale();
    // Shot timing first, so screen drawing can never delay a button edge by more than one frame.
    shot.tick(millis(), gWeight, scaleOk(), targetG());

    handleKnob();
    handleKnobSwitch();
    saveTargetIfIdle();

    static uint32_t lastUi = 0;
    if (millis() - lastUi >= 100) {
        lastUi = millis();
        updateUi();
    }
    lv_timer_handler();
    delay(2);
}
