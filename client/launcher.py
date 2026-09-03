"""
Launcher & Game Session Lifecycle Manager for Somalia Client
Handles GTA directory persistence, validation, and safe session deployment.
"""

import os
import sys
import shutil
import json
import subprocess
import time
from typing import Tuple, Dict, Any

from protector import ModuleProtector
from embedded_assets import SOMALIA_TEST_LUA, SOMALIA_IMGUI_TEST_LUA

class GameLauncher:
    def __init__(self, root_dir: str):
        self.root_dir = self.resolve_root_dir(root_dir)
        self.config_path = self.get_config_path()
        self.config = self.load_config()
        self.running_process = None
        self.temp_files_created = []
        self.protector = ModuleProtector(self.root_dir)

    def resolve_root_dir(self, candidate: str) -> str:
        paths_to_check = [
            candidate,
            os.path.abspath(os.path.join(os.path.dirname(__file__), "..")),
            os.path.abspath(os.path.join(os.path.dirname(sys.executable), "..", "..")),
            os.path.dirname(sys.executable),
            os.getcwd(),
            getattr(sys, '_MEIPASS', '')
        ]
        for p in paths_to_check:
            if p and os.path.exists(os.path.join(p, "main.lua")):
                return p
        return candidate

    def get_config_path(self) -> str:
        """Localiza o arquivo somalia_client.json no diretório do executável ou projeto"""
        if getattr(sys, 'frozen', False):
            base_dir = os.path.dirname(sys.executable)
        else:
            base_dir = self.root_dir
        return os.path.join(base_dir, "somalia_client.json")

    def load_config(self) -> Dict[str, Any]:
        cfg_file = self.get_config_path()
        if os.path.exists(cfg_file):
            try:
                with open(cfg_file, "r", encoding="utf-8") as f:
                    return json.load(f)
            except Exception:
                pass
        return {
            "gta_path": "",
            "auto_start_samp": True,
            "remember_user": True,
            "last_username": ""
        }

    def save_config(self):
        cfg_file = self.get_config_path()
        try:
            with open(cfg_file, "w", encoding="utf-8") as f:
                json.dump(self.config, f, indent=4)
        except Exception:
            pass

    def validate_gta_directory(self, path: str) -> Tuple[bool, Dict[str, Any]]:
        if not path or not os.path.isdir(path):
            return False, {
                "gta_sa": False,
                "samp": False,
                "moonloader_asi": False,
                "moonloader_folder": False,
                "moonloader_lib": False,
                "moonloader_log": False,
                "has_imgui": False,
                "log_time": "N/A",
                "valid": False,
                "details": "Diretório não encontrado."
            }

        has_gta = os.path.isfile(os.path.join(path, "gta_sa.exe"))
        has_samp = os.path.isfile(os.path.join(path, "samp.exe"))
        has_moon_asi = os.path.isfile(os.path.join(path, "MoonLoader.asi")) or os.path.isfile(os.path.join(path, "moonloader.asi"))
        has_moon_dir = os.path.isdir(os.path.join(path, "moonloader"))
        has_moon_lib = os.path.isdir(os.path.join(path, "moonloader", "lib"))

        log_path = os.path.join(path, "moonloader", "moonloader.log")
        has_log = os.path.isfile(log_path)
        log_time = "N/A"
        if has_log:
            try:
                mtime = os.path.getmtime(log_path)
                log_time = time.strftime("%H:%M:%S (%d/%m)", time.localtime(mtime))
            except Exception:
                pass

        lib_dir = os.path.join(path, "moonloader", "lib")
        has_imgui = False
        if os.path.isdir(lib_dir):
            has_imgui = (
                os.path.isfile(os.path.join(lib_dir, "imgui.lua")) or 
                os.path.isdir(os.path.join(lib_dir, "imgui")) or 
                os.path.isfile(os.path.join(lib_dir, "mimgui.lua")) or
                os.path.isdir(os.path.join(lib_dir, "mimgui"))
            )

        is_valid = has_gta and has_samp

        return is_valid, {
            "gta_sa": has_gta,
            "samp": has_samp,
            "moonloader_asi": has_moon_asi,
            "moonloader_folder": has_moon_dir,
            "moonloader_lib": has_moon_lib,
            "moonloader_log": has_log,
            "has_imgui": has_imgui,
            "log_time": log_time,
            "valid": is_valid,
            "details": "Diretório do GTA validado." if is_valid else "Arquivos principais ausentes."
        }

    def clean_all_somalia_scripts(self, moon_dir: str):
        scripts_to_remove = [
            "somalia_test.lua",
            "somalia_imgui_test.lua",
            "somalia_main.lua",
            "somalia_showcase.lua"
        ]
        for s in scripts_to_remove:
            p = os.path.join(moon_dir, s)
            if os.path.exists(p):
                try:
                    os.remove(p)
                except Exception:
                    pass

        # Remove arquivos INI antigos para evitar conflitos de versão
        try:
            for item in os.listdir(moon_dir):
                if item.startswith("somalia_") and item.endswith(".ini"):
                    try:
                        os.remove(os.path.join(moon_dir, item))
                    except Exception:
                        pass
        except Exception:
            pass

    def deploy_test_script(self, gta_path: str) -> Tuple[bool, str]:
        is_valid, status = self.validate_gta_directory(gta_path)
        if not is_valid:
            return False, "Caminho do GTA inválido ou não encontrado. Selecione novamente."

        moon_dir = os.path.join(gta_path, "moonloader")
        os.makedirs(moon_dir, exist_ok=True)
        self.clean_all_somalia_scripts(moon_dir)

        dst_test = os.path.join(moon_dir, "somalia_test.lua")
        try:
            src_test = os.path.join(self.root_dir, "somalia_test.lua")
            if os.path.exists(src_test):
                shutil.copy2(src_test, dst_test)
            else:
                with open(dst_test, "w", encoding="utf-8") as f:
                    f.write(SOMALIA_TEST_LUA)

            self.temp_files_created.append(dst_test)
            return True, f"[ETAPA 1] Script de teste preparado em: {dst_test}"
        except Exception as e:
            return False, f"Erro ao copiar script de teste: {str(e)}"

    def deploy_imgui_test_script(self, gta_path: str) -> Tuple[bool, str]:
        is_valid, status = self.validate_gta_directory(gta_path)
        if not is_valid:
            return False, "Caminho do GTA inválido ou não encontrado. Selecione novamente."

        moon_dir = os.path.join(gta_path, "moonloader")
        os.makedirs(moon_dir, exist_ok=True)
        self.clean_all_somalia_scripts(moon_dir)

        dst_test = os.path.join(moon_dir, "somalia_imgui_test.lua")
        try:
            src_test = os.path.join(self.root_dir, "somalia_imgui_test.lua")
            if os.path.exists(src_test):
                shutil.copy2(src_test, dst_test)
            else:
                with open(dst_test, "w", encoding="utf-8") as f:
                    f.write(SOMALIA_IMGUI_TEST_LUA)

            self.temp_files_created.append(dst_test)
            return True, f"[ETAPA 5] Teste ImGui preparado em: {dst_test}"
        except Exception as e:
            return False, f"Erro ao copiar teste ImGui: {str(e)}"

    def deploy_direct_unprotected(self, gta_path: str) -> Tuple[bool, str]:
        is_valid, status = self.validate_gta_directory(gta_path)
        if not is_valid:
            return False, "Caminho do GTA inválido ou não encontrado. Selecione novamente."

        moon_dir = os.path.join(gta_path, "moonloader")
        os.makedirs(moon_dir, exist_ok=True)
        self.clean_all_somalia_scripts(moon_dir)

        try:
            src_main = os.path.join(self.root_dir, "main.lua")
            dst_main = os.path.join(moon_dir, "somalia_main.lua")
            if os.path.exists(src_main):
                shutil.copy2(src_main, dst_main)
            else:
                payload = self.protector.build_bundled_payload("DirectTest")
                with open(dst_main, "w", encoding="utf-8") as f:
                    f.write(payload)
            self.temp_files_created.append(dst_main)

            src_somalia = os.path.join(self.root_dir, "somalia")
            dst_somalia = os.path.join(moon_dir, "somalia")
            if os.path.exists(src_somalia):
                os.makedirs(dst_somalia, exist_ok=True)
                for item in os.listdir(src_somalia):
                    s = os.path.join(src_somalia, item)
                    d = os.path.join(dst_somalia, item)
                    if os.path.isfile(s):
                        shutil.copy2(s, d)
                        self.temp_files_created.append(d)

            return True, "[ETAPA 3] Arquivos diretos instalados com sucesso!"
        except Exception as e:
            return False, f"Erro na cópia direta: {str(e)}"

    def deploy_protected_package(self, gta_path: str, username: str = "User") -> Tuple[bool, str]:
        is_valid, status = self.validate_gta_directory(gta_path)
        if not is_valid:
            return False, "Caminho do GTA inválido ou não encontrado. Selecione novamente."

        moon_dir = os.path.join(gta_path, "moonloader")
        os.makedirs(moon_dir, exist_ok=True)
        self.clean_all_somalia_scripts(moon_dir)

        try:
            dst_pkg = os.path.join(moon_dir, "somalia_showcase.lua")
            ok, msg = self.protector.export_protected_package(dst_pkg, username)
            if ok:
                self.temp_files_created.append(dst_pkg)
                return True, "[ETAPA 4] Pacote protegido preparado com sucesso!"
            return False, msg
        except Exception as e:
            return False, f"Erro no pacote protegido: {str(e)}"

    def launch_game(self, gta_path: str) -> Tuple[bool, str]:
        samp_exe = os.path.join(gta_path, "samp.exe")
        gta_exe = os.path.join(gta_path, "gta_sa.exe")
        target_exe = samp_exe if os.path.exists(samp_exe) else gta_exe

        try:
            self.running_process = subprocess.Popen(
                [target_exe],
                cwd=gta_path,
                creationflags=subprocess.CREATE_NEW_PROCESS_GROUP if os.name == 'nt' else 0
            )
            return True, f"Jogo iniciado com sucesso! (PID: {self.running_process.pid})"
        except Exception as e:
            return False, f"Falha ao executar o jogo: {str(e)}"

    def is_game_running(self) -> bool:
        if self.running_process:
            return self.running_process.poll() is None
        return False

    def read_moonloader_log(self, gta_path: str) -> str:
        log_file = os.path.join(gta_path, "moonloader", "moonloader.log")
        if os.path.exists(log_file):
            try:
                with open(log_file, "r", encoding="utf-8", errors="ignore") as f:
                    lines = f.readlines()
                    if lines:
                        return "".join(lines[-40:])
                    return "O arquivo moonloader.log está vazio."
            except Exception as e:
                return f"Erro ao ler moonloader.log: {str(e)}"
        return "Arquivo moonloader.log NÃO encontrado na pasta moonloader/.\nIsso indica que o GTA ainda não executou o MoonLoader.asi."

    def cleanup_session(self):
        for f in self.temp_files_created:
            if os.path.exists(f):
                try:
                    os.remove(f)
                except Exception:
                    pass
        self.temp_files_created = []
        self.running_process = None
