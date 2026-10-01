import esphome.codegen as cg
from esphome.components import i2c
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_REFERENCE_VOLTAGE

CODEOWNERS = ["@SolderedElectronics"]
DEPENDENCIES = ["i2c"]
AUTO_LOAD = ["sensor"]
MULTI_CONF = True

CONF_ADS1219_ID = "ads1219_id"
CONF_VOLTAGE_REFERENCE = "voltage_reference"

ads1219_ns = cg.esphome_ns.namespace("ads1219")
ADS1219Component = ads1219_ns.class_("ADS1219Component", cg.Component, i2c.I2CDevice)

ADS1219VoltageReference = ads1219_ns.enum("ADS1219VoltageReference")
VOLTAGE_REFERENCES = {
    "INTERNAL": ADS1219VoltageReference.ADS1219_VREF_INTERNAL,
    "EXTERNAL": ADS1219VoltageReference.ADS1219_VREF_EXTERNAL,
}

INTERNAL_REFERENCE_VOLTAGE = 2.048


def _validate_reference(config):
    if config[CONF_VOLTAGE_REFERENCE] == "EXTERNAL":
        if CONF_REFERENCE_VOLTAGE not in config:
            raise cv.Invalid(
                f"'{CONF_REFERENCE_VOLTAGE}' is required when '{CONF_VOLTAGE_REFERENCE}' is external "
                "(voltage between the REFP and REFN pins)"
            )
    elif CONF_REFERENCE_VOLTAGE in config:
        raise cv.Invalid(
            f"'{CONF_REFERENCE_VOLTAGE}' can only be set when '{CONF_VOLTAGE_REFERENCE}' is external, "
            "the internal reference is fixed at 2.048V"
        )
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(ADS1219Component),
            cv.Optional(CONF_VOLTAGE_REFERENCE, default="INTERNAL"): cv.enum(
                VOLTAGE_REFERENCES, upper=True
            ),
            cv.Optional(CONF_REFERENCE_VOLTAGE): cv.All(
                cv.voltage, cv.Range(min=0.5, max=5.5)
            ),
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
    .extend(i2c.i2c_device_schema(0x40)),
    _validate_reference,
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await i2c.register_i2c_device(var, config)

    cg.add(var.set_voltage_reference(config[CONF_VOLTAGE_REFERENCE]))
    cg.add(
        var.set_reference_voltage(
            config.get(CONF_REFERENCE_VOLTAGE, INTERNAL_REFERENCE_VOLTAGE)
        )
    )
