/**
 * @file ads1219.cpp
 * @brief Implementation of the ads1219 ESPHome component
 * @author Soldered Electronics
 */

#include "ads1219.h"

#include <algorithm>
#include <cmath>

#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome {
namespace ads1219 {

static const char *const TAG = "ads1219";

static const uint8_t ADS1219_CMD_RESET = 0x06;
static const uint8_t ADS1219_CMD_START_SYNC = 0x08;
static const uint8_t ADS1219_CMD_RDATA = 0x10;
static const uint8_t ADS1219_REG_CONFIG_READ = 0x20;
static const uint8_t ADS1219_REG_STATUS_READ = 0x24;
static const uint8_t ADS1219_REG_CONFIG_WRITE = 0x40;

static const uint8_t ADS1219_STATUS_DRDY = 0x80;

/// Extra time on top of a few conversion periods before a conversion is declared lost.
static const uint32_t ADS1219_TIMEOUT_MARGIN_MS = 100;

static uint32_t data_rate_to_sps(ADS1219DataRate data_rate) {
  switch (data_rate) {
    case ADS1219_DATA_RATE_90SPS:
      return 90;
    case ADS1219_DATA_RATE_330SPS:
      return 330;
    case ADS1219_DATA_RATE_1000SPS:
      return 1000;
    case ADS1219_DATA_RATE_20SPS:
    default:
      return 20;
  }
}

void ADS1219Component::setup() {
  if (!this->write_command_(ADS1219_CMD_RESET)) {
    ESP_LOGE(TAG, "Reset command failed");
    this->mark_failed();
    return;
  }
  delay(1);  // datasheet: >100 us after reset before the next command

  uint8_t config;
  if (this->read_register(ADS1219_REG_CONFIG_READ, &config, 1) != i2c::ERROR_OK) {
    ESP_LOGE(TAG, "Reading config register failed");
    this->mark_failed();
    return;
  }
  if (config != 0x00) {
    ESP_LOGE(TAG, "Unexpected config register value after reset: 0x%02X", config);
    this->mark_failed();
    return;
  }

  // Nothing to do until a sensor requests a conversion
  this->disable_loop();
}

void ADS1219Component::dump_config() {
  ESP_LOGCONFIG(TAG, "ADS1219:");
  LOG_I2C_DEVICE(this);
  if (this->is_failed()) {
    ESP_LOGE(TAG, ESP_LOG_MSG_COMM_FAIL);
  }
  ESP_LOGCONFIG(TAG, "  Voltage reference: %s, %.3f V",
                this->voltage_reference_ == ADS1219_VREF_EXTERNAL ? "external" : "internal", this->reference_voltage_);
}

void ADS1219Component::request_measurement(ADS1219Sensor *sensor) {
  if (this->is_failed())
    return;
  if (std::find(this->queue_.begin(), this->queue_.end(), sensor) != this->queue_.end()) {
    ESP_LOGD(TAG, "'%s': previous conversion still pending, skipping", sensor->get_name().c_str());
    return;
  }
  this->queue_.push_back(sensor);
  this->enable_loop();
}

void ADS1219Component::loop() {
  if (!this->converting_) {
    if (this->queue_.empty()) {
      this->disable_loop();
      return;
    }
    if (!this->start_conversion_(this->queue_.front())) {
      this->status_set_warning();
      this->queue_.erase(this->queue_.begin());
    }
    return;
  }

  uint32_t elapsed = millis() - this->conversion_start_;
  if (elapsed < this->conversion_wait_)
    return;

  ADS1219Sensor *sensor = this->queue_.front();

  uint8_t status;
  if (this->read_register(ADS1219_REG_STATUS_READ, &status, 1) != i2c::ERROR_OK) {
    ESP_LOGW(TAG, "'%s': reading status register failed", sensor->get_name().c_str());
    this->status_set_warning();
    this->finish_conversion_();
    return;
  }
  if ((status & ADS1219_STATUS_DRDY) == 0) {
    if (elapsed > this->conversion_timeout_) {
      ESP_LOGW(TAG, "'%s': conversion timed out", sensor->get_name().c_str());
      this->status_set_warning();
      this->finish_conversion_();
    }
    return;
  }

  // RDATA returns the result as 24-bit big-endian two's complement
  uint8_t raw[3];
  if (this->read_register(ADS1219_CMD_RDATA, raw, sizeof(raw)) != i2c::ERROR_OK) {
    ESP_LOGW(TAG, "'%s': reading conversion result failed", sensor->get_name().c_str());
    this->status_set_warning();
    this->finish_conversion_();
    return;
  }
  uint32_t code = (uint32_t(raw[0]) << 16) | (uint32_t(raw[1]) << 8) | uint32_t(raw[2]);
  if (code & 0x00800000)
    code |= 0xFF000000;
  auto value = static_cast<int32_t>(code);

  float volts = static_cast<float>(value) / 8388608.0f * this->reference_voltage_;  // 2^23 = 8388608
  if (sensor->get_gain() == ADS1219_GAIN_4)
    volts /= 4.0f;

  ESP_LOGD(TAG, "'%s': raw=%" PRId32 ", voltage=%.6f V", sensor->get_name().c_str(), value, volts);
  this->status_clear_warning();
  sensor->publish_state(volts);
  this->finish_conversion_();
}

bool ADS1219Component::start_conversion_(ADS1219Sensor *sensor) {
  // Conversion mode bit [1] stays 0 (single-shot), each START/SYNC runs exactly one conversion
  uint8_t config = (uint8_t(sensor->get_multiplexer()) << 5) | (uint8_t(sensor->get_gain()) << 4) |
                   (uint8_t(sensor->get_data_rate()) << 2) | uint8_t(this->voltage_reference_);
  if (this->write_register(ADS1219_REG_CONFIG_WRITE, &config, 1) != i2c::ERROR_OK) {
    ESP_LOGW(TAG, "'%s': writing config register failed", sensor->get_name().c_str());
    return false;
  }
  if (!this->write_command_(ADS1219_CMD_START_SYNC)) {
    ESP_LOGW(TAG, "'%s': START/SYNC command failed", sensor->get_name().c_str());
    return false;
  }

  // DRDY is not polled before one conversion period has passed, then on every loop() until the timeout
  uint32_t sps = data_rate_to_sps(sensor->get_data_rate());
  uint32_t period_ms = (1000 + sps - 1) / sps;
  this->conversion_start_ = millis();
  this->conversion_wait_ = period_ms;
  this->conversion_timeout_ = 4 * period_ms + ADS1219_TIMEOUT_MARGIN_MS;
  this->converting_ = true;
  return true;
}

void ADS1219Component::finish_conversion_() {
  this->converting_ = false;
  this->queue_.erase(this->queue_.begin());
}

bool ADS1219Component::write_command_(uint8_t command) { return this->write(&command, 1) == i2c::ERROR_OK; }

void ADS1219Sensor::update() { this->parent_->request_measurement(this); }

void ADS1219Sensor::dump_config() {
  LOG_SENSOR("", "ADS1219 Sensor", this);
  ESP_LOGCONFIG(TAG,
                "  Multiplexer: %u\n"
                "  Gain: %u\n"
                "  Data rate: %" PRIu32 " SPS",
                this->multiplexer_, this->gain_ == ADS1219_GAIN_4 ? 4 : 1, data_rate_to_sps(this->data_rate_));
  LOG_UPDATE_INTERVAL(this);
}

}  // namespace ads1219
}  // namespace esphome
