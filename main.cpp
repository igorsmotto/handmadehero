#include <windows.h>
#include <stdint.h>
#include <mmdeviceapi.h>
#include <audioclient.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

#define internal static
#define global static

// XInput definitions (normally from xinput.h)
#define XINPUT_GAMEPAD_DPAD_UP 0x0001
#define XINPUT_GAMEPAD_DPAD_DOWN 0x0002
#define XINPUT_GAMEPAD_DPAD_LEFT 0x0004
#define XINPUT_GAMEPAD_DPAD_RIGHT 0x0008
#define XINPUT_GAMEPAD_START 0x0010
#define XINPUT_GAMEPAD_BACK 0x0020
#define XINPUT_GAMEPAD_LEFT_THUMB 0x0040
#define XINPUT_GAMEPAD_RIGHT_THUMB 0x0080
#define XINPUT_GAMEPAD_LEFT_SHOULDER 0x0100
#define XINPUT_GAMEPAD_RIGHT_SHOULDER 0x0200
#define XINPUT_GAMEPAD_A 0x1000
#define XINPUT_GAMEPAD_B 0x2000
#define XINPUT_GAMEPAD_X 0x4000
#define XINPUT_GAMEPAD_Y 0x8000

#define XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE 7849
#define XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE 8689
#define XINPUT_GAMEPAD_TRIGGER_THRESHOLD 30

typedef struct _XINPUT_GAMEPAD
{
    uint16 wButtons;
    uint8 bLeftTrigger;
    uint8 bRightTrigger;
    int16 sThumbLX;
    int16 sThumbLY;
    int16 sThumbRX;
    int16 sThumbRY;
} XINPUT_GAMEPAD, *PXINPUT_GAMEPAD;

typedef struct _XINPUT_STATE
{
    uint32 dwPacketNumber;
    XINPUT_GAMEPAD Gamepad;
} XINPUT_STATE, *PXINPUT_STATE;

typedef uint32 WINAPI xinput_get_state(uint32 dwUserIndex, XINPUT_STATE *pState);
internal xinput_get_state *XInputGetStateFunc = 0;
internal uint32 WINAPI XInputGetStateStub(uint32 dwUserIndex, XINPUT_STATE *pState)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}

internal void LoadXInput()
{
    HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
    if (!XInputLibrary)
    {
        XInputLibrary = LoadLibraryA("xinput1_3.dll");
    }

    if (XInputLibrary)
    {
        XInputGetStateFunc = (xinput_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
        if (!XInputGetStateFunc)
        {
            XInputGetStateFunc = XInputGetStateStub;
        }
        OutputDebugStringA("XInput library loaded successfully\n");
    }
    else
    {
        XInputGetStateFunc = XInputGetStateStub;
        OutputDebugStringA("XInput library not found, using stub functions\n");
    }
}

struct window_dimension
{
    int Width;
    int Height;
};

struct offscreen_buffer
{
    int Width;
    int Height;
    BITMAPINFO Info;
    void *Memory;
    int Pitch;
    int BytesPerPixel;
};

global bool Running;
global offscreen_buffer Buffer;
global int SpeedMultiplier = 1;

internal window_dimension GetWindowDimension(HWND Window)
{
    window_dimension WindowDimension;
    RECT ClientRect;

    GetClientRect(Window, &ClientRect);
    WindowDimension.Width = ClientRect.right - ClientRect.left;
    WindowDimension.Height = ClientRect.bottom - ClientRect.top;

    return WindowDimension;
}

internal void RenderGradient(offscreen_buffer Buffer, int XOffset, int YOffset)
{
    uint8 *Row = (uint8 *)Buffer.Memory;
    for (int Y = 0; Y < Buffer.Height; ++Y)
    {
        uint32 *Pixel = (uint32 *)Row;
        for (int X = 0; X < Buffer.Width; ++X)
        {
            uint8 Red = 255;
            uint8 Green = (Y + YOffset);
            uint8 Blue = (X + XOffset);

            uint32 color = (Red << 16) | (Green << 8) | Blue;

            *Pixel++ = color;
        }

        Row += Buffer.Pitch;
    }
}

internal void ResizeDIBSection(offscreen_buffer *Buffer, int Width, int Height)
{
    if (Buffer->Memory)
    {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Info = {};
    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Width;
    Buffer->Info.bmiHeader.biHeight = -Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    Buffer->BytesPerPixel = 4;
    Buffer->Height = Height;
    Buffer->Width = Width;
    Buffer->Pitch = Buffer->Width * Buffer->BytesPerPixel;

    Buffer->Memory = VirtualAlloc(0, (Width * Height) * Buffer->BytesPerPixel, MEM_COMMIT, PAGE_READWRITE);
}

internal void UpdateWindow(offscreen_buffer Buffer, HDC DeviceContext, int Width, int Height)
{
    StretchDIBits(DeviceContext,
                  0, 0, Width, Height,
                  0, 0, Buffer.Width, Buffer.Height,
                  Buffer.Memory,
                  &Buffer.Info,
                  DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK WindowProc(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;

    switch (Message)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT Paint;
        HDC DeviceContext = BeginPaint(Window, &Paint);

        window_dimension Dimension = GetWindowDimension(Window);
        UpdateWindow(Buffer, DeviceContext, Dimension.Width, Dimension.Height);

        EndPaint(Window, &Paint);
    }
    break;
    case WM_CLOSE:
    {
        Running = false;
        PostQuitMessage(0);
    }
    break;
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYUP:
    case WM_KEYDOWN:
    {
        bool WasDown = (LParam & (1 << 30)) != 0;
        bool IsDown = (LParam & (1 << 31)) != 0;

        switch (WParam)
        {
        case 'A':
        {
            OutputDebugStringA("A key pressed\n");
        }
        break;
        case 'W':
        {
            OutputDebugStringA("W key pressed\n");
        }
        break;
        case 'D':
        {
            OutputDebugStringA("D key pressed\n");
        }
        break;
        case 'S':
        {
            OutputDebugStringA("S key pressed\n");
        }
        break;
        case VK_ESCAPE:
        {
            OutputDebugStringA("Escape key pressed\n");
            Running = false;
        }
        break;
        case VK_F4:
        {
            bool AltKeyWasDown = (LParam & (1 << 29)) != 0;
            OutputDebugStringA("F4 key pressed\n");

            if (AltKeyWasDown)
            {
                OutputDebugStringA("Alt+F4 pressed\n");
                Running = false;
            }
        }
        break;
        }
    }
    break;
    case WM_DESTROY:
    {
        Running = false;
        PostQuitMessage(0);
    }
    break;
    default:
        Result = DefWindowProc(Window, Message, WParam, LParam);
    }

    return Result;
}

internal void HandleControllerInput()
{
    if (XInputGetStateFunc == XInputGetStateStub)
    {
        return; // No XInput available, skip controller input
    }

    // Check all 4 possible controller indices (0-3)
    for (uint32 ControllerIndex = 0; ControllerIndex < 4; ++ControllerIndex)
    {
        XINPUT_STATE ControllerState;
        if (XInputGetStateFunc(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
        {
            XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

            // Check buttons
            if (Pad->wButtons & XINPUT_GAMEPAD_A)
            {
                SpeedMultiplier = 2;
                OutputDebugStringA("A button: Speed 2x faster\n");
            }
            if (Pad->wButtons & XINPUT_GAMEPAD_B)
            {
                OutputDebugStringA("B button: Speed 2x slower\n");
            }
            if (Pad->wButtons & XINPUT_GAMEPAD_X)
            {
                SpeedMultiplier = 1;
                OutputDebugStringA("Controller X button pressed\n");
            }
            if (Pad->wButtons & XINPUT_GAMEPAD_Y)
            {
                OutputDebugStringA("Controller Y button pressed\n");
            }
            if (Pad->wButtons & XINPUT_GAMEPAD_START)
            {
                OutputDebugStringA("Controller Start button pressed\n");
                Running = false; // Exit on Start button
            }

            // Check D-pad
            if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP)
            {
                OutputDebugStringA("Controller D-pad UP\n");
            }
            if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
            {
                OutputDebugStringA("Controller D-pad DOWN\n");
            }
            if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
            {
                OutputDebugStringA("Controller D-pad LEFT\n");
            }
            if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
            {
                OutputDebugStringA("Controller D-pad RIGHT\n");
            }

            // Check thumbsticks (basic deadzone handling)
            if (Pad->sThumbLX > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE ||
                Pad->sThumbLX < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
            {
                if (Pad->sThumbLX > 0)
                    OutputDebugStringA("Left stick RIGHT\n");
                else
                    OutputDebugStringA("Left stick LEFT\n");
            }

            if (Pad->sThumbLY > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE ||
                Pad->sThumbLY < -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
            {
                if (Pad->sThumbLY > 0)
                    OutputDebugStringA("Left stick UP\n");
                else
                    OutputDebugStringA("Left stick DOWN\n");
            }
        }
    }
}

internal void LoadAudio()
{
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    IMMDeviceEnumerator *deviceEnumerator;
    CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void **)&deviceEnumerator);

    IMMDevice *audioDevice;
    deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &audioDevice);

    IAudioClient3 *audioClient;
    audioDevice->Activate(__uuidof(IAudioClient3), CLSCTX_ALL, nullptr, (void **)&audioClient);

    REFERENCE_TIME defaultPeriod;
    REFERENCE_TIME minimumPeriod;

    audioClient->GetDevicePeriod(&defaultPeriod, &minimumPeriod);

    WAVEFORMATEX *mixFormat;
    audioClient->GetMixFormat(&mixFormat);

    audioClient->Initialize(
        AUDCLNT_SHAREMODE_SHARED,
        AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
        defaultPeriod,
        0,
        mixFormat,
        nullptr);

    // Note: The above code can fail in any step (since I'm not checking HRESULTs), so in a real application we should handle errors properly.
}

int WINAPI WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLine, int CmdShow)
{
    WNDCLASS wc = {};

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.hInstance = Instance;
    // wc.hIcon =
    wc.lpszClassName = "HandmadeHeroWindowClass";
    wc.lpfnWndProc = WindowProc;

    LoadAudio();

    RegisterClass(&wc);
    HWND Window = CreateWindowExA(
        0,
        wc.lpszClassName,
        "Handmade Hero",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        0,
        0,
        Instance,
        0);

    if (!Window)
    {
        return -1; // Failed to create window - log error
    }

    int Offset = 0;
    Running = true;

    LoadXInput();

    ResizeDIBSection(&Buffer, 1280, 720);

    while (Running)
    {
        MSG Message;
        while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
        {
            if (Message.message == WM_QUIT)
            {
                Running = false;
                break;
            }

            TranslateMessage(&Message);
            DispatchMessage(&Message);
        }

        HandleControllerInput();
        Offset += SpeedMultiplier;

        RenderGradient(Buffer, Offset, Offset);

        window_dimension Dimension = GetWindowDimension(Window);
        HDC DeviceContext = GetDC(Window);
        UpdateWindow(Buffer, DeviceContext, Dimension.Width, Dimension.Height);
        ReleaseDC(Window, DeviceContext);
    }

    return 0;
}