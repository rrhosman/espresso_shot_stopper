// Shot sequencer. Pure logic: the two callbacks do the real work, so this can
// be unit-tested on a PC (see test/test_shot.cpp).
//
//   start -> button DOWN, scale timer starts
//         -> after holdMs: button UP               (shot is running)
//         -> target weight reached: button DOWN, scale timer stops
//         -> after stopPressMs: button UP          (done)
#pragma once
#include <stdint.h>

struct ShotConfig {
    uint32_t holdMs;
    uint32_t stopPressMs;
    uint32_t abortGapMs;
    uint32_t maxShotMs;
    float    stopOffsetG;
    bool     tareOnStart;
};

class ShotController {
public:
    enum State  { IDLE, HOLDING, BREWING, ABORT_GAP, STOP_PRESS };
    enum Reason { NONE, TARGET_REACHED, USER_STOP, TIMEOUT, SCALE_LOST };
    enum ScaleCmd { SC_RESET_TIMER, SC_START_TIMER, SC_TARE_AND_START, SC_STOP_TIMER };

    typedef void (*ButtonFn)(bool pressed);
    typedef void (*ScaleFn)(ScaleCmd cmd);

    ShotController(const ShotConfig& cfg, ButtonFn button, ScaleFn scale)
        : _cfg(cfg), _button(button), _scale(scale) {}

    State  state()      const { return _state; }
    Reason lastReason() const { return _reason; }
    bool   active()     const { return _state != IDLE; }

    bool start(uint32_t now) {
        if (_state != IDLE) return false;
        _reason = NONE;
        _button(true);
        if (_cfg.tareOnStart) {
            _scale(SC_TARE_AND_START);
        } else {
            _scale(SC_RESET_TIMER);
            _scale(SC_START_TIMER);
        }
        _shotStart = _t0 = now;
        _state = HOLDING;
        return true;
    }

    // User pressed STOP.
    void stop(uint32_t now) { end(now, USER_STOP); }

    // Call often. scaleOk = connected and sending fresh weight.
    void tick(uint32_t now, float grams, bool scaleOk, float targetG) {
        switch (_state) {
        case IDLE:
            break;
        case HOLDING:
            if (checkEnd(now, grams, scaleOk, targetG)) break;
            if (now - _t0 >= _cfg.holdMs) { _button(false); _state = BREWING; }
            break;
        case BREWING:
            checkEnd(now, grams, scaleOk, targetG);
            break;
        case ABORT_GAP:
            if (now - _t0 >= _cfg.abortGapMs) pressStop(now);
            break;
        case STOP_PRESS:
            if (now - _t0 >= _cfg.stopPressMs) { _button(false); _state = IDLE; }
            break;
        }
    }

private:
    // Right after a tare the scale can still report the old weight for a moment.
    static const uint32_t GRACE_MS = 1500;

    bool checkEnd(uint32_t now, float grams, bool scaleOk, float targetG) {
        uint32_t elapsed = now - _shotStart;
        if (!scaleOk)                         { end(now, SCALE_LOST);     return true; }
        if (elapsed >= _cfg.maxShotMs)        { end(now, TIMEOUT);        return true; }
        if (elapsed >= GRACE_MS && grams >= targetG - _cfg.stopOffsetG)
                                              { end(now, TARGET_REACHED); return true; }
        return false;
    }

    void end(uint32_t now, Reason why) {
        if (_state == BREWING) {
            _reason = why;
            pressStop(now);
        } else if (_state == HOLDING) {
            // Still holding the start press: let go, pause, then do the stop press.
            _reason = why;
            _button(false);
            _t0 = now;
            _state = ABORT_GAP;
        }
    }

    void pressStop(uint32_t now) {
        _button(true);              // GPIO first - the BLE write can wait
        _scale(SC_STOP_TIMER);
        _t0 = now;
        _state = STOP_PRESS;
    }

    ShotConfig _cfg;
    ButtonFn   _button;
    ScaleFn    _scale;
    State      _state  = IDLE;
    Reason     _reason = NONE;
    uint32_t   _t0 = 0, _shotStart = 0;
};
