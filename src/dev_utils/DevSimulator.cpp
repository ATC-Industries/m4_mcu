#include "dev_utils/DevSimulator.h"

#include <Arduino.h>

#include "StateManager.h"

namespace devSimulator {

// Constants for more realistic behavior
const float INITIAL_ACCELERATION = 0.4f;      // Initial acceleration rate
const float SLED_RESISTANCE_START = 0.025f;   // Initial sled resistance factor
const float SLED_RESISTANCE_GAIN = 0.0006f;   // Base resistance increase with distance
const float SLED_EXPONENTIAL = 1.6f;          // Exponential factor for sled resistance
const float ENGINE_RESPONSE_DELAY = 0.85f;    // How quickly engine responds to load (lower = faster)
const float TRACK_VARIANCE = 0.015f;          // Soil/track condition variance
const float MAX_SPEED = 32.0f;                // Absolute maximum possible speed in ideal conditions
const float GEAR_SHIFT_POINT = 145.0f;        // Distance at which shifting typically happens
const float TRACK_END = 300.0f;               // End of the track in feet
const float BRAKING_DECELERATION = 1.2f;      // How quickly tractor slows when braking
const float DRIVER_SKILL_VARIANCE = 0.15f;    // How much driver skill affects performance
const float SECOND_ALARM_SLED_FACTOR = 2.0f;  // How much heavier the sled gets at second alarm point

// Simulation state variables
static unsigned long lastUpdate = 0;
static unsigned long pullStart = 0;
static float speed = 0.0f;
static float tach = 0.0f;
static float distance = 0.0f;
static bool fullPull = false;
static bool engineStalled = false;
static bool gearShifted = false;
static bool braking = false;
static float engineLoad = 0.0f;
static float trackCondition = 1.0f;       // Multiplier for traction (randomized at start)
static float sledWeight = 1.0f;           // Multiplier for sled weight (randomized at start)
static float driverSkill = 1.0f;          // Multiplier for driver performance (randomized at start)
static float maxPossibleDistance = 0.0f;  // Calculated max distance for this pull based on conditions
static bool passedFirstAlarm = false;     // Flag for passing first distance alarm point
static bool passedSecondAlarm = false;    // Flag for passing second distance alarm point
static bool isLimitingSpeed = false;      // Flag to indicate if driver is actively limiting speed
static bool isLimitingRPM = false;        // Flag to indicate if driver is actively limiting RPM
static float targetSpeed = 0.0f;          // Driver's target speed based on alarms

// Engine sound simulation
static int tachDirection = 1;
static float tachTargetRPM = 2200.0f;
static float tachVariance = 0.0f;

void update() {
  unsigned long now = millis();
  if (now - lastUpdate < 50) return;
  unsigned long deltaTime = now - lastUpdate;
  lastUpdate = now;

  // Reset simulation for a new pull
  if (pullStart == 0 || now - pullStart >= 20000) {
    pullStart = now;
    speed = 0.0f;
    tach = 0.0f;
    distance = 0.0f;
    fullPull = false;
    engineStalled = false;
    gearShifted = false;
    braking = false;
    engineLoad = 0.0f;
    tachDirection = 1;
    tachTargetRPM = 2200.0f;
    tachVariance = 0.0f;
    passedFirstAlarm = false;
    passedSecondAlarm = false;
    isLimitingSpeed = false;
    isLimitingRPM = false;
    targetSpeed = 0.0f;

    // Randomize track conditions for each pull (0.82-1.18 range)
    trackCondition = 0.82f + (random(361) / 1000.0f);

    // Randomize sled weight for each pull (0.9-1.15 range)
    sledWeight = 0.9f + (random(251) / 1000.0f);

    // Randomize driver skill for each pull (0.85-1.15 range)
    driverSkill = 0.85f + (random(301) / 1000.0f);

    // Calculate max possible distance for this run based on conditions
    // This creates runs that naturally stop between 200-300ft
    float runQuality = trackCondition * driverSkill / sledWeight;
    maxPossibleDistance = TRACK_END * (0.7f + (runQuality * 0.3f));

    // Ensure maxPossibleDistance is between 200-300ft with bias toward shorter pulls
    if (maxPossibleDistance > TRACK_END) maxPossibleDistance = TRACK_END;
    if (maxPossibleDistance < 200.0f) maxPossibleDistance = 200.0f + (random(501) / 10.0f);

    // Set alarms based on driver skill and competition class
    // More experienced drivers get closer to the limit
    float skillFactor = 0.8f + (driverSkill * 0.2f);

    // Randomize alarm points slightly to simulate different competition settings
    float distanceAlarm1 = 50.0f + (random(151) / 10.0f);       // Between 50-65ft
    float distanceAlarm2 = 180.0f + (random(41) / 10.0f);       // Between 180-184ft
    float tachAlarm1 = 2400.0f + (random(401));                 // Between 2400-2800 RPM
    float tachAlarm2 = tachAlarm1 + 150.0f + (random(251));     // 150-400 RPM above first alarm
    float mphAlarm1 = 18.0f + (random(51) / 10.0f);             // Between 18-23 MPH
    float mphAlarm2 = mphAlarm1 + 2.0f + (random(31) / 10.0f);  // 2-5 MPH above first alarm

    StateManager::prefs().distanceAlarm1 = distanceAlarm1;
    StateManager::prefs().distanceAlarm2 = distanceAlarm2;
    StateManager::prefs().tachAlarm1 = tachAlarm1;
    StateManager::prefs().tachAlarm2 = tachAlarm2;
    StateManager::prefs().mphAlarm1 = mphAlarm1;
    StateManager::prefs().mphAlarm2 = mphAlarm2;
  }

  unsigned long elapsed = now - pullStart;

  // Stage 1: Idling at start line
  if (elapsed < 3000) {
    speed = distance = 0.0f;
    tachTargetRPM = 900.0f + (random(201) - 100) / 10.0f;  // Idle around 900 RPM with small variations
    tachVariance = 50.0f;

    // Stage 2: Revving up before pull
  } else if (elapsed < 5000) {
    speed = distance = 0.0f;
    tachTargetRPM = 1800.0f + (random(401) - 200) / 10.0f;  // Rev around 1800 RPM
    tachVariance = 100.0f;

    // Stage 3: Pull begins
  } else if (!engineStalled && !fullPull) {
    // Check distance alarms
    if (!passedFirstAlarm && distance >= StateManager::prefs().distanceAlarm1) {
      passedFirstAlarm = true;

      // After first alarm, driver tries to maintain speed just under alarm1
      targetSpeed = StateManager::prefs().mphAlarm1 * (0.95f + (driverSkill * 0.04f));
      isLimitingSpeed = true;

      // Set appropriate relay for first distance alarm
      StateManager::setRelayState(2, RelayState::ENGAGED);
    }

    if (!passedSecondAlarm && distance >= StateManager::prefs().distanceAlarm2) {
      passedSecondAlarm = true;

      // After second alarm, increase sled weight dramatically
      sledWeight *= SECOND_ALARM_SLED_FACTOR;

      // Set appropriate relay for second distance alarm (sled pan drop)
      StateManager::setRelayState(3, RelayState::ENGAGED);
    }

    // Check if tach is getting too high, skilled drivers manage RPM better
    if (tach > StateManager::prefs().tachAlarm1 * (0.97f + (driverSkill * 0.03f))) {
      isLimitingRPM = true;
      tachTargetRPM = StateManager::prefs().tachAlarm1 * (0.95f + (driverSkill * 0.03f));
    } else {
      isLimitingRPM = false;
    }

    // Calculate sled resistance (increases with distance exponentially)
    float distanceRatio = distance / TRACK_END;
    float sledResistance = SLED_RESISTANCE_START +
                           (distance * SLED_RESISTANCE_GAIN * sledWeight) * pow(1.0f + distanceRatio, SLED_EXPONENTIAL);

    // Extra resistance as we approach the calculated max distance for this run
    if (distance > maxPossibleDistance * 0.8f) {
      float approachFactor = (distance - (maxPossibleDistance * 0.8f)) / (maxPossibleDistance * 0.2f);
      sledResistance += approachFactor * sledWeight * 0.4f;
    }

    // Driver braking near the end of track
    if (distance > TRACK_END - 10.0f && !braking) {
      braking = true;
    }

    float acceleration = 0.0f;

    if (braking) {
      // Hard braking at end of track
      acceleration = -BRAKING_DECELERATION;
    } else {
      // Normal acceleration calculation
      acceleration = (INITIAL_ACCELERATION * trackCondition * driverSkill) - sledResistance;

      // Random track variations (bumps, soil differences)
      acceleration += ((random(201) / 1000.0f) - 0.1f) * TRACK_VARIANCE;

      // Driver actively limits speed after first alarm point
      if (isLimitingSpeed && speed > targetSpeed) {
        // Apply "braking" proportional to how much over target speed
        float overSpeedFactor = (speed - targetSpeed) / 5.0f;
        acceleration -= overSpeedFactor * (0.1f + (driverSkill * 0.2f));
      }
    }

    // Apply acceleration to speed
    if (acceleration > 0) {
      speed += acceleration;
    } else {
      // Slowing down due to resistance or braking
      speed += acceleration * 2.0f;  // Deceleration happens faster than acceleration
    }

    // Full stop after braking
    if (braking && speed <= 0.1f) {
      speed = 0.0f;
    }

    // Speed limits - if we're over the second alarm, simulate being disqualified
    if (speed > StateManager::prefs().mphAlarm2 && passedFirstAlarm) {
      // Very skilled drivers catch this faster
      if (random(0, 100) < (30 + (driverSkill * 70))) {
        speed = StateManager::prefs().mphAlarm1;

        // Blink the speed alarm indicator
        StateManager::setLimitSwitchTriggered(0, true);
      }
    }

    // Gear shift simulation around typical shift point
    if (!gearShifted && distance > GEAR_SHIFT_POINT && distance < GEAR_SHIFT_POINT + 10) {
      speed *= 0.85f;  // Momentary speed drop during shift
      tach *= 0.7f;    // RPM drops during shift
      gearShifted = true;
    }

    // Engine load increases with distance as sled gets heavier
    engineLoad = min(1.0f, distance / 250.0f);

    // Limit max speed
    if (speed > MAX_SPEED) speed = MAX_SPEED;
    if (speed < 0.0f) speed = 0.0f;

    // Engine can stall if speed gets too low after starting the pull
    if (elapsed > 8000 && speed < 0.2f && distance > 10.0f) {
      engineStalled = true;
      tachTargetRPM = 0.0f;
    }

    // Calculate distance based on speed (mph to feet conversion factor)
    float distanceIncrement = speed * 0.05f * 1.467f;
    distance += distanceIncrement;

    // Target RPM changes based on load and speed
    tachTargetRPM = 2200.0f + (800.0f * (1.0f - engineLoad));

    // RPM variations increase with load
    tachVariance = 150.0f + (300.0f * engineLoad);

    // Check for full pull or stopping at max possible distance
    if (distance >= TRACK_END) {
      distance = TRACK_END;
      fullPull = true;
      braking = true;
    } else if (distance >= maxPossibleDistance && acceleration <= 0) {
      // Tractor bogged down and stopped
      if (speed < 0.5f) {
        speed = 0;
        fullPull = false;  // Not a full pull
      }
    }

    // Stage 4: After pull completion or engine stall
  } else {
    if (speed > 0.0f) {
      speed -= 0.3f;
      if (speed < 0.0f) speed = 0.0f;
    }

    if (engineStalled) {
      tachTargetRPM = 0.0f;
    } else {
      // After successful pull, engine revs down
      tachTargetRPM = max(900.0f, tachTargetRPM - 5.0f);
    }

    tachVariance = 50.0f;
  }

  // Smooth tach changes to mimic real engine behavior
  float tachDiff = tachTargetRPM - tach;
  tach += tachDiff * 0.1f;

  // Add realistic RPM fluctuations
  tach += ((random(int(tachVariance * 2 + 1)) - tachVariance) / 10.0f);

  if (tach < 0.0f) tach = 0.0f;

  // Randomly toggle relays and switches for dashboard effects
  // Tied more to the stages of the pull rather than just random
  if (elapsed > 5000 && random(0, 100) < 3) {
    // Fuel pump always engaged during active pull
    StateManager::setRelayState(0, RelayState::ENGAGED);

    // Water pump relay active at higher RPMs
    StateManager::setRelayState(1, tach > 2000.0f ? RelayState::ENGAGED : RelayState::DISENGAGED);

    // First distance alarm relay - managed in pull logic
    // Second distance alarm relay - managed in pull logic

    // Speed alarm indicator flashes if too close to limit
    if (passedFirstAlarm && speed > StateManager::prefs().mphAlarm1 * 0.95f) {
      // Flash speed warning
      StateManager::setLimitSwitchTriggered(0, (elapsed / 250) % 2 == 0);
    } else {
      StateManager::setLimitSwitchTriggered(0, false);
    }

    // RPM alarm indicator flashes if too close to limit
    if (tach > StateManager::prefs().tachAlarm1 * 0.95f) {
      // Flash RPM warning
      StateManager::setLimitSwitchTriggered(1, (elapsed / 250) % 2 == 0);
    } else {
      StateManager::setLimitSwitchTriggered(1, false);
    }
  }

  // Additional instrumentation to indicate pull quality
  static float maxReachedSpeed = 0.0f;
  if (speed > maxReachedSpeed) maxReachedSpeed = speed;

  // If the pull is done (either stopped or reached track end)
  if ((fullPull || (elapsed > 8000 && speed < 0.1f && distance > 10.0f)) && elapsed > 5000) {
    // Log max stats to serial debug if available
    if (Serial) {
      if (elapsed % 1000 < 100) {  // Only print occasionally
        Serial.print("Pull stats - Max Speed: ");
        Serial.print(maxReachedSpeed);
        Serial.print(" MPH, Distance: ");
        Serial.print(distance);
        Serial.print(" ft, Full Pull: ");
        Serial.print(fullPull ? "YES" : "NO");
        Serial.print(", Alarms: Speed=");
        Serial.print(StateManager::prefs().mphAlarm1);
        Serial.print("/");
        Serial.print(StateManager::prefs().mphAlarm2);
        Serial.print(" Dist=");
        Serial.print(StateManager::prefs().distanceAlarm1);
        Serial.print("/");
        Serial.print(StateManager::prefs().distanceAlarm2);
        Serial.println();
      }
    }
  } else if (elapsed < 3000) {
    // Reset max stats at the beginning of each pull
    maxReachedSpeed = 0.0f;
  }

  // Update state
  StateManager::setSpeed(speed);
  StateManager::setRPM(tach);
  StateManager::setDistance(distance);
}

}  // namespace devSimulator