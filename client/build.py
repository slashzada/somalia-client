"""
Build Script for compiling Somalia Client into a standalone Somalia.exe binary
"""

import os
import subprocess
import sys

def build():
    client_dir = os.path.dirname(os.path.abspath(__file__))
    app_path = os.path.join(client_dir, "app.py")
    dist_path = os.path.abspath(os.path.join(client_dir, "..", "dist"))

    cmd = [
        sys.executable, "-m", "PyInstaller",
        "--noconfirm",
        "--onedir",
        "--windowed",
        "--name", "Somalia",
        "--paths", client_dir,
        "--hidden-import", "auth",
        "--hidden-import", "keyauth",
        "--hidden-import", "launcher",
        "--hidden-import", "protector",
        "--hidden-import", "embedded_assets",
        "--hidden-import", "updater",
        "--hidden-import", "customtkinter",
        "--distpath", dist_path,
        "--workpath", os.path.join(dist_path, "build"),
        "--specpath", os.path.join(dist_path, "spec"),
        app_path
    ]

    print("Iniciando compilacao do Somalia.exe...")
    res = subprocess.run(cmd)
    if res.returncode == 0:
        print(f"\n[OK] Somalia.exe compilado com sucesso em: {os.path.join(dist_path, 'Somalia', 'Somalia.exe')}")
    else:
        print(f"\n[ERRO] Falha ao compilar Somalia.exe (Codigo {res.returncode})")

if __name__ == "__main__":
    build()
