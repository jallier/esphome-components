import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import alarm_control_panel as acp
from esphome.const import CONF_CODE, CONF_ID
from .. import CrowAlarmPanel, CrowAlarmControlPanel, CONF_CROW_ALARM_PANEL_ID

DEPENDENCIES = ["crow_alarm_panel"]

CONF_REQUIRE_CODE_TO_ARM = "requires_code_to_arm"
CONF_REQUIRE_CODE = "requires_code"

CONFIG_SCHEMA = (
    acp.alarm_control_panel_schema(CrowAlarmControlPanel)
    .extend(
        {
            cv.GenerateID(): cv.declare_id(CrowAlarmControlPanel),
            cv.GenerateID(CONF_CROW_ALARM_PANEL_ID): cv.use_id(CrowAlarmPanel),
            cv.Optional(CONF_CODE): cv.string,
            cv.Optional(CONF_REQUIRE_CODE_TO_ARM, default=False): cv.boolean,
            cv.Optional(CONF_REQUIRE_CODE, default=True): cv.boolean,
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    parent = await cg.get_variable(config[CONF_CROW_ALARM_PANEL_ID])
    var = await acp.new_alarm_control_panel(config)

    cg.add(var.set_parent(parent))
    cg.add(parent.register_alarm_control_panel(var))
    cg.add(var.set_requires_code(config[CONF_REQUIRE_CODE]))
    cg.add(var.set_requires_code_to_arm(config[CONF_REQUIRE_CODE_TO_ARM]))

    if CONF_CODE in config:
        cg.add(var.set_code(config[CONF_CODE]))

    await cg.register_component(var, config)
