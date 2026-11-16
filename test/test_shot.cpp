// PC-side check of the shot sequence and protocol helpers:
//   g++ -std=c++17 -Isrc test/test_shot.cpp -o /tmp/t && /tmp/t
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>
#include "bookoo.h"
#include "shot.h"

static std::vector<std::string> ev;
static uint32_t T;
static void btn(bool p) { ev.push_back(std::to_string(T) + (p ? ":DOWN" : ":UP")); }
static void sc(ShotController::ScaleCmd c) { ev.push_back(std::to_string(T) + ":cmd" + std::to_string(c)); }
static const ShotConfig cfg = {10000, 250, 300, 90000, 0.0f, false};

static void dump(const char* name) { printf("%s:", name); for (auto& e : ev) printf(" %s", e.c_str()); printf("\n"); }

int main() {
    // --- normal shot: weight rises 2 g/s after 12 s, target 36 g
    { ev.clear(); ShotController s(cfg, btn, sc); T = 1000; assert(s.start(T));
      for (T = 1000; T < 60000; T += 5) { float w = T > 13000 ? (T - 13000) * 0.002f : 0; s.tick(T, w, true, 36.0f); }
      dump("normal");
      std::vector<std::string> want = {"1000:DOWN","1000:cmd0","1000:cmd1","11000:UP","31000:DOWN","31000:cmd3","31250:UP"};
      assert(ev == want); assert(!s.active()); assert(s.lastReason() == ShotController::TARGET_REACHED); }
    // --- user stop during the 10 s hold
    { ev.clear(); ShotController s(cfg, btn, sc); T = 0; s.start(T);
      for (T = 0; T < 20000; T += 5) { if (T == 4000) s.stop(T); s.tick(T, 0, true, 36.0f); }
      dump("abort ");
      std::vector<std::string> want = {"0:DOWN","0:cmd0","0:cmd1","4000:UP","4300:DOWN","4300:cmd3","4550:UP"};
      assert(ev == want); assert(s.lastReason() == ShotController::USER_STOP); }
    // --- scale lost mid-shot, and timeout
    { ev.clear(); ShotController s(cfg, btn, sc); T = 0; s.start(T);
      for (T = 0; T < 30000; T += 5) s.tick(T, 1.0f, T < 15000, 36.0f);
      dump("lost  "); assert(ev.back() == "15250:UP"); assert(s.lastReason() == ShotController::SCALE_LOST); }
    { ev.clear(); ShotController s(cfg, btn, sc); T = 0; s.start(T);
      for (T = 0; T < 100000; T += 5) s.tick(T, 1.0f, true, 36.0f);
      dump("tmout "); assert(ev.back() == "90250:UP"); assert(s.lastReason() == ShotController::TIMEOUT); }
    // --- millis() wrap-around mid-shot
    { ev.clear(); ShotController s(cfg, btn, sc); uint32_t t0 = 0xFFFFF000u; T = t0; s.start(T);
      for (uint32_t i = 0; i < 40000; i += 5) { T = t0 + i; s.tick(T, i > 20000 ? 40.0f : 0, true, 36.0f); }
      assert(ev.size() == 7 && !s.active()); }

    // --- protocol
    auto f = bookoo::makeCmd(bookoo::CMD_TIMER_START); assert(f.b[5] == 0x0D);
    f = bookoo::makeCmd(bookoo::CMD_TIMER_STOP);  assert(f.b[5] == 0x0C);
    f = bookoo::makeCmd(bookoo::CMD_TIMER_RESET); assert(f.b[5] == 0x0F);
    f = bookoo::makeCmd(bookoo::CMD_TARE);        assert(f.b[5] == 0x08);
    f = bookoo::makeCmd(bookoo::CMD_TARE_AND_START); assert(f.b[5] == 0x0E);
    f = bookoo::makeCmd(bookoo::CMD_SET_MODE, 1, 1); assert(f.b[5] == 0x05);
    uint8_t d[20] = {0x03,0x0B, 0x00,0x61,0xA8, 0x01, '+', 0x00,0x0E,0x10, '+', 0x00,0xC8, 87, 0,0,0,0,0, 0};
    for (int i = 0; i < 19; i++) d[19] ^= d[i];
    bookoo::Reading r; assert(bookoo::parseWeight(d, 20, r));
    assert(r.timerMs == 25000 && r.grams == 36.0f && r.flow == 2.0f && r.battery == 87);
    d[6] = '-'; d[19] ^= '+' ^ '-'; assert(bookoo::parseWeight(d, 20, r) && r.grams == -36.0f);
    d[19] ^= 1; assert(!bookoo::parseWeight(d, 20, r));
    puts("ALL OK");
}
