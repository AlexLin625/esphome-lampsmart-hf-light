import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import esp32, light
from esphome.const import (
    CONF_CONSTANT_BRIGHTNESS,
    CONF_COLD_WHITE_COLOR_TEMPERATURE,
    CONF_DURATION,
    CONF_MIN_BRIGHTNESS,
    CONF_OUTPUT_ID,
    CONF_REVERSED,
    CONF_WARM_WHITE_COLOR_TEMPERATURE,
)

AUTO_LOAD = []
DEPENDENCIES = ["esp32", "api"]

CONF_ADDRESS = "address"
CONF_GROUP = "group"

lampsmart_hf_ns = cg.esphome_ns.namespace("lampsmarthf")
LampSmartHFLight = lampsmart_hf_ns.class_(
    "LampSmartHFLight", cg.Component, light.LightOutput
)

NIMBLE_INCLUDE_FLAGS = [
    "-I${platformio.packages_dir}/framework-espidf/components/bt/host/nimble/nimble/nimble/host/include",
    "-I${platformio.packages_dir}/framework-espidf/components/bt/host/nimble/nimble/nimble/host/util/include",
    "-I${platformio.packages_dir}/framework-espidf/components/bt/host/nimble/nimble/nimble/host/services/gap/include",
    "-I${platformio.packages_dir}/framework-espidf/components/bt/host/nimble/nimble/porting/nimble/include",
]


def _validate_address(value):
    value = cv.All([cv.hex_uint8_t], cv.Length(min=4, max=4))(value)
    return value


CONFIG_SCHEMA = cv.All(
    light.RGB_LIGHT_SCHEMA.extend(
        {
            cv.GenerateID(CONF_OUTPUT_ID): cv.declare_id(LampSmartHFLight),
            cv.Required(CONF_ADDRESS): _validate_address,
            cv.Optional(CONF_GROUP, default=0x10): cv.hex_uint8_t,
            cv.Optional(CONF_DURATION, default=2000): cv.positive_int,
            cv.Optional(CONF_COLD_WHITE_COLOR_TEMPERATURE): cv.color_temperature,
            cv.Optional(CONF_WARM_WHITE_COLOR_TEMPERATURE): cv.color_temperature,
            cv.Optional(CONF_CONSTANT_BRIGHTNESS, default=False): cv.boolean,
            cv.Optional(CONF_REVERSED, default=False): cv.boolean,
            cv.Optional(CONF_MIN_BRIGHTNESS, default=40): cv.int_range(min=1, max=1000),
        }
    ),
    cv.has_none_or_all_keys(
        [CONF_COLD_WHITE_COLOR_TEMPERATURE, CONF_WARM_WHITE_COLOR_TEMPERATURE]
    ),
    light.validate_color_temperature_channels,
)


async def to_code(config):
    esp32.include_builtin_idf_component("bt")
    esp32.add_idf_sdkconfig_option("CONFIG_BT_ENABLED", True)
    esp32.add_idf_sdkconfig_option("CONFIG_BT_BLE_ENABLED", True)
    esp32.add_idf_sdkconfig_option("CONFIG_BT_NIMBLE_ENABLED", True)
    esp32.add_idf_sdkconfig_option("CONFIG_BT_NIMBLE_ROLE_BROADCASTER", True)
    for include_flag in NIMBLE_INCLUDE_FLAGS:
        cg.add_build_flag(include_flag)

    var = cg.new_Pvariable(config[CONF_OUTPUT_ID])
    await cg.register_component(var, config)
    await light.register_light(var, config)

    address = config[CONF_ADDRESS]
    cg.add(var.set_address(address[0], address[1], address[2], address[3]))
    cg.add(var.set_group_id(config[CONF_GROUP]))
    cg.add(var.set_tx_duration(config[CONF_DURATION]))
    cg.add(var.set_constant_brightness(config[CONF_CONSTANT_BRIGHTNESS]))
    cg.add(var.set_reversed(config[CONF_REVERSED]))
    cg.add(var.set_min_brightness(config[CONF_MIN_BRIGHTNESS]))

    if CONF_COLD_WHITE_COLOR_TEMPERATURE in config:
        cg.add(
            var.set_cold_white_temperature(config[CONF_COLD_WHITE_COLOR_TEMPERATURE])
        )

    if CONF_WARM_WHITE_COLOR_TEMPERATURE in config:
        cg.add(
            var.set_warm_white_temperature(config[CONF_WARM_WHITE_COLOR_TEMPERATURE])
        )
