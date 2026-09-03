"""
Authentication Manager for Somalia Client
Wraps KeyAuthApp API with fallback and user data management.
"""

import os
import sys

try:
    from keyauth import KeyAuthApp
except ImportError:
    from client.keyauth import KeyAuthApp

class KeyAuthManager:
    def __init__(self):
        # Credenciais KeyAuth (Pode ser configurado pelo usuário)
        self.app = KeyAuthApp(
            name="somalia",
            ownerid="5bU1fK1ki3",
            secret="bbcdeb35fe1ba5a8898309632f14da6cbb941af50927c173baa11953f145d07c",
            version="1.0"
        )
        self.app.init()
        self.user_data = self.app.user_data

    def login(self, username, password):
        res = self.app.login(username, password)
        if res.get("success"):
            self.user_data = self.app.user_data
            return True, res.get("message", "Login realizado com sucesso!")
        return False, res.get("message", "Falha na autenticação.")

    def register(self, username, password, key):
        res = self.app.register(username, password, key)
        if res.get("success"):
            self.user_data = self.app.user_data
            return True, res.get("message", "Registro realizado com sucesso!")
        return False, res.get("message", "Falha no registro.")

    def get_user_info(self):
        return self.user_data
