#include <windows.h>
#include <stdint.h>

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
            uint8 Red = 0;
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

int WINAPI WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLine, int CmdShow)
{
    WNDCLASS wc = {};

    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.hInstance = Instance;
    // wc.hIcon =
    wc.lpszClassName = "HandmadeHeroWindowClass";
    wc.lpfnWndProc = WindowProc;

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

        RenderGradient(Buffer, (Offset), (Offset++) * -1);

        window_dimension Dimension = GetWindowDimension(Window);
        HDC DeviceContext = GetDC(Window);
        UpdateWindow(Buffer, DeviceContext, Dimension.Width, Dimension.Height);
        ReleaseDC(Window, DeviceContext);
    }

    return 0;
}