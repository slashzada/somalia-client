#include <windows.h>
#include <d3d9.h>
#include <tchar.h>
#include <cstdarg>
#include <cstdio>
#include "../SomaliaNative/Render/ImGui/imgui.h"
#include "../SomaliaNative/Render/ImGui/imgui_impl_dx9.h"
#include "../SomaliaNative/Render/ImGui/imgui_impl_win32.h"
#include "UI/LoaderMenu.h"
#include "Injector/Injector.h"
#include "Config/LoaderConfig.h"

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "advapi32.lib")

#ifdef _DEBUG
static void DebugLog(const char* fmt, ...)
{
    FILE* f = fopen("loader_debug.log", "a");
    if (f)
    {
        va_list args;
        va_start(args, fmt);
        vfprintf(f, fmt, args);
        va_end(args);
        fclose(f);
    }
}
#else
static inline void DebugLog(const char*, ...) {}
#endif

// Símbolos globais requeridos pelo fork customizado de ImGui do Somalia
float accent_colour[4] = { 137.f / 255.f, 207.f / 255.f, 240.f / 255.f, 1.0f };
float content_animation = 0.0f;
ImFont* poppins = nullptr;
ImFont* font_icon = nullptr;

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static LPDIRECT3D9              g_pD3D = NULL;
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

bool CreateDeviceD3D(HWND hWnd)
{
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
    {
        DebugLog("Direct3DCreate9 returned NULL!\n");
        return false;
    }
    DebugLog("Direct3DCreate9 OK: %p\n", g_pD3D);

    D3DFORMAT formats[] = { D3DFMT_A8R8G8B8, D3DFMT_UNKNOWN, D3DFMT_X8R8G8B8, D3DFMT_R5G6B5 };
    DWORD vpTypes[] = { D3DCREATE_HARDWARE_VERTEXPROCESSING, D3DCREATE_SOFTWARE_VERTEXPROCESSING, D3DCREATE_MIXED_VERTEXPROCESSING };

    RECT rc;
    GetClientRect(hWnd, &rc);
    UINT width = (rc.right - rc.left > 0) ? (rc.right - rc.left) : 538;
    UINT height = (rc.bottom - rc.top > 0) ? (rc.bottom - rc.top) : 336;

    for (D3DFORMAT fmt : formats)
    {
        for (DWORD vp : vpTypes)
        {
            ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
            g_d3dpp.Windowed = TRUE;
            g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
            g_d3dpp.BackBufferFormat = fmt;
            g_d3dpp.BackBufferWidth = width;
            g_d3dpp.BackBufferHeight = height;
            g_d3dpp.EnableAutoDepthStencil = FALSE;
            g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
            g_d3dpp.hDeviceWindow = hWnd;

            HRESULT hr = g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, vp, &g_d3dpp, &g_pd3dDevice);
            if (SUCCEEDED(hr))
            {
                DebugLog("CreateDevice SUCCESS: fmt=%d, vp=0x%X, Device=%p\n", fmt, vp, g_pd3dDevice);
                return true;
            }
            else
            {
                DebugLog("Attempt fmt=%d, vp=0x%X, w=%d, h=%d -> hr=0x%08X\n", fmt, vp, width, height, hr);
            }
        }
    }

    // Tenta também com BackBufferWidth = 0 e BackBufferHeight = 0
    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
    g_d3dpp.EnableAutoDepthStencil = FALSE;
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    g_d3dpp.hDeviceWindow = hWnd;

    HRESULT hr = g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice);
    if (SUCCEEDED(hr))
    {
        DebugLog("CreateDevice SUCCESS: SW VP zero-size, Device=%p\n", g_pd3dDevice);
        return true;
    }

    DebugLog("All CreateDevice attempts failed! Zero-size hr=0x%08X\n", hr);
    return false;
}

void CleanupDeviceD3D()
{
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
    if (g_pD3D) { g_pD3D->Release(); g_pD3D = NULL; }
}

void ResetDevice()
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
    if (SUCCEEDED(hr))
    {
        ImGui_ImplDX9_CreateDeviceObjects();
    }
}

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            g_d3dpp.BackBufferWidth = LOWORD(lParam);
            g_d3dpp.BackBufferHeight = HIWORD(lParam);
            ResetDevice();

            HRGN hRgn = CreateRoundRectRgn(0, 0, LOWORD(lParam) + 1, HIWORD(lParam) + 1, 14, 14);
            if (hRgn)
            {
                if (!SetWindowRgn(hWnd, hRgn, TRUE))
                {
                    DeleteObject(hRgn);
                }
            }
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // Dimensões unificadas e elegantes da Janela
    const int windowWidth = 560;
    const int windowHeight = 400;

    // Registra classe da janela
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, hInstance, NULL, NULL, NULL, NULL, _T("SomaliaLoaderClass"), NULL };
    RegisterClassEx(&wc);

    // Centraliza janela na tela
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenW - windowWidth) / 2;
    int posY = (screenH - windowHeight) / 2;

    HWND hWnd = CreateWindowEx(
        WS_EX_APPWINDOW,
        wc.lpszClassName,
        _T("Somalia Group"),
        WS_POPUP | WS_MINIMIZEBOX | WS_VISIBLE,
        posX, posY, windowWidth, windowHeight,
        NULL, NULL, wc.hInstance, NULL
    );

    DebugLog("WinMain: hWnd = %p\n", hWnd);

    if (!CreateDeviceD3D(hWnd))
    {
        DebugLog("CreateDeviceD3D FAILED in WinMain!\n");
        MessageBoxA(hWnd, "Falha ao inicializar DirectX 9.\nCertifique-se de que os drivers de video estao atualizados.", "Somalia Loader - Erro", MB_OK | MB_ICONERROR);
        CleanupDeviceD3D();
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ShowWindow(hWnd, SW_SHOWDEFAULT);
    UpdateWindow(hWnd);

    // Cantos arredondados aplicados na janela Win32
    HRGN hRgn = CreateRoundRectRgn(0, 0, windowWidth + 1, windowHeight + 1, 14, 14);
    if (hRgn)
    {
        if (!SetWindowRgn(hWnd, hRgn, TRUE))
        {
            DeleteObject(hRgn);
        }
    }

    // Inicializa ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.IniFilename = NULL; // Nao grava imgui.ini

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 5.0f;
    style.ChildRounding = 6.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;

    // Cores oficiais Somalia
    ImVec4 accentColor = ImVec4(137.f / 255.f, 207.f / 255.f, 240.f / 255.f, 1.0f);
    style.Colors[ImGuiCol_CheckMark]        = accentColor;
    style.Colors[ImGuiCol_SliderGrab]       = accentColor;
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(165.f / 255.f, 222.f / 255.f, 248.f / 255.f, 1.0f);
    style.Colors[ImGuiCol_Header]           = ImVec4(accentColor.x, accentColor.y, accentColor.z, 0.35f);
    style.Colors[ImGuiCol_HeaderHovered]    = ImVec4(accentColor.x, accentColor.y, accentColor.z, 0.55f);
    style.Colors[ImGuiCol_HeaderActive]     = ImVec4(accentColor.x, accentColor.y, accentColor.z, 0.75f);
    style.Colors[ImGuiCol_ButtonActive]     = ImVec4(accentColor.x, accentColor.y, accentColor.z, 0.65f);
    style.Colors[ImGuiCol_FrameBg]          = ImVec4(16.f / 255.f, 18.f / 255.f, 26.f / 255.f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered]   = ImVec4(24.f / 255.f, 28.f / 255.f, 40.f / 255.f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive]    = ImVec4(30.f / 255.f, 36.f / 255.f, 52.f / 255.f, 1.0f);

    // Registra fontes customizadas
    LoaderMenu::SetupFonts();

    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);

    // Inicializa telas e modulos do loader
    LoaderMenu::Init(hWnd, g_pd3dDevice);

    // Loop de Mensagens Principal
    bool bRunning = true;
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));

    while (msg.message != WM_QUIT && bRunning)
    {
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Renderiza interface
        LoaderMenu::Render();

        // Movimentação da janela clicando em qualquer espaço vazio do fundo
        if (ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered() && !ImGui::IsAnyItemActive())
        {
            ReleaseCapture();
            SendMessageA(hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        }

        ImGui::EndFrame();
        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

        D3DCOLOR clearColor = D3DCOLOR_RGBA(26, 26, 26, 255);
        g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clearColor, 1.0f, 0);

        if (g_pd3dDevice->BeginScene() >= 0)
        {
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            g_pd3dDevice->EndScene();
        }

        HRESULT result = g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
        if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
            ResetDevice();

        // Limita a taxa de quadros para poupar CPU quando em repouso
        Sleep(10);
    }

    // Shutdown ordenado e sincronizado de todos os subsistemas
    LoaderMenu::Shutdown();
    Injector::Shutdown();
    ConfigManager::Shutdown();

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(hWnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}
