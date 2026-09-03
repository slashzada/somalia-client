#include "LoaderMenu.h"
#include "imgui_internal.h"
#include "../Auth/KeyAuth.h"
#include "../Config/LoaderConfig.h"
#include "../Injector/Injector.h"
#pragma warning(push)
#pragma warning(disable: 4828)
#include "Fonts/bytearray.h"
#pragma warning(pop)
#include "TextureLoader.h"
#include <shlobj.h>
#include <commctrl.h>
#include <string>
#include <thread>
#include <vector>
#include <map>
#include <cmath>
#include <algorithm>

namespace LoaderMenu
{
    static HWND s_hWnd = NULL;
    static IDirect3DDevice9* s_pDevice = nullptr;
    static Screen s_CurrentScreen = Screen::Login;

    static char s_InputUser[64] = { 0 };
    static char s_InputPass[64] = { 0 };
    static char s_InputKey[64]  = { 0 };
    static std::string s_StatusText = "";
    static ImVec4 s_StatusColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    static bool s_IsAuthenticating = false;

    // Engrenagem / Modal do GTA SA
    static bool s_ShowGtaModal = false;
    static char s_GtaModalPathBuf[MAX_PATH] = { 0 };

    static KeyAuthClient* s_pKeyAuth = nullptr;
    static IDirect3DTexture9* s_pUserTexture = nullptr;

    // Fontes oficiais do Somalia
    static ImFont* s_FontTitle  = nullptr;
    static ImFont* s_FontButton = nullptr;
    static ImFont* s_FontBody   = nullptr;
    static ImFont* s_FontSmall  = nullptr;
    static ImFont* s_FontIcon   = nullptr;

    // Animação de transição global
    static float s_OpenAlpha = 0.0f;

    // Cores fiéis ao SomaliaNative (Theme.cpp)
    static const ImVec4 ColAccent       = ImVec4(137.f / 255.f, 207.f / 255.f, 240.f / 255.f, 1.0f); // #89CFF0 Baby Blue
    static const ImVec4 ColBgContent    = ImVec4(26.f  / 255.f, 26.f  / 255.f, 26.f  / 255.f, 1.0f); // #1A1A1A
    static const ImVec4 ColBgSidebar    = ImVec4(32.f  / 255.f, 32.f  / 255.f, 32.f  / 255.f, 1.0f); // #202020
    static const ImVec4 ColBgUserCard   = ImVec4(41.f  / 255.f, 41.f  / 255.f, 41.f  / 255.f, 1.0f); // #292929
    static const ImVec4 ColBorder       = ImVec4(50.f  / 255.f, 50.f  / 255.f, 50.f  / 255.f, 1.0f); // #323232
    static const ImVec4 ColTextMuted    = ImVec4(105.f / 255.f, 105.f / 255.f, 105.f / 255.f, 1.0f); // #696969
    static const ImVec4 ColSuccess      = ImVec4(50.f  / 255.f, 220.f / 255.f, 100.f / 255.f, 1.0f);
    static const ImVec4 ColDanger       = ImVec4(255.f / 255.f, 85.f  / 255.f, 95.f  / 255.f, 1.0f);

    void UpdateWindowSize(Screen s)
    {
        // O tamanho da janela é unificado (560x400) para evitar que o Direct3D 9
        // execute resets de dispositivo no meio da renderização do frame.
    }

    // Partículas flutuantes idênticas ao SomaliaNative
    static void RenderParticles()
    {
        ImVec2 screen_size = ImGui::GetIO().DisplaySize;
        if (screen_size.x <= 0 || screen_size.y <= 0)
            return;

        static ImVec2 particle_pos[50];
        static ImVec2 particle_target_pos[50];
        static float particle_speed[50];
        static float particle_radius[50];
        static bool s_InitParticles = false;

        if (!s_InitParticles)
        {
            for (int i = 0; i < 50; i++)
                particle_pos[i] = ImVec2(0, 0);
            s_InitParticles = true;
        }

        float dt = ImGui::GetIO().DeltaTime;
        if (dt <= 0.0f) dt = 0.016f;

        ImDrawList* draw = ImGui::GetWindowDrawList();

        for (int i = 0; i < 50; i++)
        {
            if (particle_pos[i].x <= 0 || particle_pos[i].y <= 0)
            {
                particle_pos[i].x = (float)(rand() % (int)screen_size.x + 1);
                particle_pos[i].y = (float)(rand() % 30) - 15.0f;
                particle_speed[i] = (float)(8 + rand() % 28);
                particle_radius[i] = (float)(1.2f + (rand() % 25) * 0.1f);

                particle_target_pos[i].x = (float)(rand() % (int)screen_size.x);
                particle_target_pos[i].y = screen_size.y * 1.5f;
            }

            particle_pos[i] = ImLerp(particle_pos[i], particle_target_pos[i], dt * (particle_speed[i] / 60.0f));

            if (particle_pos[i].y > screen_size.y)
            {
                particle_pos[i].x = 0;
                particle_pos[i].y = 0;
            }

            draw->AddCircleFilled(particle_pos[i], particle_radius[i], ImColor(137, 207, 240, (int)(115.0f * s_OpenAlpha)));
        }
    }

    // Botão X minimalista no canto superior direito
    static void RenderCloseButton(float posX, float posY)
    {
        ImVec2 btnPos(posX, posY);
        ImVec2 btnSize(16.0f, 16.0f);
        ImGui::SetCursorPos(btnPos);
        bool clicked = ImGui::InvisibleButton("##title_close", btnSize) || ImGui::IsItemClicked();
        bool hovered = ImGui::IsItemHovered();
        if (clicked)
            PostQuitMessage(0);

        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 p0 = ImGui::GetItemRectMin();
        ImVec2 p1 = ImGui::GetItemRectMax();

        ImU32 col = hovered ? IM_COL32(165, 225, 255, 255) : IM_COL32(95, 130, 160, 255);
        float pad = 2.0f;
        draw->AddLine(ImVec2(p0.x + pad, p0.y + pad), ImVec2(p1.x - pad, p1.y - pad), col, 2.0f);
        draw->AddLine(ImVec2(p1.x - pad, p0.y + pad), ImVec2(p0.x + pad, p1.y - pad), col, 2.0f);
    }

    // Botão Minimizar
    static void RenderMinButton(float posX, float posY)
    {
        ImVec2 btnPos(posX, posY);
        ImVec2 btnSize(16.0f, 16.0f);
        ImGui::SetCursorPos(btnPos);
        bool clicked = ImGui::InvisibleButton("##title_min", btnSize) || ImGui::IsItemClicked();
        bool hovered = ImGui::IsItemHovered();
        if (clicked)
            ShowWindow(s_hWnd, SW_MINIMIZE);

        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 p0 = ImGui::GetItemRectMin();
        ImVec2 p1 = ImGui::GetItemRectMax();

        ImU32 col = hovered ? IM_COL32(165, 225, 255, 255) : IM_COL32(95, 130, 160, 255);
        draw->AddLine(ImVec2(p0.x + 2, (p0.y + p1.y) * 0.5f), ImVec2(p1.x - 2, (p0.y + p1.y) * 0.5f), col, 2.0f);
    }

    // Botão de Engrenagem (Abre modal do GTA SA)
    static void RenderGearButton(float posX, float posY)
    {
        ImVec2 btnPos(posX, posY);
        ImVec2 btnSize(20.0f, 20.0f);
        ImGui::SetCursorPos(btnPos);
        bool clicked = ImGui::InvisibleButton("##title_gear", btnSize) || ImGui::IsItemClicked();
        bool hovered = ImGui::IsItemHovered();
        if (clicked)
        {
            s_ShowGtaModal = !s_ShowGtaModal;
            if (s_ShowGtaModal)
            {
                LoaderConfig& cfg = ConfigManager::Get();
                strncpy_s(s_GtaModalPathBuf, cfg.gtaPath.c_str(), sizeof(s_GtaModalPathBuf) - 1);
            }
        }

        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 p0 = ImGui::GetItemRectMin();
        ImVec2 p1 = ImGui::GetItemRectMax();

        ImU32 col = (hovered || s_ShowGtaModal) ? IM_COL32(137, 207, 240, 255) : IM_COL32(110, 140, 170, 255);

        if (s_FontIcon)
        {
            // O glifo 'c' na icon_font do Somalia é o ícone de engrenagem/configs
            draw->AddText(s_FontIcon, 19.0f, ImVec2(p0.x + 1, p0.y - 2), col, "c");
        }
        else
        {
            // Fallback desenho vetorial de engrenagem
            ImVec2 center((p0.x + p1.x) * 0.5f, (p0.y + p1.y) * 0.5f);
            draw->AddCircle(center, 6.0f, col, 12, 1.8f);
            draw->AddCircleFilled(center, 2.5f, col);
        }
    }

    // Input escuro com placeholder integrado
    static bool SomaliaInput(const char* label, char* buf, int bufSize, const char* placeholder, float width, float height, ImGuiInputTextFlags flags = 0)
    {
        ImGui::SetNextItemWidth(width);

        float fontSize = ImGui::GetFontSize();
        float padY = (height - fontSize) * 0.5f;
        if (padY < 4.0f) padY = 4.0f;

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, padY));

        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(20, 20, 20, 255));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(28, 28, 28, 255));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, IM_COL32(28, 28, 28, 255));
        ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(50, 50, 50, 255));
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(245, 245, 245, 255));

        bool changed = ImGui::InputText(label, buf, bufSize, flags);

        ImVec2 rMin = ImGui::GetItemRectMin();

        ImGui::PopStyleColor(5);
        ImGui::PopStyleVar(3);

        if (buf[0] == '\0' && !ImGui::IsItemActive())
        {
            ImDrawList* draw = ImGui::GetWindowDrawList();
            draw->AddText(ImVec2(rMin.x + 12.0f, rMin.y + padY), IM_COL32(105, 105, 105, 255), placeholder);
        }

        return changed;
    }

    // Botão com animação fluida e gradiente Baby Blue do Somalia
    static bool SomaliaButton(const char* label, ImVec2 size, ImVec2 pos = ImVec2(-1, -1))
    {
        if (pos.x >= 0.0f && pos.y >= 0.0f)
            ImGui::SetCursorPos(pos);

        bool clicked = ImGui::InvisibleButton(label, size) || ImGui::IsItemClicked();
        bool hovered = ImGui::IsItemHovered();
        bool active  = ImGui::IsItemActive();

        ImVec2 p0 = ImGui::GetItemRectMin();
        ImVec2 p1 = ImGui::GetItemRectMax();
        ImDrawList* draw = ImGui::GetWindowDrawList();

        ImGuiID id = ImGui::GetID(label);
        static std::map<ImGuiID, float> anim_hover;
        float& hAnim = anim_hover[id];
        float targetHover = (active ? 0.7f : (hovered ? 1.0f : 0.0f));
        float dt = ImGui::GetIO().DeltaTime;
        if (dt <= 0.0f) dt = 0.016f;
        hAnim = ImLerp(hAnim, targetHover, 12.0f * dt);

        float r = 7.0f;

        // Gradiente Baby Blue #89CFF0 -> #68BCE8
        int r0 = 137, g0 = 207, b0 = 240;
        int r1 = 104, g1 = 188, b1 = 232;

        r0 = (int)(r0 + 20 * hAnim);
        g0 = (int)(g0 + 15 * hAnim);
        b0 = (int)(b0 + 15 * hAnim);
        r1 = (int)(r1 + 25 * hAnim);
        g1 = (int)(g1 + 20 * hAnim);
        b1 = (int)(b1 + 20 * hAnim);

        r0 = (std::min)(255, r0); g0 = (std::min)(255, g0); b0 = (std::min)(255, b0);
        r1 = (std::min)(255, r1); g1 = (std::min)(255, g1); b1 = (std::min)(255, b1);

        int cols = (int)size.x;
        for (int i = 0; i < cols; ++i)
        {
            float t = (float)i / (float)(cols - 1);
            int cr = (int)(r0 + (r1 - r0) * t);
            int cg = (int)(g0 + (g1 - g0) * t);
            int cb = (int)(b0 + (b1 - b0) * t);
            ImU32 col = IM_COL32(cr, cg, cb, 255);

            float top, bot;
            if (i < r)
            {
                float dx = r - (float)i - 0.5f;
                float dy = sqrtf((std::max)(0.0f, r * r - dx * dx));
                top = p0.y + r - dy;
                bot = p1.y - r + dy;
            }
            else if (i >= cols - r)
            {
                float dx = (float)i + 0.5f - (size.x - r);
                float dy = sqrtf((std::max)(0.0f, r * r - dx * dx));
                top = p0.y + r - dy;
                bot = p1.y - r + dy;
            }
            else
            {
                top = p0.y;
                bot = p1.y;
            }

            draw->AddLine(ImVec2(p0.x + (float)i + 0.5f, top), ImVec2(p0.x + (float)i + 0.5f, bot), col, 1.0f);
        }

        const char* textEnd = strchr(label, '#');
        if (!textEnd) textEnd = label + strlen(label);
        std::string cleanLabel(label, textEnd);

        if (s_FontButton)
            ImGui::PushFont(s_FontButton);

        ImVec2 textSize = ImGui::CalcTextSize(cleanLabel.c_str());
        ImVec2 textPos(p0.x + (size.x - textSize.x) * 0.5f, p0.y + (size.y - textSize.y) * 0.5f);
        draw->AddText(textPos, IM_COL32(255, 255, 255, 255), cleanLabel.c_str());

        if (s_FontButton)
            ImGui::PopFont();

        return clicked;
    }

    static std::string BrowseGtaFolder()
    {
        char path[MAX_PATH] = { 0 };
        BROWSEINFOA bi = { 0 };
        bi.hwndOwner = s_hWnd;
        bi.lpszTitle = "Selecione a Pasta Raiz do GTA San Andreas:";
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

        LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
        if (pidl != 0)
        {
            SHGetPathFromIDListA(pidl, path);
            IMalloc* imalloc = 0;
            if (SUCCEEDED(SHGetMalloc(&imalloc)))
            {
                imalloc->Free(pidl);
                imalloc->Release();
            }
            return std::string(path);
        }
        return "";
    }

    static std::string GetAsiPath()
    {
        LoaderConfig& cfg = ConfigManager::Get();
        if (!cfg.gtaPath.empty())
        {
            std::string candidate = cfg.gtaPath + "\\SomaliaNative.asi";
            if (GetFileAttributesA(candidate.c_str()) != INVALID_FILE_ATTRIBUTES)
                return candidate;
        }
        char exePath[MAX_PATH] = { 0 };
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        std::string dir = exePath;
        size_t lastSlash = dir.find_last_of("\\/");
        if (lastSlash != std::string::npos)
        {
            dir = dir.substr(0, lastSlash);
            std::vector<std::string> candidates = {
                dir + "\\SomaliaNative.asi",
                dir + "\\build\\SomaliaNative.asi",
                dir + "\\..\\SomaliaNative\\build\\SomaliaNative.asi",
                dir + "\\..\\SomaliaNative.asi",
                dir + "\\..\\dist\\SomaliaNative.asi",
                dir + "\\dist\\SomaliaNative.asi",
                dir + "\\SomaliaNative\\build\\SomaliaNative.asi"
            };
            for (const auto& c : candidates)
            {
                if (GetFileAttributesA(c.c_str()) != INVALID_FILE_ATTRIBUTES)
                    return c;
            }
        }
        if (GetFileAttributesA("SomaliaNative.asi") != INVALID_FILE_ATTRIBUTES)
            return "SomaliaNative.asi";
        if (GetFileAttributesA("dist\\SomaliaNative.asi") != INVALID_FILE_ATTRIBUTES)
            return "dist\\SomaliaNative.asi";
        if (GetFileAttributesA("SomaliaNative\\build\\SomaliaNative.asi") != INVALID_FILE_ATTRIBUTES)
            return "SomaliaNative\\build\\SomaliaNative.asi";

        return "SomaliaNative.asi";
    }

    void Init(HWND hWnd, IDirect3DDevice9* pDevice)
    {
        s_hWnd = hWnd;
        s_pDevice = pDevice;

        ConfigManager::Load();
        LoaderConfig& cfg = ConfigManager::Get();
        if (!cfg.lastUsername.empty())
            strncpy_s(s_InputUser, cfg.lastUsername.c_str(), sizeof(s_InputUser) - 1);

        s_pKeyAuth = new KeyAuthClient(cfg.keyauthName, cfg.keyauthOwner, cfg.keyauthSecret, cfg.keyauthVersion);
        s_pKeyAuth->Init();
    }

    void SetupFonts()
    {
        ImGuiIO& io = ImGui::GetIO();

        static const ImWchar ranges[] =
        {
            0x0020, 0x00FF, // Basic Latin + Latin Supplement
            0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
            0x2DE0, 0x2DFF, // Cyrillic Extended-A
            0xA640, 0xA69F, // Cyrillic Extended-B
            0xE000, 0xE226, // Icons
            0,
        };

        ImFontConfig font_config;
        font_config.PixelSnapH = false;
        font_config.OversampleH = 5;
        font_config.OversampleV = 5;
        font_config.RasterizerMultiply = 1.2f;
        font_config.GlyphRanges = ranges;

        s_FontBody   = io.Fonts->AddFontFromMemoryTTF(poppin_font, sizeof(poppin_font), 15.0f, &font_config, ranges);
        s_FontIcon   = io.Fonts->AddFontFromMemoryTTF(icon_font, sizeof(icon_font), 22.0f, &font_config, ranges);
        s_FontTitle  = io.Fonts->AddFontFromMemoryTTF(poppin_font, sizeof(poppin_font), 25.0f, &font_config, ranges);
        s_FontButton = io.Fonts->AddFontFromMemoryTTF(poppin_font, sizeof(poppin_font), 16.0f, &font_config, ranges);
        s_FontSmall  = io.Fonts->AddFontFromMemoryTTF(poppin_font, sizeof(poppin_font), 13.0f, &font_config, ranges);

        if (!s_FontBody)   s_FontBody   = io.Fonts->AddFontDefault();
        if (!s_FontTitle)  s_FontTitle  = s_FontBody;
        if (!s_FontButton) s_FontButton = s_FontBody;
        if (!s_FontSmall)  s_FontSmall  = s_FontBody;
    }

    Screen GetCurrentScreen() { return s_CurrentScreen; }
    void SetCurrentScreen(Screen s)
    {
        s_CurrentScreen = s;
        UpdateWindowSize(s);
    }

    // ==========================================================
    //  TELA DE LOGIN / REGISTRO (Sem sobreposição nem bugs)
    // ==========================================================
    static void RenderLoginScreen()
    {
        ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        float winW = displaySize.x;
        float winH = displaySize.y;

        // Botões de topo
        RenderCloseButton(winW - 24.0f, 10.0f);
        RenderMinButton(winW - 48.0f, 10.0f);

        bool isRegister = (s_CurrentScreen == Screen::Register);

        // Título "Somalia Group"
        float titleY = isRegister ? 32.0f : 44.0f;
        {
            const char* title = "Somalia Group";
            if (s_FontTitle) ImGui::PushFont(s_FontTitle);

            ImVec2 titleSize = ImGui::CalcTextSize(title);
            float titleX = (winW - titleSize.x) * 0.5f;
            ImGui::SetCursorPos(ImVec2(titleX, titleY));
            ImGui::TextColored(ColAccent, "%s", title);

            if (s_FontTitle) ImGui::PopFont();
        }

        // Inputs com dimensões e espaçamentos impecáveis
        float inputW = 290.0f;
        float inputH = 40.0f;
        float inputX = (winW - inputW) * 0.5f;

        if (!isRegister)
        {
            // Login: Username e Password
            float userY = 110.0f;
            float passY = 168.0f;

            ImGui::SetCursorPos(ImVec2(inputX, userY));
            SomaliaInput("##user", s_InputUser, sizeof(s_InputUser), "Username", inputW, inputH);

            ImGui::SetCursorPos(ImVec2(inputX, passY));
            SomaliaInput("##pass", s_InputPass, sizeof(s_InputPass), "Password", inputW, inputH, ImGuiInputTextFlags_Password);
        }
        else
        {
            // Registro: Username, Password e License Key
            float userY = 82.0f;
            float passY = 136.0f;
            float keyY  = 190.0f;

            ImGui::SetCursorPos(ImVec2(inputX, userY));
            SomaliaInput("##user", s_InputUser, sizeof(s_InputUser), "Username", inputW, inputH);

            ImGui::SetCursorPos(ImVec2(inputX, passY));
            SomaliaInput("##pass", s_InputPass, sizeof(s_InputPass), "Password", inputW, inputH, ImGuiInputTextFlags_Password);

            ImGui::SetCursorPos(ImVec2(inputX, keyY));
            SomaliaInput("##key", s_InputKey, sizeof(s_InputKey), "License Key", inputW, inputH);
        }

        // Mensagem de Status (se houver)
        if (!s_StatusText.empty())
        {
            float statusY = isRegister ? 240.0f : 216.0f;
            ImVec2 statusSize = ImGui::CalcTextSize(s_StatusText.c_str());
            float statusX = (winW - statusSize.x) * 0.5f;
            ImGui::SetCursorPos(ImVec2(statusX, statusY));
            ImGui::TextColored(s_StatusColor, "%s", s_StatusText.c_str());
        }

        // Botão Login / Register
        float btnW = 180.0f;
        float btnH = 42.0f;
        float btnX = (winW - btnW) * 0.5f;
        float btnY = isRegister ? 266.0f : 246.0f;

        const char* btnLabel = isRegister ? "Register##btn" : "Login##btn";
        if (SomaliaButton(btnLabel, ImVec2(btnW, btnH), ImVec2(btnX, btnY)) && !s_IsAuthenticating)
        {
            s_IsAuthenticating = true;
            s_StatusText = "Validando...";
            s_StatusColor = ColAccent;

            std::thread authThread([isRegister]()
            {
                if (!isRegister)
                {
                    AuthResponse res = s_pKeyAuth->Login(s_InputUser, s_InputPass);
                    if (res.success)
                    {
                        const KeyAuthUser& u = s_pKeyAuth->GetUser();
                        LoaderConfig& cfg = ConfigManager::Get();
                        cfg.lastUsername = s_InputUser;
                        cfg.userSubscription = u.subscription.empty() ? "VIP Lifetime" : u.subscription;
                        cfg.userExpiry = u.expiry.empty() ? "Vitalicio" : u.expiry;
                        cfg.userDaysLeft = u.daysLeft.empty() ? "Ilimitado" : u.daysLeft;
                        cfg.sessionId = s_pKeyAuth->GetSessionId();
                        ConfigManager::Save();

                        // Sincroniza o arquivo com a pasta do GTA para o SomaliaNative.asi ler diretamente
                        if (!cfg.gtaPath.empty())
                        {
                            std::string gtaCfg = cfg.gtaPath + "\\somalia_client.json";
                            ConfigManager::Save(gtaCfg);
                        }

                        s_StatusText = "";
                        s_CurrentScreen = Screen::Dashboard;
                        UpdateWindowSize(Screen::Dashboard);
                    }
                    else
                    {
                        s_StatusText = res.message;
                        s_StatusColor = ColDanger;
                    }
                }
                else
                {
                    AuthResponse res = s_pKeyAuth->Register(s_InputUser, s_InputPass, s_InputKey);
                    if (res.success)
                    {
                        s_StatusText = res.message;
                        s_StatusColor = ColSuccess;
                        s_CurrentScreen = Screen::Login;
                        UpdateWindowSize(Screen::Login);
                    }
                    else
                    {
                        s_StatusText = res.message;
                        s_StatusColor = ColDanger;
                    }
                }
                s_IsAuthenticating = false;
            });
            authThread.detach();
        }

        // Link Footer ("Ainda não tenho uma conta? Register")
        float footerY = isRegister ? 336.0f : 318.0f;
        {
            const char* text1 = (!isRegister)
                ? "Ainda não tenho uma conta?  "
                : "Já possui uma conta?  ";
            const char* text2 = (!isRegister) ? "Register" : "Login";

            if (s_FontSmall) ImGui::PushFont(s_FontSmall);

            ImVec2 t1Size = ImGui::CalcTextSize(text1);
            ImVec2 t2Size = ImGui::CalcTextSize(text2);
            float totalW = t1Size.x + t2Size.x;
            float startX = (winW - totalW) * 0.5f;

            ImGui::SetCursorPos(ImVec2(startX, footerY));
            ImGui::TextColored(ColTextMuted, "%s", text1);

            ImGui::SetCursorPos(ImVec2(startX + t1Size.x, footerY));
            bool linkClicked = ImGui::InvisibleButton("##footer_toggle", t2Size) || ImGui::IsItemClicked();
            bool linkHovered = ImGui::IsItemHovered();

            if (linkClicked)
            {
                s_CurrentScreen = (!isRegister) ? Screen::Register : Screen::Login;
                s_StatusText = "";
                UpdateWindowSize(s_CurrentScreen);
            }

            ImU32 linkCol = linkHovered ? IM_COL32(137, 207, 240, 255) : IM_COL32(220, 235, 250, 255);
            ImDrawList* draw = ImGui::GetWindowDrawList();
            draw->AddText(ImGui::GetItemRectMin(), linkCol, text2);

            if (s_FontSmall) ImGui::PopFont();
        }
    }

    // ==========================================================
    //  MODAL: CONFIGURAÇÕES DO GTA SAN ANDREAS (Via Engrenagem)
    // ==========================================================
    static void RenderGtaModal(float winW, float winH)
    {
        ImDrawList* draw = ImGui::GetWindowDrawList();

        // 1. Fundo escuro semi-transparente bloqueando o fundo
        draw->AddRectFilled(ImVec2(0, 0), ImVec2(winW, winH), IM_COL32(0, 0, 0, 195));

        // 2. Card Central
        float modalW = 500.0f;
        float modalH = 240.0f;
        float modalX = (winW - modalW) * 0.5f;
        float modalY = (winH - modalH) * 0.5f;

        draw->AddRectFilled(ImVec2(modalX, modalY), ImVec2(modalX + modalW, modalY + modalH), IM_COL32(32, 32, 32, 255), 10.0f);
        draw->AddRect(ImVec2(modalX, modalY), ImVec2(modalX + modalW, modalY + modalH), IM_COL32(50, 50, 50, 255), 10.0f, 0, 1.2f);

        // Barra de Título do Modal
        {
            draw->AddText(s_FontTitle, 19.0f, ImVec2(modalX + 20, modalY + 16), IM_COL32(137, 207, 240, 255), "Configurações do GTA San Andreas");

            // Botão Fechar Modal (X)
            ImVec2 closePos(modalX + modalW - 32, modalY + 16);
            ImGui::SetCursorPos(closePos);
            bool closeClicked = ImGui::InvisibleButton("##modal_close", ImVec2(20, 20)) || ImGui::IsItemClicked();
            bool closeHover = ImGui::IsItemHovered();
            if (closeClicked)
                s_ShowGtaModal = false;

            ImU32 closeCol = closeHover ? IM_COL32(255, 100, 100, 255) : IM_COL32(130, 130, 130, 255);
            ImVec2 cP0 = ImGui::GetItemRectMin();
            ImVec2 cP1 = ImGui::GetItemRectMax();
            draw->AddLine(ImVec2(cP0.x + 3, cP0.y + 3), ImVec2(cP1.x - 3, cP1.y - 3), closeCol, 2.0f);
            draw->AddLine(ImVec2(cP1.x - 3, cP0.y + 3), ImVec2(cP0.x + 3, cP1.y - 3), closeCol, 2.0f);

            draw->AddLine(ImVec2(modalX, modalY + 46), ImVec2(modalX + modalW, modalY + 46), IM_COL32(50, 50, 50, 255), 1.0f);
        }

        // Conteúdo do Modal
        {
            float contentX = modalX + 24.0f;
            float contentW = modalW - 48.0f;

            // Label
            ImGui::SetCursorPos(ImVec2(contentX, modalY + 60));
            ImGui::TextColored(ColTextMuted, "Pasta Raiz do Jogo (onde se encontra o gta_sa.exe):");

            // Input do Caminho
            ImGui::SetCursorPos(ImVec2(contentX, modalY + 84));
            SomaliaInput("##modal_gta_path", s_GtaModalPathBuf, sizeof(s_GtaModalPathBuf), "Caminho do GTA San Andreas...", contentW, 36.0f);

            // Verificação de existência do gta_sa.exe
            std::string checkExe = std::string(s_GtaModalPathBuf) + "\\gta_sa.exe";
            bool exists = (s_GtaModalPathBuf[0] != '\0') && (GetFileAttributesA(checkExe.c_str()) != INVALID_FILE_ATTRIBUTES);

            ImGui::SetCursorPos(ImVec2(contentX, modalY + 128));
            if (s_GtaModalPathBuf[0] == '\0')
            {
                ImGui::TextColored(ColTextMuted, "Nenhuma pasta configurada no momento.");
            }
            else if (exists)
            {
                ImGui::TextColored(ColSuccess, "[OK] gta_sa.exe localizado com sucesso nesta pasta!");
            }
            else
            {
                ImGui::TextColored(ColDanger, "[ERRO] gta_sa.exe nao encontrado no caminho informado.");
            }

            // Botão "Selecionar Pasta"
            float btnW = 190.0f;
            float btnH = 38.0f;

            if (SomaliaButton("Selecionar Pasta##modal_browse", ImVec2(btnW, btnH), ImVec2(contentX, modalY + 175)))
            {
                std::string chosen = BrowseGtaFolder();
                if (!chosen.empty())
                {
                    strncpy_s(s_GtaModalPathBuf, chosen.c_str(), sizeof(s_GtaModalPathBuf) - 1);
                    LoaderConfig& cfg = ConfigManager::Get();
                    cfg.gtaPath = chosen;
                    ConfigManager::Save();
                }
            }

            // Botão "Salvar e Fechar"
            if (SomaliaButton("Salvar e Fechar##modal_save", ImVec2(btnW, btnH), ImVec2(contentX + contentW - btnW, modalY + 175)))
            {
                LoaderConfig& cfg = ConfigManager::Get();
                cfg.gtaPath = s_GtaModalPathBuf;
                ConfigManager::Save();
                s_ShowGtaModal = false;
            }
        }
    }

    static bool s_ShowSelfDestructModal = false;

    static void RenderSelfDestructModal(float winW, float winH)
    {
        auto draw = ImGui::GetWindowDrawList();

        // 1. Backdrop escurecido
        draw->AddRectFilled(ImVec2(0, 0), ImVec2(winW, winH), IM_COL32(5, 5, 8, 220));

        // 2. Caixa do Modal Centralizada
        float modalW = 440.0f;
        float modalH = 195.0f;
        float modalX = (winW - modalW) * 0.5f;
        float modalY = (winH - modalH) * 0.5f;

        draw->AddRectFilled(ImVec2(modalX, modalY), ImVec2(modalX + modalW, modalY + modalH), IM_COL32(32, 32, 32, 255), 8.0f);
        draw->AddRect(ImVec2(modalX, modalY), ImVec2(modalX + modalW, modalY + modalH), IM_COL32(50, 50, 50, 255), 8.0f, 0, 1.2f);

        // Faixa de destaque vermelho no topo
        draw->AddRectFilled(ImVec2(modalX + 1, modalY + 1), ImVec2(modalX + modalW - 1, modalY + 4), IM_COL32(230, 60, 60, 255), 8.0f, ImDrawCornerFlags_Top);

        // Título
        draw->AddText(s_FontTitle, 16.0f, ImVec2(modalX + 24, modalY + 18), IM_COL32(240, 80, 80, 255), "AUTODESTRUICAO & BYPASS TOTAL");

        // Textos informativos
        draw->AddText(s_FontBody, 13.0f, ImVec2(modalX + 24, modalY + 48), IM_COL32(220, 220, 220, 255),
            "Esta acao ira apagar e desinjetar tudo na hora:");
        draw->AddText(s_FontSmall, 12.0f, ImVec2(modalX + 32, modalY + 68), IM_COL32(180, 180, 180, 255),
            "- Desinjeta o menu e o cheat do GTA SA imediatamente");
        draw->AddText(s_FontSmall, 12.0f, ImVec2(modalX + 32, modalY + 86), IM_COL32(180, 180, 180, 255),
            "- Apaga logs, configs, SomaliaNative.asi e o executavel do loader");

        // Botões de Confirmação
        float btnW = 195.0f;
        float btnH = 38.0f;
        float btnY = modalY + 135.0f;

        if (SomaliaButton("DESINJETAR & APAGAR##self_confirm", ImVec2(btnW, btnH), ImVec2(modalX + 20, btnY)))
        {
            // 1. Desinjeta o cheat do GTA SA em tempo real se o jogo estiver aberto
            std::string unloadErr;
            Injector::UnloadGame("SomaliaNative.asi", unloadErr);
            Sleep(150);

            // 2. Apaga arquivos de log, configs e o proprio .asi
            const char* logsToDelete[] = {
                "somalia_native.log",
                "loader_debug.log",
                "somalia_config.json",
                "somalia_client.json",
                "SomaliaNative.asi"
            };
            for (const char* logFile : logsToDelete) DeleteFileA(logFile);

            LoaderConfig& cfg = ConfigManager::Get();
            if (!cfg.gtaPath.empty())
            {
                for (const char* logFile : logsToDelete)
                {
                    std::string fullPath = cfg.gtaPath + "\\" + logFile;
                    DeleteFileA(fullPath.c_str());
                }
            }

            char recentPath[MAX_PATH];
            if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_RECENT, NULL, 0, recentPath)))
            {
                std::string searchPattern = std::string(recentPath) + "\\*Somalia*";
                WIN32_FIND_DATAA fd;
                HANDLE hFind = FindFirstFileA(searchPattern.c_str(), &fd);
                if (hFind != INVALID_HANDLE_VALUE)
                {
                    do {
                        std::string item = std::string(recentPath) + "\\" + fd.cFileName;
                        DeleteFileA(item.c_str());
                    } while (FindNextFileA(hFind, &fd));
                    FindClose(hFind);
                }
            }

            char exePath[MAX_PATH];
            GetModuleFileNameA(NULL, exePath, MAX_PATH);

            char cmd[MAX_PATH * 2];
            snprintf(cmd, sizeof(cmd), "/c timeout /t 1 > nul & del /f /q \"%s\"", exePath);
            ShellExecuteA(NULL, "open", "cmd.exe", cmd, NULL, SW_HIDE);

            ExitProcess(0);
        }

        if (SomaliaButton("CANCELAR##self_cancel", ImVec2(btnW, btnH), ImVec2(modalX + modalW - btnW - 24, btnY)))
        {
            s_ShowSelfDestructModal = false;
        }
    }

    // ==========================================================
    //  DASHBOARD LIMPO, ELEGANTE E COM A ENGRENAGEM NO TOPO
    // ==========================================================
    static void RenderDashboardScreen()
    {
        ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        float winW = displaySize.x;
        float winH = displaySize.y;

        const KeyAuthUser& user = s_pKeyAuth->GetUser();

        // 1. Topo: Título e Controles (Engrenagem, Minimizar, Fechar)
        {
            if (s_FontTitle) ImGui::PushFont(s_FontTitle);
            ImGui::SetCursorPos(ImVec2(24, 10));
            ImGui::TextColored(ColAccent, "Somalia Group");
            if (s_FontTitle) ImGui::PopFont();

            RenderGearButton(winW - 74.0f, 12.0f);
            RenderMinButton(winW - 48.0f, 12.0f);
            RenderCloseButton(winW - 24.0f, 12.0f);
        }

        ImDrawList* draw = ImGui::GetWindowDrawList();

        // Separador sutil
        draw->AddLine(ImVec2(24, 44), ImVec2(winW - 24, 44), IM_COL32(45, 45, 45, 255), 1.0f);

        // 2. CARD 1: Perfil do Usuário
        float card1Y = 54.0f;
        float card1W = winW - 48.0f;
        float card1H = 82.0f;

        draw->AddRectFilled(ImVec2(24, card1Y), ImVec2(24 + card1W, card1Y + card1H), IM_COL32(32, 32, 32, 255), 8.0f);
        draw->AddRect(ImVec2(24, card1Y), ImVec2(24 + card1W, card1Y + card1H), IM_COL32(50, 50, 50, 255), 8.0f, 0, 1.0f);

        // Avatar
        {
            float avCenterX = 64.0f;
            float avCenterY = card1Y + 41.0f;

            // Avatar Moderno e Minimalista (Sem foto de anime)
            draw->AddCircleFilled(ImVec2(avCenterX, avCenterY), 22.0f, IM_COL32(24, 28, 38, 255));
            draw->AddCircle(ImVec2(avCenterX, avCenterY), 22.0f, IM_COL32(137, 207, 240, 255), 32, 1.5f);
            char initial[2] = { (char)toupper(user.username.empty() ? (s_InputUser[0] ? s_InputUser[0] : 'S') : user.username[0]), '\0' };
            draw->AddText(s_FontTitle, 20.0f, ImVec2(avCenterX - 6.0f, avCenterY - 14.0f), IM_COL32(137, 207, 240, 255), initial);

            // Info de Texto
            ImGui::SetCursorPos(ImVec2(98, card1Y + 16));
            ImGui::TextColored(ImVec4(0.96f, 0.96f, 0.98f, 1.0f), "%s", user.username.empty() ? s_InputUser : user.username.c_str());

            ImGui::SetCursorPos(ImVec2(98, card1Y + 42));
            ImGui::TextColored(ColTextMuted, "Plano: %s", user.subscription.empty() ? "VIP Lifetime" : user.subscription.c_str());

            // Badge Tempo Restante
            std::string timeBadge = "Tempo: " + (user.daysLeft.empty() ? "Ilimitado" : user.daysLeft);
            if (s_FontSmall) ImGui::PushFont(s_FontSmall);
            ImVec2 bSize = ImGui::CalcTextSize(timeBadge.c_str());

            float bPadX = 14.0f;
            float bW = bSize.x + bPadX * 2;
            float bH = 28.0f;
            float bX = 24 + card1W - bW - 18;
            float bY = card1Y + (card1H - bH) * 0.5f;

            draw->AddRectFilled(ImVec2(bX, bY), ImVec2(bX + bW, bY + bH), IM_COL32(24, 28, 38, 255), 5.0f);
            draw->AddRect(ImVec2(bX, bY), ImVec2(bX + bW, bY + bH), IM_COL32(137, 207, 240, 180), 5.0f, 0, 1.0f);
            draw->AddText(ImVec2(bX + bPadX, bY + (bH - bSize.y) * 0.5f), IM_COL32(137, 207, 240, 255), timeBadge.c_str());

            if (s_FontSmall) ImGui::PopFont();
        }

        // 3. CARD 2: Injetor e Status do GTA SA
        float card2Y = 148.0f;
        float card2W = winW - 48.0f;
        float card2H = 215.0f;

        draw->AddRectFilled(ImVec2(24, card2Y), ImVec2(24 + card2W, card2Y + card2H), IM_COL32(32, 32, 32, 255), 8.0f);
        draw->AddRect(ImVec2(24, card2Y), ImVec2(24 + card2W, card2Y + card2H), IM_COL32(50, 50, 50, 255), 8.0f, 0, 1.0f);

        bool isGameRunning = Injector::IsGameRunning();
        bool isWaiting = Injector::IsAutoInjectWaiting();

        // Linha de Status com Ponto Pulsante
        {
            float statusDotX = 48.0f;
            float statusDotY = card2Y + 36.0f;

            ImU32 dotCol;
            const char* mainStatus;
            const char* subStatus;

            if (isGameRunning)
            {
                DWORD pid = Injector::FindProcessId("gta_sa.exe");
                static char runningBuf[128];
                sprintf_s(runningBuf, "GTA SAN ANDREAS DETECTADO (PID: %d)", pid);
                mainStatus = runningBuf;
                subStatus = "O executavel gta_sa.exe esta ativo e pronto para receber o SomaliaNative.asi.";
                dotCol = IM_COL32(50, 220, 100, 255);
            }
            else if (isWaiting)
            {
                mainStatus = "AGUARDANDO INICIALIZACAO DO GTA SAN ANDREAS...";
                subStatus = "Abra seu GTA normalmente. O Somalia sera injetado no primeiro instante do jogo.";
                dotCol = IM_COL32(137, 207, 240, 255);
            }
            else
            {
                mainStatus = "GTA SAN ANDREAS NAO DETECTADO";
                subStatus = "Inicie o jogo ou clique em INJETAR para ativar a deteccao automatica.";
                dotCol = IM_COL32(140, 140, 140, 255);
            }

            draw->AddCircleFilled(ImVec2(statusDotX, statusDotY), 5.0f, dotCol);
            draw->AddCircle(ImVec2(statusDotX, statusDotY), 8.0f, dotCol, 16, 1.0f);

            draw->AddText(s_FontBody, 15.0f, ImVec2(statusDotX + 16, statusDotY - 9), dotCol, mainStatus);
            draw->AddText(s_FontSmall, 13.0f, ImVec2(statusDotX + 16, statusDotY + 14), IM_COL32(140, 140, 140, 255), subStatus);
        }

        // Mensagem de log do injetor (se houver)
        std::string injectMsg = Injector::GetStatusMessage();
        if (!injectMsg.empty())
        {
            draw->AddText(s_FontSmall, 13.0f, ImVec2(48, card2Y + 88), IM_COL32(137, 207, 240, 255), injectMsg.c_str());
        }

        static std::string s_BypassStatus = "";
        static uint64_t s_BypassTime = 0;

        auto ExecuteLogBypass = []()
        {
            const char* logsToDelete[] = {
                "somalia_native.log",
                "loader_debug.log",
                "somalia_config.json"
            };

            for (const char* logFile : logsToDelete)
            {
                DeleteFileA(logFile);
            }

            LoaderConfig& cfg = ConfigManager::Get();
            if (!cfg.gtaPath.empty())
            {
                for (const char* logFile : logsToDelete)
                {
                    std::string fullPath = cfg.gtaPath + "\\" + logFile;
                    DeleteFileA(fullPath.c_str());
                }
            }

            char recentPath[MAX_PATH];
            if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_RECENT, NULL, 0, recentPath)))
            {
                std::string searchPattern = std::string(recentPath) + "\\*Somalia*";
                WIN32_FIND_DATAA fd;
                HANDLE hFind = FindFirstFileA(searchPattern.c_str(), &fd);
                if (hFind != INVALID_HANDLE_VALUE)
                {
                    do {
                        std::string item = std::string(recentPath) + "\\" + fd.cFileName;
                        DeleteFileA(item.c_str());
                    } while (FindNextFileA(hFind, &fd));
                    FindClose(hFind);
                }
            }

            s_BypassStatus = "Logs e rastros residuais foram limpos com sucesso!";
            s_BypassTime = GetTickCount64();
        };

        // Status ou Dica acima dos botões
        if (!s_BypassStatus.empty() && (GetTickCount64() - s_BypassTime < 5000))
        {
            draw->AddText(s_FontSmall, 12.0f, ImVec2(48, card2Y + 115), IM_COL32(50, 220, 100, 255), s_BypassStatus.c_str());
        }
        else
        {
            draw->AddText(s_FontSmall, 12.0f, ImVec2(48, card2Y + 115), IM_COL32(105, 105, 105, 255),
                "Dica: O botao Bypass apaga todos os logs e arquivos residuais do menu e loader.");
        }

        // 4. Botões: "INJETAR" e "LIMPAR LOGS (BYPASS)"
        {
            float totalW = card2W - 48.0f;
            float btnH = 46.0f;
            float btnInjectW = 275.0f;
            float btnBypassW = totalW - btnInjectW - 10.0f;
            float btnX = 24.0f + 24.0f;
            float btnY = card2Y + 145.0f;

            const char* btnLabel;
            if (isGameRunning)
                btnLabel = "INJETAR CHEAT##hero_inject";
            else if (isWaiting)
                btnLabel = "CANCELAR AGUARDO##hero_inject";
            else
                btnLabel = "INJETAR (AUTO)##hero_inject";

            if (SomaliaButton(btnLabel, ImVec2(btnInjectW, btnH), ImVec2(btnX, btnY)))
            {
                std::string asiPath = GetAsiPath();
                if (isWaiting)
                {
                    Injector::StopAutoInjectThread();
                }
                else if (isGameRunning)
                {
                    std::string err;
                    if (!Injector::InjectGame(asiPath, err))
                    {
                        char fullAsi[MAX_PATH];
                        GetFullPathNameA(asiPath.c_str(), MAX_PATH, fullAsi, NULL);
                        Injector::InjectGame(fullAsi, err);
                    }
                }
                else
                {
                    char fullAsi[MAX_PATH];
                    GetFullPathNameA(asiPath.c_str(), MAX_PATH, fullAsi, NULL);
                    Injector::StartAutoInjectThread(fullAsi);
                }
            }

            if (SomaliaButton("BYPASS / AUTODESTRUIR##hero_bypass", ImVec2(btnBypassW, btnH), ImVec2(btnX + btnInjectW + 10.0f, btnY)))
            {
                s_ShowSelfDestructModal = true;
            }
        }

        // 5. Se o modal da engrenagem estiver aberto, renderiza por cima de tudo
        if (s_ShowGtaModal)
        {
            RenderGtaModal(winW, winH);
        }

        // 6. Se o modal de autodestruição estiver aberto, renderiza por cima de tudo
        if (s_ShowSelfDestructModal)
        {
            RenderSelfDestructModal(winW, winH);
        }
    }

    // ==========================================================
    //  RENDER LOOP PRINCIPAL COM PARTÍCULAS E FADE IN
    // ==========================================================
    void Render()
    {
        ImGuiIO& io = ImGui::GetIO();
        float dt = io.DeltaTime;
        if (dt <= 0.0f) dt = 0.016f;

        // Fade in suave
        s_OpenAlpha = ImLerp(s_OpenAlpha, 1.0f, 10.0f * dt);

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                 ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar;

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ColBgContent);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, s_OpenAlpha);

        if (s_FontBody)
            ImGui::PushFont(s_FontBody);

        ImGui::Begin("##SomaliaLoaderMain", nullptr, flags);
        {
            // Partículas de fundo ativas em todas as telas
            RenderParticles();

            if (s_CurrentScreen == Screen::Login || s_CurrentScreen == Screen::Register)
                RenderLoginScreen();
            else if (s_CurrentScreen == Screen::Dashboard)
                RenderDashboardScreen();
        }
        ImGui::End();

        if (s_FontBody)
            ImGui::PopFont();

        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor();
    }
}
