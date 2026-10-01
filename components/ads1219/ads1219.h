/**
 * @file ads1219.h
 * @brief Public API for the ads1219 ESPHome component
 * @author Soldered Electronics
 *
 * Driver for the Texas Instruments ADS1219 24-bit, 4-channel delta-sigma ADC, ported from the Soldered ADS1219
 * Arduino library (itself based on the SparkFun ADS1219 Arduino library). The hub owns the chip and runs one
 * single-shot conversion at a time; every ADS1219Sensor queues a conversion with its own multiplexer, gain and data
 * rate and gets the result published once the chip reports data ready. Conversions are non-blocking, the main loop is
 * never stalled for the duration of a conversion (up to ~50 ms at 20 SPS).
 */

#pragma once

#include <vector>

#include "esphome/components/i2c/i2c.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

namespace esphome {
namespace ads1219 {

/// Input multiplexer setting, config register bits [7:5].
enum ADS1219Multiplexer : uint8_t {
  ADS1219_MUX_P0_N1 = 0b000,    ///< differential AIN0 - AIN1 (power-on default)
  ADS1219_MUX_P2_N3 = 0b001,    ///< differential AIN2 - AIN3
  ADS1219_MUX_P1_N2 = 0b010,    ///< differential AIN1 - AIN2
  ADS1219_MUX_P0_NG = 0b011,    ///< single-ended AIN0 - AGND
  ADS1219_MUX_P1_NG = 0b100,    ///< single-ended AIN1 - AGND
  ADS1219_MUX_P2_NG = 0b101,    ///< single-ended AIN2 - AGND
  ADS1219_MUX_P3_NG = 0b110,    ///< single-ended AIN3 - AGND
  ADS1219_MUX_SHORTED = 0b111,  ///< both inputs shorted to AVDD / 2, for offset measurement
};

/// PGA gain, config register bit [4].
enum ADS1219Gain : uint8_t {
  ADS1219_GAIN_1 = 0,  ///< full-scale = +/- VREF
  ADS1219_GAIN_4 = 1,  ///< full-scale = +/- VREF / 4
};

/// Data rate, config register bits [3:2].
enum ADS1219DataRate : uint8_t {
  ADS1219_DATA_RATE_20SPS = 0b00,
  ADS1219_DATA_RATE_90SPS = 0b01,
  ADS1219_DATA_RATE_330SPS = 0b10,
  ADS1219_DATA_RATE_1000SPS = 0b11,
};

/// Voltage reference, config register bit [0].
enum ADS1219VoltageReference : uint8_t {
  ADS1219_VREF_INTERNAL = 0,  ///< internal 2.048 V reference (power-on default)
  ADS1219_VREF_EXTERNAL = 1,  ///< external reference between the REFP and REFN pins
};

class ADS1219Sensor;

/// Hub component, owns the I2C device and serializes conversions requested by ADS1219Sensor instances.
class ADS1219Component : public Component, public i2c::I2CDevice {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  void set_voltage_reference(ADS1219VoltageReference voltage_reference) {
    this->voltage_reference_ = voltage_reference;
  }
  void set_reference_voltage(float reference_voltage) { this->reference_voltage_ = reference_voltage; }

  /// Queue a single-shot conversion for the given sensor, the result is published from loop() when ready.
  void request_measurement(ADS1219Sensor *sensor);

 protected:
  bool start_conversion_(ADS1219Sensor *sensor);
  void finish_conversion_();
  bool write_command_(uint8_t command);

  ADS1219VoltageReference voltage_reference_{ADS1219_VREF_INTERNAL};
  float reference_voltage_{2.048f};

  std::vector<ADS1219Sensor *> queue_;
  bool converting_{false};
  uint32_t conversion_start_{0};
  uint32_t conversion_wait_{0};
  uint32_t conversion_timeout_{0};
};

/// One ADC channel, publishes the measured voltage in volts.
class ADS1219Sensor : public sensor::Sensor, public PollingComponent, public Parented<ADS1219Component> {
 public:
  void update() override;
  void dump_config() override;

  void set_multiplexer(ADS1219Multiplexer multiplexer) { this->multiplexer_ = multiplexer; }
  void set_gain(ADS1219Gain gain) { this->gain_ = gain; }
  void set_data_rate(ADS1219DataRate data_rate) { this->data_rate_ = data_rate; }

  ADS1219Multiplexer get_multiplexer() const { return this->multiplexer_; }
  ADS1219Gain get_gain() const { return this->gain_; }
  ADS1219DataRate get_data_rate() const { return this->data_rate_; }

 protected:
  ADS1219Multiplexer multiplexer_{ADS1219_MUX_P0_N1};
  ADS1219Gain gain_{ADS1219_GAIN_1};
  ADS1219DataRate data_rate_{ADS1219_DATA_RATE_20SPS};
};

}  // namespace ads1219
}  // namespace esphome
