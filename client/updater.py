"""
Updater Module for Somalia Client
Handles version checks and automated package synchronization.
"""

import json
import requests
from typing import Tuple, Dict, Any

CURRENT_VERSION = "1.3.0"

class ClientUpdater:
    def __init__(self, api_endpoint: str = ""):
        self.api_endpoint = api_endpoint
        self.current_version = CURRENT_VERSION

    def check_for_updates(self) -> Tuple[bool, Dict[str, Any]]:
        """Verifica se existe uma versão mais recente disponível"""
        if not self.api_endpoint:
            return False, {
                "current": self.current_version,
                "latest": self.current_version,
                "has_update": False,
                "changelog": "Versao mais recente instalada e operacional."
            }

        try:
            resp = requests.get(f"{self.api_endpoint}/version", timeout=4)
            if resp.status_code == 200:
                data = resp.json()
                latest = data.get("version", self.current_version)
                has_update = (latest != self.current_version)
                return has_update, {
                    "current": self.current_version,
                    "latest": latest,
                    "has_update": has_update,
                    "changelog": data.get("changelog", "Melhorias de desempenho e estabilidade.")
                }
        except Exception:
            pass

        return False, {
            "current": self.current_version,
            "latest": self.current_version,
            "has_update": False,
            "changelog": "Sistema sincronizado."
        }
