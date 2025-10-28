// ESP32 + TWAI + ODrive: Official example, simplified for ESP32 TWAI only
// Crappy ai code, take it with a grain of salt

#include <Arduino.h>
#include <ESP32-TWAI-CAN.hpp>    // handmade0octopus driver
#include "ODriveCAN.h"
#include "ODriveEsp32Twai.hpp"   // provides pumpEvents(), wrap_can_intf(); requires onCanFrame()

/* ----------------- User-config ----------------- */

// CAN bus baudrate: must match all devices on the bus (official example uses 250k)
#define CAN_BAUDRATE     500000

// ODrive node_id for odrv0
#define ODRV0_NODE_ID    0

// ESP32 TWAI pins
#define TX_GPIO_NUM      5
#define RX_GPIO_NUM      4

/* ----------------- CAN interface ---------------- */

auto& can_intf = ESP32Can;

// Minimal CAN setup for ESP32 TWAI driver
static bool setupCan() {
  const auto kbps = CAN_BAUDRATE / 1000;
  // You can omit setPins if begin() accepts pins; kept explicit for clarity
  ESP32Can.setPins(TX_GPIO_NUM, RX_GPIO_NUM);
  ESP32Can.setRxQueueSize(16);
  ESP32Can.setTxQueueSize(16);
  return ESP32Can.begin(ESP32Can.convertSpeed(kbps), TX_GPIO_NUM, RX_GPIO_NUM);
}

/* ----------------- ODrive wiring ---------------- */

// Instantiate ODrive objects
ODriveCAN odrv0(wrap_can_intf(can_intf), ODRV0_NODE_ID);
ODriveCAN* odrives[] = { &odrv0 };

// Per-ODrive user data (same fields as official example)
struct ODriveUserData {
  Heartbeat_msg_t              last_heartbeat;
  bool                         received_heartbeat = false;
  Get_Encoder_Estimates_msg_t  last_feedback;
  bool                         received_feedback  = false;
} odrv0_user_data;

// Called on Heartbeat from ODrive
void onHeartbeat(Heartbeat_msg_t& msg, void* user_data) {
  auto* d = static_cast<ODriveUserData*>(user_data);
  d->last_heartbeat = msg;
  d->received_heartbeat = true;
}

// Called on encoder feedback from ODrive
void onFeedback(Get_Encoder_Estimates_msg_t& msg, void* user_data) {
  auto* d = static_cast<ODriveUserData*>(user_data);
  d->last_feedback = msg;
  d->received_feedback = true;
}

// Your TWAI adapter expects this hook; forward to all ODriveCAN instances
void onCanFrame(uint32_t id, uint8_t len, const uint8_t* data) {
  for (auto* odrive : odrives) {
    odrive->onReceive(id, len, data);
  }
}

/* ----------------- Setup ---------------- */

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 30 && !Serial; ++i) delay(100);
  delay(200);

  Serial.println("Starting ODriveCAN demo (ESP32 TWAI)");

  // Register ODrive callbacks
  odrv0.onFeedback(onFeedback, &odrv0_user_data);
  odrv0.onStatus(onHeartbeat, &odrv0_user_data);

  // Init CAN
  if (!setupCan()) {
    Serial.println("CAN failed to initialize: reset required");
    while (true) { delay(50); }
  }

  // Wait for ODrive heartbeat (pump events; add tiny yield)
  Serial.println("Waiting for ODrive...");
  while (!odrv0_user_data.received_heartbeat) {
    pumpEvents(can_intf);
    delay(1);
  }
  Serial.println("Found ODrive");

  // Request bus voltage/current (1s timeout)
  Serial.println("Attempting to read bus voltage and current");
  Get_Bus_Voltage_Current_msg_t vbus;
  if (!odrv0.request(vbus, 1000)) {
    Serial.println("vbus request failed!");
    while (true) { delay(50); }
  }
  Serial.print("DC voltage [V]: "); Serial.println(vbus.Bus_Voltage);
  Serial.print("DC current [A]: "); Serial.println(vbus.Bus_Current);

  // Enter CLOSED_LOOP_CONTROL with periodic event pumping (mirrors official flow)
  Serial.println("Enabling CLOSED_LOOP_CONTROL...");
  while (odrv0_user_data.last_heartbeat.Axis_State != ODriveAxisState::AXIS_STATE_CLOSED_LOOP_CONTROL) {
    odrv0.clearErrors();
    delay(1);
    odrv0.setState(ODriveAxisState::AXIS_STATE_CLOSED_LOOP_CONTROL);

    // Pump events for ~150ms to ensure reliable state transition even on busy bus
    for (int i = 0; i < 15; ++i) {
      delay(10);
      pumpEvents(can_intf);
    }
  }

  Serial.println("ODrive running!");
}

/* ----------------- Loop ---------------- */

void loop() {
  // Required to handle incoming feedback frames
  pumpEvents(can_intf);

  // Official example motion: sine position with velocity feedforward
  const float SINE_PERIOD_S = 2.0f;
  float t = 0.001f * millis();
  float phase = t * (TWO_PI / SINE_PERIOD_S);

  odrv0.setPosition(
    sinf(phase),                          // position (turns)
    cosf(phase) * (TWO_PI / SINE_PERIOD_S) // velocity feedforward
  );

  // Print position & velocity for Serial Plotter
  if (odrv0_user_data.received_feedback) {
    auto fb = odrv0_user_data.last_feedback;
    odrv0_user_data.received_feedback = false;
    Serial.print("odrv0-pos:");
    Serial.print(fb.Pos_Estimate);
    Serial.print(",");
    Serial.print("odrv0-vel:");
    Serial.println(fb.Vel_Estimate);
  }

  // Keep loop light; TWAI RX is already polled via pumpEvents()
  delay(2);
}