"""
Somalia Client Application - GUI Frontend
CustomTkinter interface matching Somalia SML Dark Neon theme
"""

import os
import sys
import threading
import time
import customtkinter as ctk
from tkinter import filedialog, messagebox

# Configuração de caminhos locais
app_dir = os.path.dirname(os.path.abspath(__file__))
if app_dir not in sys.path:
    sys.path.insert(0, app_dir)

try:
    from auth import KeyAuthManager
    from launcher import GameLauncher
except ImportError:
    from client.auth import KeyAuthManager
    from client.launcher import GameLauncher

# Paleta SML Original 1:1
THEME = {
    "bg": "#0A0B10",
    "card": "#0F1117",
    "card_border": "#1E232E",
    "accent": "#0080FF",
    "accent_hover": "#1E96FF",
    "accent_dark": "#182640",
    "text_primary": "#F5F7FF",
    "text_secondary": "#808A9E",
    "text_muted": "#505A6E",
    "success": "#1ED760",
    "danger": "#FF3C3C",
    "warning": "#FFAE19"
}

class SomaliaClientApp(ctk.CTk):
    def __init__(self):
        super().__init__()
        ctk.set_appearance_mode("dark")
        ctk.set_default_color_theme("blue")

        self.title("Somalia Showcase Edition - Client")
        self.geometry("820x540")
        self.resizable(False, False)
        self.configure(fg_color=THEME["bg"])

        # Inicializa gerenciadores
        app_dir = os.path.dirname(os.path.abspath(__file__))
        self.auth = KeyAuthManager()
        self.launcher = GameLauncher(app_dir)

        self.current_frame = None
        self.show_login_screen()

    def clear_screen(self):
        if self.current_frame:
            self.current_frame.destroy()
            self.current_frame = None

    # =========================================================================
    # TELA DE AUTENTICAÇÃO
    # =========================================================================
    def show_login_screen(self):
        self.clear_screen()
        self.current_frame = ctk.CTkFrame(self, fg_color=THEME["bg"])
        self.current_frame.pack(fill="both", expand=True)

        card = ctk.CTkFrame(
            self.current_frame,
            fg_color=THEME["card"],
            border_color=THEME["card_border"],
            border_width=1,
            corner_radius=12,
            width=380,
            height=460
        )
        card.place(relx=0.5, rely=0.5, anchor="center")
        card.pack_propagate(False)

        logo_frame = ctk.CTkFrame(card, fg_color="transparent")
        logo_frame.pack(pady=(25, 10))

        lbl_sm = ctk.CTkLabel(logo_frame, text="SM", font=ctk.CTkFont(family="Segoe UI", size=28, weight="bold"), text_color=THEME["text_primary"])
        lbl_sm.pack(side="left")
        lbl_l = ctk.CTkLabel(logo_frame, text="L", font=ctk.CTkFont(family="Segoe UI", size=28, weight="bold"), text_color=THEME["accent"])
        lbl_l.pack(side="left")

        lbl_sub = ctk.CTkLabel(card, text="SOMALIA CLIENT SHOWCASE", font=ctk.CTkFont(family="Segoe UI", size=11, weight="bold"), text_color=THEME["text_muted"])
        lbl_sub.pack(pady=(0, 20))

        last_user = self.launcher.config.get("last_username", "")
        self.entry_user = ctk.CTkEntry(
            card,
            placeholder_text="Usuário",
            fg_color="#141720",
            border_color="#262D3D",
            text_color=THEME["text_primary"],
            width=300,
            height=38,
            corner_radius=8
        )
        self.entry_user.pack(pady=6)
        if last_user:
            self.entry_user.insert(0, last_user)

        self.entry_pass = ctk.CTkEntry(
            card,
            placeholder_text="Senha",
            show="•",
            fg_color="#141720",
            border_color="#262D3D",
            text_color=THEME["text_primary"],
            width=300,
            height=38,
            corner_radius=8
        )
        self.entry_pass.pack(pady=6)

        self.entry_key = ctk.CTkEntry(
            card,
            placeholder_text="Chave / Licença (Para Registro)",
            fg_color="#141720",
            border_color="#262D3D",
            text_color=THEME["text_primary"],
            width=300,
            height=38,
            corner_radius=8
        )
        self.entry_key.pack(pady=6)

        self.lbl_status = ctk.CTkLabel(card, text="", font=ctk.CTkFont(family="Segoe UI", size=12), text_color=THEME["warning"])
        self.lbl_status.pack(pady=4)

        btn_login = ctk.CTkButton(
            card,
            text="ENTRAR",
            font=ctk.CTkFont(family="Segoe UI", size=13, weight="bold"),
            fg_color=THEME["accent"],
            hover_color=THEME["accent_hover"],
            text_color="#FFFFFF",
            width=300,
            height=40,
            corner_radius=8,
            command=self.handle_login
        )
        btn_login.pack(pady=(8, 4))

        btn_reg = ctk.CTkButton(
            card,
            text="REGISTRAR CONTA",
            font=ctk.CTkFont(family="Segoe UI", size=12),
            fg_color="#182030",
            hover_color="#222C40",
            text_color=THEME["text_secondary"],
            width=300,
            height=34,
            corner_radius=8,
            command=self.handle_register
        )
        btn_reg.pack(pady=4)

    def handle_login(self):
        user = self.entry_user.get().strip()
        pwd = self.entry_pass.get().strip()
        if not user or not pwd:
            self.lbl_status.configure(text="Preencha usuário e senha.", text_color=THEME["danger"])
            return

        self.lbl_status.configure(text="Autenticando...", text_color=THEME["accent"])
        self.update()

        def auth_thread():
            ok, msg = self.auth.login(user, pwd)
            self.after(0, lambda: self.post_auth(ok, msg, user))

        threading.Thread(target=auth_thread, daemon=True).start()

    def handle_register(self):
        user = self.entry_user.get().strip()
        pwd = self.entry_pass.get().strip()
        key = self.entry_key.get().strip()
        if not user or not pwd or not key:
            self.lbl_status.configure(text="Preencha usuário, senha e chave.", text_color=THEME["danger"])
            return

        self.lbl_status.configure(text="Registrando...", text_color=THEME["accent"])
        self.update()

        def reg_thread():
            ok, msg = self.auth.register(user, pwd, key)
            self.after(0, lambda: self.post_auth(ok, msg, user))

        threading.Thread(target=reg_thread, daemon=True).start()

    def post_auth(self, ok: bool, msg: str, user: str):
        if ok:
            self.launcher.config["last_username"] = user
            self.launcher.save_config()
            self.show_dashboard_screen()
        else:
            self.lbl_status.configure(text=msg, text_color=THEME["danger"])

    # =========================================================================
    # TELA PRINCIPAL (DASHBOARD & GERENCIAMENTO DE CARREGAMENTO)
    # =========================================================================
    def show_dashboard_screen(self):
        self.clear_screen()
        self.current_frame = ctk.CTkFrame(self, fg_color=THEME["bg"])
        self.current_frame.pack(fill="both", expand=True)

        # Header Superior
        header = ctk.CTkFrame(self.current_frame, fg_color=THEME["card"], height=56, corner_radius=0, border_color=THEME["card_border"], border_width=1)
        header.pack(fill="x", side="top")
        header.pack_propagate(False)

        logo_frame = ctk.CTkFrame(header, fg_color="transparent")
        logo_frame.pack(side="left", padx=20)
        lbl_sm = ctk.CTkLabel(logo_frame, text="SM", font=ctk.CTkFont(family="Segoe UI", size=22, weight="bold"), text_color=THEME["text_primary"])
        lbl_sm.pack(side="left")
        lbl_l = ctk.CTkLabel(logo_frame, text="L", font=ctk.CTkFont(family="Segoe UI", size=22, weight="bold"), text_color=THEME["accent"])
        lbl_l.pack(side="left")
        ctk.CTkLabel(logo_frame, text="SHOWCASE", font=ctk.CTkFont(family="Segoe UI", size=11, weight="bold"), text_color=THEME["text_muted"]).pack(side="left", padx=8)

        # Perfil do Usuário
        user_info = self.auth.get_user_info()
        prof_frame = ctk.CTkFrame(header, fg_color="transparent")
        prof_frame.pack(side="right", padx=20)

        ctk.CTkLabel(prof_frame, text=user_info.get("username", "Usuário"), font=ctk.CTkFont(family="Segoe UI", size=13, weight="bold"), text_color=THEME["text_primary"]).pack(anchor="e")
        ctk.CTkLabel(prof_frame, text=f"Expira: {user_info.get('expiry', 'N/A')}", font=ctk.CTkFont(family="Segoe UI", size=10), text_color=THEME["text_muted"]).pack(anchor="e")

        # Layout Central Dividido em Duas Colunas
        grid_frame = ctk.CTkFrame(self.current_frame, fg_color="transparent")
        grid_frame.pack(fill="both", expand=True, padx=20, pady=16)

        # Coluna Esquerda: Diagnóstico de Ambiente
        col_left = ctk.CTkFrame(grid_frame, fg_color=THEME["card"], width=300, border_color=THEME["card_border"], border_width=1, corner_radius=10)
        col_left.pack(side="left", fill="both", padx=(0, 12))
        col_left.pack_propagate(False)

        ctk.CTkLabel(col_left, text="AUDITORIA DE AMBIENTE", font=ctk.CTkFont(family="Segoe UI", size=13, weight="bold"), text_color=THEME["text_secondary"]).pack(anchor="w", padx=16, pady=(14, 8))

        self.lbl_audit_gta = self.create_audit_row(col_left, "gta_sa.exe:")
        self.lbl_audit_samp = self.create_audit_row(col_left, "samp.exe:")
        self.lbl_audit_moon_asi = self.create_audit_row(col_left, "MoonLoader.asi:")
        self.lbl_audit_moon_dir = self.create_audit_row(col_left, "Pasta moonloader/:")
        self.lbl_audit_moon_lib = self.create_audit_row(col_left, "Pasta lib/:")
        self.lbl_audit_imgui = self.create_audit_row(col_left, "Biblioteca ImGui:")
        self.lbl_audit_log = self.create_audit_row(col_left, "moonloader.log:")

        btn_view_logs = ctk.CTkButton(
            col_left,
            text="LER MOONLOADER.LOG AGORA",
            font=ctk.CTkFont(family="Segoe UI", size=11, weight="bold"),
            fg_color="#182338",
            hover_color="#223250",
            text_color=THEME["accent"],
            height=32,
            corner_radius=6,
            command=self.show_moonloader_logs_window
        )
        btn_view_logs.pack(side="bottom", fill="x", padx=14, pady=(0, 6))

        btn_logout = ctk.CTkButton(
            col_left,
            text="DESCONECTAR",
            font=ctk.CTkFont(family="Segoe UI", size=11, weight="bold"),
            fg_color="#1F151B",
            hover_color="#301A24",
            text_color=THEME["danger"],
            height=30,
            corner_radius=6,
            command=self.show_login_screen
        )
        btn_logout.pack(side="bottom", fill="x", padx=14, pady=(0, 6))

        # Coluna Direita: Seletor & Execuções em Camadas
        col_right = ctk.CTkFrame(grid_frame, fg_color=THEME["card"], border_color=THEME["card_border"], border_width=1, corner_radius=10)
        col_right.pack(side="right", fill="both", expand=True)

        ctk.CTkLabel(col_right, text="PASTA DO GTA & TESTES EM CAMADAS", font=ctk.CTkFont(family="Segoe UI", size=13, weight="bold"), text_color=THEME["text_secondary"]).pack(anchor="w", padx=16, pady=(12, 6))

        # Seletor com persistência imediata
        path_box = ctk.CTkFrame(col_right, fg_color="transparent")
        path_box.pack(fill="x", padx=16, pady=(0, 8))

        saved_path = self.launcher.config.get("gta_path", "")
        self.entry_gta_path = ctk.CTkEntry(
            path_box,
            placeholder_text="Selecione a pasta do GTA San Andreas...",
            fg_color="#141720",
            border_color="#262D3D",
            text_color=THEME["text_primary"],
            height=34,
            corner_radius=6
        )
        self.entry_gta_path.pack(side="left", fill="x", expand=True, padx=(0, 8))
        self.entry_gta_path.bind("<KeyRelease>", lambda e: self.on_gta_path_typed())
        
        if saved_path:
            self.entry_gta_path.insert(0, saved_path)

        btn_browse = ctk.CTkButton(
            path_box,
            text="PROCURAR",
            font=ctk.CTkFont(family="Segoe UI", size=12, weight="bold"),
            fg_color=THEME["accent"],
            hover_color=THEME["accent_hover"],
            width=85,
            height=34,
            corner_radius=6,
            command=self.handle_browse_gta
        )
        btn_browse.pack(side="right")

        # Container dos Botões de Teste
        test_box = ctk.CTkFrame(col_right, fg_color="#12151E", border_color="#1E232E", border_width=1, corner_radius=8)
        test_box.pack(fill="both", expand=True, padx=16, pady=(0, 10))

        ctk.CTkLabel(test_box, text="MODOS DE EXECUÇÃO & DIAGNÓSTICO:", font=ctk.CTkFont(family="Segoe UI", size=12, weight="bold"), text_color=THEME["text_secondary"]).pack(anchor="w", padx=12, pady=(10, 6))

        # Botão Etapa 1: Teste MoonLoader Isolado
        self.btn_step1 = ctk.CTkButton(
            test_box,
            text="[ ETAPA 1 ] TESTAR MOONLOADER ISOLADO (somalia_test.lua)",
            font=ctk.CTkFont(family="Segoe UI", size=12, weight="bold"),
            fg_color="#2A2210",
            hover_color="#403218",
            text_color=THEME["warning"],
            height=36,
            corner_radius=6,
            command=lambda: self.launch_mode("step1")
        )
        self.btn_step1.pack(fill="x", padx=12, pady=4)

        # Botão Etapa 5: Teste ImGui Isolado
        self.btn_step5 = ctk.CTkButton(
            test_box,
            text="[ ETAPA 5 ] TESTAR IMGUI ISOLADO (somalia_imgui_test.lua)",
            font=ctk.CTkFont(family="Segoe UI", size=12, weight="bold"),
            fg_color="#1C2E20",
            hover_color="#26422C",
            text_color=THEME["success"],
            height=36,
            corner_radius=6,
            command=lambda: self.launch_mode("step5")
        )
        self.btn_step5.pack(fill="x", padx=12, pady=4)

        # Botão Etapa 3: Instalação Direta
        self.btn_step3 = ctk.CTkButton(
            test_box,
            text="[ ETAPA 3 ] INICIAR MODO DIRETO (somalia_main.lua)",
            font=ctk.CTkFont(family="Segoe UI", size=12, weight="bold"),
            fg_color="#1A2438",
            hover_color="#243452",
            text_color=THEME["accent_hover"],
            height=36,
            corner_radius=6,
            command=lambda: self.launch_mode("step3")
        )
        self.btn_step3.pack(fill="x", padx=12, pady=4)

        # Botão Principal de Lançamento Protegido
        self.btn_launch = ctk.CTkButton(
            test_box,
            text="⚡ [ ETAPA 4 ] INICIAR SOMALIA PROTEGIDO (Modo Completo)",
            font=ctk.CTkFont(family="Segoe UI", size=13, weight="bold"),
            fg_color=THEME["accent"],
            hover_color=THEME["accent_hover"],
            text_color="#FFFFFF",
            height=42,
            corner_radius=6,
            command=lambda: self.launch_mode("protected")
        )
        self.btn_launch.pack(fill="x", padx=12, pady=(8, 10))

        self.validate_current_path()

    def create_audit_row(self, parent, label: str):
        f = ctk.CTkFrame(parent, fg_color="transparent")
        f.pack(fill="x", padx=16, pady=3)
        ctk.CTkLabel(f, text=label, font=ctk.CTkFont(family="Segoe UI", size=11), text_color=THEME["text_muted"]).pack(side="left")
        lbl_val = ctk.CTkLabel(f, text="⚪ ...", font=ctk.CTkFont(family="Segoe UI", size=11, weight="bold"), text_color=THEME["text_muted"])
        lbl_val.pack(side="right")
        return lbl_val

    def on_gta_path_typed(self):
        path = self.entry_gta_path.get().strip()
        self.launcher.config["gta_path"] = path
        self.launcher.save_config()
        self.validate_current_path()

    def handle_browse_gta(self):
        folder = filedialog.askdirectory(title="Selecione a pasta de instalacao do GTA San Andreas")
        if folder:
            self.entry_gta_path.delete(0, "end")
            self.entry_gta_path.insert(0, folder)
            self.launcher.config["gta_path"] = folder
            self.launcher.save_config()
            self.validate_current_path()

    def validate_current_path(self):
        path = self.entry_gta_path.get().strip()
        is_valid, status = self.launcher.validate_gta_directory(path)

        def set_lbl(lbl, ok, success_txt, fail_txt, is_warn=False):
            if ok:
                lbl.configure(text=f"✓ {success_txt}", text_color=THEME["success"])
            else:
                lbl.configure(text=f"✕ {fail_txt}", text_color=THEME["warning"] if is_warn else THEME["danger"])

        set_lbl(self.lbl_audit_gta, status.get("gta_sa"), "Encontrado", "Ausente")
        set_lbl(self.lbl_audit_samp, status.get("samp"), "Encontrado", "Ausente")
        set_lbl(self.lbl_audit_moon_asi, status.get("moonloader_asi"), "Instalado", "Ausente (Instale MoonLoader)", is_warn=True)
        set_lbl(self.lbl_audit_moon_dir, status.get("moonloader_folder"), "Existe", "Ausente")
        set_lbl(self.lbl_audit_moon_lib, status.get("moonloader_lib"), "Existe", "Ausente")
        set_lbl(self.lbl_audit_imgui, status.get("has_imgui"), "Encontrada", "Ausente", is_warn=True)
        
        log_txt = f"Ativo ({status.get('log_time')})" if status.get("moonloader_log") else "Não gerado ainda"
        set_lbl(self.lbl_audit_log, status.get("moonloader_log"), log_txt, "Ausente", is_warn=True)

    def launch_mode(self, mode: str):
        path = self.entry_gta_path.get().strip()
        is_valid, status = self.launcher.validate_gta_directory(path)
        if not is_valid:
            messagebox.showerror(
                "GTA Inválido",
                "Caminho do GTA inválido ou não encontrado.\nSelecione novamente uma pasta válida que contenha gta_sa.exe e samp.exe."
            )
            return

        username = self.auth.user_data.get("username", "User")

        if mode == "step1":
            ok, msg = self.launcher.deploy_test_script(path)
        elif mode == "step3":
            ok, msg = self.launcher.deploy_direct_unprotected(path)
        elif mode == "step5":
            ok, msg = self.launcher.deploy_imgui_test_script(path)
        else: # protected
            ok, msg = self.launcher.deploy_protected_package(path, username)

        if not ok:
            messagebox.showerror("Erro ao Preparar", msg)
            return

        ok_launch, msg_launch = self.launcher.launch_game(path)
        if not ok_launch:
            messagebox.showerror("Erro ao Executar", msg_launch)
            return

        messagebox.showinfo("Jogo Iniciado", f"Modo [{mode.upper()}] preparado e SA-MP iniciado!\nEntre no servidor e verifique se as mensagens aparecem no chat ou no moonloader.log.")
        threading.Thread(target=self.session_monitor_thread, daemon=True).start()

    def show_moonloader_logs_window(self):
        path = self.entry_gta_path.get().strip()
        log_content = self.launcher.read_moonloader_log(path)

        log_win = ctk.CTkToplevel(self)
        log_win.title("Diagnóstico - Logs do MoonLoader")
        log_win.geometry("720x460")
        log_win.configure(fg_color=THEME["bg"])

        ctk.CTkLabel(log_win, text="CONTEÚDO DE MOONLOADER.LOG (ÚLTIMAS LINHAS)", font=ctk.CTkFont(family="Segoe UI", size=13, weight="bold"), text_color=THEME["text_primary"]).pack(padx=16, pady=(12, 6), anchor="w")

        txt = ctk.CTkTextbox(log_win, fg_color="#12151E", text_color="#A5B8D8", font=ctk.CTkFont(family="Consolas", size=11))
        txt.pack(fill="both", expand=True, padx=16, pady=(0, 12))
        txt.insert("1.0", log_content)
        txt.configure(state="disabled")

    def session_monitor_thread(self):
        while self.launcher.is_game_running():
            time.sleep(1.5)
        self.launcher.cleanup_session()

if __name__ == "__main__":
    app = SomaliaClientApp()
    app.mainloop()
