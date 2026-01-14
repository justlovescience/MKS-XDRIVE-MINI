/*
 *  One way motion  control test
 * 
 * Test setup:
 * - Motor 0: Torque control mode with zero torque (move by hand)
 * - Motor 1: Position control mode following Motor 0's position
 * 
 * This tests the transition from torque control to position control.
 * 
 * Hardware:
 * - ESP32 + CAN transceiver
 * - Two ODrive motors (Node 0 and 1)
 */

#include <Arduino.h>
#include <ESP32-TWAI-CAN.hpp>
#include "ODriveCAN.h"
#include "ODriveEsp32Twai.hpp"

// Configuration
#define CAN_BAUDRATE   500000
#define TX_GPIO_NUM    5
#define RX_GPIO_NUM    4
#define ODRV0_NODE_ID  0
#define ODRV1_NODE_ID  1

// Position control parameters
#define POS_GAIN       20.0f   // Position gain for slave motor
#define VEL_GAIN       0.1f    // Velocity gain for slave motor
#define VEL_INT_GAIN   0.0f    // Velocity integrator gain
#define VEL_LIMIT      10.0f   // Velocity limit (rev/s)
#define CURRENT_LIMIT  10.0f   // Current limit (A)

// Update frequency parameters
#define UPDATE_FREQ_HZ  100    // Update frequency in Hz (100Hz = 10ms)
#define PRINT_FREQ_HZ  5       // Print frequency in Hz (5Hz = 200ms)

auto& can_intf = ESP32Can;

// ODrive instances
ODriveCAN odrv0(wrap_can_intf(can_intf), ODRV0_NODE_ID);
ODriveCAN odrv1(wrap_can_intf(can_intf), ODRV1_NODE_ID);

// Motor data
struct MotorData {
  float position_rad = 0.0f;
  float velocity_rad_s = 0.0f;
  bool has_feedback = false;
} master, slave;

// Callbacks
void onFB0(Get_Encoder_Estimates_msg_t& msg, void* user) {
  auto* d = static_cast<MotorData*>(user);
  d->position_rad = msg.Pos_Estimate * TWO_PI;
  d->velocity_rad_s = msg.Vel_Estimate * TWO_PI;
  d->has_feedback = true;
}

void onFB1(Get_Encoder_Estimates_msg_t& msg, void* user) {
  auto* d = static_cast<MotorData*>(user);
  d->position_rad = msg.Pos_Estimate * TWO_PI;
  d->velocity_rad_s = msg.Vel_Estimate * TWO_PI;
  d->has_feedback = true;
}

void onCanFrame(uint32_t id, uint8_t len, const uint8_t* data) {
  odrv0.onReceive(id, len, data);
  odrv1.onReceive(id, len, data);
}

// CAN setup
bool setupCan() {
  const auto kbps = CAN_BAUDRATE / 1000;
  can_intf.setPins(TX_GPIO_NUM, RX_GPIO_NUM);
  can_intf.setRxQueueSize(16);
  can_intf.setTxQueueSize(16);
  return can_intf.begin(can_intf.convertSpeed(kbps), TX_GPIO_NUM, RX_GPIO_NUM);
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 4000) {}

  Serial.println("Torque to Position Control Test");
  Serial.println("===============================");
  Serial.println("Motor 0: Torque control (zero torque - move by hand)");
  Serial.println("Motor 1: Position control (follows Motor 0)");
  Serial.println();

  // Register callbacks
  odrv0.onFeedback(onFB0, &master);
  odrv1.onFeedback(onFB1, &slave);

  // Setup CAN
  if (!setupCan()) {
    Serial.println("CAN failed!");
    while (true) delay(100);
  }

  // Wait for ODrives to be ready
  Serial.println("Waiting for ODrives...");
  delay(2000);
  Serial.println("ODrives ready!");

  // Configure Motor 0 for torque control
  Serial.println("Configuring Motor 0 (Master) for torque control...");
  odrv0.setControllerMode(ODriveControlMode::CONTROL_MODE_TORQUE_CONTROL, ODriveInputMode::INPUT_MODE_PASSTHROUGH);
  odrv0.clearErrors();
  odrv0.setState(ODriveAxisState::AXIS_STATE_CLOSED_LOOP_CONTROL);

  // Configure Motor 1 for position control
  Serial.println("Configuring Motor 1 (Slave) for position control...");
  odrv1.setControllerMode(ODriveControlMode::CONTROL_MODE_POSITION_CONTROL, ODriveInputMode::INPUT_MODE_PASSTHROUGH);
  odrv1.setPosGain(POS_GAIN);
  odrv1.setVelGains(VEL_GAIN, VEL_INT_GAIN);
  odrv1.setLimits(VEL_LIMIT, CURRENT_LIMIT);
  odrv1.clearErrors();
  odrv1.setState(ODriveAxisState::AXIS_STATE_CLOSED_LOOP_CONTROL);

  // Wait for both motors to be in closed loop
  delay(3000);
  
  Serial.println("Configuration complete!");
  Serial.println("Motor 0: Torque control (move by hand)");
  Serial.println("Motor 1: Position control (will follow Motor 0)");
  Serial.println("Position Gain: " + String(POS_GAIN));
  Serial.println("Velocity Gain: " + String(VEL_GAIN));
  Serial.println("Velocity Limit: " + String(VEL_LIMIT) + " rev/s");
  Serial.println("Update Frequency: " + String(UPDATE_FREQ_HZ) + " Hz");
  Serial.println("Print Frequency: " + String(PRINT_FREQ_HZ) + " Hz");
  Serial.println();
}

void loop() {
  static uint32_t last_update = 0;
  static uint32_t last_print = 0;
  
  // Calculate update intervals
  uint32_t update_interval_ms = 1000 / UPDATE_FREQ_HZ;
  uint32_t print_interval_ms = 1000 / PRINT_FREQ_HZ;
  
  pumpEvents(can_intf);

  // Update control at specified frequency
  if (millis() - last_update >= update_interval_ms) {
    // Get feedback from both motors
    Get_Encoder_Estimates_msg_t fb;
    if (odrv0.getFeedback(fb, 10)) {
      master.position_rad = fb.Pos_Estimate * TWO_PI;
      master.velocity_rad_s = fb.Vel_Estimate * TWO_PI;
      master.has_feedback = true;
    }
    
    if (odrv1.getFeedback(fb, 10)) {
      slave.position_rad = fb.Pos_Estimate * TWO_PI;
      slave.velocity_rad_s = fb.Vel_Estimate * TWO_PI;
      slave.has_feedback = true;
    }

    // Motor 0: Apply zero torque (move by hand)
    odrv0.setTorque(0.0f);

    // Motor 1: Set position to follow Motor 0
    if (master.has_feedback && slave.has_feedback) {
      // Convert master position from radians to revolutions for ODrive
      float master_pos_rev = master.position_rad / TWO_PI;
      
      // Set slave motor position to match master
      odrv1.setPosition(master_pos_rev);
    }
    
    last_update = millis();
  }

  // Print status at specified frequency
  if (millis() - last_print >= print_interval_ms) {
    if (master.has_feedback && slave.has_feedback) {
      float pos_diff_rad = master.position_rad - slave.position_rad;
      float pos_diff_rev = pos_diff_rad / TWO_PI;
      
      Serial.print("Master: "); Serial.print(master.position_rad, 3);
      Serial.print("rad ("); Serial.print(master.position_rad / TWO_PI, 3);
      Serial.print("rev) | Slave: "); Serial.print(slave.position_rad, 3);
      Serial.print("rad ("); Serial.print(slave.position_rad / TWO_PI, 3);
      Serial.print("rev) | Diff: "); Serial.print(pos_diff_rad, 3);
      Serial.print("rad ("); Serial.print(pos_diff_rev, 3);
      Serial.print("rev) | Freq: "); Serial.print(UPDATE_FREQ_HZ);
      Serial.println("Hz");
    }
    last_print = millis();
  }

  // Small delay to prevent overwhelming the system
  delay(1);
}
