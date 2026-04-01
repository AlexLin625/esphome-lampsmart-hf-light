import esphome.codegen as cg
from esphome.components import button
import esphome.config_validation as cv

from .light import LampSmartHFLight, lampsmart_hf_ns

CONF_LAMPSMART_HF_LIGHT_ID = "lampsmart_hf_light_id"

ICON_LIGHTBULB_AUTO = "mdi:lightbulb-auto"

LampSmartHFPairButton = lampsmart_hf_ns.class_("LampSmartHFPairButton", button.Button)

CONFIG_SCHEMA = button.button_schema(
    LampSmartHFPairButton,
    icon=ICON_LIGHTBULB_AUTO,
    entity_category="config",
).extend(
    {
        cv.GenerateID(CONF_LAMPSMART_HF_LIGHT_ID): cv.use_id(LampSmartHFLight),
    }
)


async def to_code(config):
    var = await button.new_button(config)
    parent = await cg.get_variable(config[CONF_LAMPSMART_HF_LIGHT_ID])
    await cg.register_parented(var, parent)
