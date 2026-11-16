// Bookoo Themis (Mini / Ultra) BLE protocol helpers. No hardware dependencies.
// Reference: github.com/BooKooCode/OpenSource -> bookoo_ultra_scale/protocols.md
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace bookoo {

const uint16_t UUID_SERVICE = 0x0FFE;
const uint16_t UUID_DATA    = 0xFF11;   // notify: weight frames
const uint16_t UUID_COMMAND = 0xFF12;   // write: commands

enum Cmd : uint8_t {
    CMD_TARE           = 0x01,
    CMD_TIMER_START    = 0x04,
    CMD_TIMER_STOP     = 0x05,
    CMD_TIMER_RESET    = 0x06,
    CMD_TARE_AND_START = 0x07,
    CMD_SET_MODE       = 0x0C,   // app-observed, undocumented: v1 = v2 = mode (1 = flow)
};

struct Frame { uint8_t b[6]; };

// 03 0A <cmd> <v1> <v2> <xor of the first five bytes>
inline Frame makeCmd(uint8_t cmd, uint8_t v1 = 0, uint8_t v2 = 0) {
    Frame f = {{0x03, 0x0A, cmd, v1, v2, 0}};
    f.b[5] = f.b[0] ^ f.b[1] ^ f.b[2] ^ f.b[3] ^ f.b[4];
    return f;
}

struct Reading {
    float    grams;
    float    flow;      // g/s
    uint32_t timerMs;   // the scale's own timer
    uint8_t  battery;   // %
};

// Parses a 20-byte "03 0B" weight notification. Returns false for anything else.
inline bool parseWeight(const uint8_t* d, size_t len, Reading& out) {
    if (len < 20 || d[0] != 0x03 || d[1] != 0x0B) return false;
    uint8_t x = 0;
    for (int i = 0; i < 19; i++) x ^= d[i];
    if (x != d[19]) return false;

    out.timerMs = ((uint32_t)d[2] << 16) | ((uint32_t)d[3] << 8) | d[4];
    uint32_t w  = ((uint32_t)d[7] << 16) | ((uint32_t)d[8] << 8) | d[9];
    out.grams   = w / 100.0f;
    if (d[6] == '-') out.grams = -out.grams;
    out.flow    = (((uint16_t)d[11] << 8) | d[12]) / 100.0f;
    if (d[10] == '-') out.flow = -out.flow;
    out.battery = d[13];
    return true;
}

}  // namespace bookoo
