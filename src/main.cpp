//TODO: App icons
//TODO: App filtering
#include "../thirdparty/imGui/imgui.h"
#include "../thirdparty/imGui/backends/imgui_impl_win32.h"
#include "../thirdparty/imGui/backends/imgui_impl_dx11.h"
#include <d3d11.h>
#include <sysinfoapi.h>
#include <tchar.h>
#include <cstdio>
#include <vector>
#include <iostream>
#include <stdint.h>
#include <string.h>
#include <string>
#include <windows.h>
#include <winuser.h>
#include "timer.h"
#include "Quicksand_SemiBold.h"

// Data
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;

const static int MAX_TIMERS = 64;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

struct WindowInfo {
    HWND hwnd;
    std::string title;
};

std::vector<WindowInfo> app_list;

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    if (!IsWindowVisible(hwnd))
        return TRUE;

    char title[256];
    GetWindowTextA(hwnd, title, sizeof(title));

    if (strlen(title) > 0) {
        app_list.push_back({ hwnd, title });
    }

    return TRUE;
}

void FindAllTopLevelWindows() {
    app_list.clear();
    EnumWindows(EnumWindowsProc, 0);
}



int set_active_timer(char *name, Timer timers[], int timers_count){
    for (int i = 0; i < timers_count; i++){
        if (strcmp(timers[i].name, name) == 0){
            timers[i].paused = false;
            return i;
        }
        else{
            timers[i].paused = true;
        }
    }
    return 0;
}

Timer add_timer(const char *name){

    Timer timer = {};
    timer.hours = 0;
    timer.minutes = 0;
    timer.seconds = 0;
    timer.paused = true;
    strncpy_s (timer.name, sizeof(timer.name), name, _TRUNCATE);
    return timer;
}

void remove_timer(Timer timers[], int& size, int index) {
    for (int i = index; i < size - 1; ++i) {
        timers[i] = timers[i + 1];
    }
    timers[size - 1] = Timer{};  // default init
    --size;
}

// void remove_timer(Timer timers[], int index){
//     timers[index] = {0};
// }

// Main code
int main(int, char**)
{


    // Enable DPI awareness and get DPI scaling factor for the primary monitor
    ImGui_ImplWin32_EnableDpiAwareness();
    HMONITOR monitor = MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY);
    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(monitor);

    // Define the desired client area size (not including title bar, borders)
    int clientWidth = (int)(300 * main_scale);
    int clientHeight = (int)(400 * main_scale);

    // Compute the full window size required to fit the client area
    RECT rect = { 0, 0, clientWidth, clientHeight };
    AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, 0);
    int windowWidth = rect.right - rect.left;
    int windowHeight = rect.bottom - rect.top;

    // Get screen size
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Center the window
    int windowX = (screenWidth - windowWidth) / 2;
    int windowY = (screenHeight - windowHeight) / 2;

    // Register and create window
    WNDCLASSEXW wc = {
        sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L,
        GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr,
        L"timer", nullptr
    };
    ::RegisterClassExW(&wc);

    HWND hwnd = ::CreateWindowW(
        wc.lpszClassName, L"Focus Timer", WS_OVERLAPPEDWINDOW,
        windowX, windowY, windowWidth, windowHeight,
        nullptr, nullptr, wc.hInstance, nullptr);


    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImFont* customFont = io.Fonts->AddFontFromMemoryTTF(
    _mnt_data_Quicksand_SemiBold_ttf, _mnt_data_Quicksand_SemiBold_ttf_len, 18.0f);

    // Setup Dear ImGui style

    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Backgrounds - darker warm taupe
    colors[ImGuiCol_WindowBg]         = ImVec4(0.32f, 0.29f, 0.27f, 1.00f); // Warm taupe
    colors[ImGuiCol_ChildBg]          = ImVec4(0.30f, 0.27f, 0.25f, 1.00f);
    colors[ImGuiCol_PopupBg]          = ImVec4(0.28f, 0.25f, 0.23f, 1.00f);

    // Text - warm light cream or tan
    colors[ImGuiCol_Text]             = ImVec4(0.95f, 0.89f, 0.85f, 1.00f); // Light tan text
    colors[ImGuiCol_TextDisabled]     = ImVec4(0.60f, 0.56f, 0.52f, 1.00f);

    // Buttons - warm terracotta tones
    colors[ImGuiCol_Button]           = ImVec4(0.65f, 0.44f, 0.36f, 1.00f); // Muted terracotta
    colors[ImGuiCol_ButtonHovered]    = ImVec4(0.72f, 0.50f, 0.40f, 1.00f);
    colors[ImGuiCol_ButtonActive]     = ImVec4(0.60f, 0.39f, 0.32f, 1.00f);

    // Frame BGs
    colors[ImGuiCol_FrameBg]          = ImVec4(0.38f, 0.34f, 0.31f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]   = ImVec4(0.42f, 0.38f, 0.35f, 1.00f);
    colors[ImGuiCol_FrameBgActive]    = ImVec4(0.47f, 0.43f, 0.40f, 1.00f);

    // Button shape
    style.FrameRounding = 6.0f;
    style.FrameBorderSize = 1.0f;
    style.FramePadding = ImVec2(10, 6);

    // Colors
    // ImVec4 buttonColor         = ImVec4(1.0f, 0.50f, 1.00f, 1.00f);
    // ImVec4 buttonHoveredColor  = ImVec4(0.35f, 0.60f, 1.00f, 1.00f);
    // ImVec4 buttonActiveColor   = ImVec4(0.15f, 0.40f, 0.90f, 1.00f);

    // ImGui::GetStyle().Colors[ImGuiCol_Button]        = buttonColor;
    // ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] = buttonHoveredColor;
    // ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]  = buttonActiveColor;

    // Setup scaling
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);


    //State
    ImVec4 clear_color = ImVec4(0.58f, 0.45f, 0.29f, 1.00f);
    static float v_spacing = 10.0f;
    static bool selection_mode = false;
    static bool pause_mode = true;
    static HWND main_hwnd = hwnd;
    HWND selected_window;
    static bool waiting_for_click = false;
    int timers_count = 1;
    int active_timer_id = 0;
    char name[64] = "";
    char is_equal[4] = "Yes";
    char not_equal[3] = "No";
    char focused_window_title[64];
    Timer timers[MAX_TIMERS];

    timers[0].hours = 0;
    timers[0].minutes = 0;
    timers[0].seconds = 0;
    timers[0].paused = true;
    std::string global_timer_name = "Total time";
    strncpy_s (timers[0].name, sizeof(timers[0].name), global_timer_name.c_str(), _TRUNCATE);


    uint64_t last_time = GetTickCount64();
    int elapsed_seconds = 0;
    static uint64_t leftover_ms = 0;
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window being minimized or screen locked
        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        {
            static float f = 0.0f;
            static int counter = 0;
            HWND focused_window = GetForegroundWindow();
            if (focused_window != nullptr) {
                GetWindowTextA(focused_window, focused_window_title, sizeof(focused_window_title));
                active_timer_id = set_active_timer(focused_window_title, timers, timers_count);
            }


            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(io.DisplaySize); // fill entire main viewport

            ImGui::Begin("MainPanel", nullptr,
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoTitleBar);

            // float fullWidth = ImGui::GetContentRegionAvail().x;

            ImGui::Dummy(ImVec2(0.0f, v_spacing/2));

            if (ImGui::Button("Add app timer", ImVec2(-1 , 0))){
                selection_mode = true;
            }

            ImGui::Dummy(ImVec2(0.0f, v_spacing/2));

            if (pause_mode){
                if (ImGui::Button("Start", ImVec2(-1 , 0))){
                    pause_mode = false;
                }
            }
            else {
                if (ImGui::Button("Pause", ImVec2(-1 , 0))){
                pause_mode = true;
                }
            }

            ImGui::Dummy(ImVec2(0.0f, v_spacing));

            timers[0].paused = pause_mode;
            ImGui::Text("%s:", timers[0].name);
            ImGui::Text("%02d:%02d:%02d", timers[0].hours, timers[0].minutes, timers[0].seconds);

            ImGui::Dummy(ImVec2(0.0f, v_spacing));

            if (selection_mode){
                FindAllTopLevelWindows();
                for (size_t i = 0; i < app_list.size(); ++i) {
                    WindowInfo win = app_list[i];
                    if (ImGui::Button(win.title.c_str())){
                            selected_window = win.hwnd;
                            strncpy_s (name, sizeof(name), win.title.c_str(), _TRUNCATE);
                            if ((timers_count + 1) < MAX_TIMERS){
                                timers[timers_count++] = add_timer(name);
                            }
                            selection_mode = false;
                        }
                }
                // selection_mode = false;
            }

            ImGui::BeginChild("TimerList", ImVec2(0, 0), false);

            for (int i = 1; i < timers_count; i++) {
                timers[i].paused = pause_mode;

                // Begin a child to encapsulate layout
                ImGui::BeginGroup();

                float line_height = ImGui::GetTextLineHeight();
                float spacing = ImGui::GetStyle().ItemSpacing.y;

                // Record current cursor Y position for first line
                float y_start = ImGui::GetCursorPosY();

                // Draw first line
                ImGui::Text("%s", timers[i].name);

                // Draw second line
                ImGui::Text("%02d:%02d:%02d", timers[i].hours, timers[i].minutes, timers[i].seconds);

                ImGui::EndGroup();

                // Move to the right side
                ImGui::SameLine(ImGui::GetContentRegionAvail().x - 80);

                // Center Y offset between the two lines
                float total_height = 2 * line_height + spacing;
                float button_height = ImGui::GetFrameHeight();
                float y_centered = y_start + (total_height - button_height) / 2.0f;

                // Save current X, move Y to center
                float x = ImGui::GetCursorPosX();
                ImGui::SetCursorPosY(y_centered);
                ImGui::SetCursorPosX(x);

                char btn_text[64];
                sprintf_s(btn_text, sizeof(btn_text), "Remove##%d", i);

                if (ImGui::Button(btn_text, ImVec2(70, 0))) {
                    remove_timer(timers, timers_count, i);
                }

                // Add a bit of spacing before next timer
                ImGui::Dummy(ImVec2(0, spacing));
            }
            ImGui::EndChild();

            uint64_t now = GetTickCount64();
            uint64_t delta_ms = now - last_time;
            last_time = now;

            leftover_ms += delta_ms;
            int add_seconds = 0;
            while (leftover_ms >= 1000)
            {
                add_seconds++;
                leftover_ms -= 1000;
            }
            for (int i = 0; i < timers_count; i++){

            }
            if (add_seconds > 0){
                if (strcmp(timers[active_timer_id].name, focused_window_title) == 0){
                    timer_update(&timers[active_timer_id], add_seconds);
                    timer_update(&timers[0], add_seconds);
                }
            }

            ImGui::End();

            /*
            ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

            ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
            // ImGui::Checkbox("Another Window", &show_another_window);

            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
            ImGui::End();
            */
        }

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Present
        HRESULT hr = g_pSwapChain->Present(1, 0);   // Present with vsync
        //HRESULT hr = g_pSwapChain->Present(0, 0); // Present without vsync
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

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
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
