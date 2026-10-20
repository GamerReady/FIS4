#pragma once
#include <stdint.h>

#ifdef FIS4_PS4
#include <orbis/libkernel.h>
#include <orbis/VideoOut.h>
#include <orbis/Sysmodule.h>
#ifdef GRAPHICS_USES_FONT
#include <proto-include.h>
#endif

struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a = 255;
};

class Scene2D {
#ifdef GRAPHICS_USES_FONT
    FT_Library ftLib;
#endif
    int width;
    int height;
    int depth;
    int video;
    off_t directMemOff;
    size_t directMemAllocationSize;
    uintptr_t videoMemSP;
    void *videoMem;
    char **frameBuffers;
    OrbisKernelEqueue flipQueue;
    OrbisVideoOutBufferAttribute attr;
    int frameBufferSize;
    int frameBufferCount;
    int activeFrameBufferIdx = 0;

    bool initFlipQueue();
    bool allocateFrameBuffers(int num);
    char *allocateDisplayMem(size_t size);
    bool allocateVideoMem(size_t size, int alignment);
    void deallocateVideoMem();

public:
    Scene2D(int w, int h, int pixelDepth);
    ~Scene2D();
    bool Init(size_t memSize, int numFrameBuffers);
    void SetActiveFrameBuffer(int index);
    void SubmitFlip(int frameID);
    void FrameWait(int frameID);
    void FrameBufferSwap();
    void FrameBufferClear();
    void FrameBufferFill(Color color);
    void DrawPixel(int x, int y, Color color);
    void DrawRectangle(int x, int y, int w, int h, Color color);
    void DrawCircle(int cx, int cy, int r, Color color);
    void DrawLine(int x0, int y0, int x1, int y1, Color color);
    void DrawRectOutline(int x, int y, int w, int h, Color color, int thickness = 1);

#ifdef GRAPHICS_USES_FONT
    bool InitFont(FT_Face *face, const char *fontPath, int fontSize);
    void DrawText(char *txt, FT_Face face, int startX, int startY, Color bgColor, Color fgColor);
    void DrawTextContainer(char *txt, FT_Face face, int startX, int startY, int maxW, int maxH, Color bgColor, Color fgColor);
#endif

    int GetWidth() const { return width; }
    int GetHeight() const { return height; }
    char* GetFrameBuffer() { return frameBuffers ? frameBuffers[activeFrameBufferIdx] : nullptr; }
};

#else
// PC stub - actual rendering done via SDL in fis4_renderer.h
struct Color {
    uint8_t r, g, b, a = 255;
};
class Scene2D {
public:
    Scene2D(int, int, int) {}
    bool Init(size_t, int) { return true; }
    void SetActiveFrameBuffer(int) {}
    void SubmitFlip(int) {}
    void FrameWait(int) {}
    void FrameBufferSwap() {}
    void FrameBufferClear() {}
    void FrameBufferFill(Color) {}
    void DrawPixel(int, int, Color) {}
    void DrawRectangle(int, int, int, int, Color) {}
    void DrawCircle(int, int, int, Color) {}
    void DrawLine(int, int, int, int, Color) {}
    void DrawRectOutline(int, int, int, int, Color, int = 1) {}
    int GetWidth() const { return 1920; }
    int GetHeight() const { return 1080; }
};
#endif
