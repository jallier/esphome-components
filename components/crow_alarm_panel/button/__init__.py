import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button
from esphome.const import CONF_ID, CONF_TYPE
from .. import crow_alarm_panel_ns, CrowAlarmPanel, CONF_CROW_ALARM_PANEL_ID

DEPENDENCIES = ["crow_alarm_panel"]

CrowAlarmPanelButton = crow_alarm_panel_ns.class_(
    "CrowAlarmPanelButton", button.Button, cg.Component
)

TYPES = ["arm_away", "arm_stay", "disarm"]

CONF_CODE = "code"

CONFIG_SCHEMA = button.button_schema(CrowAlarmPanelButton).extend(
    {
        cv.GenerateID(CONF_CROW_ALARM_PANEL_ID): cv.use_id(CrowAlarmPanel),
        cv.Required(CONF_TYPE): cv.one_of(*TYPES, lower=True),
        cv.Optional(CONF_CODE): cv.string,  # Only needed for disarm
    }
).extend(cv.COMPONENT_SCHEMA)


async def to_code(config):
    paren = await cg.get_variable(config[CONF_CROW_ALARM_PANEL_ID])
    var = cg.new_Pvariable(config[CONF_ID])
    
    await button.register_button(var, config)
    await cg.register_component(var, config)
    
    cg.add(var.set_parent(paren))
    cg.add(var.set_button_type(config[CONF_TYPE]))
    
    if CONF_CODE in config:
        cg.add(var.set_code(config[CONF_CODE]))
