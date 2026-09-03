#include "Menu.h"
#include "../Render/ImGui/imgui_internal.h"
#include "../Config/Config.h"
#include "../Config/ConfigManager.h"
#include "../Render/TextureLoader.h"
#include "Fonts/bytearray.h"
#include "Theme.h"
#include "../Core/Logger.h"
#include "../Core/Main.h"
#include "../Features/AntiAim/AntiAim.h"
#include <vector>
#include <stdlib.h>
#include <fstream>
#include <sstream>

// Fontes compartilhadas com os widgets customizados do ImGui (Rendertab / MenuChild)
ImFont* tab_title = nullptr;
ImFont* font_icon = nullptr;
ImFont* poppins = nullptr;

// Texturas D3D9 em memória
static IDirect3DTexture9* s_pLogoOne   = nullptr;
static IDirect3DTexture9* s_pLogoTwo   = nullptr;

static float s_OpenAlpha = 0.0f;

namespace Menu
{
    void InvalidateDeviceObjects()
    {
        if (s_pLogoOne)  { s_pLogoOne->Release();  s_pLogoOne = nullptr; }
        if (s_pLogoTwo)  { s_pLogoTwo->Release();  s_pLogoTwo = nullptr; }
    }

    void CreateDeviceObjects(IDirect3DDevice9* pDevice)
    {
        if (!pDevice) return;

        if (!s_pLogoOne)
            s_pLogoOne = TextureLoader::CreateTextureFromMemory(pDevice, logo_one, sizeof(logo_one));

        if (!s_pLogoTwo)
            s_pLogoTwo = TextureLoader::CreateTextureFromMemory(pDevice, logo_two, sizeof(logo_two));
    }

    void Initialize(IDirect3DDevice9* pDevice)
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

        io.Fonts->AddFontFromMemoryTTF(poppin_font, sizeof(poppin_font), 16.0f, &font_config, ranges);
        font_icon = io.Fonts->AddFontFromMemoryTTF(icon_font, sizeof(icon_font), 25.0f, &font_config, ranges);
        poppins   = io.Fonts->AddFontFromMemoryTTF(poppin_font, sizeof(poppin_font), 25.0f, &font_config, ranges);

        unsigned char* pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        CreateDeviceObjects(pDevice);
    }

    void Shutdown()
    {
        InvalidateDeviceObjects();
    }

    static void Particles()
    {
        if (!g_MenuState.misc.particles)
            return;

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
            {
                particle_pos[i] = ImVec2(0, 0);
            }
            s_InitParticles = true;
        }

        for (int i = 1; i < 50; i++)
        {
            if (particle_pos[i].x == 0 || particle_pos[i].y == 0)
            {
                particle_pos[i].x = (float)(rand() % (int)screen_size.x + 1);
                particle_pos[i].y = 15.0f;
                particle_speed[i] = (float)(1 + rand() % 25);
                particle_radius[i] = (float)(rand() % 4);

                particle_target_pos[i].x = (float)(rand() % (int)screen_size.x);
                particle_target_pos[i].y = screen_size.y * 2.0f;
            }

            particle_pos[i] = ImLerp(particle_pos[i], particle_target_pos[i], ImGui::GetIO().DeltaTime * (particle_speed[i] / 60.0f));

            if (particle_pos[i].y > screen_size.y)
            {
                particle_pos[i].x = 0;
                particle_pos[i].y = 0;
            }

            ImGui::GetWindowDrawList()->AddCircleFilled(particle_pos[i], particle_radius[i], ImColor(137, 207, 240, int(140 * s_OpenAlpha)));
        }
    }

    static void Decoration()
    {
        auto draw = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetWindowPos();

        // 1. Fundo da Sidebar Esquerda (Largura 161px, Altura 535px)
        draw->AddRectFilled(ImVec2(pos.x, pos.y), ImVec2(pos.x + 161, pos.y + 535), ImColor(32, 32, 32, int(255 * s_OpenAlpha)), 10.0f, ImDrawCornerFlags_Left);
        draw->AddRect(ImVec2(pos.x, pos.y), ImVec2(pos.x + 161, pos.y + 535), ImColor(50, 50, 50, int(255 * s_OpenAlpha)), 10.0f, ImDrawCornerFlags_Left, 1.0f);

        // 2. Fundo da Área de Conteúdo Direita (Largura 677px, Altura 535px)
        draw->AddRectFilled(ImVec2(pos.x + 160, pos.y), ImVec2(pos.x + 838, pos.y + 535), ImColor(26, 26, 26, int(255 * s_OpenAlpha)), 10.0f, ImDrawCornerFlags_Right);
        draw->AddRect(ImVec2(pos.x + 160, pos.y), ImVec2(pos.x + 838, pos.y + 535), ImColor(50, 50, 50, int(255 * s_OpenAlpha)), 10.0f, ImDrawCornerFlags_Right, 1.0f);

        // 3. Título no topo da Sidebar: "Somalia"
        if (poppins)
        {
            ImVec2 sz = poppins->CalcTextSizeA(24.0f, FLT_MAX, 0.0f, "Somalia");
            float tx = pos.x + (161.0f - sz.x) * 0.5f;
            draw->AddText(poppins, 24.0f, ImVec2(tx + 1.0f, pos.y + 26.0f + 1.0f), IM_COL32(0, 0, 0, int(180 * s_OpenAlpha)), "Somalia");
            draw->AddText(poppins, 24.0f, ImVec2(tx, pos.y + 26.0f), ImColor(Theme::AccentColor.x, Theme::AccentColor.y, Theme::AccentColor.z, s_OpenAlpha), "Somalia");
        }
        else
        {
            ImVec2 sz = ImGui::CalcTextSize("Somalia");
            float tx = pos.x + (161.0f - sz.x) * 0.5f;
            draw->AddText(ImVec2(tx, pos.y + 26.0f), ImColor(Theme::AccentColor.x, Theme::AccentColor.y, Theme::AccentColor.z, s_OpenAlpha), "Somalia");
        }
    }

    static std::string s_AccountUser = "Somalia";
    static std::string s_AccountPlan = "VIP: Ilimitado";

    static void LoadAccountDetails()
    {
        static bool s_Loaded = false;
        if (s_Loaded) return;
        s_Loaded = true;

        std::ifstream f("somalia_client.json");
        if (!f.is_open()) return;

        std::stringstream ss;
        ss << f.rdbuf();
        std::string json = ss.str();

        auto extractField = [](const std::string& str, const std::string& key) -> std::string {
            std::string search = "\"" + key + "\"";
            size_t pos = str.find(search);
            if (pos == std::string::npos) return "";
            size_t colon = str.find(':', pos + search.length());
            if (colon == std::string::npos) return "";
            size_t start = str.find('\"', colon + 1);
            if (start == std::string::npos) return "";
            size_t end = str.find('\"', start + 1);
            if (end == std::string::npos) return "";
            return str.substr(start + 1, end - start - 1);
        };

        std::string u = extractField(json, "last_username");
        if (!u.empty()) s_AccountUser = u;

        std::string sub = extractField(json, "user_subscription");
        std::string days = extractField(json, "user_days_left");
        if (!days.empty())
        {
            if (days.find("Ilimitad") != std::string::npos || days.find("Vital") != std::string::npos || days.find("Life") != std::string::npos)
                s_AccountPlan = "Vitalicio";
            else
                s_AccountPlan = days;
        }
        else if (!sub.empty())
        {
            if (sub.find("Life") != std::string::npos || sub.find("Vital") != std::string::npos)
                s_AccountPlan = "Vitalicio";
            else
                s_AccountPlan = sub;
        }
        else
        {
            s_AccountPlan = "Vitalicio";
        }
    }

    static void user_info()
    {
        LoadAccountDetails();

        auto draw = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetWindowPos();

        float boxMinX = pos.x + 9.0f;
        float boxMaxX = pos.x + 154.0f;
        float boxMinY = pos.y + 486.0f;
        float boxMaxY = pos.y + 524.0f;

        draw->AddRectFilled(ImVec2(boxMinX, boxMinY), ImVec2(boxMaxX, boxMaxY), ImColor(41, 41, 41, int(255 * s_OpenAlpha)), 5.0f, ImDrawCornerFlags_All);
        draw->AddRect(ImVec2(boxMinX, boxMinY), ImVec2(boxMaxX, boxMaxY), ImColor(50, 50, 50, int(255 * s_OpenAlpha)), 5.0f, ImDrawCornerFlags_All, 1.0f);

        // Avatar Moderno e Minimalista (Sem foto de anime)
        float avX = pos.x + 26.0f;
        float avY = pos.y + 505.0f;
        draw->AddCircleFilled(ImVec2(avX, avY), 12.0f, IM_COL32(24, 28, 38, int(255 * s_OpenAlpha)));
        draw->AddCircle(ImVec2(avX, avY), 12.0f, ImColor(Theme::AccentColor.x, Theme::AccentColor.y, Theme::AccentColor.z, s_OpenAlpha), 32, 1.2f);

        char initial[2] = { (char)toupper(s_AccountUser.empty() ? 'S' : s_AccountUser[0]), '\0' };
        draw->AddText(ImVec2(avX - 4.0f, avY - 7.0f), ImColor(Theme::AccentColor.x, Theme::AccentColor.y, Theme::AccentColor.z, s_OpenAlpha), initial);

        // Textos com Clipping rigoroso para NUNCA vazar da caixa
        draw->PushClipRect(ImVec2(pos.x + 44, boxMinY + 1), ImVec2(boxMaxX - 3, boxMaxY - 1), true);

        // Nome da conta
        std::string displayUser = s_AccountUser;
        if (displayUser.length() > 12) displayUser = displayUser.substr(0, 11) + "..";
        draw->AddColoredText(ImVec2(pos.x + 44, pos.y + 488), ImColor(105, 105, 105, int(255 * s_OpenAlpha)), ImColor(255, 255, 255, int(255 * s_OpenAlpha)), displayUser.c_str());

        // Validade / Plano formatado
        std::string displayPlan = s_AccountPlan;
        if (displayPlan.length() > 14) displayPlan = displayPlan.substr(0, 13) + "..";
        draw->AddColoredText(ImVec2(pos.x + 44, pos.y + 504), ImColor(105, 105, 105, int(255 * s_OpenAlpha)), ImColor(int(Theme::AccentColor.x * 255), int(Theme::AccentColor.y * 255), int(Theme::AccentColor.z * 255), int(255 * s_OpenAlpha)), displayPlan.c_str());

        draw->PopClipRect();
    }

    // ─────────────────────────────────────────────────────────────
    // ABA 0: LEGIT BOT (Preservado 100% Intacto)
    // ─────────────────────────────────────────────────────────────
    static void RenderLegitBotTab()
    {
        // 1. Seleção de Categoria de Arma (Target Weapon)
        ImGui::SetCursorPos(ImVec2(169, 38));
        ImGui::MenuChild("Target Weapon", ImVec2(320, 57));
        {
            const char* type[] = { "Auto Snipers (Sniper, Country)", "Pistols (Desert Eagle)", "Rifles (M4, AK-47)", "Shotguns (Combat, Sawnoff)" };
            ImGui::Combo("##snipers", &g_MenuState.legitBot.autoSnipersType, type, IM_ARRAYSIZE(type));
        }
        ImGui::EndChild();

        // Obtém a referência do perfil da arma selecionada para edição
        int weaponIdx = g_MenuState.legitBot.autoSnipersType;
        if (weaponIdx < 0 || weaponIdx >= 4) weaponIdx = 0;
        auto& w = g_MenuState.legitBot.weapons[weaponIdx];

        // 2. Configurações Gerais da Arma
        ImGui::SetCursorPos(ImVec2(169, 105));
        ImGui::MenuChild("General", ImVec2(320, 275));
        {
            ImGui::Spacing();
            ImGui::Checkbox("Master Enable Legit Bot", &g_MenuState.legitBot.enabled);
            ImGui::Checkbox("Enable for this Weapon", &w.enabled);
            ImGui::SliderFloat("FOV", &w.fov, 1.0f, 100.0f, "%.0f%%");
            ImGui::SliderFloat("Smooth", &w.smooth, 1.0f, 30.0f, "%.1f");
            ImGui::SliderFloat("Max Distance", &w.maxDistance, 10.0f, 350.0f, "%.0fm");
            ImGui::Checkbox("Target Indicator [ O ]", &w.drawTargetMarker);
            ImGui::Checkbox("Draw Tracer Line", &w.drawTracer);
        }
        ImGui::EndChild();

        // 3. Exploits e Opções Auxiliares
        ImGui::SetCursorPos(ImVec2(169, 390));
        ImGui::MenuChild("Exploits", ImVec2(320, 130));
        {
            ImGui::Spacing();
            ImGui::Checkbox("Silent Aim", &g_MenuState.legitBot.silentAim);
            ImGui::Checkbox("Lag Peek", &g_MenuState.legitBot.exploitLagPeek);
            ImGui::Checkbox("Hide Shots", &g_MenuState.legitBot.exploitHideShots);
            ImGui::Checkbox("Double Tap", &g_MenuState.legitBot.exploitDoubleTap);
        }
        ImGui::EndChild();

        // 4. Seleção de Alvo e Prioridade
        ImGui::SetCursorPos(ImVec2(505, 38));
        ImGui::MenuChild("Target Selection", ImVec2(320, 235));
        {
            const char* priorities[] = { "Closest to Crosshair", "Closest Distance (3D)", "Lowest Health" };
            const char* bones[] = { "Head", "Neck", "Chest", "Pelvis" };
            const char* activations[] = { "Always", "While Aiming (RMB)", "While Shooting (LMB)", "Aim + Shoot" };

            ImGui::Spacing();
            ImGui::Combo("Target Priority", &w.priority, priorities, IM_ARRAYSIZE(priorities));
            ImGui::Combo("Target Bone", &w.bone, bones, IM_ARRAYSIZE(bones));
            ImGui::Combo("Activation", &w.activationMode, activations, IM_ARRAYSIZE(activations));
            ImGui::Spacing();
            ImGui::Checkbox("Draw Smooth Vector", &w.drawSmoothVector);
            ImGui::Checkbox("Prefer body aim", &g_MenuState.legitBot.preferBodyAim);
        }
        ImGui::EndChild();

        // 5. Filtros de Alvo
        ImGui::SetCursorPos(ImVec2(505, 283));
        ImGui::MenuChild("Target Filters", ImVec2(320, 237));
        {
            ImGui::Spacing();
            ImGui::Checkbox("Ignore Dead Players", &w.ignoreDead);
            ImGui::Checkbox("Team Check (Amigos)", &w.teamCheck);
            ImGui::Checkbox("Visibility Check (Paredes)", &w.visibilityCheck);
            ImGui::Checkbox("Ignore limbs when moving", &g_MenuState.legitBot.ignoreLimbs);
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextColored(Theme::TextMuted, "Configuracao independente por arma.");
            ImGui::TextColored(Theme::AccentColor, "Perfil atual: %s",
                weaponIdx == 0 ? "Snipers" : (weaponIdx == 1 ? "Pistols" : (weaponIdx == 2 ? "Rifles" : "Shotguns")));
        }
        ImGui::EndChild();
    }

    // ─────────────────────────────────────────────────────────────
    // ABA 1: RAGEBOT — TELA DEDICADA E INDEPENDENTE
    // ─────────────────────────────────────────────────────────────
    static void RenderRageBotTab()
    {
        // 1. Seleção de Categoria de Arma (Target Weapon Profile)
        ImGui::SetCursorPos(ImVec2(169, 38));
        ImGui::MenuChild("Target Weapon Profile", ImVec2(320, 57));
        {
            const char* type[] = { "Auto Snipers (Sniper, Country)", "Pistols (Desert Eagle)", "Rifles (M4, AK-47)", "Shotguns (Combat, Sawnoff)" };
            ImGui::Combo("##ragewp", &g_MenuState.rageBot.currentWeaponGroup, type, IM_ARRAYSIZE(type));
        }
        ImGui::EndChild();

        int weaponIdx = g_MenuState.rageBot.currentWeaponGroup;
        if (weaponIdx < 0 || weaponIdx >= 4) weaponIdx = 0;
        auto& rw = g_MenuState.rageBot.weapons[weaponIdx];

        // 2. Painel Geral do Ragebot
        ImGui::SetCursorPos(ImVec2(169, 105));
        ImGui::MenuChild("Ragebot General", ImVec2(320, 415));
        {
            ImGui::Spacing();
            ImGui::Checkbox("Master Enable Ragebot", &g_MenuState.rageBot.enabled);
            ImGui::Checkbox("Enable for this Weapon", &rw.enabled);
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::SliderFloat("FOV", &rw.fov, 1.0f, 100.0f, "%.0f%%");
            ImGui::SliderFloat("Aggressiveness", &rw.aggressiveness, 0.0f, 100.0f, "%.0f%%");
            ImGui::SliderFloat("Max Distance", &rw.maxDistance, 10.0f, 500.0f, "%.0fm");

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Checkbox("Target Indicator [ RAGE ]", &rw.targetIndicator);
            ImGui::Checkbox("Draw Rage FOV Circle", &rw.drawFov);
            ImGui::Checkbox("Debug Convergence Vector", &rw.debugVector);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextColored(Theme::TextMuted, "Escala de Agressividade:");
            const char* aggrDesc = "0% (Minimo)";
            if (rw.aggressiveness >= 95.0f)      aggrDesc = "100% (Maximo / Imediato)";
            else if (rw.aggressiveness >= 70.0f) aggrDesc = "75% (Alto)";
            else if (rw.aggressiveness >= 40.0f) aggrDesc = "50% (Medio)";
            else if (rw.aggressiveness > 10.0f)  aggrDesc = "25% (Baixo)";
            ImGui::TextColored(Theme::AccentColor, "%s", aggrDesc);
        }
        ImGui::EndChild();

        // 3. Seleção de Alvo e Ativação do Ragebot
        ImGui::SetCursorPos(ImVec2(505, 38));
        ImGui::MenuChild("Target Selection & Activation", ImVec2(320, 235));
        {
            const char* activations[] = { "Always", "While Aiming (RMB)", "While Shooting (LMB)", "Aim + Shoot" };
            const char* bones[] = { "HEAD (Osso 8)", "NECK (Osso 5)", "CHEST (Osso 4)", "PELVIS (Osso 2)" };
            const char* priorities[] = { "Closest to Crosshair", "Closest Distance (3D)", "Lowest Health" };

            ImGui::Spacing();
            ImGui::Combo("Activation", &rw.activationMode, activations, IM_ARRAYSIZE(activations));
            ImGui::Combo("Target Bone", &rw.bone, bones, IM_ARRAYSIZE(bones));
            ImGui::Combo("Target Priority", &rw.priority, priorities, IM_ARRAYSIZE(priorities));

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextColored(Theme::AccentColor, "Padrao Rage: HEAD prioritario");
            ImGui::TextColored(Theme::TextMuted, "Isolado do bone do Legit Bot.");
        }
        ImGui::EndChild();

        // 4. Filtros de Alvo do Ragebot
        ImGui::SetCursorPos(ImVec2(505, 283));
        ImGui::MenuChild("Target Filters & Status", ImVec2(320, 237));
        {
            ImGui::Spacing();
            ImGui::Checkbox("Ignore Dead Players", &rw.ignoreDead);
            ImGui::Checkbox("Team Check (Amigos)", &rw.teamCheck);
            ImGui::Checkbox("Visibility Check (Paredes)", &rw.visibilityCheck);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextColored(Theme::TextMuted, "Configuracao independente por arma.");
            ImGui::TextColored(Theme::AccentColor, "Perfil Rage atual: %s",
                weaponIdx == 0 ? "Snipers" : (weaponIdx == 1 ? "Pistols" : (weaponIdx == 2 ? "Rifles" : "Shotguns")));
            ImGui::TextColored(Theme::TextMuted, "Estado: %s",
                (g_MenuState.rageBot.enabled && rw.enabled) ? "ARMADO (Ativo)" : "DESATIVADO");
        }
        ImGui::EndChild();
    }

    // ─────────────────────────────────────────────────────────────
    // ABA 2: SILENT AIM
    // ─────────────────────────────────────────────────────────────
    static void RenderSilentAimTab()
    {
        int weaponIdx = g_MenuState.silentAim.currentWeaponGroup;
        if (weaponIdx < 0 || weaponIdx >= 4) weaponIdx = 0;
        SilentWeaponConfig& sw = g_MenuState.silentAim.weapons[weaponIdx];

        // 1. Target Weapon Profile
        ImGui::SetCursorPos(ImVec2(169, 38));
        ImGui::MenuChild("Target Weapon Profile", ImVec2(320, 57));
        {
            const char* weaponGroups[] = {
                "Auto Snipers (Sniper, Country)",
                "Pistols (Desert Eagle)",
                "Rifles (M4, AK-47)",
                "Shotguns (Combat, Sawnoff)"
            };
            ImGui::Spacing();
            ImGui::Combo("##SilentWeaponGroup", &g_MenuState.silentAim.currentWeaponGroup, weaponGroups, IM_ARRAYSIZE(weaponGroups));
        }
        ImGui::EndChild();

        // 2. Silent Aim Configuration
        ImGui::SetCursorPos(ImVec2(169, 107));
        ImGui::MenuChild("Silent Aim Configuration", ImVec2(320, 413));
        {
            ImGui::Spacing();
            ImGui::Checkbox("Master Enable Silent Aim", &g_MenuState.silentAim.enabled);
            ImGui::Checkbox("Enable for this Weapon", &sw.enabled);
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::SliderFloat("Silent FOV", &sw.fov, 1.0f, 100.0f, "%.1f%%");
            ImGui::SliderInt("Hit Chance", &sw.hitChance, 1, 100, "%d%%");
            ImGui::SliderFloat("Max Distance", &sw.maxDistance, 10.0f, 500.0f, "%.0f m");

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Checkbox("Silent Target Marker [ O ]", &sw.targetIndicator);
            ImGui::Checkbox("Draw Silent FOV Circle", &sw.drawFov);
            ImGui::Checkbox("Draw Tracer Line", &sw.drawTracer);
        }
        ImGui::EndChild();

        // 3. Target Selection & Bones
        ImGui::SetCursorPos(ImVec2(505, 38));
        ImGui::MenuChild("Target Selection & Bones", ImVec2(320, 233));
        {
            const char* priorities[] = { "Closest to Crosshair", "Closest Distance 3D", "Lowest Health" };
            const char* bones[] = { "Head (Osso 8)", "Neck (Osso 5)", "Chest (Osso 4)", "Pelvis (Osso 2)", "Random Hitbox" };
            const char* activations[] = { "Always", "While Aiming (RMB)", "While Shooting (LMB) [Silent]", "Aim + Shoot" };

            ImGui::Spacing();
            ImGui::Combo("Priority", &sw.priority, priorities, IM_ARRAYSIZE(priorities));
            ImGui::Combo("Target Bone", &sw.bone, bones, IM_ARRAYSIZE(bones));
            ImGui::Combo("Activation", &sw.activationMode, activations, IM_ARRAYSIZE(activations));
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextColored(Theme::AccentColor, "O Silent Aim crava os disparos no alvo");
            ImGui::TextColored(Theme::TextMuted, "mantendo controle total de hitbox e FOV.");
        }
        ImGui::EndChild();

        // 4. Target Filters & Status
        ImGui::SetCursorPos(ImVec2(505, 283));
        ImGui::MenuChild("Target Filters & Status", ImVec2(320, 237));
        {
            ImGui::Spacing();
            ImGui::Checkbox("Ignore Dead Players", &sw.ignoreDead);
            ImGui::Checkbox("Team Check (Amigos)", &sw.teamCheck);
            ImGui::Checkbox("Visibility Check (Paredes)", &sw.visibilityCheck);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextColored(Theme::TextMuted, "Configuracao independente por arma.");
            ImGui::TextColored(Theme::AccentColor, "Perfil Silent atual: %s",
                weaponIdx == 0 ? "Snipers" : (weaponIdx == 1 ? "Pistols" : (weaponIdx == 2 ? "Rifles" : "Shotguns")));
            ImGui::TextColored(Theme::TextMuted, "Estado: %s",
                (g_MenuState.silentAim.enabled && sw.enabled) ? "ARMADO (Ativo)" : "DESATIVADO");
            ImGui::TextColored(Theme::AccentColor, "Hit Chance: %d%%", sw.hitChance);
        }
        ImGui::EndChild();
    }

    // ─────────────────────────────────────────────────────────────
    // ABA 3: VISUALS — PLAYERS (ESP COMPLETO + ANTI-AIM NO CANTO)
    // ─────────────────────────────────────────────────────────────
    static void RenderPlayersVisualsTab()
    {
        ImGui::SetCursorPos(ImVec2(169, 38));
        ImGui::MenuChild("Player ESP", ImVec2(320, 482));
        {
            const char* boxTypes[] = { "2D Box Outline", "Corner Box" };
            const char* origins[]  = { "Bottom Screen", "Center Screen" };

            ImGui::Spacing();
            ImGui::Checkbox("Master Enable ESP", &g_MenuState.visuals.enableESP);
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Checkbox("Box 2D", &g_MenuState.visuals.boxESP);
            ImGui::Combo("Box Style", &g_MenuState.visuals.boxType, boxTypes, IM_ARRAYSIZE(boxTypes));
            ImGui::Spacing();

            ImGui::Checkbox("Player Names & ID", &g_MenuState.visuals.nameESP);
            ImGui::Checkbox("Health Bar", &g_MenuState.visuals.healthESP);
            ImGui::Checkbox("Armor Bar", &g_MenuState.visuals.armorESP);
            ImGui::Checkbox("Distance Tag", &g_MenuState.visuals.distanceESP);
            ImGui::Spacing();

            ImGui::Checkbox("Snaplines", &g_MenuState.visuals.snaplines);
            ImGui::Combo("Snapline Origin", &g_MenuState.visuals.snaplineOrigin, origins, IM_ARRAYSIZE(origins));
        }
        ImGui::EndChild();

        // Filtros ESP no canto superior direito
        ImGui::SetCursorPos(ImVec2(505, 38));
        ImGui::MenuChild("ESP Filters & Limits", ImVec2(320, 155));
        {
            ImGui::Spacing();
            ImGui::SliderInt("Max Render Distance", &g_MenuState.visuals.maxDistance, 20, 500, "%d m");
            ImGui::Checkbox("Enemy Only", &g_MenuState.visuals.enemyOnly);
            ImGui::Checkbox("Skeleton / Bones", &g_MenuState.visuals.bonesESP);
        }
        ImGui::EndChild();

        // Controles de Anti-Aim no cantinho inferior direito da tela de Players
        ImGui::SetCursorPos(ImVec2(505, 205));
        ImGui::MenuChild("Anti-Aim & Angles", ImVec2(320, 315));
        {
            const char* pitchList[] = { "Disabled", "Emotion (-89°)", "Up (89°)", "Zero (0°)" };
            const char* yawList[]   = { "Disabled", "Backward (180°)", "Spinbot", "Jitter", "Random" };

            ImGui::Spacing();
            ImGui::Checkbox("Enable Anti-Aim", &g_MenuState.antiAim.enabled);
            ImGui::Combo("Pitch Mode", &g_MenuState.antiAim.pitchMode, pitchList, IM_ARRAYSIZE(pitchList));
            ImGui::Combo("Yaw Mode", &g_MenuState.antiAim.yawMode, yawList, IM_ARRAYSIZE(yawList));
            ImGui::SliderInt("Spin Speed", &g_MenuState.antiAim.spinSpeed, 1, 50, "%d");
            ImGui::Checkbox("Desync Angles", &g_MenuState.antiAim.desync);
            ImGui::Checkbox("Invertebred", &g_MenuState.antiAim.invertebred);
            ImGui::Spacing();
            ImGui::Checkbox("Enable Fake Lag", &g_MenuState.antiAim.fakeLag);
            ImGui::SliderInt("Choked Ticks", &g_MenuState.antiAim.fakeLagLimit, 1, 16, "%d ticks");
        }
        ImGui::EndChild();
    }

    // ─────────────────────────────────────────────────────────────
    // ABA 4: VISUALS — WORLD
    // ─────────────────────────────────────────────────────────────
    static void RenderWorldVisualsTab()
    {
        ImGui::SetCursorPos(ImVec2(169, 38));
        ImGui::MenuChild("World Entities ESP", ImVec2(320, 240));
        {
            ImGui::Spacing();
            ImGui::Checkbox("Vehicles ESP", &g_MenuState.visuals.vehicleESP);
            ImGui::Checkbox("Pickups & Items ESP", &g_MenuState.visuals.pickupESP);
            ImGui::Checkbox("3D Text Labels ESP", &g_MenuState.visuals.objectESP);
            ImGui::Spacing();
            ImGui::TextColored(Theme::AccentColor, "Entidades de mundo ativas e renderizadas.");
        }
        ImGui::EndChild();

        ImGui::SetCursorPos(ImVec2(169, 317));
        ImGui::MenuChild("Atmosphere & Weather", ImVec2(320, 203));
        {
            const char* weathers[] = { "Sunny / Clear", "Foggy", "Rainy / Storm", "Night Extra Dark", "Sunset Red" };
            ImGui::Spacing();
            ImGui::Checkbox("Night Mode Effect", &g_MenuState.visuals.nightMode);
            ImGui::Checkbox("Custom Weather", &g_MenuState.visuals.weatherChanger);
            ImGui::Combo("Weather ID", &g_MenuState.visuals.weatherID, weathers, IM_ARRAYSIZE(weathers));
        }
        ImGui::EndChild();

        ImGui::SetCursorPos(ImVec2(505, 38));
        ImGui::MenuChild("Time & Lighting", ImVec2(320, 482));
        {
            ImGui::Spacing();
            ImGui::Checkbox("Custom Game Hour", &g_MenuState.visuals.timeChanger);
            ImGui::SliderInt("Clock Hour", &g_MenuState.visuals.timeHour, 0, 23, "%d:00");
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextColored(Theme::AccentColor, "Controle de Iluminacao Ativo");
            ImGui::TextColored(Theme::TextMuted, "Sincronizado diretamente no motor D3D9.");
        }
        ImGui::EndChild();
    }

    // ─────────────────────────────────────────────────────────────
    // ABA 5: VISUALS — VIEW & CAMERA
    // ─────────────────────────────────────────────────────────────
    static void RenderViewVisualsTab()
    {
        ImGui::SetCursorPos(ImVec2(169, 38));
        ImGui::MenuChild("FOV & Crosshair", ImVec2(320, 482));
        {
            ImGui::Spacing();
            ImGui::Checkbox("Draw Aimbot FOV Circle", &g_MenuState.visuals.drawFOVCircle);
            ImGui::SliderInt("FOV Circle Radius", &g_MenuState.visuals.fovCircleRadius, 10, 150, "%d px");
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Checkbox("Custom Screen Crosshair", &g_MenuState.visuals.customCrosshair);
            ImGui::Checkbox("Hitmarker on Damage", &g_MenuState.visuals.hitmarker);
            ImGui::Checkbox("Damage Informer", &g_MenuState.visuals.damageInformer);
        }
        ImGui::EndChild();

        ImGui::SetCursorPos(ImVec2(505, 38));
        ImGui::MenuChild("Display Indicators", ImVec2(320, 482));
        {
            ImGui::Spacing();
            ImGui::TextColored(Theme::AccentColor, "FOV Radius Atual: %d px", g_MenuState.visuals.fovCircleRadius * 4);
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Text("O circulo de FOV e a Crosshair");
            ImGui::Text("sao renderizados localmente.");
            ImGui::Spacing();
            ImGui::TextColored(Theme::AccentColor, "Hitmarker e Damage Informer integrados");
            ImGui::TextColored(Theme::TextMuted, "ao motor de dano em tempo real.");
        }
        ImGui::EndChild();
    }

    // ─────────────────────────────────────────────────────────────
    // ABA 6: MAIN — PLAYER MODS
    // ─────────────────────────────────────────────────────────────
    static void RenderPlayerModsTab()
    {
        ImGui::SetCursorPos(ImVec2(169, 38));
        ImGui::MenuChild("Local Player Attributes", ImVec2(320, 482));
        {
            ImGui::Spacing();
            ImGui::Checkbox("Godmode (Local Proofs)", &g_MenuState.player.godmode);
            ImGui::Checkbox("Infinite Ammo", &g_MenuState.player.infAmmo);
            ImGui::Checkbox("Infinite Stamina", &g_MenuState.player.infStamina);
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Checkbox("Fast Sprint", &g_MenuState.player.fastRun);
            ImGui::Checkbox("Mega Jump", &g_MenuState.player.megaJump);
            ImGui::Checkbox("Anti-Stun", &g_MenuState.player.antiStun);
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Checkbox("Invertebred", &g_MenuState.antiAim.invertebred);
        }
        ImGui::EndChild();

        ImGui::SetCursorPos(ImVec2(505, 38));
        ImGui::MenuChild("Combat Helpers", ImVec2(320, 482));
        {
            ImGui::Spacing();
            ImGui::Checkbox("Fast Weapon Reload", &g_MenuState.player.fastReload);
            ImGui::Checkbox("Automatic C-Bug Helper", &g_MenuState.player.autoCBug);
            ImGui::Checkbox("No Spread", &g_MenuState.player.noSpread);
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextColored(Theme::AccentColor, "Assistencias de combate ativas.");
            ImGui::TextColored(Theme::TextMuted, "C-Bug automatico com Deagle/armas.");
            ImGui::TextColored(Theme::TextMuted, "Recarga instantanea sem delay.");
        }
        ImGui::EndChild();
    }

    // ─────────────────────────────────────────────────────────────
    // ABA 7: INVENTORY — VEHICLE MODS
    // ─────────────────────────────────────────────────────────────
    static void RenderVehicleModsTab()
    {
        ImGui::SetCursorPos(ImVec2(169, 38));
        ImGui::MenuChild("Vehicle Physics", ImVec2(320, 482));
        {
            ImGui::Spacing();
            ImGui::Checkbox("Engine Always On", &g_MenuState.vehicle.engineAlwaysOn);
            ImGui::Checkbox("Car Godmode", &g_MenuState.vehicle.carGodmode);
            ImGui::SliderInt("Speed Multiplier (Shift)", &g_MenuState.vehicle.speedMultiplier, 1, 10, "%dx");
            ImGui::Checkbox("Auto Flip Vehicle", &g_MenuState.vehicle.autoFlip);
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Checkbox("Fly Car Mode", &g_MenuState.vehicle.flyCar);
        }
        ImGui::EndChild();

        ImGui::SetCursorPos(ImVec2(505, 38));
        ImGui::MenuChild("Vehicle Handling", ImVec2(320, 482));
        {
            ImGui::Spacing();
            ImGui::Checkbox("Instant Vehicle Repair (Tecla R)", &g_MenuState.vehicle.instantRepair);
            ImGui::Checkbox("No Bike Fall", &g_MenuState.vehicle.noBikeFall);
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextColored(Theme::AccentColor, "Modificadores Locais Ativos");
            ImGui::TextColored(Theme::TextMuted, "Pressione 'R' no veiculo para reparar.");
            ImGui::TextColored(Theme::TextMuted, "Segure Shift para acelerar.");
        }
        ImGui::EndChild();
    }

    // ─────────────────────────────────────────────────────────────
    // ABA 8: C-SLIDE & MOVEMENT MECHANICS
    // ─────────────────────────────────────────────────────────────
    static void RenderSlideTab()
    {
        // CARD 1: C-Slide & Mecânicas Gerais
        ImGui::SetCursorPos(ImVec2(169, 38));
        ImGui::MenuChild("C-Slide & Mechanics", ImVec2(320, 482));
        {
            ImGui::Spacing();
            ImGui::Checkbox("Master Enable Slide", &g_MenuState.slide.enabled);
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Checkbox("C-Slide Ativo (Crouch Slide)", &g_MenuState.slide.cSlideActive);
            ImGui::Checkbox("Auto Slide (Quick Switch)", &g_MenuState.slide.autoSlideActive);
            ImGui::Spacing();

            ImGui::SliderInt("Duracao da Tecla C", &g_MenuState.slide.durationC, 5, 100, "%d ms");
            ImGui::SliderInt("Delay Pos-Tiro", &g_MenuState.slide.delayTroca, 0, 250, "%d ms");
            ImGui::SliderFloat("Slide Speed Boost", &g_MenuState.slide.slideBoost, 1.0f, 3.5f, "%.1fx");
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::TextColored(Theme::AccentColor, "Mecanica de C-Slide");
            ImGui::TextColored(Theme::TextMuted, "Cancela o recuo da animacao ao soltar");
            ImGui::TextColored(Theme::TextMuted, "a mira (RMB) enquanto se movimenta.");
            ImGui::Spacing();
            ImGui::TextColored(Theme::AccentColor, "Auto Slide (Quick Switch)");
            ImGui::TextColored(Theme::TextMuted, "Troca para o soco (slot 0) apos o tiro,");
            ImGui::TextColored(Theme::TextMuted, "permitindo correr imediatamente.");
        }
        ImGui::EndChild();

        // CARD 2: Margens por Arma (Delays de Disparo)
        ImGui::SetCursorPos(ImVec2(505, 38));
        ImGui::MenuChild("Weapon Delays (Margens de Disparo)", ImVec2(320, 482));
        {
            ImGui::Spacing();
            ImGui::TextColored(Theme::AccentColor, "Margens de Compensacao por Arma");
            ImGui::TextColored(Theme::TextMuted, "Tempo minimo pos-disparo antes do slide:");
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::SliderInt("Desert Eagle (Deagle)", &g_MenuState.slide.marginDeagle, 0, 1000, "%d ms");
            ImGui::SliderInt("Shotgun", &g_MenuState.slide.marginShotgun, 0, 1000, "%d ms");
            ImGui::SliderInt("Sniper Rifle", &g_MenuState.slide.marginSniper, 0, 1000, "%d ms");
            ImGui::SliderInt("M4 Assault", &g_MenuState.slide.marginM4, 0, 1000, "%d ms");
            ImGui::SliderInt("AK-47", &g_MenuState.slide.marginAK47, 0, 1000, "%d ms");
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::TextColored(Theme::TextMuted, "Configuracoes independentes por arma.");
            ImGui::TextColored(Theme::TextMuted, "Valores sincronizados e salvos no JSON.");
        }
        ImGui::EndChild();
    }

    // ─────────────────────────────────────────────────────────────
    // ABA 9: CONFIGS — SETTINGS & PRESETS
    // ─────────────────────────────────────────────────────────────
    static void RenderConfigsTab()
    {
        static bool s_LoggedRender = false;
        if (!s_LoggedRender)
        {
            Logger::Log("[SOMALIA][CONFIG] UI_RENDERED");
            s_LoggedRender = true;
        }

        static std::string s_StatusMsg = "";
        static uint64_t s_StatusTime = 0;
        static int s_SelectedConfigIdx = -1;

        ImGui::SetCursorPos(ImVec2(169, 38));
        ImGui::MenuChild("Configuration Presets", ImVec2(320, 482));
        {
            ImGui::Spacing();
            ImGui::TextColored(Theme::TextMuted, "Config Name");
            ImGui::InputText("##cfgName", g_MenuState.misc.configName, sizeof(g_MenuState.misc.configName));
            ImGui::Spacing();

            // ── CLOUD STORAGE (KEYAUTH) ──
            ImGui::TextColored(Theme::AccentColor, "NUVEM (KEYAUTH CLOUD)");
            if (ImGui::Button("SAVE CONFIG (CLOUD)", ImVec2(280, 28)))
            {
                std::string name = g_MenuState.misc.configName;
                if (name.empty()) name = "default";
                Config::SaveToCloud(name, s_StatusMsg);
                s_StatusTime = GetTickCount64();
            }
            if (ImGui::Button("LOAD CONFIG (CLOUD)", ImVec2(280, 28)))
            {
                std::string name = g_MenuState.misc.configName;
                if (name.empty()) name = "default";
                Config::LoadFromCloud(name, s_StatusMsg);
                s_StatusTime = GetTickCount64();
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // ── LOCAL JSON STORAGE ──
            ImGui::TextColored(Theme::AccentColor, "LOCAL (JSON FILE)");
            if (ImGui::Button("SAVE CONFIG (LOCAL)", ImVec2(136, 26)))
            {
                std::string name = g_MenuState.misc.configName;
                if (name.empty()) name = "somalia_config";
                if (ConfigManager::SaveConfig(name))
                    s_StatusMsg = "Config salva: " + name + ".json";
                else
                    s_StatusMsg = "Falha ao salvar config.";
                s_StatusTime = GetTickCount64();
            }
            ImGui::SameLine();
            if (ImGui::Button("LOAD CONFIG (LOCAL)", ImVec2(136, 26)))
            {
                std::string name = g_MenuState.misc.configName;
                if (name.empty()) name = "somalia_config";
                if (ConfigManager::LoadConfig(name))
                    s_StatusMsg = "Config carregada: " + name + ".json";
                else
                    s_StatusMsg = "Falha ao carregar config.";
                s_StatusTime = GetTickCount64();
            }

            if (ImGui::Button("DELETE CONFIG", ImVec2(136, 24)))
            {
                std::string name = g_MenuState.misc.configName;
                if (!name.empty() && ConfigManager::DeleteConfig(name))
                    s_StatusMsg = "Config deletada: " + name + ".json";
                s_StatusTime = GetTickCount64();
            }
            ImGui::SameLine();
            if (ImGui::Button("REFRESH", ImVec2(136, 24)))
            {
                ConfigManager::Refresh();
                s_StatusMsg = "Lista atualizada.";
                s_StatusTime = GetTickCount64();
            }

            ImGui::Spacing();
            ImGui::TextColored(Theme::TextMuted, "Configs Disponiveis:");
            const auto& configs = ConfigManager::GetConfigList();
            ImGui::BeginChild("##cfgList", ImVec2(280, 100), true);
            for (size_t i = 0; i < configs.size(); i++)
            {
                bool isSelected = (s_SelectedConfigIdx == static_cast<int>(i)) ||
                                  (strcmp(g_MenuState.misc.configName, configs[i].c_str()) == 0);
                if (ImGui::Selectable(configs[i].c_str(), isSelected))
                {
                    s_SelectedConfigIdx = static_cast<int>(i);
                    strncpy_s(g_MenuState.misc.configName, configs[i].c_str(), sizeof(g_MenuState.misc.configName) - 1);
                }
            }
            ImGui::EndChild();

            if (!s_StatusMsg.empty() && (GetTickCount64() - s_StatusTime < 5000))
            {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.4f, 1.0f), s_StatusMsg.c_str());
            }
        }
        ImGui::EndChild();

        ImGui::SetCursorPos(ImVec2(505, 38));
        ImGui::MenuChild("Menu Settings & Lifecycle", ImVec2(320, 482));
        {
            ImGui::Spacing();
            ImGui::Checkbox("Enable Background Particles", &g_MenuState.misc.particles);
            ImGui::Checkbox("Show Somalia Watermark", &g_MenuState.misc.watermark);
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextColored(Theme::AccentColor, "SomaliaNative Client C++");
            ImGui::Text("Client nativo SA-MP 0.3.7-R1");
            ImGui::Spacing();
            if (ImGui::Button("Resetar Padroes", ImVec2(280, 26)))
            {
                Config::ResetToDefaults();
                s_StatusMsg = "Padroes restaurados.";
                s_StatusTime = GetTickCount64();
            }
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // ── BOTAO DE UNLOAD SEGURO ──
            ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "DESCARREGAMENTO SEGURO");
            ImGui::TextWrapped("Restaura todos os hooks de renderizacao, input e memoria do jogo.");
            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.15f, 0.15f, 0.85f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.90f, 0.20f, 0.20f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.60f, 0.10f, 0.10f, 1.0f));
            if (ImGui::Button("UNLOAD SOMALIA (DESINJETAR)", ImVec2(280, 36)))
            {
                Main::RequestUnload();
            }
            ImGui::PopStyleColor(3);
        }
        ImGui::EndChild();
    }

    // ─────────────────────────────────────────────────────────────
    // NAVEGAÇÃO DA SIDEBAR
    // ─────────────────────────────────────────────────────────────
    static void RenderTab()
    {
        auto draw = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetWindowPos();

        // Cabeçalhos de Seções na Sidebar
        draw->AddText(poppins, 17, ImVec2(pos.x + 13, pos.y + 81),  ImColor(105, 105, 105, int(255 * s_OpenAlpha)), "Aimbot");
        draw->AddText(poppins, 17, ImVec2(pos.x + 13, pos.y + 210), ImColor(105, 105, 105, int(255 * s_OpenAlpha)), "Visuals");
        draw->AddText(poppins, 17, ImVec2(pos.x + 13, pos.y + 348), ImColor(105, 105, 105, int(255 * s_OpenAlpha)), "Miscellaneous");

        // 9 Abas originais do Menu Phobia com roteamento garantido de páginas Aimbot
        ImGui::SetCursorPos(ImVec2(13, 99));
        bool isLegitActive = (g_MenuState.currentTab == 0 && g_MenuState.currentAimbotPage == 0);
        if (ImGui::Rendertab("r", "Legit Bot", isLegitActive))
        {
            if (g_MenuState.currentTab != 0 || g_MenuState.currentAimbotPage != 0)
            {
                g_MenuState.currentTab = 0;
                g_MenuState.currentAimbotPage = 0;
                Logger::Log("[SOMALIA][UI] AimbotPage=LEGIT");
            }
        }

        ImGui::SetCursorPos(ImVec2(13, 136));
        bool isRageActive = (g_MenuState.currentTab == 1 || (g_MenuState.currentTab == 0 && g_MenuState.currentAimbotPage == 1));
        if (ImGui::Rendertab("e", "Rage Bot", isRageActive))
        {
            if (g_MenuState.currentTab != 1 || g_MenuState.currentAimbotPage != 1)
            {
                g_MenuState.currentTab = 1;
                g_MenuState.currentAimbotPage = 1;
                Logger::Log("[SOMALIA][UI] AimbotPage=RAGE");
            }
        }

        ImGui::SetCursorPos(ImVec2(13, 174));
        if (ImGui::Rendertab("a", "Silent Aim", g_MenuState.currentTab == 2)) g_MenuState.currentTab = 2;

        ImGui::SetCursorPos(ImVec2(13, 228));
        if (ImGui::Rendertab("x", "Players", g_MenuState.currentTab == 3)) g_MenuState.currentTab = 3;

        ImGui::SetCursorPos(ImVec2(13, 266));
        if (ImGui::Rendertab("w", "World", g_MenuState.currentTab == 4)) g_MenuState.currentTab = 4;

        ImGui::SetCursorPos(ImVec2(13, 304));
        if (ImGui::Rendertab("v", "View", g_MenuState.currentTab == 5)) g_MenuState.currentTab = 5;

        ImGui::SetCursorPos(ImVec2(13, 369));
        if (ImGui::Rendertab("z", "Main", g_MenuState.currentTab == 6)) g_MenuState.currentTab = 6;

        ImGui::SetCursorPos(ImVec2(13, 407));
        if (ImGui::Rendertab("s", "Inventory", g_MenuState.currentTab == 7)) g_MenuState.currentTab = 7;

        ImGui::SetCursorPos(ImVec2(13, 445));
        if (ImGui::Rendertab("f", "C-Slide", g_MenuState.currentTab == 8)) g_MenuState.currentTab = 8;

        ImGui::SetCursorPos(ImVec2(13, 483));
        if (ImGui::Rendertab("c", "Configs", g_MenuState.currentTab == 9)) g_MenuState.currentTab = 9;

        // Renderização estritamente exclusiva: apenas UMA página renderizada por frame
        if (g_MenuState.currentTab == 0 || g_MenuState.currentTab == 1)
        {
            int activeAimbotPage = (g_MenuState.currentTab == 1) ? 1 : g_MenuState.currentAimbotPage;
            if (activeAimbotPage == 1)
            {
                RenderRageBotTab();
            }
            else
            {
                RenderLegitBotTab();
            }
        }
        else
        {
            switch (g_MenuState.currentTab)
            {
            case 2: RenderSilentAimTab(); break;        // Silent Aim
            case 3: RenderPlayersVisualsTab(); break;  // Players (ESP)
            case 4: RenderWorldVisualsTab(); break;    // World
            case 5: RenderViewVisualsTab(); break;     // View & Camera
            case 6: RenderPlayerModsTab(); break;      // Main (Player)
            case 7: RenderVehicleModsTab(); break;     // Inventory (Vehicles)
            case 8: RenderSlideTab(); break;           // C-Slide & Movement
            case 9: RenderConfigsTab(); break;         // Configs
            default: RenderLegitBotTab(); break;
            }
        }
    }

    void Render()
    {
        if (!g_MenuState.menuOpen)
        {
            s_OpenAlpha = 0.0f;
            return;
        }

        s_OpenAlpha = ImLerp(s_OpenAlpha, 1.0f, 10.0f * ImGui::GetIO().DeltaTime);

        ImGui::SetNextWindowSize(ImVec2(838.0f, 535.0f));
        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));

        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, s_OpenAlpha);
        if (ImGui::Begin("PhobiaMenu", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground))
        {
            Decoration();
            RenderTab();
            user_info();
            Particles();
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }
}
