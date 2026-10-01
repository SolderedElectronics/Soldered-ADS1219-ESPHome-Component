import esphome.codegen as cg
from esphome.components import sensor
import esphome.config_validation as cv
from esphome.const import (
    CONF_GAIN,
    CONF_MULTIPLEXER,
    CONF_SAMPLE_RATE,
    DEVICE_CLASS_VOLTAGE,
    STATE_CLASS_MEASUREMENT,
    UNIT_VOLT,
)

from . import CONF_ADS1219_ID, ADS1219Component, ads1219_ns

ADS1219Multiplexer = ads1219_ns.enum("ADS1219Multiplexer")
MUX = {
    "A0_A1": ADS1219Multiplexer.ADS1219_MUX_P0_N1,
    "A2_A3": ADS1219Multiplexer.ADS1219_MUX_P2_N3,
    "A1_A2": ADS1219Multiplexer.ADS1219_MUX_P1_N2,
    "A0_GND": ADS1219Multiplexer.ADS1219_MUX_P0_NG,
    "A1_GND": ADS1219Multiplexer.ADS1219_MUX_P1_NG,
    "A2_GND": ADS1219Multiplexer.ADS1219_MUX_P2_NG,
    "A3_GND": ADS1219Multiplexer.ADS1219_MUX_P3_NG,
    "SHORTED": ADS1219Multiplexer.ADS1219_MUX_SHORTED,
}

ADS1219Gain = ads1219_ns.enum("ADS1219Gain")
GAIN = {
    1: ADS1219Gain.ADS1219_GAIN_1,
    4: ADS1219Gain.ADS1219_GAIN_4,
}

ADS1219DataRate = ads1219_ns.enum("ADS1219DataRate")
DATA_RATE = {
    20: ADS1219DataRate.ADS1219_DATA_RATE_20SPS,
    90: ADS1219DataRate.ADS1219_DATA_RATE_90SPS,
    330: ADS1219DataRate.ADS1219_DATA_RATE_330SPS,
    1000: ADS1219DataRate.ADS1219_DATA_RATE_1000SPS,
}

ADS1219Sensor = ads1219_ns.class_(
    "ADS1219Sensor", sensor.Sensor, cg.PollingComponent, cg.Parented.template(ADS1219Component)
)

CONFIG_SCHEMA = (
    sensor.sensor_schema(
        ADS1219Sensor,
        unit_of_measurement=UNIT_VOLT,
        accuracy_decimals=6,
        device_class=DEVICE_CLASS_VOLTAGE,
        state_class=STATE_CLASS_MEASUREMENT,
    )
    .extend(
        {
            cv.GenerateID(CONF_ADS1219_ID): cv.use_id(ADS1219Component),
            cv.Required(CONF_MULTIPLEXER): cv.enum(MUX, upper=True, space="_"),
            cv.Optional(CONF_GAIN, default=1): cv.enum(GAIN, int=True),
            cv.Optional(CONF_SAMPLE_RATE, default=20): cv.enum(DATA_RATE, int=True),
        }
    )
    .extend(cv.polling_component_schema("60s"))
)


async def to_code(config):
    var = await sensor.new_sensor(config)
    await cg.register_component(var, config)
    await cg.register_parented(var, config[CONF_ADS1219_ID])

    cg.add(var.set_multiplexer(config[CONF_MULTIPLEXER]))
    cg.add(var.set_gain(config[CONF_GAIN]))
    cg.add(var.set_data_rate(config[CONF_SAMPLE_RATE]))
