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
#include "../Features/PlayerSlap/PlayerSlap.h"
#include "../Features/FastSwitch/FastSwitch.h"
#include "../Features/LuaSlide/LuaSlide.h"
#include "../Core/RuntimeState.h"
#include <vector>
#include <stdlib.h>
#include <fstream>
#include <sstream>
#include <cmath>

// Fontes compartilhadas com os widgets customizados do ImGui (Rendertab / MenuChild)
ImFont* tab_title = nullptr;
ImFont* font_icon = nullptr;
ImFont* poppins = nullptr;

// Texturas D3D9 em memória
static IDirect3DTexture9* s_pLogoOne   = nullptr;
static IDirect3DTexture9* s_pLogoTwo   = nullptr;

static float s_OpenAlpha = 0.0f;
static float s_ContentAlpha = 0.0f;
static int s_LastAnimatedTab = -1;
static bool s_ShowColorSelector = false;

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

            ImGui::GetWindowDrawList()->AddCircleFilled(particle_pos[i], particle_radius[i], ImColor(accent_colour[0], accent_colour[1], accent_colour[2], 0.55f * s_OpenAlpha));
        }
    }

    static void Decoration()
    {
        auto draw = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetWindowPos();
        float pulse = (sinf(ImGui::GetTime() * 2.6f) + 1.0f) * 0.5f;

        draw->AddRectFilledMultiColor(
            ImVec2(pos.x, pos.y),
            ImVec2(pos.x + 838, pos.y + 535),
            IM_COL32(18, 20, 24, int(255 * s_OpenAlpha)),
            IM_COL32(22, 29, 34, int(255 * s_OpenAlpha)),
            IM_COL32(16, 16, 18, int(255 * s_OpenAlpha)),
            IM_COL32(24, 24, 27, int(255 * s_OpenAlpha)));

        draw->AddRectFilled(ImVec2(pos.x, pos.y), ImVec2(pos.x + 161, pos.y + 535), ImColor(26, 28, 32, int(238 * s_OpenAlpha)), 12.0f, ImDrawCornerFlags_Left);
        draw->AddRectFilledMultiColor(ImVec2(pos.x + 1, pos.y + 1), ImVec2(pos.x + 160, pos.y + 534),
            IM_COL32(44, 52, 60, int(55 * s_OpenAlpha)), IM_COL32(24, 27, 31, int(20 * s_OpenAlpha)),
            IM_COL32(19, 20, 23, int(15 * s_OpenAlpha)), IM_COL32(32, 40, 48, int(40 * s_OpenAlpha)));
        draw->AddRect(ImVec2(pos.x, pos.y), ImVec2(pos.x + 161, pos.y + 535), ImColor(65, 78, 88, int(130 * s_OpenAlpha)), 12.0f, ImDrawCornerFlags_Left, 1.0f);

        draw->AddRectFilled(ImVec2(pos.x + 160, pos.y), ImVec2(pos.x + 838, pos.y + 535), ImColor(19, 20, 23, int(246 * s_OpenAlpha)), 12.0f, ImDrawCornerFlags_Right);
        draw->AddRect(ImVec2(pos.x + 160, pos.y), ImVec2(pos.x + 838, pos.y + 535), ImColor(57, 67, 76, int(115 * s_OpenAlpha)), 12.0f, ImDrawCornerFlags_Right, 1.0f);
        draw->AddLine(ImVec2(pos.x + 160, pos.y + 18), ImVec2(pos.x + 160, pos.y + 517), ImColor(accent_colour[0], accent_colour[1], accent_colour[2], ((40.0f + 55.0f * pulse) / 255.0f) * s_OpenAlpha), 1.0f);
        draw->AddCircleFilled(ImVec2(pos.x + 775, pos.y + 62), 78.0f, ImColor(accent_colour[0], accent_colour[1], accent_colour[2], (11.0f / 255.0f) * s_OpenAlpha));
        draw->AddCircleFilled(ImVec2(pos.x + 704, pos.y + 492), 56.0f, ImColor(92, 138, 170, int(10 * s_OpenAlpha)));

        // 3. Título no topo da Sidebar: "Somalia"
        if (poppins)
        {
            ImVec2 sz = poppins->CalcTextSizeA(24.0f, FLT_MAX, 0.0f, "Somalia");
            float tx = pos.x + (161.0f - sz.x) * 0.5f;
            draw->AddText(poppins, 24.0f, ImVec2(tx + 1.0f, pos.y + 26.0f + 1.0f), IM_COL32(0, 0, 0, int(180 * s_OpenAlpha)), "Somalia");
            draw->AddText(poppins, 24.0f, ImVec2(tx, pos.y + 26.0f), ImColor(Theme::AccentColor.x, Theme::AccentColor.y, Theme::AccentColor.z, s_OpenAlpha), "Somalia");
            draw->AddLine(ImVec2(pos.x + 45, pos.y + 58), ImVec2(pos.x + 116, pos.y + 58), ImColor(accent_colour[0], accent_colour[1], accent_colour[2], ((70.0f + 90.0f * pulse) / 255.0f) * s_OpenAlpha), 2.0f);
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

        Config::AccountInfo acc = Config::GetAccountInfo();
        if (!acc.sessionId.empty() || (!acc.username.empty() && acc.username != "Somalia"))
        {
            s_Loaded = true;
        }

        if (!acc.username.empty())
            s_AccountUser = acc.username;

        if (!acc.daysLeft.empty())
        {
            if (acc.daysLeft.find("Ilimitad") != std::string::npos || acc.daysLeft.find("Vital") != std::string::npos || acc.daysLeft.find("Life") != std::string::npos)
                s_AccountPlan = "Vitalicio";
            else
                s_AccountPlan = acc.daysLeft;
        }
        else if (!acc.subscription.empty())
        {
            if (acc.subscription.find("Life") != std::string::npos || acc.subscription.find("Vital") != std::string::npos)
                s_AccountPlan = "Vitalicio";
            else
                s_AccountPlan = acc.subscription;
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
        int weaponIdx = g_MenuState.legitBot.autoSnipersType;
        if (weaponIdx < 0 || weaponIdx >= 4) weaponIdx = 0;
        auto& w = g_MenuState.legitBot.weapons[weaponIdx];

        // 1. Arma Selecionada e Configuração Geral (Esquerda Topo)
        ImGui::SetCursorPos(ImVec2(169, 38));
        ImGui::MenuChild("Weapon & General", ImVec2(320, 260));
        {
            const char* type[] = { "Auto Snipers (Sniper, Country)", "Pistols (Desert Eagle)", "Rifles (M4, AK-47)", "Shotguns (Combat, Sawnoff)" };
            ImGui::Spacing();
            ImGui::Combo("Target Weapon", &g_MenuState.legitBot.autoSnipersType, type, IM_ARRAYSIZE(type));
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

        // 2. Exploits e Defesa (Esquerda Base) — Sem duplicação de Silent Aim
        ImGui::SetCursorPos(ImVec2(169, 326));
        ImGui::MenuChild("Exploits & Defense", ImVec2(320, 194));
        {
            ImGui::Spacing();
            ImGui::Checkbox("Lag Peek", &g_MenuState.legitBot.exploitLagPeek);
            ImGui::Checkbox("Hide Shots", &g_MenuState.legitBot.exploitHideShots);
            ImGui::Checkbox("Double Tap", &g_MenuState.legitBot.exploitDoubleTap);
            ImGui::Checkbox("Anti-HS (Headshot Proof)", &g_MenuState.player.antiHS);
        }
        ImGui::EndChild();

        // 3. Seleção de Alvo (Direita Topo) — Altura 215px otimizada para dar folga ao Triggerbot abaixo
        ImGui::SetCursorPos(ImVec2(505, 38));
        ImGui::MenuChild("Target Selection", ImVec2(320, 215));
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

        // 4. Filtros de Alvo e Triggerbot (Direita Base) — Y=280, H=240 garante 223px uteis, sem nenhum corte de slider
        ImGui::SetCursorPos(ImVec2(505, 280));
        ImGui::MenuChild("Target Filters & Triggerbot", ImVec2(320, 240));
        {
            ImGui::Spacing();
            ImGui::Checkbox("Ignore Dead Players", &w.ignoreDead);
            ImGui::Checkbox("Team Check (Amigos)", &w.teamCheck);
            ImGui::Checkbox("Visibility Check (Paredes)", &w.visibilityCheck);
            ImGui::Checkbox("Ignore limbs when moving", &g_MenuState.legitBot.ignoreLimbs);
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Checkbox("Triggerbot", &g_MenuState.triggerBot.enabled);
            ImGui::SliderInt("Reaction Delay", &g_MenuState.triggerBot.reactionDelay, 0, 200, "%d ms");
        }
        ImGui::EndChild();
    }

    // ─────────────────────────────────────────────────────────────
    // ABA 1: RAGEBOT — TELA DEDICADA E INDEPENDENTE
    // ─────────────────────────────────────────────────────────────
    static void RenderRageBotTab()
    {
        int weaponIdx = g_MenuState.rageBot.currentWeaponGroup;
        if (weaponIdx < 0 || weaponIdx >= 4) weaponIdx = 0;
        auto& rw = g_MenuState.rageBot.weapons[weaponIdx];

        // 1. Painel Geral do Ragebot (Coluna Esquerda Completa - H=482)
        ImGui::SetCursorPos(ImVec2(169, 38));
        ImGui::MenuChild("Ragebot Configuration", ImVec2(320, 482));
        {
            const char* type[] = { "Auto Snipers (Sniper, Country)", "Pistols (Desert Eagle)", "Rifles (M4, AK-47)", "Shotguns (Combat, Sawnoff)" };
            ImGui::Spacing();
            ImGui::Combo("Target Weapon Profile", &g_MenuState.rageBot.currentWeaponGroup, type, IM_ARRAYSIZE(type));
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
        }
        ImGui::EndChild();

        // 2. Seleção de Alvo e Ativação do Ragebot (Direita Topo - H=230)
        ImGui::SetCursorPos(ImVec2(505, 38));
        ImGui::MenuChild("Target Selection & Activation", ImVec2(320, 230));
        {
            const char* activations[] = { "Always", "While Aiming (RMB)", "While Shooting (LMB)", "Aim + Shoot" };
            const char* bones[] = { "HEAD (Osso 8)", "NECK (Osso 5)", "CHEST (Osso 4)", "PELVIS (Osso 2)" };
            const char* priorities[] = { "Closest to Crosshair", "Closest Distance (3D)", "Lowest Health" };

            ImGui::Spacing();
            ImGui::Combo("Activation", &rw.activationMode, activations, IM_ARRAYSIZE(activations));
            ImGui::Combo("Target Bone", &rw.bone, bones, IM_ARRAYSIZE(bones));
            ImGui::Combo("Target Priority", &rw.priority, priorities, IM_ARRAYSIZE(priorities));
        }
        ImGui::EndChild();

        // 3. Filtros de Alvo do Ragebot (Direita Base - Y=296, H=224)
        ImGui::SetCursorPos(ImVec2(505, 296));
        ImGui::MenuChild("Target Filters", ImVec2(320, 224));
        {
            ImGui::Spacing();
            ImGui::Checkbox("Ignore Dead Players", &rw.ignoreDead);
            ImGui::Checkbox("Team Check (Amigos)", &rw.teamCheck);
            ImGui::Checkbox("Visibility Check (Paredes)", &rw.visibilityCheck);
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

        // 1. Configuração Completa do Silent Aim (Coluna Esquerda - H=482)
        ImGui::SetCursorPos(ImVec2(169, 38));
        ImGui::MenuChild("Silent Aim Configuration", ImVec2(320, 482));
        {
            const char* weaponGroups[] = {
                "Auto Snipers (Sniper, Country)",
                "Pistols (Desert Eagle)",
                "Rifles (M4, AK-47)",
                "Shotguns (Combat, Sawnoff)"
            };
            ImGui::Spacing();
            ImGui::Combo("Target Weapon Profile", &g_MenuState.silentAim.currentWeaponGroup, weaponGroups, IM_ARRAYSIZE(weaponGroups));
            ImGui::Spacing();

            if (ImGui::Checkbox("Master Enable Silent Aim", &g_MenuState.silentAim.enabled))
            {
                g_MenuState.legitBot.silentAim = g_MenuState.silentAim.enabled;
            }
            ImGui::Checkbox("Enable for this Weapon", &sw.enabled);
            ImGui::Spacing();
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

        // 2. Seleção de Alvo e Ossos (Direita Topo - H=230)
        ImGui::SetCursorPos(ImVec2(505, 38));
        ImGui::MenuChild("Target Selection & Bones", ImVec2(320, 230));
        {
            const char* priorities[] = { "Closest to Crosshair", "Closest Distance 3D", "Lowest Health" };
            const char* bones[] = { "Head (Osso 8)", "Neck (Osso 5)", "Chest (Osso 4)", "Pelvis (Osso 2)", "Random Hitbox" };
            const char* activations[] = { "Always", "While Aiming (RMB)", "While Shooting (LMB) [Silent]", "Aim + Shoot" };

            ImGui::Spacing();
            ImGui::Combo("Priority", &sw.priority, priorities, IM_ARRAYSIZE(priorities));
            ImGui::Combo("Target Bone", &sw.bone, bones, IM_ARRAYSIZE(bones));
            ImGui::Combo("Activation", &sw.activationMode, activations, IM_ARRAYSIZE(activations));
        }
        ImGui::EndChild();

        // 3. Filtros de Alvo (Direita Base - Y=296, H=224)
        ImGui::SetCursorPos(ImVec2(505, 296));
        ImGui::MenuChild("Target Filters", ImVec2(320, 224));
        {
            ImGui::Spacing();
            ImGui::Checkbox("Ignore Dead Players", &sw.ignoreDead);
            ImGui::Checkbox("Team Check (Amigos)", &sw.teamCheck);
            ImGui::Checkbox("Visibility Check (Paredes)", &sw.visibilityCheck);
        }
        ImGui::EndChild();
    }

    // ─────────────────────────────────────────────────────────────
    // ABA 3: VISUALS — PLAYERS (ESP COMPLETO + ANTI-AIM NO CANTO)
    // ─────────────────────────────────────────────────────────────
    static void RenderPlayersVisualsTab()
    {
        // 1. ESP de Jogadores (Coluna Esquerda Completa - H=482)
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
            ImGui::Checkbox("Weapon Name ESP", &g_MenuState.visuals.weaponESP);
            ImGui::Spacing();

            ImGui::Checkbox("Snaplines", &g_MenuState.visuals.snaplines);
            ImGui::Combo("Snapline Origin", &g_MenuState.visuals.snaplineOrigin, origins, IM_ARRAYSIZE(origins));
            ImGui::Spacing();

            ImGui::Checkbox("Skeleton / Bones", &g_MenuState.visuals.bonesESP);
        }
        ImGui::EndChild();

        // 2. Filtros de Alvos (Superior Direito - H=230)
        ImGui::SetCursorPos(ImVec2(505, 38));
        ImGui::MenuChild("ESP Filters & Limits", ImVec2(320, 230));
        {
            ImGui::Spacing();
            ImGui::SliderInt("Max Render Distance", &g_MenuState.visuals.maxDistance, 20, 500, "%d m");
            ImGui::Spacing();
            ImGui::Checkbox("Enemy Only", &g_MenuState.visuals.enemyOnly);
            ImGui::Checkbox("Off-screen Arrows", &g_MenuState.visuals.offscreenArrows);
        }
        ImGui::EndChild();

        // 3. Destaques Visuais & Bullet Sync (Inferior Direito - Y=296, H=224)
        ImGui::SetCursorPos(ImVec2(505, 296));
        ImGui::MenuChild("Target Highlights & Bullet Sync", ImVec2(320, 224));
        {
            ImGui::Spacing();
            ImGui::Checkbox("Target Locked Highlight", &g_MenuState.visuals.targetHighlight);
            ImGui::Checkbox("Line of Sight (Look Direction)", &g_MenuState.visuals.lineOfSight);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Checkbox("Sky Bullet Sync (HS do Ceu)", &g_MenuState.silentAim.skyBulletSync);
            if (g_MenuState.silentAim.skyBulletSync)
            {
                ImGui::SliderFloat("Altura no Ceu##skyHeight", &g_MenuState.silentAim.skyHeight, 50.0f, 120.0f, "%.0f m");
            }
        }
        ImGui::EndChild();
    }

    // ─────────────────────────────────────────────────────────────
    // ABA 4: VISUALS — WORLD (TOTALMENTE BALANCEADA E PREENCHIDA)
    // ─────────────────────────────────────────────────────────────
    static void RenderWorldVisualsTab()
    {
        // 1. World Entities ESP (Superior Esquerdo - H=230)
        ImGui::SetCursorPos(ImVec2(169, 38));
        ImGui::MenuChild("World Entities ESP", ImVec2(320, 230));
        {
            ImGui::Spacing();
            ImGui::Checkbox("Vehicles ESP", &g_MenuState.visuals.vehicleESP);
            ImGui::Checkbox("Pickups & Items ESP", &g_MenuState.visuals.pickupESP);
            ImGui::Checkbox("3D Text Labels ESP", &g_MenuState.visuals.objectESP);
            ImGui::SliderInt("World Max Distance", &g_MenuState.visuals.worldMaxDist, 50, 500, "%d m");
        }
        ImGui::EndChild();

        // 2. Atmosphere & Sky (Inferior Esquerdo - Y=296, H=224)
        ImGui::SetCursorPos(ImVec2(169, 296));
        ImGui::MenuChild("Atmosphere & Weather", ImVec2(320, 224));
        {
            const char* weathers[] = { "Sunny / Clear", "Foggy", "Rainy / Storm", "Night Extra Dark", "Sunset Red" };
            ImGui::Spacing();
            ImGui::Checkbox("Custom Weather", &g_MenuState.visuals.weatherChanger);
            ImGui::Combo("Weather ID", &g_MenuState.visuals.weatherID, weathers, IM_ARRAYSIZE(weathers));
            ImGui::Spacing();
            ImGui::Checkbox("Night Mode Effect", &g_MenuState.visuals.nightMode);
            ImGui::Checkbox("Remove Fog (Clear Horizon)", &g_MenuState.visuals.noFog);
            ImGui::Checkbox("Remove Clouds / Clear Sky", &g_MenuState.visuals.clearSky);
        }
        ImGui::EndChild();

        // 3. Time & World Lighting (Superior Direito - H=230)
        ImGui::SetCursorPos(ImVec2(505, 38));
        ImGui::MenuChild("Time & World Lighting", ImVec2(320, 230));
        {
            ImGui::Spacing();
            ImGui::Checkbox("Custom Game Hour", &g_MenuState.visuals.timeChanger);
            ImGui::SliderInt("Clock Hour", &g_MenuState.visuals.timeHour, 0, 23, "%d:00");
            ImGui::Checkbox("Lock Game Hour (Freeze Time)", &g_MenuState.visuals.lockHour);
            ImGui::Checkbox("Fullbright (Ambient Boost)", &g_MenuState.visuals.fullbright);
        }
        ImGui::EndChild();

        // 4. World Environment & FPS Boost (Inferior Direito - Y=296, H=224)
        ImGui::SetCursorPos(ImVec2(505, 296));
        ImGui::MenuChild("Environment & FPS Boost", ImVec2(320, 224));
        {
            ImGui::Spacing();
            ImGui::Checkbox("Remove Grass & Foliage (FPS+)", &g_MenuState.visuals.removeGrass);
            ImGui::Checkbox("Remove Rain Particles", &g_MenuState.visuals.removeRain);
            ImGui::Checkbox("Water Transparency", &g_MenuState.visuals.clearWater);
            ImGui::Checkbox("Extended Draw Distance", &g_MenuState.visuals.extendedDrawDist);
        }
        ImGui::EndChild();
    }

    // ─────────────────────────────────────────────────────────────
    // ABA 5: VISUALS — VIEW & CAMERA (TOTALMENTE BALANCEADA)
    // ─────────────────────────────────────────────────────────────
    static void RenderViewVisualsTab()
    {
        // 1. Crosshair & Overlays (Superior Esquerdo - H=230)
        ImGui::SetCursorPos(ImVec2(169, 38));
        ImGui::MenuChild("Crosshair & Overlays", ImVec2(320, 230));
        {
            ImGui::Spacing();
            ImGui::Checkbox("Draw Aimbot FOV Circle", &g_MenuState.visuals.drawFOVCircle);
            ImGui::SliderInt("FOV Circle Radius", &g_MenuState.visuals.fovCircleRadius, 10, 150, "%d px");
            ImGui::Spacing();
            ImGui::Checkbox("Custom Screen Crosshair", &g_MenuState.visuals.customCrosshair);
        }
        ImGui::EndChild();

        // 2. Combat Feedback (Inferior Esquerdo - Y=296, H=224)
        ImGui::SetCursorPos(ImVec2(169, 296));
        ImGui::MenuChild("Combat Feedback", ImVec2(320, 224));
        {
            ImGui::Spacing();
            ImGui::Checkbox("Hitmarker on Damage", &g_MenuState.visuals.hitmarker);
            ImGui::Checkbox("Damage Informer (Floating Numbers)", &g_MenuState.visuals.damageInformer);
        }
        ImGui::EndChild();

        // 3. Camera & Field of View (Superior Direito - H=230)
        ImGui::SetCursorPos(ImVec2(505, 38));
        ImGui::MenuChild("Camera & Field of View", ImVec2(320, 230));
        {
            ImGui::Spacing();
            ImGui::Checkbox("Custom Camera FOV", &g_MenuState.visuals.customCameraFOV);
            ImGui::SliderFloat("FOV Angle", &g_MenuState.visuals.cameraFOV, 60.0f, 115.0f, "%.1f deg");
            ImGui::Spacing();
            ImGui::Checkbox("Disable Weapon Cam Shake", &g_MenuState.visuals.noCamShake);
        }
        ImGui::EndChild();

        // 4. HUD & Interface Cleanup (Inferior Direito - Y=296, H=224)
        ImGui::SetCursorPos(ImVec2(505, 296));
        ImGui::MenuChild("HUD & Display Cleanup", ImVec2(320, 224));
        {
            ImGui::Spacing();
            ImGui::Checkbox("Hide Default Radar", &g_MenuState.visuals.hideRadar);
            ImGui::Checkbox("Hide SA-MP HUD Elements", &g_MenuState.visuals.hideHUD);
            ImGui::Checkbox("Show Performance FPS/Ping", &g_MenuState.visuals.showFPS);
        }
        ImGui::EndChild();
    }

    static std::string s_ConfigStatusMsg = "";
    static uint64_t    s_ConfigStatusTime = 0;
    static int         s_ConfigSelectedIdx = -1;



    // ─────────────────────────────────────────────────────────────
    // ABA 6: MAIN — PLAYER MODS & EXPLOITS
    // ─────────────────────────────────────────────────────────────
    static void RenderPlayerModsTab()
    {
        // 1. Atributos e Movimentação (Coluna Esquerda Topo - H=230)
        ImGui::SetCursorPos(ImVec2(169, 38));
        ImGui::MenuChild("Player Attributes & Movement", ImVec2(320, 230));
        {
            ImGui::Spacing();
            ImGui::Checkbox("Godmode (Local Proofs)", &g_MenuState.player.godmode);
            ImGui::Checkbox("Infinite Ammo", &g_MenuState.player.infAmmo);
            ImGui::Checkbox("Infinite Stamina", &g_MenuState.player.infStamina);
            ImGui::Spacing();
            ImGui::Checkbox("Fast Sprint", &g_MenuState.player.fastRun);
            ImGui::Checkbox("Mega Jump", &g_MenuState.player.megaJump);
            ImGui::Checkbox("Auto Bunnyhop", &g_MenuState.player.autoBhop);
            ImGui::Checkbox("Fall Damage Proof", &g_MenuState.player.fallProof);
        }
        ImGui::EndChild();

        // 2. Defesa em Combate e Auxiliares (Coluna Esquerda Base - Y=296, H=224)
        ImGui::SetCursorPos(ImVec2(169, 296));
        ImGui::MenuChild("Combat Defense & Helper", ImVec2(320, 224));
        {
            ImGui::Spacing();
            ImGui::Checkbox("Anti-Stun", &g_MenuState.player.antiStun);
            ImGui::Checkbox("Master Enable Anti-HS", &g_MenuState.player.antiHS);
            if (g_MenuState.player.antiHS)
            {
                ImGui::SliderFloat("Max HS Dmg Cap", &g_MenuState.player.antiHSDamageCap, 20.0f, 50.0f, "%.1f HP");
            }
            ImGui::Spacing();
            ImGui::Checkbox("Fast Weapon Reload", &g_MenuState.player.fastReload);
            ImGui::Checkbox("Automatic C-Bug Helper", &g_MenuState.player.autoCBug);
            ImGui::Checkbox("No Spread", &g_MenuState.player.noSpread);
        }
        ImGui::EndChild();

        // 3. Anti-Aim & Network Angles (Superior Direito - H=375)
        ImGui::SetCursorPos(ImVec2(505, 38));
        ImGui::MenuChild("Anti-Aim & Network Angles", ImVec2(320, 365), false, ImGuiWindowFlags_NoScrollWithMouse);
        {
            const char* pitchList[] = { "Disabled", "Emotion (-89°)", "Up (89°)", "Zero (0°)" };
            const char* yawList[]   = { "Disabled", "Backward (180°)", "Spinbot", "Jitter", "Random" };

            ImGui::Spacing();
            ImGui::Checkbox("Enable Anti-Aim", &g_MenuState.antiAim.enabled);
            ImGui::Combo("Pitch Mode", &g_MenuState.antiAim.pitchMode, pitchList, IM_ARRAYSIZE(pitchList));
            ImGui::Combo("Yaw Mode", &g_MenuState.antiAim.yawMode, yawList, IM_ARRAYSIZE(yawList));
            ImGui::SliderInt("Spin Speed", &g_MenuState.antiAim.spinSpeed, 1, 50, "%d");
            ImGui::Checkbox("Desync Angles", &g_MenuState.antiAim.desync);
            ImGui::SameLine(160);
            ImGui::Checkbox("Invertebred", &g_MenuState.antiAim.invertebred);
            ImGui::Checkbox("Enable Fake Lag", &g_MenuState.antiAim.fakeLag);
            if (g_MenuState.antiAim.fakeLag)
            {
                ImGui::SliderInt("Choked Ticks", &g_MenuState.antiAim.fakeLagLimit, 1, 16, "%d ticks");
            }
        }
        ImGui::EndChild();

        // 4. CLEO Exploits & Scripts (Inferior Direito - Y=415, H=105)
        ImGui::SetCursorPos(ImVec2(505, 415));
        ImGui::MenuChild("CLEO Exploits (slapxx & arquive)", ImVec2(320, 105));
        {
            ImGui::Spacing();
            if (ImGui::Checkbox("Player Slap (/tapa)", &g_MenuState.playerSlap.enabled))
            {
                if (g_MenuState.playerSlap.enabled)
                    PlayerSlap::ShowToast("[PlayerSlap] ATIVADO (ON)", 0xFF00FF88, 3000);
                else
                    PlayerSlap::ShowToast("[PlayerSlap] DESATIVADO (OFF)", 0xFFFF4444, 3000);
            }
            ImGui::Spacing();
            ImGui::Checkbox("Fast Switch", &g_MenuState.fastSwitch.enabled);
        }
        ImGui::EndChild();
    }

    // ─────────────────────────────────────────────────────────────
    // ABA 7: VEHICLES — CONTROLES E MODS DE VEICULO
    // ─────────────────────────────────────────────────────────────
    static void RenderVehicleModsTab()
    {
        // 1. Vehicle Physics & Engine (Esquerda Topo - H=250)
        ImGui::SetCursorPos(ImVec2(169, 38));
        ImGui::MenuChild("Vehicle Physics & Engine", ImVec2(320, 250));
        {
            ImGui::Spacing();
            ImGui::Checkbox("Engine Always On", &g_MenuState.vehicle.engineAlwaysOn);
            ImGui::Checkbox("Car Godmode", &g_MenuState.vehicle.carGodmode);
            ImGui::SliderInt("Speed Multiplier (Shift)", &g_MenuState.vehicle.speedMultiplier, 1, 10, "%dx");
            ImGui::Checkbox("Auto Flip Vehicle", &g_MenuState.vehicle.autoFlip);
            ImGui::Checkbox("Super Brake (Space / S)", &g_MenuState.vehicle.superBrake);
            ImGui::Spacing();
            ImGui::Checkbox("Fly Car Mode", &g_MenuState.vehicle.flyCar);
        }
        ImGui::EndChild();

        // 2. Ações Rápidas de Veículo (Esquerda Base - Y=316, H=204) — Preenche a tela vazia
        ImGui::SetCursorPos(ImVec2(169, 316));
        ImGui::MenuChild("Quick Vehicle Actions", ImVec2(320, 204));
        {
            ImGui::Spacing();
            if (ImGui::Button("REPARAR VEICULO (1000 HP)", ImVec2(280, 28)))
            {
                void* pLocalPed = RuntimeState::GetLocalPed();
                if (pLocalPed)
                {
                    uintptr_t pedAddr = reinterpret_cast<uintptr_t>(pLocalPed);
                    void* pVehicle = *reinterpret_cast<void**>(pedAddr + 0x58C);
                    if (pVehicle)
                    {
                        uintptr_t vehAddr = reinterpret_cast<uintptr_t>(pVehicle);
                        *reinterpret_cast<float*>(vehAddr + 0x4C0) = 1000.0f;
                        *reinterpret_cast<uint8_t*>(vehAddr + 0x428) |= 0x10;
                        Logger::Log("[SOMALIA][VEHICLE] Reparado via botao rapido (1000.0 HP)");
                    }
                }
            }

            ImGui::Spacing();
            if (ImGui::Button("DESVIRAR VEICULO (FLIP)", ImVec2(280, 28)))
            {
                void* pLocalPed = RuntimeState::GetLocalPed();
                if (pLocalPed)
                {
                    uintptr_t pedAddr = reinterpret_cast<uintptr_t>(pLocalPed);
                    void* pVehicle = *reinterpret_cast<void**>(pedAddr + 0x58C);
                    if (pVehicle)
                    {
                        uintptr_t vehAddr = reinterpret_cast<uintptr_t>(pVehicle);
                        uintptr_t pMatrix = *reinterpret_cast<uintptr_t*>(vehAddr + 0x14);
                        if (pMatrix)
                        {
                            *reinterpret_cast<float*>(pMatrix + 0x20) = 0.0f;
                            *reinterpret_cast<float*>(pMatrix + 0x24) = 0.0f;
                            *reinterpret_cast<float*>(pMatrix + 0x28) = 1.0f;
                            *reinterpret_cast<float*>(vehAddr + 0x50) = 0.0f;
                            *reinterpret_cast<float*>(vehAddr + 0x54) = 0.0f;
                            *reinterpret_cast<float*>(vehAddr + 0x58) = 0.0f;
                            *reinterpret_cast<float*>(pMatrix + 0x38) += 0.15f;
                            Logger::Log("[SOMALIA][VEHICLE] Desvirado via botao rapido");
                        }
                    }
                }
            }

            ImGui::Spacing();
            if (ImGui::Button("LIGAR MOTOR (ENGINE ON)", ImVec2(280, 28)))
            {
                void* pLocalPed = RuntimeState::GetLocalPed();
                if (pLocalPed)
                {
                    uintptr_t pedAddr = reinterpret_cast<uintptr_t>(pLocalPed);
                    void* pVehicle = *reinterpret_cast<void**>(pedAddr + 0x58C);
                    if (pVehicle)
                    {
                        uintptr_t vehAddr = reinterpret_cast<uintptr_t>(pVehicle);
                        *reinterpret_cast<uint8_t*>(vehAddr + 0x428) |= 0x10;
                    }
                }
            }
        }
        ImGui::EndChild();

        // 3. Vehicle Handling & Utility (Direita Topo - H=250)
        ImGui::SetCursorPos(ImVec2(505, 38));
        ImGui::MenuChild("Vehicle Handling & Utility", ImVec2(320, 250));
        {
            ImGui::Spacing();
            ImGui::Checkbox("Instant Vehicle Repair (Tecla R)", &g_MenuState.vehicle.instantRepair);
            ImGui::Checkbox("No Bike Fall", &g_MenuState.vehicle.noBikeFall);
            ImGui::Checkbox("Heavy Vehicle (Ram Protection)", &g_MenuState.vehicle.heavyVehicle);
            ImGui::Checkbox("Drift Mode (Reduced Friction)", &g_MenuState.vehicle.driftMode);
            ImGui::Checkbox("Unlimited Nitro", &g_MenuState.vehicle.unlimitedNitro);
        }
        ImGui::EndChild();

        // 4. Status e Telemetria do Veiculo (Direita Base - Y=316, H=204) — Preenche a tela vazia
        ImGui::SetCursorPos(ImVec2(505, 316));
        ImGui::MenuChild("Vehicle Telemetry & Speed", ImVec2(320, 204));
        {
            void* pLocalPed = RuntimeState::GetLocalPed();
            bool inVeh = false;
            float vehHp = 0.0f;
            if (pLocalPed)
            {
                uintptr_t pedAddr = reinterpret_cast<uintptr_t>(pLocalPed);
                void* pVehicle = *reinterpret_cast<void**>(pedAddr + 0x58C);
                if (pVehicle)
                {
                    inVeh = true;
                    vehHp = *reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(pVehicle) + 0x4C0);
                }
            }

            ImGui::Spacing();
            ImGui::TextColored(Theme::AccentColor, "Status no Veiculo:");
            if (inVeh)
            {
                ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.4f, 1.0f), "  No Veiculo: SIM");
                ImGui::Text("  Integridade: %.0f HP", vehHp);
            }
            else
            {
                ImGui::TextColored(Theme::TextMuted, "  No Veiculo: A PE (FORA)");
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextColored(Theme::AccentColor, "Multiplicador de Velocidade:");
            ImGui::Text("  Shift Pressionado: %dx", g_MenuState.vehicle.speedMultiplier);
        }
        ImGui::EndChild();
    }

    // ─────────────────────────────────────────────────────────────
    // ABA 8: SLIDE & COMBAT MOVEMENT
    // ─────────────────────────────────────────────────────────────
    static void RenderSlideTab()
    {
        // ── Coluna 1 (Esquerda): Apenas Ativações e Toggles ──
        ImGui::SetCursorPos(ImVec2(169, 38));
        ImGui::MenuChild("Ativações", ImVec2(320, 482));
        {
            ImGui::Spacing();
            ImGui::Checkbox("Ativar Auto Slide", &g_MenuState.luaSlide.enabled);

            ImGui::Spacing();
            if (ImGui::Checkbox("Ativar KFC Slide", &g_MenuState.kfcSlide.enabled))
            {
                if (g_MenuState.kfcSlide.enabled)
                    PlayerSlap::ShowToast("[KFC Slide] ATIVADO (ON)", 0xFF00FF88, 3000);
                else
                    PlayerSlap::ShowToast("[KFC Slide] DESATIVADO (OFF)", 0xFFFF4444, 3000);
            }

            ImGui::Spacing();
            ImGui::Checkbox("Ativar Fast Switch", &g_MenuState.fastSwitch.enabled);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::Checkbox("Ativar Auto Punch", &g_MenuState.autoPunch.enabled))
            {
                if (g_MenuState.autoPunch.enabled)
                    PlayerSlap::ShowToast("[AutoPunch] ATIVADO (ON)", 0xFF00FF88, 3000);
                else
                    PlayerSlap::ShowToast("[AutoPunch] DESATIVADO (OFF)", 0xFFFF4444, 3000);
            }

            if (g_MenuState.autoPunch.enabled)
            {
                ImGui::Spacing();
                ImGui::SliderInt("Delay do Soco", &g_MenuState.autoPunch.delayMs, 20, 300, "%d ms");
                ImGui::SliderInt("Cooldown do Soco", &g_MenuState.autoPunch.cooldownMs, 100, 800, "%d ms");
            }
        }
        ImGui::EndChild();

        // ── Coluna 2 (Direita - Topo): KFC Slide com regulador de velocidade ──
        ImGui::SetCursorPos(ImVec2(505, 38));
        ImGui::MenuChild("KFC Slide", ImVec2(320, 130));
        {
            ImGui::Spacing();
            ImGui::SliderFloat("Velocidade", &g_MenuState.kfcSlide.speed, 1.0f, 10.0f, "%.1fx");
        }
        ImGui::EndChild();

        // ── Coluna 2 (Direita - Base): Configuráveis do Slide (Delays por Arma) ──
        ImGui::SetCursorPos(ImVec2(505, 180));
        ImGui::MenuChild("Configurações do Slide", ImVec2(320, 340));
        {
            ImGui::Spacing();
            ImGui::SliderInt("Sniper", &g_MenuState.luaSlide.marginSnp, 0, 1000, "%d ms");
            ImGui::Spacing();
            ImGui::SliderInt("Desert Eagle", &g_MenuState.luaSlide.marginDesert, 0, 1000, "%d ms");
            ImGui::Spacing();
            ImGui::SliderInt("M4", &g_MenuState.luaSlide.marginM4, 0, 1000, "%d ms");
            ImGui::Spacing();
            ImGui::SliderInt("AK-47", &g_MenuState.luaSlide.marginAK, 0, 1000, "%d ms");
            ImGui::Spacing();
            ImGui::SliderInt("Shotgun", &g_MenuState.luaSlide.marginShot, 0, 1000, "%d ms");
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
                Config::SaveToCloud(name, s_ConfigStatusMsg);
                s_ConfigStatusTime = GetTickCount64();
            }
            if (ImGui::Button("LOAD CONFIG (CLOUD)", ImVec2(280, 28)))
            {
                std::string name = g_MenuState.misc.configName;
                if (name.empty()) name = "default";
                Config::LoadFromCloud(name, s_ConfigStatusMsg);
                s_ConfigStatusTime = GetTickCount64();
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
                    s_ConfigStatusMsg = "Config salva: " + name + ".json";
                else
                    s_ConfigStatusMsg = "Falha ao salvar config.";
                s_ConfigStatusTime = GetTickCount64();
            }
            ImGui::SameLine();
            if (ImGui::Button("LOAD CONFIG (LOCAL)", ImVec2(136, 26)))
            {
                std::string name = g_MenuState.misc.configName;
                if (name.empty()) name = "somalia_config";
                if (ConfigManager::LoadConfig(name))
                    s_ConfigStatusMsg = "Config carregada: " + name + ".json";
                else
                    s_ConfigStatusMsg = "Falha ao carregar config.";
                s_ConfigStatusTime = GetTickCount64();
            }

            if (ImGui::Button("DELETE CONFIG", ImVec2(136, 24)))
            {
                std::string name = g_MenuState.misc.configName;
                if (!name.empty() && ConfigManager::DeleteConfig(name))
                    s_ConfigStatusMsg = "Config deletada: " + name + ".json";
                s_ConfigStatusTime = GetTickCount64();
            }
            ImGui::SameLine();
            if (ImGui::Button("REFRESH", ImVec2(136, 24)))
            {
                ConfigManager::Refresh();
                s_ConfigStatusMsg = "Lista atualizada.";
                s_ConfigStatusTime = GetTickCount64();
            }

            ImGui::Spacing();
            ImGui::TextColored(Theme::TextMuted, "Configs Disponiveis:");
            const auto& configs = ConfigManager::GetConfigList();
            ImGui::BeginChild("##cfgList", ImVec2(280, 100), true);
            for (size_t i = 0; i < configs.size(); i++)
            {
                bool isSelected = (s_ConfigSelectedIdx == static_cast<int>(i)) ||
                                  (strcmp(g_MenuState.misc.configName, configs[i].c_str()) == 0);
                if (ImGui::Selectable(configs[i].c_str(), isSelected))
                {
                    s_ConfigSelectedIdx = static_cast<int>(i);
                    strncpy_s(g_MenuState.misc.configName, configs[i].c_str(), sizeof(g_MenuState.misc.configName) - 1);
                }
            }
            ImGui::EndChild();

            if (!s_ConfigStatusMsg.empty() && (GetTickCount64() - s_ConfigStatusTime < 5000))
            {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.2f, 0.9f, 0.4f, 1.0f), s_ConfigStatusMsg.c_str());
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

            ImGui::TextColored(Theme::AccentColor, "TEMA & COR DO MENU");
            ImGui::Text("Cor Atual:");
            ImGui::SameLine();
            if (ImGui::ColorButton("##CurrentAccentBtn", ImVec4(accent_colour[0], accent_colour[1], accent_colour[2], 1.0f), ImGuiColorEditFlags_NoTooltip, ImVec2(34, 20)))
            {
                s_ShowColorSelector = !s_ShowColorSelector;
            }
            ImGui::SameLine();
            if (ImGui::Button(s_ShowColorSelector ? "Fechar Seletor" : "Selecionar Cor...", ImVec2(130, 20)))
            {
                s_ShowColorSelector = !s_ShowColorSelector;
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::TextColored(Theme::AccentColor, "SEGURANCA & DISCRICAO");
            ImGui::Checkbox("StreamProof (Bypass OBS / Discord / Print)", &g_MenuState.misc.streamProof);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            if (ImGui::Button("Resetar Padroes", ImVec2(280, 26)))
            {
                Config::ResetToDefaults();
                s_ConfigStatusMsg = "Padroes restaurados.";
                s_ConfigStatusTime = GetTickCount64();
            }
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // ── BOTAO DE UNLOAD SEGURO ──
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
        int previousTab = g_MenuState.currentTab;

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
        if (ImGui::Rendertab("s", "Vehicles", g_MenuState.currentTab == 7)) g_MenuState.currentTab = 7;

        ImGui::SetCursorPos(ImVec2(13, 445));
        if (ImGui::Rendertab("f", "Slide & KFC", g_MenuState.currentTab == 8)) g_MenuState.currentTab = 8;

        ImGui::SetCursorPos(ImVec2(13, 483));
        if (ImGui::Rendertab("c", "Configs", g_MenuState.currentTab == 9)) g_MenuState.currentTab = 9;

        // Renderização estritamente exclusiva: apenas UMA página renderizada por frame
        if (previousTab != g_MenuState.currentTab || s_LastAnimatedTab != g_MenuState.currentTab)
        {
            s_ContentAlpha = 0.0f;
            s_LastAnimatedTab = g_MenuState.currentTab;
        }

        s_ContentAlpha = ImLerp(s_ContentAlpha, 1.0f, 12.0f * ImGui::GetIO().DeltaTime);
        float slideOffset = (1.0f - s_ContentAlpha) * 14.0f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + slideOffset);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * s_ContentAlpha);

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
            case 8: RenderSlideTab(); break;           // KFC Slide
            case 9: RenderConfigsTab(); break;         // Configs
            default: RenderLegitBotTab(); break;
            }
        }
        ImGui::PopStyleVar();
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

        // Janela flutuante "Color Selector" (conforme solicitado pelo usuário)
        if (s_ShowColorSelector)
        {
            ImVec2 centerPos = ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f + 440.0f, ImGui::GetIO().DisplaySize.y * 0.5f);
            if (centerPos.x + 320.0f > ImGui::GetIO().DisplaySize.x)
                centerPos.x = ImGui::GetIO().DisplaySize.x * 0.5f;

            ImGui::SetNextWindowPos(centerPos, ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
            ImGui::SetNextWindowSize(ImVec2(320.0f, 370.0f), ImGuiCond_FirstUseEver);

            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, s_OpenAlpha);
            ImGui::PushStyleColor(ImGuiCol_Border, Theme::AccentColor);
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(20.f / 255.f, 22.f / 255.f, 26.f / 255.f, 0.98f));
            ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(26.f / 255.f, 28.f / 255.f, 32.f / 255.f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(32.f / 255.f, 36.f / 255.f, 42.f / 255.f, 1.0f));

            if (ImGui::Begin("Color Selector", &s_ShowColorSelector, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGuiColorEditFlags picker_flags = ImGuiColorEditFlags_NoAlpha | 
                                                   ImGuiColorEditFlags_PickerHueBar | 
                                                   ImGuiColorEditFlags_DisplayRGB | 
                                                   ImGuiColorEditFlags_DisplayHSV |
                                                   ImGuiColorEditFlags_NoSidePreview;

                ImGui::SetNextItemWidth(260.0f);
                if (ImGui::ColorPicker3("##ColorSelectorPicker", accent_colour, picker_flags))
                {
                    Theme::SetAccentColor(accent_colour[0], accent_colour[1], accent_colour[2], 1.0f);
                    g_MenuState.misc.accentColor[0] = accent_colour[0];
                    g_MenuState.misc.accentColor[1] = accent_colour[1];
                    g_MenuState.misc.accentColor[2] = accent_colour[2];
                    g_MenuState.misc.accentColor[3] = 1.0f;
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::Button("Fechar", ImVec2(ImGui::GetContentRegionAvail().x, 26)))
                {
                    s_ShowColorSelector = false;
                }
            }
            ImGui::End();

            ImGui::PopStyleColor(4);
            ImGui::PopStyleVar(3);
        }
    }
}
