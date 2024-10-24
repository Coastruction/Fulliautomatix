import os
import platform
import stat
import sys
from pathlib import Path

from UM.Logger import Logger
from UM.Settings.ContainerRegistry import ContainerRegistry
from UM.i18n import i18nCatalog
from cura.BackendPlugin import BackendPlugin
from cura.CuraApplication import CuraApplication

from . import constants


catalog = i18nCatalog("curaengine_onlyfans")


class OnlyFans(BackendPlugin):
    def __init__(self):
        super().__init__()
        self.definition_file_paths = [Path(__file__).parent.joinpath("onlyfans_settings.def.json").as_posix()]
        if not self.isDebug():
            if not self.binaryPath().exists():
                Logger.error(f"Could not find {constants.curaengine_plugin_name} binary at {self.binaryPath().as_posix()}")
            if platform.system() != "Windows" and self.binaryPath().exists():
                st = os.stat(self.binaryPath())
                if (st.st_mode & stat.S_IEXEC) == 0:
                    os.chmod(self.binaryPath(), st.st_mode | stat.S_IEXEC)

            self._plugin_command = [self.binaryPath().as_posix()]

        self._supported_slots = [101]  # POSTPROCESS_MODIFY SlotID
        ContainerRegistry.getInstance().containerLoadComplete.connect(self._on_container_load_complete)

    def _on_container_load_complete(self, container_id) -> None:
        pass

    def usePlugin(self):
        machine_manager = CuraApplication.getInstance().getMachineManager()
        return any([extr.getProperty(f"{constants.settings_prefix}_onlyfans_enabled", "value") for extr in machine_manager.activeMachine.extruderList if extr.hasProperty(f"{constants.settings_prefix}_onlyfans_enabled", "value")])

    def getPort(self):
        #return 7856  # enable this line instead of the next to use it with a fixed port (for debugging)
        return super().getPort() if not self.isDebug() else int(os.environ["CURAENGINE_ONLYFANS_PORT"])  #use this for release

    def isDebug(self):
        return not hasattr(sys, "frozen") and os.environ.get("CURAENGINE_ONLYFANS_PORT", None) is not None   # use this for release
        # return os.environ.get("CURAENGINE_ONLYFANS_PORT", None) is not None  # use this line for debugging

    def start(self):
        if not self.isDebug():
            super().start()

    def binaryPath(self) -> Path:
        ext = ".exe" if platform.system() == "Windows" else ""
        service_path = Path(CuraApplication.getInstance().getAppFolderPrefix()).joinpath(f"{constants.curaengine_plugin_name}{ext}").resolve()
        if service_path.exists():
            return service_path

        machine = platform.machine()
        if machine == "AMD64":
            machine = "x86_64"
        return Path(__file__).parent.joinpath(machine, platform.system(), f"{constants.curaengine_plugin_name}{ext}").resolve()
