import platform
from UM.i18n import i18nCatalog


catalog = i18nCatalog("curaengine_onlyfans")

from . import constants

if platform.machine() in ["AMD64", "x86_64"]:
    from . import OnlyFans


    def getMetaData():
        return {}

    def register(app):
        return { "backend_plugin":  OnlyFans.OnlyFans() }
else:
    from UM.Logger import Logger

    Logger.error(f"{constants.cura_plugin_name} plugin is only supported on x86_64 systems")

    def getMetaData():
        return {}

    def register(app):
        return {}
