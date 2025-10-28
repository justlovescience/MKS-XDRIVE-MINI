#pragma once
#include <Arduino.h>
#include <ESP32-TWAI-CAN.hpp>   // handmade0octopus driver
#include "ODriveCAN.h"          // provides CREATE_CAN_INTF_WRAPPER + ODriveCanIntfWrapper

//crappy ai code, use with skepticism.

extern void onCanFrame(uint32_t id, uint8_t len, const uint8_t* data);

// --- Required by ODriveCAN.h ---
// Send one classic CAN frame
inline bool sendMsg(TwaiCAN& can, uint32_t id, uint8_t length, const uint8_t* data) {
  CanFrame f{};
  f.identifier        = id;
  f.extd              = 0;                          // 11-bit ID
  f.rtr               = (data == nullptr);          // null data => RTR (remote request)
  uint8_t n           = (length > 8) ? 8 : length;  // classic CAN only
  f.data_length_code  = n;
  if (data && n) memcpy(f.data, data, n);
  return can.writeFrame(f);                         // non-blocking send (timeout overload exists)
}

// Pump the driver and forward any frames to your hook
inline void pumpEvents(TwaiCAN& can) {
  for (;;) {
    CanFrame f{};
    if (!can.readFrame(f, 0)) break;               // 0ms timeout: poll once
    uint8_t n = f.data_length_code > 8 ? 8 : f.data_length_code;
    uint8_t buf[8]; if (n) memcpy(buf, f.data, n);
    onCanFrame(f.identifier, n, buf);
  }
}

// Generate: ODriveCanIntfWrapper wrap_can_intf(TwaiCAN& intf)
CREATE_CAN_INTF_WRAPPER(TwaiCAN)
