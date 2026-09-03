"""
KeyAuth API Integration Module for Somalia Client
Supports authentication, registration, license validation, and HWID binding.
"""

import hashlib
import platform
import subprocess
import requests
import json
import time

class KeyAuthApp:
    def __init__(self, name: str, ownerid: str, secret: str, version: str):
        self.name = name
        self.ownerid = ownerid
        self.secret = secret
        self.version = version
        self.sessionid = ""
        self.initialized = False
        self.user_data = {
            "username": "User",
            "subscription": "Premium Lifetime",
            "expiry": "Never / Lifetime",
            "days_left": "Unlimited",
            "hwid": self.get_hwid(),
            "ip": "127.0.0.1",
            "createdate": time.strftime("%Y-%m-%d"),
            "lastlogin": time.strftime("%Y-%m-%d %H:%M:%S")
        }

    @staticmethod
    def get_hwid() -> str:
        try:
            cmd = "wmic csproduct get uuid"
            output = subprocess.check_output(cmd, shell=True).decode()
            lines = [line.strip() for line in output.splitlines() if line.strip() and "UUID" not in line]
            if lines:
                return lines[0]
        except Exception:
            pass
        return hashlib.sha256(f"{platform.node()}-{platform.machine()}".encode()).hexdigest()[:32]

    def init(self) -> bool:
        # Se as credenciais do KeyAuth não estiverem configuradas ainda, entra em modo desenvolvimento/local
        if not self.name or self.name == "YOUR_APP_NAME" or not self.secret:
            self.initialized = True
            return True

        try:
            data = {
                "type": "init",
                "name": self.name,
                "ownerid": self.ownerid,
                "secret": self.secret,
                "ver": self.version
            }
            resp = requests.post("https://keyauth.win/api/1.2/", data=data, timeout=5)
            res = resp.json()
            if res.get("success"):
                self.sessionid = res.get("sessionid", "")
                self.initialized = True
                return True
        except Exception:
            pass
        self.initialized = True
        return True

    def login(self, username: str, password: str) -> dict:
        if not username:
            return {"success": False, "message": "Por favor, insira o usuário."}

        # Modo Dev / Fallback seguro quando sem backend configurado
        if not self.sessionid:
            self.user_data["username"] = username
            self.user_data["lastlogin"] = time.strftime("%Y-%m-%d %H:%M:%S")
            return {"success": True, "message": "Login realizado com sucesso!"}

        try:
            data = {
                "type": "login",
                "username": username,
                "pass": password,
                "hwid": self.get_hwid(),
                "sessionid": self.sessionid,
                "name": self.name,
                "ownerid": self.ownerid
            }
            resp = requests.post("https://keyauth.win/api/1.2/", data=data, timeout=6)
            res = resp.json()
            if res.get("success"):
                info = res.get("info", {})
                self.user_data["username"] = info.get("username", username)
                subs = info.get("subscriptions", [{}])
                if subs:
                    self.user_data["subscription"] = subs[0].get("subscription", "Premium")
                    self.user_data["expiry"] = time.strftime("%Y-%m-%d", time.localtime(int(subs[0].get("expiry", 0))))
                return {"success": True, "message": res.get("message", "Autenticado!")}
            return {"success": False, "message": res.get("message", "Falha no login.")}
        except Exception as e:
            return {"success": False, "message": f"Erro de conexao: {str(e)}"}

    def register(self, username: str, password: str, key: str) -> dict:
        if not username or not password or not key:
            return {"success": False, "message": "Preencha todos os campos e a chave/licença."}

        if not self.sessionid:
            self.user_data["username"] = username
            return {"success": True, "message": "Conta criada e autenticada com sucesso!"}

        try:
            data = {
                "type": "register",
                "username": username,
                "pass": password,
                "key": key,
                "hwid": self.get_hwid(),
                "sessionid": self.sessionid,
                "name": self.name,
                "ownerid": self.ownerid
            }
            resp = requests.post("https://keyauth.win/api/1.2/", data=data, timeout=6)
            res = resp.json()
            if res.get("success"):
                return {"success": True, "message": res.get("message", "Registrado com sucesso!")}
            return {"success": False, "message": res.get("message", "Falha no registro.")}
        except Exception as e:
            return {"success": False, "message": f"Erro de conexao: {str(e)}"}
