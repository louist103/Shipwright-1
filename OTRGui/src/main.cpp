#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#define USE_SDL 1
#include <array>
#include <map>
#include <queue>
#include <filesystem>

#include "ImGui/imgui.h"

#ifdef _WIN32
#include "ImGui/backends/imgui_impl_win32.h"
#endif

#ifdef USE_DX9
#include "ImGui/backends/imgui_impl_dx9.h"
#include <d3d9.h>
static LPDIRECT3D9              g_pD3D = NULL;
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};
static bool CreateDeviceD3D(HWND hWnd);
static void CleanupDeviceD3D();
static void ResetDevice();
#include <tchar.h>

static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#elif  USE_SDL
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl2.h"
#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#endif 


#include <thread>

#include "ImGui/ImGuiFileDialog.h"
#include "BaseromUtils/ExtractBaserom.h"
#include "BaseromUtils/md5.h"
#include "BaseromUtils/EndianUtils.h"

///#pragma comment(lib,"d3d9.lib")

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

typedef enum class OtrExtractState {
    Waiting,
    Extracting,
    Done,
} OtrExtractState;

static OtrExtractState sOtrExtractState = OtrExtractState::Waiting;


static void BuildPathQueue(void);
static void PrepareForExtraction(const bool useOldStyle,const bool useNewBaserom, const std::string& originalBaseromPath);
static void ExtractSingleThreaded(const std::wstring &romPath);
static void ExtractNewStyle(const std::wstring& romPath);

static void printRomInfo(void) {
    switch (gRomCrc) {
    case NTSC_10:
        ImGui::Text("Detected version NTSC 1.0. CRC: %X", gRomCrc);
        break;
    case NTSC_11:
        ImGui::Text("Detected version NTSC 1.1. CRC: %X", gRomCrc);
        break;
    case NTSC_12:
        ImGui::Text("Detected version NTSC 1.2. CRC: %X", gRomCrc);
        break;
    case PAL_10:
        ImGui::Text("Detected version PAL 1.0. CRC: %X", gRomCrc);
        break;
    case PAL_11:
        ImGui::Text("Detected version PAL 1.1. CRC: %X", gRomCrc);
        break;
    case NTSC_JP_GC_CE:
        ImGui::Text("Detected version NTSC JP GC CE. CRC: %X", gRomCrc);
        break;
    case NTSC_JP_GC:
        ImGui::Text("Detected version NTSC JP GC. CRC: %X", gRomCrc);
        break;
    case NTSC_US_GC:
        ImGui::Text("Detected version NTSC US GC. CRC: %X", gRomCrc);
        break;
    case PAL_GC:
        ImGui::Text("Detected version PAL GC. CRC: %X", gRomCrc);
        break;
    case NTSC_JP_MQ:
        ImGui::Text("Detected version NTSC JP MQ. CRC: %X", gRomCrc);
        break;
    case NTSC_US_MQ:
        ImGui::Text("Detected version NTSC US MQ. CRC: %X", gRomCrc);
        break;
    case PAL_MQ:
        ImGui::Text("Detected version PAL MQ. CRC: %X", gRomCrc);
        break;
    case PAL_GC_DBG1:
        ImGui::Text("Detected version PAL GC DBG (3-21-2002). CRC: %X", gRomCrc);
        break;
    case PAL_GC_DBG2:
        ImGui::Text("Detected version PAL GC DBG (3-13-2002). CRC: %X", gRomCrc);
        break;
    case PAL_GC_MQ_DBG:
        ImGui::Text("Detected version PAL MQ DBG. CRC: %X", gRomCrc);
        break;
    case IQUE_TW:
        ImGui::Text("Detected version IQUE TW. CRC: %X", gRomCrc);
        break;
    case IQUE_CN:
        ImGui::Text("Detected version IQUE CN. CRC: %X", gRomCrc);
        break;
    default:
        ImGui::Text("Unknown or unsupported version detected. CRC: %X", gRomCrc);
        break;
    }
}

//Uncomment other versions to add support for them
static std::map<const uint32_t, const std::array<unsigned char, 16>> md5Map {
    {PAL_GC_DBG1, { 0x9c,0x1d,0x79,0x54,0x29,0x22,0x0f,0x53,0x89,0x04,0x56,0x93,0xa0,0x11,0xb8,0xf6, } },
    {PAL_GC, { 0x2c, 0x27, 0xb4, 0xe0, 0x00, 0xe8, 0x5f, 0xd7, 0x8d, 0xbc, 0xa5, 0x51, 0xf1, 0xb1, 0xc9, 0x65 } },
    {PAL_GC_MQ_DBG, {0xf0, 0xb7, 0xf3, 0x53, 0x75, 0xf9, 0xcc, 0x8c, 0xa1, 0xb2, 0xd5, 0x9d, 0x78, 0xe3, 0x54, 0x05 }}
};

static void AskToExtractOtr(const bool extractFromRam, const std::string& romPath) {
    static bool useOldExtractionStyle = false;
    static bool sOtrExistsLatch = false;

        ImGui::Checkbox("Use old extraction style?", &useOldExtractionStyle);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Extracts all files to disk then combines into an archive. If unsure don't use this.");
        }
        if (ImGui::Button("Generate OTR") || sOtrExistsLatch) {
            sOtrExistsLatch = true;
            FILE* otrFile = fopen("oot.otr", "rb");
            if (otrFile != nullptr) {
                fclose(otrFile);
                ImGui::OpenPopup("OTR file exists");
                if (ImGui::BeginPopupModal("OTR file exists")) {
                    ImGui::Text("OOT.otr already exists. Delete it and make a new one?");
                    if (ImGui::Button("Yes", ImVec2(120, 0))) {
                        sOtrExistsLatch = false;
                        remove("oot.otr");
                        ImGui::CloseCurrentPopup();
                        PrepareForExtraction(useOldExtractionStyle, extractFromRam, romPath);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("No", ImVec2(120, 0))) {
                        sOtrExistsLatch = false;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }
            }
            else {
                PrepareForExtraction(useOldExtractionStyle, extractFromRam, romPath);
            }
        }
}

int main(void) {

    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
#ifdef USE_DX9
    const WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("OTRExporter GUI"), NULL };
    ::RegisterClassEx(&wc);
    HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("OTRExporter GUI DX9"), WS_OVERLAPPEDWINDOW, 100, 100, 800, 600, NULL, NULL, wc.hInstance, NULL);
#elif USE_SDL
    // Setup SDL
    // (Some versions of SDL before <2.0.10 appears to have performance/stalling issues on a minority of Windows systems,
    // depending on whether SDL_INIT_GAMECONTROLLER is enabled or disabled.. updating to latest version of SDL is recommended!)
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // Setup window
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL2+OpenGL example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, window_flags);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync
#endif

    
    char newRomName[20];
    std::string romPath;
    FILE* romFile = nullptr;
    FILE* fileList = nullptr; //MSVC isn't smart enough to know that it is always set at the time of its use
    bool isRomOK = false;
    bool romChecked = false;
    bool extractFromRam = false;
    void* romData = nullptr;
    RomEndian endian = RomEndian::Unchecked;
    
    std::array<unsigned char, 16> romMd5;

#ifdef USE_DX9
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);
#endif

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
#ifdef USE_DX9
    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);
#elif USE_SDL
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL2_Init();
#endif

    const ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    const ImVec4 redColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    const ImVec4 greenColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
    const ImVec4 blueColor = ImVec4(0.0f, 0.0f, 1.0f, 1.0f);
    const ImVec4 yellowColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);

    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
#ifdef USE_DX9
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
#elif USE_SDL
SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }
         // Start the Dear ImGui frame
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
#endif


        ImGui::Begin("Hello, world!");

        ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtention, ".z64", greenColor);
        ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtention, ".n64", yellowColor);
        ImGuiFileDialog::Instance()->SetFileStyle(IGFD_FileStyleByExtention, ".v64", yellowColor);

        if (ImGui::Button("Open Rom")) {
            ImGuiFileDialog::Instance()->OpenDialog("OpenRomBox", "Open Rom", ".z64,.v64,.n64", ".");
        }
        if (ImGuiFileDialog::Instance()->Display("OpenRomBox", ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove, ImVec2(WINDOW_WIDTH - 10, WINDOW_HEIGHT / 2), ImVec2(WINDOW_WIDTH - 10, WINDOW_HEIGHT / 2))) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                romPath = ImGuiFileDialog::Instance()->GetFilePathName();
                romFile = fopen(romPath.c_str(), "rb");
                romChecked = false;
                
            }
            ImGuiFileDialog::Instance()->Close();
        }

        if ((romFile != nullptr) && (romChecked != true)) {
            romChecked = true;
            endian = GetRomEndian(romFile);
            if (endian != RomEndian::Error) {
                switch (endian) {
                    case RomEndian::Big: //Rom is already big endian so we don't need to fix it up at all
                        gRomCrc = getVersion(romFile, &fileList, false);
                        if (gRomSize == ROM_64_MB) {  //Size is changed to 54MB after ConvertToBigEndianRam runs. 
                            extractFromRam = true;
                            romData = ConvertToBigEndianRam(romFile);
                            md5Blob(static_cast<uint8_t*>(romData), romMd5.data(), gRomSize);
                        } else {
                            extractFromRam = false; // We can extract the baserom from the orignal file 
                            fseek(romFile, 0, SEEK_SET);
                            md5File(romFile, romMd5.data());
                        }
                        break;
                    case RomEndian::Half:
                    case RomEndian::Little:
                        extractFromRam = true;
                        romData = ConvertToBigEndianRam(romFile);
                        gRomCrc = getVersion(romData, &fileList, true);
                        md5Blob(static_cast<uint8_t*>(romData), romMd5.data(), gRomSize);
                        break;

                }

                if ((md5Map.contains(gRomCrc)) && (memcmp(romMd5.data(), md5Map[gRomCrc].data(), romMd5.size()) == 0)) {
                    if (extractFromRam) {
                        sprintf(newRomName, "baserom_%X.z64", gRomCrc);
                        FILE* newOutFile = fopen(newRomName, "wb+");
                        ImGui::Text("Your rom needed to be altered a little. A new copy has been saved to baserom_%X.z64", gRomCrc);
                        fwrite(romData, gRomSize, 1, newOutFile);
                        fclose(newOutFile);
                    }
                    isRomOK = true;
                }
                else {
                    isRomOK = false;
                }
            }
            else {
                isRomOK = false;
            }
        }
        if (endian == RomEndian::Error) {
            ImGui::OpenPopup("Rom Error");
            if (ImGui::BeginPopupModal("Rom Error")) {
                ImGui::Text("The rom you selected is invalid. See the output window for more information");
                if (ImGui::Button("OK", ImVec2(120, 0))) {
                    endian = RomEndian::Unchecked;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

        }

        if (romChecked && (fileList == nullptr)) {
            ImGui::TextColored(redColor, "Rom checked but couldn't open a file list.");
        }
        else if (romChecked && (fileList != nullptr)) {
            ImGui::TextColored(greenColor, "Rom file list opened");
        }

        {
            std::unique_lock<std::mutex> fileLock(gFileMutex);
            if (gExtractorWaiting) {
                ImGui::OpenPopup("Baserom exists");
                if (ImGui::BeginPopupModal("Baserom exists")) {
                    ImGui::Text("Baserom folder already exists. Delete it?");
                    if (ImGui::Button("Yes", ImVec2(120, 0))) { 
                        gImGuiResponse = ImGuiResponse::DeleteFiles;
                        gExtractorWaiting = false;
                        cv.notify_all();
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("No", ImVec2(120, 0))) { 
                        gImGuiResponse = ImGuiResponse::KeepFiles;
                        gExtractorWaiting = false;
                        cv.notify_all();
                        ImGui::CloseCurrentPopup();

                    }
                    ImGui::EndPopup();
                }
            }
        }
        if (gRomCrc != 0xFFFFFFFF) {
            printRomInfo();
        }
        if (romChecked) {
            ImGui::Text("Rom MD5: ");
            for (const unsigned char c : romMd5) {
            ImGui::SameLine(0.0f,0.0f);
            
            ImGui::Text("%02X", c);
            }
            if (isRomOK) {
                ImGui::TextColored(greenColor, "Rom OK");
            } else{
                ImGui::TextColored(redColor, "Rom MD5 is not valid or not supported. Check your rom.");
            }
        }

        if (isRomOK && (fileList != nullptr)) {
            if (ImGui::Button("Extract Baserom")) {
                if (extractFromRam) {
                    std::thread extractorThread(extractRam, romData, fileList);
                    extractorThread.detach();
                }
                else {
                    fseek(romFile, 0, SEEK_SET);
                    std::thread extractorThread(extract, romFile, fileList);
                    extractorThread.detach();
                }
            }
        }

        if (extractFromRam) {
            sprintf(newRomName, "baserom_%X.z64", gRomCrc);
            ImGui::Text("Your rom needed to be altered a little. A new copy has been saved to baserom_%X.z64", gRomCrc);
        }


        if (gExtractorState == ExtractorState::Extracting) {
            ImGui::Text("Extracting: %s", currentFile);
        }

        if (isRomOK) {
            AskToExtractOtr(extractFromRam, romPath);
        }

        switch (sOtrExtractState) {
            case OtrExtractState::Extracting:
                ImGui::TextColored(yellowColor, "Creating OTR file. This may take a few minutes.");
                break;
            case OtrExtractState::Done:
                ImGui::TextColored(greenColor, "OTR file generated.");
                break;
        }


        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        ImGui::End();


        // Rendering
#ifdef USE_DX9
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
#elif USE_SDL
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        //glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
#endif
    }


#ifdef USE_DX9
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);
#elif USE_SDL
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
#endif
    return 0;
}

#ifdef _MSC_VER
static constexpr const wchar_t ZAPD_EXEC[9] = L"ZAPD.exe";
#else
static constexpr const char ZAPD_EXEC[9] = "ZAPD.out";
#endif

static std::queue<std::filesystem::path> xmlQueue;

static const char* GetXmlPath(void) {
    switch (gRomCrc) {
    case PAL_GC_DBG1:
        return "assets/extractor/xmls/GC_NMQ_D";
    case PAL_GC_MQ_DBG:
        return "assets/extractor/xmls/GC_MQ_D";
    case PAL_GC:
        return "assets/extractor/xmls/GC_NMQ_PAL_F";
    default:
        return nullptr;
    }
}

static const wchar_t* GetXmlPathW(void) {
    switch (gRomCrc) {
    case PAL_GC_DBG1:
        return L"assets/extractor/xmls/GC_NMQ_D";
    case PAL_GC_MQ_DBG:
        return L"assets/extractor/xmls/GC_MQ_D";
    case PAL_GC:
        return L"assets/extractor/xmls/GC_NMQ_PAL_F";
    default:
        return nullptr;
    }
}

static void PrepareForExtraction(const bool useOldStyle, const bool useNewBaserom, const std::string& originalBaseromPath) {
    static std::wstring romPath; //Needs to be static for threads
    wchar_t targetBaserom[21];
    // There is probably a better way to do this
    if (useNewBaserom) {
        swprintf(targetBaserom, 21, L"baserom_%X.z64", gRomCrc);
        romPath = targetBaserom;
    }
    else {
        std::wstring temp(originalBaseromPath.begin(), originalBaseromPath.end());
        romPath = temp;
    }
    
    if (useOldStyle) {
        BuildPathQueue();
        //std::thread queueBuilderThread(BuildPathQueue); //TODO is this even faster than just running it on this thread?
        //queueBuilderThread.join();
        ExtractSingleThreaded(romPath);
    }
    else {
        //ExtractNewStyle(romPath);
        std::thread extractNewStyleThread(ExtractNewStyle, romPath);
        extractNewStyleThread.detach();
    }
}

static void BuildPathQueue(void) {
    const char* xmlPath = GetXmlPath(); //TODO check for errors or something    

    if (std::filesystem::exists(xmlPath) && std::filesystem::is_directory(xmlPath)) {
        for (const auto& file : std::filesystem::recursive_directory_iterator(xmlPath)) {
            if (std::filesystem::is_regular_file(file) && file.path().extension() == ".xml") {
                //xmlQueue.push(std::filesystem::absolute(file.path()));
                xmlQueue.push(file.path());
            }
        }
    }
}

static const wchar_t* GetConfigPath(void) {
    switch (gRomCrc) {
    case PAL_GC_DBG1:
        return L"assets/extractor/Config_GC_NMQ_D.xml";
    case PAL_GC_MQ_DBG:
        return L"assets/extractor/Config_GC_MQ_D.xml";
    case PAL_GC:
        return L"assets/extractor/Config_GC_NMQ_PAL_F.xml";
    default:
        return nullptr;
    }
}

//TODO audio files
static void ExtractSingleThreaded(const std::wstring& romPath) {
#if 0 //TODO linux and ifdefs
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    wchar_t zapdArgs[512];

    LPWSTR name = const_cast<LPWSTR>(ZAPD_EXEC); //Oh no

    const wchar_t* configPath = GetConfigPath();
    const wchar_t* xmlPath = GetXmlPathW();
        
    while (!xmlQueue.empty()) {
        memset(&si, 0, sizeof(si));
        memset(&pi, 0, sizeof(pi));
        si.cb = sizeof(si);
        //args... xmlPath, baseromFolder, 
        swprintf(zapdArgs, 512, L" ed -eh -i %s -b %s -fl assets/extractor/filelists/ -o placeholder -osf placeholder  gsf 1 -rconf %s -se OTR",xmlPath, romPath.c_str(), configPath);
        wprintf(L"%s %s\n", name, zapdArgs);
        if (CreateProcess(name, zapdArgs, nullptr, nullptr, false, 0, nullptr, nullptr, &si, &pi)) {
            WaitForSingleObject(pi.hProcess, INFINITE);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            puts("Generation complete");
        }
        xmlQueue.pop();
    }
   // BuildOtr();
#endif
}

static void BuildOtr(void) {
    puts("building OTR file");
}


//TODO audio files foR MQ
static void ExtractNewStyle(const std::wstring& romPath) {
#if 0
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    wchar_t zapdArgs[1024];
    
    LPWSTR name = const_cast<LPWSTR>(ZAPD_EXEC); //Oh no

    const wchar_t* configPath = GetConfigPath();
    const wchar_t* xmlPath = GetXmlPathW();
    sOtrExtractState = OtrExtractState::Extracting;
    memset(&si, 0, sizeof(si));
    memset(&pi, 0, sizeof(pi));
    si.cb = sizeof(si);
    //args... xmlPath, baseromFolder, 
    swprintf(zapdArgs, 1024, L" ed -eh -i %s -b %s -fl assets/extractor/filelists/ -o placeholder -osf placeholder  gsf 1 -rconf %s -se OTR", xmlPath, romPath.c_str(), configPath);
    wprintf(L"%s %s\n", name, zapdArgs);
    if (CreateProcess(name, zapdArgs, nullptr, nullptr, false, 0, nullptr, nullptr, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        sOtrExtractState = OtrExtractState::Done;
    }
#endif
}

#ifdef USE_DX9

static bool CreateDeviceD3D(HWND hWnd)
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

static void CleanupDeviceD3D()
{
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
    if (g_pD3D) { g_pD3D->Release(); g_pD3D = NULL; }
}

static void ResetDevice()
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
static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
#endif
