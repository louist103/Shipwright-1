#define _CRT_SECURE_NO_WARNINGS
#include <array>
#include <map>

#include "ImGui/imgui.h"
#include "ImGui/backends/imgui_impl_dx9.h"
#include "ImGui/backends/imgui_impl_win32.h"
#include "ImGui/ImGuiFileDialog.h"
#include <d3d9.h>
#include <tchar.h>

#include <thread>

#include "BaseromUtils/ExtractBaserom.h"
#include "BaseromUtils/md5.h"


#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

static LPDIRECT3D9              g_pD3D = NULL;
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


void printRomInfo(uint32_t romCrc) {
    switch (romCrc) {
    case NTSC_10:
        ImGui::Text("Detected version NTSC 1.0. CRC: %X", romCrc);
        break;
    case NTSC_11:
        ImGui::Text("Detected version NTSC 1.1. CRC: %X", romCrc);
        break;
    case NTSC_12:
        ImGui::Text("Detected version NTSC 1.2. CRC: %X", romCrc);
        break;
    case PAL_10:
        ImGui::Text("Detected version PAL 1.0. CRC: %X", romCrc);
        break;
    case PAL_11:
        ImGui::Text("Detected version PAL 1.1. CRC: %X", romCrc);
        break;
    case NTSC_JP_GC_CE:
        ImGui::Text("Detected version NTSC JP GC CE. CRC: %X", romCrc);
        break;
    case NTSC_JP_GC:
        ImGui::Text("Detected version NTSC JP GC. CRC: %X", romCrc);
        break;
    case NTSC_US_GC:
        ImGui::Text("Detected version NTSC US GC. CRC: %X", romCrc);
        break;
    case PAL_GC:
        ImGui::Text("Detected version PAL GC. CRC: %X", romCrc);
        break;
    case NTSC_JP_MQ:
        ImGui::Text("Detected version NTSC JP MQ. CRC: %X", romCrc);
        break;
    case NTSC_US_MQ:
        ImGui::Text("Detected version NTSC US MQ. CRC: %X", romCrc);
        break;
    case PAL_MQ:
        ImGui::Text("Detected version PAL MQ. CRC: %X", romCrc);
        break;
    case PAL_GC_DBG1:
        ImGui::Text("Detected version PAL GC DBG (3-21-2002). CRC: %X", romCrc);
        break;
    case PAL_GC_DBG2:
        ImGui::Text("Detected version PAL GC DBG (3-13-2002). CRC: %X", romCrc);
        break;
    case PAL_GC_MQ_DBG:
        ImGui::Text("Detected version PAL MQ DBG. CRC: %X", romCrc);
        break;
    case IQUE_TW:
        ImGui::Text("Detected version IQUE TW. CRC: %X", romCrc);
        break;
    case IQUE_CN:
        ImGui::Text("Detected version IQUE CN. CRC: %X", romCrc);
        break;
    default:
        ImGui::Text("Unknown or unsupported version detected. CRC: %X", romCrc);
        break;
    }
}


std::map<const uint32_t, const std::array<unsigned char, 16>> md5Map {
    {PAL_GC_DBG1, { 0x9c,0x1d,0x79,0x54,0x29,0x22,0x0f,0x53,0x89,0x04,0x56,0x93,0xa0,0x11,0xb8,0xf6, } },
    {PAL_GC, { 0x2c, 0x27, 0xb4, 0xe0, 0x00, 0xe8, 0x5f, 0xd7, 0x8d, 0xbc, 0xa5, 0x51, 0xf1, 0xb1, 0xc9, 0x65 } }, 
};

int main(void) {

    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("ImGui Example"), NULL };
    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("Dear ImGui DirectX9 Example"), WS_OVERLAPPEDWINDOW, 100, 100, 800, 600, NULL, NULL, wc.hInstance, NULL);
    
    std::string romPath;
    uint32_t romCrc = 0xFFFFFFFF;
    FILE* romFile = nullptr;
    FILE* fileList = nullptr; //MSVC isn't smart enough to know that it is always set at the time of its use
    bool isRomOK = false;
    bool romChecked = false;
    char romMd5[16];

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);


    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
    //io.ConfigViewportsNoAutoMerge = true;
    //io.ConfigViewportsNoTaskBarIcon = true;

    ImGui::StyleColorsDark();


    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);

    const ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    const ImVec4 redColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    const ImVec4 greenColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
    const ImVec4 blueColor = ImVec4(0.0f, 0.0f, 1.0f, 1.0f);
    const ImVec4 yellowColor = ImVec4(1.0f, 1.0f, 0.0f, 0.0f);

    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Start the Dear ImGui frame
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();



        ImGui::Begin("Hello, world!");

        ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtention, ".z64", greenColor);
        ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtention, ".n64", yellowColor);
        ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtention, ".v64", yellowColor);

        if (ImGui::Button("Open Rom")) {
            ImGuiFileDialog::Instance()->OpenDialog("Open Rom", "Open Rom", ".z64", "");
        }
        if (ImGuiFileDialog::Instance()->Display("Open Rom", ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove, ImVec2(WINDOW_WIDTH - 10, WINDOW_HEIGHT / 2), ImVec2(WINDOW_WIDTH - 10, WINDOW_HEIGHT / 2))) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                romPath = ImGuiFileDialog::Instance()->GetFilePathName();
                romFile = fopen(romPath.c_str(), "rb");
                romCrc = getVersion(romFile, &fileList);
                fseek(romFile, 0, SEEK_SET);
                md5File(romFile, romMd5);
                romChecked = true;
                if (memcmp(romMd5, md5Map[romCrc].data(), sizeof(romMd5)) == 0) { //TODO is an invalid crc UB?
                    isRomOK = true;
                }
                else {
                    isRomOK = false;
                }

            }
            ImGuiFileDialog::Instance()->Close();
        }

        if (romChecked && (fileList == nullptr)) {
            ImGui::TextColored(redColor, "Rom checked but couldn't open a file list.");
        }
        else if (romChecked && (fileList != nullptr)) {
            ImGui::TextColored(greenColor, "Rom file list opened");
        }

        if (romCrc != 0xFFFFFFFF) {
            printRomInfo(romCrc);
        }
        if (romChecked) {
            if (isRomOK) {
                ImGui::TextColored(greenColor, "Rom OK");
            } else{
                ImGui::TextColored(redColor, "Rom MD5 is not valid or not supported. Check your rom.");
            }
        }
        if (isRomOK && (fileList != nullptr)) {
            if (ImGui::Button("Extract Baserom")) {
                std::thread extractorThread(extract, romPath.c_str());
                extractorThread.detach();
            }
        }

        if (gExtractorState == ExtractorExtracting) {
            auto lock = std::lock_guard<std::mutex>(gMutex);
            ImGui::Text("Extracting: %s", currentFile);
        }

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();


        // Rendering
        ImGui::EndFrame();
        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(clear_color.x * clear_color.w * 255.0f), (int)(clear_color.y * clear_color.w * 255.0f), (int)(clear_color.z * clear_color.w * 255.0f), (int)(clear_color.w * 255.0f));
        g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
        if (g_pd3dDevice->BeginScene() >= 0)
        {
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            g_pd3dDevice->EndScene();
        }

        // Update and Render additional Platform Windows
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        HRESULT result = g_pd3dDevice->Present(NULL, NULL, NULL, NULL);

        // Handle loss of D3D9 device
        if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
            ResetDevice();
    }



    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}



bool CreateDeviceD3D(HWND hWnd)
{
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
        return false;

    // Create the D3DDevice
    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed = TRUE;
    g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN; // Need to use an explicit format with alpha if needing per-pixel alpha composition.
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;           // Present with vsync
    //g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate
    if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
        return false;

    return true;
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
    if (hr == D3DERR_INVALIDCALL)
        IM_ASSERT(0);
    ImGui_ImplDX9_CreateDeviceObjects();
}

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0 // From Windows SDK 8.1+ headers
#endif

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
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
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    case WM_DPICHANGED:
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DpiEnableScaleViewports)
        {
            //const int dpi = HIWORD(wParam);
            //printf("WM_DPICHANGED to %d (%.0f%%)\n", dpi, (float)dpi / 96.0f * 100.0f);
            const RECT* suggested_rect = (RECT*)lParam;
            ::SetWindowPos(hWnd, NULL, suggested_rect->left, suggested_rect->top, suggested_rect->right - suggested_rect->left, suggested_rect->bottom - suggested_rect->top, SWP_NOZORDER | SWP_NOACTIVATE);
        }
        break;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

