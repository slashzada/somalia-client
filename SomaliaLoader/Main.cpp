#include <windows.h>
#include <d3d9.h>
#include <tchar.h>
#include "../SomaliaNative/Render/ImGui/imgui.h"
#include "../SomaliaNative/Render/ImGui/imgui_impl_dx9.h"
#include "../SomaliaNative/Render/ImGui/imgui_impl_win32.h"
#include "UI/LoaderMenu.h"
#include "Injector/Injector.h"

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "advapi32.lib")

// Símbolos globais requeridos pelo fork customizado de ImGui do Somalia
float accent_colour[4] = { 137.f / 255.f, 207.f / 255.f, 240.f / 255.f, 1.0f };
float content_animation = 0.0f;
ImFont* poppins = nullptr;
ImFont* font_icon = nullptr;

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static LPDIRECT3D9              g_pD3D = NULL;
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static IDirect3D9Ex*            g_pD3DEx = NULL;
static IDirect3DDevice9Ex*      g_pd3dDeviceEx = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

bool CreateDeviceD3D(HWND hWnd)
{
    // Try Direct3DCreate9Ex first (recommended for Windows 10/11 modern WDDM drivers)
    typedef HRESULT (WINAPI *LPDIRECT3DCREATE9EX)(UINT SDKVersion, IDirect3D9Ex**);
    HMODULE hD3D9 = LoadLibraryA("d3d9.dll");
    if (hD3D9)
    {
        LPDIRECT3DCREATE9EX pDirect3DCreate9Ex = (LPDIRECT3DCREATE9EX)GetProcAddress(hD3D9, "Direct3DCreate9Ex");
        if (pDirect3DCreate9Ex && SUCCEEDED(pDirect3DCreate9Ex(D3D_SDK_VERSION, &g_pD3DEx)))
        {
            ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
            g_d3dpp.Windowed = TRUE;
            g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
            g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
            g_d3dpp.EnableAutoDepthStencil = FALSE;
            g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
            g_d3dpp.hDeviceWindow = hWnd;

            if (SUCCEEDED(g_pD3DEx->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
                D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, NULL, &g_pd3dDeviceEx)))
            {
                g_pd3dDevice = g_pd3dDeviceEx;
                return true;
            }

            if (SUCCEEDED(g_pD3DEx->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
                D3DCREATE_SOFTWARE_VERTEXPROCESSING, &g_d3dpp, NULL, &g_pd3dDeviceEx)))
            {
                g_pd3dDevice = g_pd3dDeviceEx;
                return true;
            }
        }
    }

    // Fallback to legacy Direct3DCreate9
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) != NULL)
    {
        ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
        g_d3dpp.Windowed = TRUE;
        g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
        g_d3dpp.EnableAutoDepthStencil = FALSE;
        g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
        g_d3dpp.hDeviceWindow = hWnd;

        if (SUCCEEDED(g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
            D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice)))
            return true;

        if (SUCCEEDED(g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
            D3DCREATE_SOFTWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice)))
            return true;
    }

    return false;
}

void CleanupDeviceD3D()
{
    if (g_pd3dDeviceEx) { g_pd3dDeviceEx->Release(); g_pd3dDeviceEx = NULL; g_pd3dDevice = NULL; }
    else if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }

    if (g_pD3DEx) { g_pD3DEx->Release(); g_pD3DEx = NULL; }
    if (g_pD3D) { g_pD3D->Release(); g_pD3D = NULL; }
}

void ResetDevice()
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    HRESULT hr = E_FAIL;
    if (g_pd3dDeviceEx)
        hr = g_pd3dDeviceEx->ResetEx(&g_d3dpp, NULL);
    else if (g_pd3dDevice)
        hr = g_pd3dDevice->Reset(&g_d3dpp);

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

            HRGN hRgn = CreateRoundRectRgn(0, 0, LOWORD(lParam) + 1, HIWORD(lParam) + 1, 18, 18);
            SetWindowRgn(hWnd, hRgn, TRUE);
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
    // Dimensões unificadas e elegantes da Janela (proporção ampliada)
    const int windowWidth = 580;
    const int windowHeight = 500;

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
        _T("Somalia Client"),
        WS_POPUP | WS_MINIMIZEBOX | WS_VISIBLE,
        posX, posY, windowWidth, windowHeight,
        NULL, NULL, wc.hInstance, NULL
    );

    FILE* logF = fopen("loader_debug.log", "w");
    if (logF)
    {
        fprintf(logF, "WinMain: hWnd = %p\n", hWnd);
        fclose(logF);
    }

    if (!CreateDeviceD3D(hWnd))
    {
        FILE* errF = fopen("loader_debug.log", "a");
        if (errF) { fprintf(errF, "CreateDeviceD3D FAILED in WinMain!\n"); fclose(errF); }
        MessageBoxA(hWnd, "Falha ao inicializar DirectX 9.\nCertifique-se de que os drivers de video estao atualizados.", "Somalia Loader - Erro", MB_OK | MB_ICONERROR);
        CleanupDeviceD3D();
        UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ShowWindow(hWnd, SW_SHOWDEFAULT);
    UpdateWindow(hWnd);

    // Cantos arredondados aplicados na janela Win32
    HRGN hRgn = CreateRoundRectRgn(0, 0, windowWidth + 1, windowHeight + 1, 18, 18);
    SetWindowRgn(hWnd, hRgn, TRUE);

    // Inicializa ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.IniFilename = NULL; // Nao grava imgui.ini

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 14.0f;
    style.FrameRounding = 8.0f;
    style.ChildRounding = 8.0f;
    style.PopupRounding = 8.0f;
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
    style.Colors[ImGuiCol_FrameBg]          = ImVec4(27.f / 255.f, 28.f / 255.f, 32.f / 255.f, 1.0f);
    style.Colors[ImGuiCol_FrameBgHovered]   = ImVec4(35.f / 255.f, 37.f / 255.f, 43.f / 255.f, 1.0f);
    style.Colors[ImGuiCol_FrameBgActive]    = ImVec4(40.f / 255.f, 42.f / 255.f, 50.f / 255.f, 1.0f);

    // Registra fontes customizadas (titulo grande, etc)
    LoaderMenu::SetupFonts();

    ImGui_ImplWin32_Init(hWnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);

    // Inicializa telas e modulos do loader
    LoaderMenu::Init(hWnd, g_pd3dDevice);

    if (strstr(lpCmdLine, "--screen register"))
        LoaderMenu::SetCurrentScreen(LoaderMenu::Screen::Register);
    else if (strstr(lpCmdLine, "--screen dashboard"))
        LoaderMenu::SetCurrentScreen(LoaderMenu::Screen::Dashboard);
    else if (strstr(lpCmdLine, "--screen loading"))
        LoaderMenu::SetCurrentScreen(LoaderMenu::Screen::Loading);

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

        // Movimentação da janela clicando em qualquer espaço vazio do fundo (protegendo a área do rodapé e controles)
        if (ImGui::IsMouseClicked(0) && !ImGui::IsAnyItemHovered() && !ImGui::IsAnyItemActive() && ImGui::GetMousePos().y < 430.0f)
        {
            ReleaseCapture();
            SendMessageA(hWnd, WM_NCLBUTTONDOWN, HTCAPTION, 0);
        }

        ImGui::EndFrame();
        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

        D3DCOLOR clearColor = D3DCOLOR_RGBA(19, 20, 23, 255);
        g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, clearColor, 1.0f, 0);

        if (g_pd3dDevice->BeginScene() >= 0)
        {
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            g_pd3dDevice->EndScene();
        }

        HRESULT result = g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
        if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
            ResetDevice();

        static int s_ScreenshotFrame = 0;
        if (strstr(lpCmdLine, "--screenshot"))
        {
            s_ScreenshotFrame++;
            if (s_ScreenshotFrame >= 20)
            {
                IDirect3DSurface9* pBackBuffer = NULL;
                if (SUCCEEDED(g_pd3dDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer)))
                {
                    D3DSURFACE_DESC desc;
                    pBackBuffer->GetDesc(&desc);
                    IDirect3DSurface9* pDestSurface = NULL;
                    if (SUCCEEDED(g_pd3dDevice->CreateOffscreenPlainSurface(desc.Width, desc.Height, desc.Format, D3DPOOL_SYSTEMMEM, &pDestSurface, NULL)))
                    {
                        if (SUCCEEDED(g_pd3dDevice->GetRenderTargetData(pBackBuffer, pDestSurface)))
                        {
                            D3DLOCKED_RECT locked;
                            if (SUCCEEDED(pDestSurface->LockRect(&locked, NULL, D3DLOCK_READONLY)))
                            {
                                BITMAPFILEHEADER bfh = { 0 };
                                BITMAPINFOHEADER bih = { 0 };
                                bfh.bfType = 0x4D42;
                                bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
                                bih.biSize = sizeof(BITMAPINFOHEADER);
                                bih.biWidth = desc.Width;
                                bih.biHeight = desc.Height;
                                bih.biPlanes = 1;
                                bih.biBitCount = 32;
                                bih.biCompression = BI_RGB;
                                bih.biSizeImage = desc.Width * desc.Height * 4;
                                bfh.bfSize = bfh.bfOffBits + bih.biSizeImage;

                                char shotName[64] = "loader_render.bmp";
                                if (strstr(lpCmdLine, "--screen register")) strcpy_s(shotName, "loader_register.bmp");
                                else if (strstr(lpCmdLine, "--screen dashboard")) strcpy_s(shotName, "loader_dashboard.bmp");
                                else if (strstr(lpCmdLine, "--screen loading")) strcpy_s(shotName, "loader_loading.bmp");

                                FILE* f = fopen(shotName, "wb");
                                if (f)
                                {
                                    fwrite(&bfh, sizeof(bfh), 1, f);
                                    fwrite(&bih, sizeof(bih), 1, f);
                                    for (int y = (int)desc.Height - 1; y >= 0; y--)
                                    {
                                        BYTE* row = (BYTE*)locked.pBits + y * locked.Pitch;
                                        fwrite(row, desc.Width * 4, 1, f);
                                    }
                                    fclose(f);
                                }
                                pDestSurface->UnlockRect();
                            }
                        }
                        pDestSurface->Release();
                    }
                    pBackBuffer->Release();
                }
                bRunning = false;
            }
        }

        // Limita a taxa de quadros para poupar CPU quando em repouso
        Sleep(10);
    }

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    DestroyWindow(hWnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}
