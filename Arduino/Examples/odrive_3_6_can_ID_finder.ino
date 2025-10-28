// -----------------------------------------------
// ODrive CANSimple Heartbeat ID Finder (ESP32-S3)
// - Logs heartbeat frames and which node IDs send them
// - No ODriveCAN dependency (pure TWAI frames)
// - Crappy ai code, do not take it seriously 
// -----------------------------------------------

#include <Arduino.h>
#include <ESP32-TWAI-CAN.hpp>

// --- User config ---
#define CAN_BAUDRATE       500000        // match your ODrives
#define TWAI_TX_GPIO       5             // change if needed
#define TWAI_RX_GPIO       4
#define HEARTBEAT_CMD_ID   0x01          // ODrive CANSimple heartbeat
#define STALE_MS           1500          // "recent" window for summary

// Uncomment for very chatty raw frame logging
// #define LOG_RAW_CAN

auto& CANX = ESP32Can;

// Helpers for ODrive CANSimple standard 11-bit ID
static inline uint16_t nodeFromStdId(uint32_t id) { return (id & 0x7FFu) >> 5; }
static inline uint8_t  cmdFromStdId(uint32_t id)  { return (uint8_t)(id & 0x1Fu); }

struct Track {
  bool ever = false;
  uint32_t last_ms = 0;
  uint32_t hb_count = 0;
};
static Track seen[64];        // node IDs 0..63
static uint32_t last_summary = 0;

static bool setupCan() {
  const auto kbps = CAN_BAUDRATE / 1000;
  CANX.setPins(TWAI_TX_GPIO, TWAI_RX_GPIO);
  CANX.setRxQueueSize(128);
  CANX.setTxQueueSize(32);
  return CANX.begin(CANX.convertSpeed(kbps), TWAI_TX_GPIO, TWAI_RX_GPIO);
}

void setup() {
  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 4000) {}

  Serial.println("\nODrive Heartbeat ID Finder (raw CAN)");
  if (!setupCan()) {
    Serial.println("CAN init FAILED");
    while (true) delay(100);
  }
  Serial.printf("CAN up @ %u bps  TX=%d RX=%d\n", CAN_BAUDRATE, TWAI_TX_GPIO, TWAI_RX_GPIO);
  Serial.println("Power each board/axis; I’ll print node IDs that heartbeat.");
}

void loop() {
  // Drain RX queue
  for (;;) {
    CanFrame f{};
    if (!CANX.readFrame(f, 0)) break;

    const uint32_t std_id = (f.identifier & 0x7FFu); // 11-bit
    const uint8_t  cmd    = cmdFromStdId(std_id);
    const uint8_t  node   = (uint8_t)nodeFromStdId(std_id);

#ifdef LOG_RAW_CAN
    Serial.printf("RX id=0x%03X node=%u cmd=%u len=%u\n",
                  (unsigned)std_id, node, cmd, f.data_length_code);
#endif

    if (cmd != HEARTBEAT_CMD_ID) continue; // only care about heartbeat

    Track &t = seen[node];
    uint32_t now = millis();

    if (!t.ever || (now - t.last_ms) > STALE_MS) {
      // New node or reappeared after being stale
      Serial.printf("HEARTBEAT: node %u (first seen / reappeared)\n", node);
    } else {
      // Regular heartbeat (comment this if too chatty)
      Serial.printf("HEARTBEAT: node %u\n", node);
    }

    t.ever = true;
    t.last_ms = now;
    t.hb_count++;
  }

  // 1 Hz summary of all nodes seen recently
  uint32_t now = millis();
  if (now - last_summary >= 1000) {
    last_summary = now;
    Serial.print("Recent nodes (≤ "); Serial.print(STALE_MS); Serial.print(" ms): ");
    bool any = false;
    for (uint8_t n = 0; n < 64; ++n) {
      if (seen[n].ever && (now - seen[n].last_ms) <= STALE_MS) {
        Serial.print((int)n); Serial.print(' ');
        any = true;
      }
    }
    if (!any) Serial.print("(none)");
    Serial.println();
  }

  delay(1);
}