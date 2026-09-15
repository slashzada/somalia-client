#include "LoaderMenu.h"
#include "imgui_internal.h"
#include "../Auth/KeyAuth.h"
#include "../Config/LoaderConfig.h"
#include "../Injector/Injector.h"
#pragma warning(push)
#pragma warning(disable: 4828)
#include "Fonts/bytearray.h"
#pragma warning(pop)
#include <shlobj.h>
#include <commctrl.h>
#include <string>
#include <thread>
#include <vector>
#include <map>
#include <cmath>
#include <algorithm>

#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_NONE             0x00000000
#define WDA_MONITOR          0x00000001
#define WDA_EXCLUDEFROMCAPTURE 0x00000011
#endif

typedef BOOL(WINAPI* tSetWindowDisplayAffinity)(HWND, DWORD);

namespace LoaderMenu
{
    static HWND s_hWnd = NULL;
    static IDirect3DDevice9* s_pDevice = nullptr;

    // Gerenciador de Telas e Crossfade
    static Screen s_CurrentScreen = Screen::Login;
    static Screen s_TargetScreen = Screen::Login;
    static float s_CrossfadeAlpha = 1.0f;
    static bool s_IsSwitchingScreen = false;

    static char s_InputUser[64] = { 0 };
    static char s_InputPass[64] = { 0 };
    static char s_InputKey[64]  = { 0 };
    static std::string s_StatusText = "";
    static ImVec4 s_StatusColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
    static bool s_IsAuthenticating = false;
    static bool s_bLastLoaderStreamProof = false;

    static KeyAuthClient* s_pKeyAuth = nullptr;

    // Fontes
    static ImFont* s_FontTitle  = nullptr;
    static ImFont* s_FontButton = nullptr;
    static ImFont* s_FontBody   = nullptr;
    static ImFont* s_FontSmall  = nullptr;
    static ImFont* s_FontLogo   = nullptr;

    // Transição de abertura
    static float s_OpenAlpha = 0.0f;

    // =========================================================================
    // CORES (Paleta Dark Minimalist com Sombreamentos Finais)
    // =========================================================================
    static const ImU32 ColWindowBgTop  = IM_COL32(24, 25, 29, 255);
    static const ImU32 ColWindowBgBot  = IM_COL32(15, 16, 19, 255);
    static const ImU32 ColCardBgTop    = IM_COL32(31, 33, 38, 255);
    static const ImU32 ColCardBgBot    = IM_COL32(23, 24, 28, 255);
    static const ImU32 ColCardBorder   = IM_COL32(40, 42, 49, 255);
    static const ImU32 ColTextWhite    = IM_COL32(255, 255, 255, 255);
    static const ImU32 ColTextMuted    = IM_COL32(114, 119, 131, 255);
    static const ImVec4 ColTextMutedVec4 = ImVec4(114.f / 255.f, 119.f / 255.f, 131.f / 255.f, 1.0f);
    static const ImU32 ColTextSecondary= IM_COL32(170, 175, 185, 255);
    static const ImU32 ColButtonWhite  = IM_COL32(255, 255, 255, 255);
    static const ImU32 ColButtonText   = IM_COL32(20, 21, 24, 255);
    static const ImU32 ColAccent       = IM_COL32(137, 207, 240, 255);

    // =========================================================================
    // MOTOR TYPEWRITER ("A de escrever")
    // =========================================================================
    struct TypewriterEffect
    {
        std::string fullText;
        float charSpeed = 0.045f;
        float startTime = 0.0f;
        bool active = false;

        void Play(const std::string& text, float speed = 0.045f)
        {
            fullText = text;
            charSpeed = speed;
            startTime = (float)ImGui::GetTime();
            active = true;
        }

        std::string GetText(bool showCaret = true)
        {
            if (!active || fullText.empty())
                return fullText;

            float elapsed = (float)ImGui::GetTime() - startTime;
            int count = (int)(elapsed / charSpeed);
            if (count >= (int)fullText.length())
            {
                bool blink = ((int)(ImGui::GetTime() * 2.2f) % 2) == 0;
                return fullText + (showCaret && blink ? " |" : "");
            }
            std::string current = fullText.substr(0, count);
            bool blink = ((int)(ImGui::GetTime() * 4.0f) % 2) == 0;
            if (showCaret && blink)
                current += " |";
            return current;
        }
    };

    static TypewriterEffect s_TitleTypewriter;
    static TypewriterEffect s_SubtitleTypewriter;
    static TypewriterEffect s_LoadingTypewriter;
    static TypewriterEffect s_StatusTypewriter;

    // Loading State
    static float s_LoadingStartTime = 0.0f;
    static float s_LoadingDuration = 1.3f;
    static Screen s_LoadingNextScreen = Screen::Dashboard;

    // Injection State
    static bool s_IsInjectingAnim = false;
    static std::string s_InjectStatusMessage = "";
    static float s_InjectStatusTime = 0.0f;

    // Função para alternar telas com Crossfade suave
    static void SwitchScreen(Screen s)
    {
        if (s_CurrentScreen == s && !s_IsSwitchingScreen)
            return;

        s_TargetScreen = s;
        s_IsSwitchingScreen = true;
    }

    static void ApplyLoaderStreamProof(bool enabled)
    {
        if (!s_hWnd) return;
        HMODULE hUser32 = GetModuleHandleA("user32.dll");
        if (!hUser32) return;
        tSetWindowDisplayAffinity pSet = (tSetWindowDisplayAffinity)GetProcAddress(hUser32, "SetWindowDisplayAffinity");
        if (!pSet) return;
        DWORD affinity = enabled ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE;
        BOOL ok = pSet(s_hWnd, affinity);
        if (!ok && enabled)
        {
            pSet(s_hWnd, WDA_MONITOR);
        }
        s_bLastLoaderStreamProof = enabled;
    }

    // =========================================================================
    // DROP SHADOW DIFUSA / GLOW (Multi-pass com opacidade gradativa)
    // =========================================================================
    static void RenderDropShadow(ImVec2 min, ImVec2 max, float rounding, int passes = 6, float spread = 8.0f)
    {
        ImDrawList* draw = ImGui::GetWindowDrawList();
        for (int i = passes; i >= 1; i--)
        {
            float d = (float)i * (spread / (float)passes);
            int alpha = (int)(22.0f * (1.0f - (float)i / (float)(passes + 1)));
            draw->AddRect(
                ImVec2(min.x - d, min.y - d),
                ImVec2(max.x + d, max.y + d),
                IM_COL32(0, 0, 0, alpha),
                rounding + d * 0.5f,
                0,
                1.5f
            );
        }
    }

    // =========================================================================
    // SISTEMA DE PARTÍCULAS (Com física, wrap-around e pulsação suave)
    // =========================================================================
    struct Particle
    {
        ImVec2 pos;
        ImVec2 vel;
        float radius;
        float baseAlpha;
        float phase;
    };

    static const int PARTICLE_COUNT = 45;
    static Particle s_Particles[PARTICLE_COUNT];
    static bool s_ParticlesInitialized = false;

    static void RenderParticles()
    {
        ImVec2 size = ImGui::GetIO().DisplaySize;
        if (size.x <= 0 || size.y <= 0) return;

        float dt = ImGui::GetIO().DeltaTime;
        if (dt <= 0.0f || dt > 0.1f) dt = 0.016f;

        if (!s_ParticlesInitialized)
        {
            for (int i = 0; i < PARTICLE_COUNT; i++)
            {
                s_Particles[i].pos = ImVec2((float)(rand() % (int)size.x), (float)(rand() % (int)size.y));
                s_Particles[i].vel = ImVec2(((rand() % 100) / 100.0f - 0.5f) * 7.5f, ((rand() % 100) / 100.0f - 0.5f) * 7.5f);
                s_Particles[i].radius = 1.0f + ((rand() % 100) / 100.0f) * 1.3f;
                s_Particles[i].baseAlpha = 0.40f + ((rand() % 100) / 100.0f) * 0.50f;
                s_Particles[i].phase = ((rand() % 100) / 100.0f) * 6.28f;
            }
            s_ParticlesInitialized = true;
        }

        ImDrawList* draw = ImGui::GetWindowDrawList();
        float time = (float)ImGui::GetTime();

        for (int i = 0; i < PARTICLE_COUNT; i++)
        {
            Particle& p = s_Particles[i];
            p.pos.x += p.vel.x * dt;
            p.pos.y += p.vel.y * dt;

            // Wrap-around nas bordas da janela
            if (p.pos.x < 0.0f) p.pos.x = size.x;
            else if (p.pos.x > size.x) p.pos.x = 0.0f;

            if (p.pos.y < 0.0f) p.pos.y = size.y;
            else if (p.pos.y > size.y) p.pos.y = 0.0f;

            // Pulsação suave do brilho
            float pulse = 0.60f + 0.40f * sinf(time * 2.2f + p.phase);
            float alpha = ImClamp(p.baseAlpha * pulse * s_OpenAlpha, 0.0f, 1.0f);

            draw->AddCircleFilled(p.pos, p.radius, IM_COL32(255, 255, 255, (int)(alpha * 255.0f)));
        }
    }

    // =========================================================================
    // SPINNER CIRCULAR DINÂMICO (PathArcTo com rotação e variação trigonométrica)
    // =========================================================================
    static void RenderArcSpinner(ImVec2 center, float radius, float thickness = 2.4f)
    {
        ImDrawList* draw = ImGui::GetWindowDrawList();
        float t = (float)ImGui::GetTime() * 6.8f;
        float arcLen = 1.6f + 0.45f * sinf((float)ImGui::GetTime() * 3.2f);

        draw->PathClear();
        draw->PathArcTo(center, radius, t, t + arcLen, 36);
        draw->PathStroke(IM_COL32(255, 255, 255, 235), 0, thickness);
    }

    // =========================================================================
    // BOTÃO FECHAR 'X'
    // =========================================================================
    static void RenderCloseButton(float posX, float posY)
    {
        ImVec2 btnPos(posX, posY);
        ImVec2 btnSize(20.0f, 20.0f);
        ImGui::SetCursorPos(btnPos);
        bool clicked = ImGui::InvisibleButton("##win_close_btn", btnSize);
        bool hovered = ImGui::IsItemHovered();
        if (hovered)
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        if (clicked)
            PostQuitMessage(0);

        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 p0 = ImGui::GetItemRectMin();
        ImVec2 p1 = ImGui::GetItemRectMax();

        ImU32 col = hovered ? IM_COL32(255, 255, 255, 255) : IM_COL32(115, 120, 132, 255);
        float pad = 3.5f;
        draw->AddLine(ImVec2(p0.x + pad, p0.y + pad), ImVec2(p1.x - pad, p1.y - pad), col, 1.8f);
        draw->AddLine(ImVec2(p1.x - pad, p0.y + pad), ImVec2(p0.x + pad, p1.y - pad), col, 1.8f);
    }

    // =========================================================================
    // TOGGLE SWITCH ANIMADO (Pílula com Imlerp)
    // =========================================================================
    static bool RenderPillToggle(const char* id, bool* value, ImVec2 pos)
    {
        ImVec2 size(38.0f, 20.0f);
        ImGui::SetCursorPos(pos);
        bool clicked = ImGui::InvisibleButton(id, size);
        if (ImGui::IsItemHovered())
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        if (clicked && value)
            *value = !(*value);

        ImDrawList* draw = ImGui::GetWindowDrawList();
        ImVec2 p0 = ImGui::GetItemRectMin();
        ImVec2 p1 = ImGui::GetItemRectMax();

        ImGuiID itemId = ImGui::GetID(id);
        static std::map<ImGuiID, float> s_ToggleAnims;
        auto it = s_ToggleAnims.find(itemId);
        if (it == s_ToggleAnims.end())
        {
            s_ToggleAnims[itemId] = (value && *value) ? 1.0f : 0.0f;
        }
        float& anim = s_ToggleAnims[itemId];
        float dt = ImGui::GetIO().DeltaTime;
        if (dt <= 0.0f) dt = 0.016f;
        anim = ImLerp(anim, (value && *value) ? 1.0f : 0.0f, 14.0f * dt);

        // Fundo do trilho da pílula
        int rBg = (int)ImLerp(42.0f, 255.0f, anim);
        int gBg = (int)ImLerp(44.0f, 255.0f, anim);
        int bBg = (int)ImLerp(51.0f, 255.0f, anim);
        draw->AddRectFilled(p0, p1, IM_COL32(rBg, gBg, bBg, 255), 10.0f);

        // Knob circular deslizante
        float knobRadius = 7.0f;
        float knobMinX = p0.x + 3.0f + knobRadius;
        float knobMaxX = p1.x - 3.0f - knobRadius;
        float knobX = ImLerp(knobMinX, knobMaxX, anim);
        float knobY = p0.y + size.y * 0.5f;

        int rKnob = (int)ImLerp(142.0f, 20.0f, anim);
        int gKnob = (int)ImLerp(146.0f, 21.0f, anim);
        int bKnob = (int)ImLerp(158.0f, 24.0f, anim);
        draw->AddCircleFilled(ImVec2(knobX, knobY), knobRadius, IM_COL32(rKnob, gKnob, bKnob, 255), 24);

        return clicked;
    }

    // =========================================================================
    // BOTÃO BRANCO DE ALTO CONTRASTE (Com ImLerp Hover & Click)
    // =========================================================================
    static bool RenderWhiteButton(const char* label, ImVec2 size, ImVec2 pos)
    {
        if (pos.x >= 0.0f && pos.y >= 0.0f)
            ImGui::SetCursorPos(pos);

        bool clicked = ImGui::InvisibleButton(label, size);
        bool hovered = ImGui::IsItemHovered();
        bool active  = ImGui::IsItemActive();
        if (hovered)
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

        ImVec2 p0 = ImGui::GetItemRectMin();
        ImVec2 p1 = ImGui::GetItemRectMax();
        ImDrawList* draw = ImGui::GetWindowDrawList();

        ImGuiID id = ImGui::GetID(label);
        static std::map<ImGuiID, float> s_BtnAnims;
        float& anim = s_BtnAnims[id];
        float dt = ImGui::GetIO().DeltaTime;
        if (dt <= 0.0f) dt = 0.016f;
        anim = ImLerp(anim, active ? 0.7f : (hovered ? 1.0f : 0.0f), 12.0f * dt);

        float pressOffset = active ? 1.0f : 0.0f;
        p0.x += pressOffset; p0.y += pressOffset;
        p1.x += pressOffset; p1.y += pressOffset;

        int btnR = (int)ImLerp(255.0f, 235.0f, anim);
        int btnG = (int)ImLerp(255.0f, 238.0f, anim);
        int btnB = (int)ImLerp(255.0f, 242.0f, anim);

        // Fundo branco com cantos arredondados
        draw->AddRectFilled(p0, p1, IM_COL32(btnR, btnG, btnB, 255), 8.0f);

        // Texto preto centralizado
        const char* textEnd = strchr(label, '#');
        if (!textEnd) textEnd = label + strlen(label);
        std::string cleanText(label, textEnd);

        if (s_FontButton) ImGui::PushFont(s_FontButton);
        ImVec2 textSize = ImGui::CalcTextSize(cleanText.c_str());
        ImVec2 textPos(p0.x + (size.x - textSize.x) * 0.5f, p0.y + (size.y - textSize.y) * 0.5f);
        draw->AddText(textPos, ColButtonText, cleanText.c_str());
        if (s_FontButton) ImGui::PopFont();

        return clicked;
    }

    // =========================================================================
    // =========================================================================
    // EFEITO TYPEWRITER NO INPUT: DETECÇÃO DE DIGITAÇÃO, CURSOR '|' E IMPACTO
    // =========================================================================
    struct InputTypewriterState
    {
        float lastKeystrokeTime = 0.0f;
        float strikeAnim = 0.0f;
        float cursorAnimX = 0.0f;
        int   lastLen = 0;
    };
    static std::map<ImGuiID, InputTypewriterState> s_TypewriterStates;

    static bool RenderStyledInput(const char* id, char* buf, int bufSize, const char* placeholder, float width, float height, ImGuiInputTextFlags flags = 0)
    {
        ImGui::SetNextItemWidth(width);
        float fontSize = ImGui::GetFontSize();
        float padY = (height - fontSize) * 0.5f;
        if (padY < 4.0f) padY = 4.0f;

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12.0f, padY));

        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(27, 28, 32, 255));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(31, 32, 38, 255));
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, IM_COL32(34, 35, 42, 255));
        ImGui::PushStyleColor(ImGuiCol_Border, ColCardBorder);
        ImGui::PushStyleColor(ImGuiCol_Text, ColTextWhite);

        ImGuiID itemId = ImGui::GetID(id);
        InputTypewriterState& tw = s_TypewriterStates[itemId];

        bool changed = ImGui::InputText(id, buf, bufSize, flags);

        int curLen = (int)strlen(buf);
        ImVec2 rMin = ImGui::GetItemRectMin();
        ImVec2 rMax = ImGui::GetItemRectMax();
        bool active = ImGui::IsItemActive();
        bool hovered = ImGui::IsItemHovered();

        ImGui::PopStyleColor(5);
        ImGui::PopStyleVar(3);

        ImDrawList* draw = ImGui::GetWindowDrawList();
        float dt = ImGui::GetIO().DeltaTime;
        if (dt <= 0.0f) dt = 0.016f;

        static std::map<ImGuiID, float> s_InputAnims;
        float& focusAnim = s_InputAnims[itemId];
        focusAnim = ImLerp(focusAnim, (active || hovered) ? 1.0f : 0.0f, 10.0f * dt);

        if (focusAnim > 0.01f)
        {
            draw->AddRect(rMin, rMax, IM_COL32(95, 100, 115, (int)(180 * focusAnim)), 8.0f, 0, 1.0f);
        }

        // =====================================================================
        // EFEITO TYPEWRITER: DETECÇÃO DO USUÁRIO ESCREVENDO
        // =====================================================================
        float now = (float)ImGui::GetTime();
        if (active)
        {
            if (changed || curLen != tw.lastLen)
            {
                tw.lastKeystrokeTime = now;
                tw.strikeAnim = 1.0f; // Dispara impacto visual da tecla
                tw.lastLen = curLen;
            }
        }
        else
        {
            tw.lastLen = curLen;
        }

        tw.strikeAnim = ImLerp(tw.strikeAnim, 0.0f, 10.0f * dt);

        // Pulso visual do impacto da tecla na borda do campo (Typewriter Strike Glow)
        if (tw.strikeAnim > 0.01f)
        {
            draw->AddRect(rMin, rMax, IM_COL32(137, 207, 240, (int)(160.0f * tw.strikeAnim)), 8.0f, 0, 1.5f);
        }

        // Placeholder (visível se vazio; esmaecido se focado)
        if (curLen == 0)
        {
            ImU32 phCol = active ? IM_COL32(90, 95, 108, 190) : ColTextMuted;
            draw->AddText(ImVec2(rMin.x + 12.0f, rMin.y + padY), phCol, placeholder);
        }

        // =====================================================================
        // CURSOR TYPEWRITER ANIMADO (|) NA PONTA DO TEXTO ENQUANTO DIGITA
        // =====================================================================
        if (active)
        {
            ImGuiInputTextState* state = ImGui::GetInputTextState(itemId);
            int cursorPos = state ? state->GetCursorPos() : curLen;
            float scrollX = state ? state->ScrollX : 0.0f;

            // Calcula a largura do texto até a posição do cursor
            std::string textBefore;
            if (flags & ImGuiInputTextFlags_Password)
            {
                int safeP = ImClamp(cursorPos, 0, curLen);
                textBefore.assign(safeP, '*');
            }
            else
            {
                int safeP = ImClamp(cursorPos, 0, curLen);
                textBefore.assign(buf, safeP);
            }

            ImVec2 txtSz = ImGui::CalcTextSize(textBefore.c_str());
            float targetCursorX = rMin.x + 12.0f + txtSz.x - scrollX;
            float cursorY1 = rMin.y + padY;
            float cursorY2 = cursorY1 + fontSize;

            if (tw.cursorAnimX == 0.0f)
                tw.cursorAnimX = targetCursorX;
            tw.cursorAnimX = ImLerp(tw.cursorAnimX, targetCursorX, 32.0f * dt);

            // Animação de piscar do cursor estilo máquina de escrever / terminal
            float timeSinceTyping = now - tw.lastKeystrokeTime;
            float cursorAlpha = 1.0f;

            // Fica 100% visível e sólido durante a digitação ativa (sem sumir no meio de uma letra)
            if (timeSinceTyping < 0.35f)
            {
                cursorAlpha = 1.0f;
            }
            else
            {
                // Pisca com cadência nítida de typewriter
                float blink = 0.5f + 0.5f * sinf(now * 7.5f);
                cursorAlpha = (blink > 0.45f) ? 1.0f : 0.0f;
            }

            if (cursorAlpha > 0.01f && targetCursorX >= rMin.x + 8.0f && targetCursorX <= rMax.x - 8.0f)
            {
                float extraH = tw.strikeAnim * 2.0f;
                float cX = tw.cursorAnimX;

                // Brilho difuso do cursor
                draw->AddRectFilled(
                    ImVec2(cX - 1.0f, cursorY1 - 1.0f - extraH * 0.5f),
                    ImVec2(cX + 3.0f, cursorY2 + 1.0f + extraH * 0.5f),
                    IM_COL32(137, 207, 240, (int)(80.0f * cursorAlpha)),
                    1.0f
                );

                // Barra vertical sólida typewriter |
                draw->AddRectFilled(
                    ImVec2(cX, cursorY1 - extraH * 0.5f),
                    ImVec2(cX + 2.0f, cursorY2 + extraH * 0.5f),
                    IM_COL32(255, 255, 255, (int)(255.0f * cursorAlpha)),
                    1.0f
                );
            }
        }

        return changed;
    }

    // =========================================================================
    // LOGO "SOMALIA"
    // =========================================================================
    static void RenderLogo(ImVec2 center)
    {
        ImDrawList* draw = ImGui::GetWindowDrawList();

        const char* logoText = "SOMALIA";
        if (s_FontLogo) ImGui::PushFont(s_FontLogo);
        else if (s_FontTitle) ImGui::PushFont(s_FontTitle);

        ImVec2 sz = ImGui::CalcTextSize(logoText);
        ImVec2 tPos(center.x - sz.x * 0.5f, center.y - sz.y * 0.5f - 8.0f);

        // Sombra suave do logo
        draw->AddText(ImVec2(tPos.x + 1.0f, tPos.y + 1.0f), IM_COL32(0, 0, 0, 160), logoText);
        // Texto branco
        draw->AddText(tPos, ColTextWhite, logoText);

        if (s_FontLogo || s_FontTitle) ImGui::PopFont();

        // Subtítulo elegante estático
        const char* subText = "Authentication";
        if (s_FontSmall) ImGui::PushFont(s_FontSmall);
        ImVec2 subSz = ImGui::CalcTextSize(subText);
        draw->AddText(ImVec2(center.x - subSz.x * 0.5f, tPos.y + sz.y + 4.0f), ColTextMuted, subText);
        if (s_FontSmall) ImGui::PopFont();
    }

    // =========================================================================
    // RESOLUÇÃO DO CAMINHO DO ASI / DLL
    // =========================================================================
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
        if (GetFileAttributesA("SomaliaNative.dll") != INVALID_FILE_ATTRIBUTES)
            return "SomaliaNative.dll";
        if (GetFileAttributesA("dist\\SomaliaNative.asi") != INVALID_FILE_ATTRIBUTES)
            return "dist\\SomaliaNative.asi";
        if (GetFileAttributesA("SomaliaNative\\build\\SomaliaNative.asi") != INVALID_FILE_ATTRIBUTES)
            return "SomaliaNative\\build\\SomaliaNative.asi";

        std::string err;
        std::string extracted = Injector::GetOrExtractPayload(err);
        if (!extracted.empty())
            return extracted;

        return "SomaliaNative.asi";
    }

    // =========================================================================
    // INICIALIZAÇÃO
    // =========================================================================
    void Init(HWND hWnd, IDirect3DDevice9* pDevice)
    {
        s_hWnd = hWnd;
        s_pDevice = pDevice;

        ConfigManager::Load();
        LoaderConfig& cfg = ConfigManager::Get();
        if (!cfg.lastUsername.empty())
            strncpy_s(s_InputUser, cfg.lastUsername.c_str(), sizeof(s_InputUser) - 1);

        s_bLastLoaderStreamProof = cfg.streamProof;
        if (cfg.streamProof)
            ApplyLoaderStreamProof(true);

        s_pKeyAuth = new KeyAuthClient(cfg.keyauthName, cfg.keyauthOwner, cfg.keyauthSecret, cfg.keyauthVersion);
        s_pKeyAuth->Init();
    }

    void SetupFonts()
    {
        ImGuiIO& io = ImGui::GetIO();

        static const ImWchar ranges[] =
        {
            0x0020, 0x00FF,
            0x0400, 0x052F,
            0x2DE0, 0x2DFF,
            0xA640, 0xA69F,
            0xE000, 0xE226,
            0,
        };

        ImFontConfig font_config;
        font_config.PixelSnapH = false;
        font_config.OversampleH = 4;
        font_config.OversampleV = 4;
        font_config.RasterizerMultiply = 1.15f;
        font_config.GlyphRanges = ranges;

        s_FontBody   = io.Fonts->AddFontFromMemoryTTF(poppin_font, sizeof(poppin_font), 16.0f, &font_config, ranges);
        s_FontLogo   = io.Fonts->AddFontFromMemoryTTF(poppin_font, sizeof(poppin_font), 36.0f, &font_config, ranges);
        s_FontTitle  = io.Fonts->AddFontFromMemoryTTF(poppin_font, sizeof(poppin_font), 22.0f, &font_config, ranges);
        s_FontButton = io.Fonts->AddFontFromMemoryTTF(poppin_font, sizeof(poppin_font), 17.0f, &font_config, ranges);
        s_FontSmall  = io.Fonts->AddFontFromMemoryTTF(poppin_font, sizeof(poppin_font), 13.5f, &font_config, ranges);

        if (!s_FontBody)   s_FontBody   = io.Fonts->AddFontDefault();
        if (!s_FontLogo)   s_FontLogo   = s_FontBody;
        if (!s_FontTitle)  s_FontTitle  = s_FontBody;
        if (!s_FontButton) s_FontButton = s_FontBody;
        if (!s_FontSmall)  s_FontSmall  = s_FontBody;
    }

    Screen GetCurrentScreen() { return s_CurrentScreen; }
    void SetCurrentScreen(Screen s)
    {
        SwitchScreen(s);
    }

    void UpdateWindowSize(Screen s) {}

    // =========================================================================
    // TELA 1: LOGIN & REGISTRO (Com alternância corrigida sem bugs)
    // =========================================================================
    static void RenderLoginScreen()
    {
        ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        float winW = displaySize.x;
        float winH = displaySize.y;

        // Botão Fechar
        RenderCloseButton(winW - 32.0f, 14.0f);

        bool isRegister = (s_CurrentScreen == Screen::Register);

        // Logo central
        float logoCenterY = isRegister ? 90.0f : 110.0f;
        RenderLogo(ImVec2(winW * 0.5f, logoCenterY));

        // Dimensões dos Inputs e Botão (Escala proporcional para 580x500)
        float fieldW = 340.0f;
        float fieldH = 46.0f;
        float fieldX = (winW - fieldW) * 0.5f;

        float userY = isRegister ? 145.0f : 180.0f;
        float passY = isRegister ? (userY + fieldH + 10.0f) : (userY + fieldH + 12.0f);
        float keyY  = passY + fieldH + 10.0f;
        float btnY  = isRegister ? (keyY + fieldH + 16.0f) : (passY + fieldH + 18.0f);

        // Input 1: Username
        ImGui::SetCursorPos(ImVec2(fieldX, userY));
        RenderStyledInput("##user_input", s_InputUser, sizeof(s_InputUser), "Username", fieldW, fieldH);

        // Input 2: Password
        ImGui::SetCursorPos(ImVec2(fieldX, passY));
        RenderStyledInput("##pass_input", s_InputPass, sizeof(s_InputPass), "Password", fieldW, fieldH, ImGuiInputTextFlags_Password);

        // Input 3: License Key (somente no Registro)
        if (isRegister)
        {
            ImGui::SetCursorPos(ImVec2(fieldX, keyY));
            RenderStyledInput("##key_input", s_InputKey, sizeof(s_InputKey), "License Key", fieldW, fieldH);
        }

        // Mensagem de Status com Typewriter
        if (!s_StatusText.empty())
        {
            std::string st = s_StatusTypewriter.GetText(false);
            if (s_FontSmall) ImGui::PushFont(s_FontSmall);
            ImVec2 sz = ImGui::CalcTextSize(st.c_str());
            float stX = (winW - sz.x) * 0.5f;
            float stY = btnY + fieldH + 10.0f;
            ImGui::SetCursorPos(ImVec2(stX, stY));
            ImGui::TextColored(s_StatusColor, "%s", st.c_str());
            if (s_FontSmall) ImGui::PopFont();
        }

        // Botão Branco Principal: "Continue" (ou "Register Account")
        const char* btnLabel = isRegister ? "Register Account##btn_reg" : "Continue##btn_login";
        if (RenderWhiteButton(btnLabel, ImVec2(fieldW, fieldH), ImVec2(fieldX, btnY)) && !s_IsAuthenticating)
        {
            s_IsAuthenticating = true;
            s_StatusText = "";

            s_LoadingStartTime = (float)ImGui::GetTime();
            s_LoadingDuration = 1.2f;
            s_LoadingTypewriter.Play(isRegister ? "Creating account..." : "Authenticating...", 0.045f);
            SwitchScreen(Screen::Loading);

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

                        s_LoadingNextScreen = Screen::Dashboard;
                    }
                    else
                    {
                        s_StatusText = res.message;
                        s_StatusColor = ImVec4(255.f / 255.f, 90.f / 255.f, 95.f / 255.f, 1.0f);
                        s_StatusTypewriter.Play(s_StatusText, 0.035f);
                        s_LoadingNextScreen = Screen::Login;
                    }
                }
                else
                {
                    AuthResponse res = s_pKeyAuth->Register(s_InputUser, s_InputPass, s_InputKey);
                    if (res.success)
                    {
                        s_StatusText = res.message;
                        s_StatusColor = ImVec4(50.f / 255.f, 220.f / 255.f, 100.f / 255.f, 1.0f);
                        s_StatusTypewriter.Play(s_StatusText, 0.035f);
                        s_LoadingNextScreen = Screen::Login;
                    }
                    else
                    {
                        s_StatusText = res.message;
                        s_StatusColor = ImVec4(255.f / 255.f, 90.f / 255.f, 95.f / 255.f, 1.0f);
                        s_StatusTypewriter.Play(s_StatusText, 0.035f);
                        s_LoadingNextScreen = Screen::Register;
                    }
                }
                s_IsAuthenticating = false;
            });
            authThread.detach();
        }

        // Rodapé Alternador (Com hitbox generosa e sem colisão com window drag)
        float footerY = winH - 36.0f;
        {
            const char* promptText = (!isRegister) ? "Don't have an account? " : "Already have an account? ";
            const char* actionText = (!isRegister) ? "Register here" : "Login here";
            const char* btnId = (!isRegister) ? "##switch_to_register" : "##switch_to_login";

            if (s_FontSmall) ImGui::PushFont(s_FontSmall);
            ImVec2 sPrompt = ImGui::CalcTextSize(promptText);
            ImVec2 sAction = ImGui::CalcTextSize(actionText);

            float totalFooterW = sPrompt.x + sAction.x;
            float startX = (winW - totalFooterW) * 0.5f;

            // Texto estático
            ImDrawList* draw = ImGui::GetWindowDrawList();
            draw->AddText(ImVec2(startX, footerY + 2.0f), ColTextMuted, promptText);

            // Botão interativo do link
            ImVec2 linkPos(startX + sPrompt.x, footerY - 2.0f);
            ImVec2 linkSize(sAction.x + 8.0f, sAction.y + 8.0f);

            ImGui::SetCursorPos(linkPos);
            bool linkClicked = ImGui::InvisibleButton(btnId, linkSize);
            bool linkHovered = ImGui::IsItemHovered();

            if (linkHovered)
                ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

            if (linkClicked)
            {
                SwitchScreen((!isRegister) ? Screen::Register : Screen::Login);
                s_StatusText = "";
            }

            ImU32 linkCol = linkHovered ? ColTextWhite : IM_COL32(185, 192, 205, 255);
            draw->AddText(ImVec2(linkPos.x + 4.0f, footerY + 2.0f), linkCol, actionText);

            // Sublinhado sutil ao passar o cursor
            if (linkHovered)
            {
                draw->AddLine(
                    ImVec2(linkPos.x + 4.0f, footerY + sAction.y + 3.0f),
                    ImVec2(linkPos.x + 4.0f + sAction.x, footerY + sAction.y + 3.0f),
                    ColTextWhite, 1.0f
                );
            }

            if (s_FontSmall) ImGui::PopFont();
        }
    }

    // =========================================================================
    // TELA 2: LOADING SPINNER
    // =========================================================================
    static void RenderLoadingScreen()
    {
        ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        float winW = displaySize.x;
        float winH = displaySize.y;

        RenderCloseButton(winW - 32.0f, 14.0f);

        ImVec2 center(winW * 0.5f, winH * 0.5f - 14.0f);
        RenderArcSpinner(center, 24.0f, 3.2f);

        std::string loadTxt = s_LoadingTypewriter.GetText(true);
        if (s_FontSmall) ImGui::PushFont(s_FontSmall);
        ImVec2 sz = ImGui::CalcTextSize(loadTxt.c_str());
        ImGui::SetCursorPos(ImVec2((winW - sz.x) * 0.5f, center.y + 42.0f));
        ImGui::TextColored(ColTextMutedVec4, "%s", loadTxt.c_str());
        if (s_FontSmall) ImGui::PopFont();

        float elapsed = (float)ImGui::GetTime() - s_LoadingStartTime;
        if (!s_IsAuthenticating && !s_IsInjectingAnim && elapsed >= s_LoadingDuration)
        {
            SwitchScreen(s_LoadingNextScreen);
        }
    }

    // =========================================================================
    // TELA 3: DASHBOARD
    // =========================================================================
    static void RenderDashboardScreen()
    {
        ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        float winW = displaySize.x;
        float winH = displaySize.y;

        ImDrawList* draw = ImGui::GetWindowDrawList();

        // 1. Barra de Topo
        {
            const char* titleStr = "SOMALIA | Private";
            if (s_FontTitle) ImGui::PushFont(s_FontTitle);
            ImGui::SetCursorPos(ImVec2(28.0f, 18.0f));
            draw->AddText(s_FontTitle, 22.0f, ImVec2(28.0f, 18.0f), ColTextWhite, titleStr);
            if (s_FontTitle) ImGui::PopFont();
        }

        // Toggle Switch & Botão Fechar
        {
            LoaderConfig& cfg = ConfigManager::Get();
            if (RenderPillToggle("##stream_toggle", &cfg.streamProof, ImVec2(winW - 84.0f, 18.0f)))
            {
                ApplyLoaderStreamProof(cfg.streamProof);
                ConfigManager::Save();
            }

            RenderCloseButton(winW - 32.0f, 18.0f);
        }

        // 2. GRID DE AÇÕES: 2 Cards lado a lado com Gradiente Vertical
        float gridY = 64.0f;
        float marginX = 28.0f;
        float spacingX = 16.0f;
        float cardW = (winW - marginX * 2.0f - spacingX) * 0.5f;
        float cardH = 104.0f;

        // --- CARD 1: INJECT ---
        {
            float cX = marginX;
            ImVec2 cMin(cX, gridY);
            ImVec2 cMax(cX + cardW, gridY + cardH);

            // Sombra e Gradiente Vertical (Profundidade 3D)
            RenderDropShadow(cMin, cMax, 8.0f, 3, 4.0f);
            draw->AddRectFilledMultiColor(cMin, cMax, ColCardBgTop, ColCardBgTop, ColCardBgBot, ColCardBgBot);
            draw->AddRect(cMin, cMax, ColCardBorder, 8.0f, 0, 1.0f);
            draw->AddLine(ImVec2(cMin.x + 6, cMin.y + 1), ImVec2(cMax.x - 6, cMin.y + 1), IM_COL32(60, 64, 75, 120), 1.0f);

            // Cabeçalho
            if (s_FontBody) ImGui::PushFont(s_FontBody);
            draw->AddText(ImVec2(cMin.x + 18.0f, cMin.y + 14.0f), ColTextWhite, "Inject");
            if (s_FontBody) ImGui::PopFont();

            // Seta vetorial para baixo
            float arrowX = cMax.x - 24.0f;
            float arrowY = cMin.y + 20.0f;
            draw->AddLine(ImVec2(arrowX, arrowY - 4.5f), ImVec2(arrowX, arrowY + 4.5f), ColTextMuted, 1.6f);
            draw->AddLine(ImVec2(arrowX - 4.0f, arrowY + 0.5f), ImVec2(arrowX, arrowY + 4.5f), ColTextMuted, 1.6f);
            draw->AddLine(ImVec2(arrowX + 4.0f, arrowY + 0.5f), ImVec2(arrowX, arrowY + 4.5f), ColTextMuted, 1.6f);

            // Botão Branco: "Inject Here"
            float btnW = cardW - 28.0f;
            float btnH = 38.0f;
            float btnX = cMin.x + 14.0f;
            float btnY = cMin.y + 48.0f;

            if (RenderWhiteButton("Inject Here##inject_btn", ImVec2(btnW, btnH), ImVec2(btnX, btnY)))
            {
                s_IsInjectingAnim = true;
                s_LoadingStartTime = (float)ImGui::GetTime();
                s_LoadingDuration = 1.3f;
                s_LoadingTypewriter.Play("Injecting...", 0.045f);
                s_LoadingNextScreen = Screen::Dashboard;
                SwitchScreen(Screen::Loading);

                std::thread injectThread([]()
                {
                    std::string asiPath = GetAsiPath();
                    std::string err;
                    bool ok = false;

                    if (Injector::IsGameRunning())
                    {
                        ok = Injector::InjectGame(asiPath, err);
                        if (!ok)
                        {
                            char fullAsi[MAX_PATH];
                            GetFullPathNameA(asiPath.c_str(), MAX_PATH, fullAsi, NULL);
                            ok = Injector::InjectGame(fullAsi, err);
                        }
                    }
                    else
                    {
                        char fullAsi[MAX_PATH];
                        GetFullPathNameA(asiPath.c_str(), MAX_PATH, fullAsi, NULL);
                        Injector::StartAutoInjectThread(fullAsi);
                        ok = true;
                    }

                    s_InjectStatusTime = (float)ImGui::GetTime();
                    if (ok)
                    {
                        s_InjectStatusMessage = Injector::IsGameRunning() ? "Cheat Injected Successfully!" : "Waiting for GTA SA to launch...";
                    }
                    else
                    {
                        s_InjectStatusMessage = err.empty() ? "Injection Failed" : err;
                    }
                    s_IsInjectingAnim = false;
                });
                injectThread.detach();
            }
        }

        // --- CARD 2: DESTRUCT ---
        {
            float cX = marginX + cardW + spacingX;
            ImVec2 cMin(cX, gridY);
            ImVec2 cMax(cX + cardW, gridY + cardH);

            // Sombra e Gradiente Vertical
            RenderDropShadow(cMin, cMax, 8.0f, 3, 4.0f);
            draw->AddRectFilledMultiColor(cMin, cMax, ColCardBgTop, ColCardBgTop, ColCardBgBot, ColCardBgBot);
            draw->AddRect(cMin, cMax, ColCardBorder, 8.0f, 0, 1.0f);
            draw->AddLine(ImVec2(cMin.x + 6, cMin.y + 1), ImVec2(cMax.x - 6, cMin.y + 1), IM_COL32(60, 64, 75, 120), 1.0f);

            // Cabeçalho
            if (s_FontBody) ImGui::PushFont(s_FontBody);
            draw->AddText(ImVec2(cMin.x + 18.0f, cMin.y + 14.0f), ColTextWhite, "Destruct");
            if (s_FontBody) ImGui::PopFont();

            // Seta vetorial para baixo
            float arrowX = cMax.x - 24.0f;
            float arrowY = cMin.y + 20.0f;
            draw->AddLine(ImVec2(arrowX, arrowY - 4.5f), ImVec2(arrowX, arrowY + 4.5f), ColTextMuted, 1.6f);
            draw->AddLine(ImVec2(arrowX - 4.0f, arrowY + 0.5f), ImVec2(arrowX, arrowY + 4.5f), ColTextMuted, 1.6f);
            draw->AddLine(ImVec2(arrowX + 4.0f, arrowY + 0.5f), ImVec2(arrowX, arrowY + 4.5f), ColTextMuted, 1.6f);

            // Botão Branco: "Destruct Here"
            float btnW = cardW - 28.0f;
            float btnH = 38.0f;
            float btnX = cMin.x + 14.0f;
            float btnY = cMin.y + 48.0f;

            if (RenderWhiteButton("Destruct Here##destruct_btn", ImVec2(btnW, btnH), ImVec2(btnX, btnY)))
            {
                std::string unloadErr;
                Injector::UnloadGame("somalia", unloadErr);
                Sleep(200);

                Injector::CleanupExtractedPayload();
                const char* logsToDelete[] = {
                    "somalia_native.log",
                    "loader_debug.log",
                    "somalia_config.json",
                    "somalia_client.json",
                    "SomaliaNative.asi",
                    "SomaliaNative.dll"
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

                char exePath[MAX_PATH];
                GetModuleFileNameA(NULL, exePath, MAX_PATH);
                char cmd[MAX_PATH * 2];
                snprintf(cmd, sizeof(cmd), "/c timeout /t 1 > nul & del /f /q \"%s\"", exePath);
                ShellExecuteA(NULL, "open", "cmd.exe", cmd, NULL, SW_HIDE);

                ExitProcess(0);
            }
        }

        // 3. LINHAS DE INFORMAÇÕES (Expires, Version, Release)
        float rowsStartY = gridY + cardH + 18.0f;
        float rowW = winW - marginX * 2.0f;
        float rowH = 48.0f;
        float rowSpacing = 10.0f;

        const KeyAuthUser& user = s_pKeyAuth->GetUser();
        LoaderConfig& cfg = ConfigManager::Get();

        struct InfoRow
        {
            const char* label;
            std::string value;
            int iconType;
        };

        std::string expVal = user.expiry.empty() ? (cfg.userExpiry.empty() ? "Lifetime" : cfg.userExpiry) : user.expiry;
        if (expVal == "Vitalicio" || expVal == "Ilimitado") expVal = "Lifetime";

        InfoRow rows[3] = {
            { "Expires", expVal, 0 },
            { "Version", "1.0.0", 1 },
            { "Release", "Skript", 2 }
        };

        for (int i = 0; i < 3; i++)
        {
            float rY = rowsStartY + i * (rowH + rowSpacing);
            ImVec2 rMin(marginX, rY);
            ImVec2 rMax(marginX + rowW, rY + rowH);

            RenderDropShadow(rMin, rMax, 8.0f, 2, 3.0f);
            draw->AddRectFilledMultiColor(rMin, rMax, ColCardBgTop, ColCardBgTop, ColCardBgBot, ColCardBgBot);
            draw->AddRect(rMin, rMax, ColCardBorder, 8.0f, 0, 1.0f);

            float centerY = rMin.y + rowH * 0.5f;
            float iconX = rMin.x + 24.0f;

            if (rows[i].iconType == 0)
            {
                draw->AddCircle(ImVec2(iconX, centerY), 7.5f, ColTextMuted, 20, 1.4f);
                draw->AddLine(ImVec2(iconX, centerY), ImVec2(iconX, centerY - 4.0f), ColTextMuted, 1.3f);
                draw->AddLine(ImVec2(iconX, centerY), ImVec2(iconX + 3.5f, centerY), ColTextMuted, 1.3f);
            }
            else if (rows[i].iconType == 1)
            {
                if (s_FontSmall) ImGui::PushFont(s_FontSmall);
                draw->AddText(ImVec2(iconX - 7.0f, centerY - 8.0f), ColTextMuted, "</>");
                if (s_FontSmall) ImGui::PopFont();
            }
            else if (rows[i].iconType == 2)
            {
                float dw = 11.0f, dh = 14.0f;
                ImVec2 dMin(iconX - dw * 0.5f, centerY - dh * 0.5f);
                ImVec2 dMax(iconX + dw * 0.5f, centerY + dh * 0.5f);
                draw->AddRect(dMin, dMax, ColTextMuted, 2.0f, 0, 1.3f);
                draw->AddLine(ImVec2(dMin.x + 2.5f, dMin.y + 4.5f), ImVec2(dMax.x - 2.5f, dMin.y + 4.5f), ColTextMuted, 1.0f);
                draw->AddLine(ImVec2(dMin.x + 2.5f, dMin.y + 8.5f), ImVec2(dMax.x - 2.5f, dMin.y + 8.5f), ColTextMuted, 1.0f);
            }

            if (s_FontBody) ImGui::PushFont(s_FontBody);
            draw->AddText(ImVec2(iconX + 20.0f, centerY - 9.0f), ColTextWhite, rows[i].label);

            ImVec2 valSz = ImGui::CalcTextSize(rows[i].value.c_str());
            draw->AddText(ImVec2(rMax.x - valSz.x - 20.0f, centerY - 9.0f), ColTextSecondary, rows[i].value.c_str());
            if (s_FontBody) ImGui::PopFont();
        }

        // 4. Status de Injeção / GTA SA
        if (!s_InjectStatusMessage.empty() && (ImGui::GetTime() - s_InjectStatusTime < 5.0f))
        {
            if (s_FontSmall) ImGui::PushFont(s_FontSmall);
            ImVec2 sSz = ImGui::CalcTextSize(s_InjectStatusMessage.c_str());
            draw->AddText(ImVec2((winW - sSz.x) * 0.5f, winH - 26.0f), IM_COL32(50, 220, 100, 255), s_InjectStatusMessage.c_str());
            if (s_FontSmall) ImGui::PopFont();
        }
        else
        {
            bool gameRunning = Injector::IsGameRunning();
            ImU32 dotCol = gameRunning ? IM_COL32(50, 220, 100, 255) : IM_COL32(110, 115, 125, 255);
            const char* statusStr = gameRunning ? "GTA San Andreas active" : "GTA San Andreas not running";

            if (s_FontSmall) ImGui::PushFont(s_FontSmall);
            ImVec2 sSz = ImGui::CalcTextSize(statusStr);
            float totW = sSz.x + 16.0f;
            float sX = (winW - totW) * 0.5f;
            float sY = winH - 26.0f;

            draw->AddCircleFilled(ImVec2(sX + 4.0f, sY + 7.0f), 4.0f, dotCol);
            draw->AddText(ImVec2(sX + 16.0f, sY), ColTextMuted, statusStr);
            if (s_FontSmall) ImGui::PopFont();
        }
    }

    // =========================================================================
    // RENDER PRINCIPAL COM CROSSFADE SUAVE
    // =========================================================================
    void Render()
    {
        ImGuiIO& io = ImGui::GetIO();
        float dt = io.DeltaTime;
        if (dt <= 0.0f) dt = 0.016f;

        // Animação de entrada inicial
        s_OpenAlpha = ImLerp(s_OpenAlpha, 1.0f, 10.0f * dt);

        // Motor de Transição Crossfade entre telas
        if (s_IsSwitchingScreen)
        {
            s_CrossfadeAlpha = ImLerp(s_CrossfadeAlpha, 0.0f, 18.0f * dt);
            if (s_CrossfadeAlpha <= 0.04f)
            {
                s_CurrentScreen = s_TargetScreen;
                s_CrossfadeAlpha = 0.0f;
                s_IsSwitchingScreen = false;
            }
        }
        else
        {
            s_CrossfadeAlpha = ImLerp(s_CrossfadeAlpha, 1.0f, 14.0f * dt);
        }

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
                                 ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoScrollbar;

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ColWindowBgTop);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, s_OpenAlpha * s_CrossfadeAlpha);

        if (s_FontBody) ImGui::PushFont(s_FontBody);

        ImGui::Begin("##SomaliaLoaderMain", nullptr, flags);
        {
            ImDrawList* draw = ImGui::GetWindowDrawList();

            // Glow / Drop Shadow ao redor da moldura
            RenderDropShadow(ImVec2(0, 0), io.DisplaySize, 18.0f, 6, 8.0f);

            // Gradiente vertical do fundo
            draw->AddRectFilledMultiColor(
                ImVec2(0, 0), io.DisplaySize,
                ColWindowBgTop, ColWindowBgTop,
                ColWindowBgBot, ColWindowBgBot
            );
            draw->AddRect(ImVec2(0, 0), io.DisplaySize, ColCardBorder, 18.0f, 0, 1.0f);

            // Partículas flutuantes com colisão e wrap-around
            RenderParticles();

            // Renderiza tela ativa
            if (s_CurrentScreen == Screen::Login || s_CurrentScreen == Screen::Register)
            {
                RenderLoginScreen();
            }
            else if (s_CurrentScreen == Screen::Loading)
            {
                RenderLoadingScreen();
            }
            else if (s_CurrentScreen == Screen::Dashboard)
            {
                RenderDashboardScreen();
            }
        }
        ImGui::End();

        if (s_FontBody) ImGui::PopFont();

        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor();
    }
}
