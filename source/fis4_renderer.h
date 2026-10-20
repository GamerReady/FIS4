#pragma once
// Renderer abstraction: PS4 (Scene2D framebuffer) vs PC (SDL2)
#include "fis4_app.h"
#include "demo_scene.h"
#include "common/graphics.h"

#ifdef FIS4_PS4
// PS4 renderer uses Scene2D directly
namespace FIS4 {
class Renderer {
public:
    Renderer(Scene2D& scene): gfx(scene) {}
    void clear(Color c){ gfx.FrameBufferFill(c); }
    void drawBall(const Ball& b, Color col);
    void drawBar(const RotatingBar& bar, Color col);
    void drawGrid(float phase);
    void drawTunnel(Vec2 offset);
    void drawHUD(App& app);
    void present(int frameId){ gfx.SubmitFlip(frameId); gfx.FrameWait(frameId); gfx.FrameBufferSwap(); }
private:
    Scene2D& gfx;
    void drawTextLine(int x,int y, const char* txt, Color fg, Color bg);
    Color hsv2rgb(float h);
};
}
#else
// PC renderer (SDL2 or headless)
#ifdef FIS4_SDL
#include <SDL2/SDL.h>
#endif

namespace FIS4 {
class Renderer {
public:
#ifdef FIS4_SDL
    Renderer(SDL_Renderer* r, int w,int h): sdlRen(r), W(w), H(h) {}
    void clear(Color c);
    void drawBall(const Ball& b, Color c);
    void drawBar(const RotatingBar& bar, Color col);
    void drawGrid(float phase);
    void drawTunnel(Vec2 offset);
    void drawHUD(App& app);
    void present(){ SDL_RenderPresent(sdlRen); }
#else
    Renderer(int w,int h): W(w), H(h) {}
    void clear(Color){} void drawBall(const Ball&,Color){} void drawBar(const RotatingBar&,Color){} void drawGrid(float){} void drawTunnel(Vec2){} void drawHUD(App&){} void present(){}
#endif
private:
#ifdef FIS4_SDL
    SDL_Renderer* sdlRen=nullptr;
#endif
    int W=1920,H=1080;
    Color hsv2rgb(float h);
    void drawCircleSDL(int cx,int cy,int r);
    void drawLineSDL(int x0,int y0,int x1,int y1);
    void drawRectSDL(int x,int y,int w,int h);
};
}
#endif
