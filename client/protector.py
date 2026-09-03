"""
Somalia Lua Module Bundler & Protection Pipeline
Bundles all modular Lua files into a single unified payload via package.preload
with embedded asset fallbacks for standalone execution.
"""

import os
import sys
import hashlib
import time
from typing import Tuple

from embedded_assets import EMBEDDED_MODULES

MODULE_ORDER = [
    ("somalia.theme", "somalia/theme.lua"),
    ("somalia.notifications", "somalia/notifications.lua"),
    ("somalia.keybinds", "somalia/keybinds.lua"),
    ("somalia.config", "somalia/config.lua"),
    ("somalia.hud", "somalia/hud.lua"),
    ("somalia.aim", "somalia/aim.lua"),
    ("somalia.visuals", "somalia/visuals.lua"),
    ("somalia.player", "somalia/player.lua"),
    ("somalia.vehicles", "somalia/vehicles.lua"),
    ("somalia.world", "somalia/world.lua"),
    ("somalia.misc", "somalia/misc.lua")
]

class ModuleProtector:
    def __init__(self, root_dir: str):
        self.root_dir = root_dir

    def get_module_content(self, rel_path: str) -> str:
        """Lê o arquivo do disco ou utiliza a versão embutida infalível"""
        paths_to_check = [
            os.path.join(self.root_dir, rel_path),
            os.path.abspath(os.path.join(os.path.dirname(__file__), "..", rel_path)),
            os.path.abspath(os.path.join(os.path.dirname(sys.executable), "..", "..", rel_path)),
            os.path.join(os.getcwd(), rel_path)
        ]
        for p in paths_to_check:
            if os.path.isfile(p):
                try:
                    with open(p, "r", encoding="utf-8") as f:
                        return f.read()
                except Exception:
                    pass
        return EMBEDDED_MODULES.get(rel_path, "")

    def build_bundled_payload(self, session_user: str = "User") -> str:
        """Empacota todos os módulos em um único script protegido com package.preload"""
        session_token = hashlib.sha256(f"{session_user}-{time.time()}".encode()).hexdigest()[:16]

        bundled_code = []
        bundled_code.append("--[[\n    SOMALIA PROTECTED PAYLOAD\n    Authorized Session: " + session_token + "\n--]]\n\n")
        bundled_code.append("local _SOMALIA_SESSION_AUTH = '" + session_token + "'\n\n")

        # 1. Registra cada módulo no package.preload do Lua
        for mod_name, rel_path in MODULE_ORDER:
            mod_content = self.get_module_content(rel_path)
            if mod_content:
                bundled_code.append(f"package.preload['{mod_name}'] = function()\n")
                bundled_code.append(mod_content)
                bundled_code.append("\nend\n\n")
                
                # Registra também alias simples (ex: 'theme' -> 'somalia.theme')
                short_name = mod_name.replace("somalia.", "")
                bundled_code.append(f"package.preload['{short_name}'] = package.preload['{mod_name}']\n\n")

        # 2. Anexa o entrypoint principal (main.lua)
        main_content = self.get_module_content("main.lua")
        if main_content:
            bundled_code.append(main_content)

        return "".join(bundled_code)

    def export_protected_package(self, output_path: str, session_user: str = "User") -> Tuple[bool, str]:
        """Exporta o pacote protegido diretamente para a pasta moonloader do GTA"""
        try:
            payload = self.build_bundled_payload(session_user)
            with open(output_path, "w", encoding="utf-8") as f:
                f.write(payload)
            return True, f"Pacote Somalia protegido gerado em: {output_path}"
        except Exception as e:
            return False, f"Falha ao gerar pacote protegido: {str(e)}"
