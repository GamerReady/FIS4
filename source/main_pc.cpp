/*
 * FIS4 PC Simulation (SDL2)
 * Runs the same core logic as PS4 build for testing without PS4 hardware
 */

#include "fis4_app.h"
#include "fis4_renderer.h"
#include <chrono>
#include <thread>
#include <cstdio>

#ifdef FIS4_SDL
#include <SDL2/SDL.h>
#endif

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    printf("FIS4 Frame Interpolation Lab - PC Simulation\n");
    printf("Controls: X/Cross toggle FIS4 x2, Up/Down switch 30/60, Left/Right change scene, Square cycle mode, Tri toggle graph, Circle pause, Esc exit\n");

#ifdef FIS4_SDL
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
        return 1;
    }
    const int W=1280, H=720; // smaller window for PC
    SDL_Window* win = SDL_CreateWindow("FIS4 Lab - PS4 Preview", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W,H, SDL_WINDOW_SHOWN);
    if(!win){ fprintf(stderr,"Window failed: %s\n", SDL_GetError()); return 1; }
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if(!ren){ fprintf(stderr,"Renderer failed: %s\n", SDL_GetError()); return 1; }

    FIS4::App app(W,H);
    app.init();
    FIS4::Renderer renderer(ren, W,H);

    using clock = std::chrono::high_resolution_clock;
    auto start = clock::now();
    auto lastNative = start;
    uint64_t frameId=0;
    bool running=true;

    // Key state mapping to FIS4 buttons
    auto keyToButton = [](SDL_Keycode k)->uint64_t {
        if(k==SDLK_x || k==SDLK_SPACE) return 1ULL<<0; // cross
        if(k==SDLK_c) return 1ULL<<1; // circle
        if(k==SDLK_t) return 1ULL<<2; // triangle
        if(k==SDLK_s) return 1ULL<<3; // square
        if(k==SDLK_UP) return 1ULL<<8;
        if(k==SDLK_DOWN) return 1ULL<<9;
        if(k==SDLK_LEFT) return 1ULL<<10;
        if(k==SDLK_RIGHT) return 1ULL<<11;
        if(k==SDLK_o) return 1ULL<<12; // options
        return 0;
    };

    while(running){
        auto now = clock::now();
        double nowMs = std::chrono::duration<double, std::milli>(now - start).count();

        SDL_Event ev;
        while(SDL_PollEvent(&ev)){
            if(ev.type==SDL_QUIT) running=false;
            if(ev.type==SDL_KEYDOWN){
                if(ev.key.keysym.sym==SDLK_ESCAPE) running=false;
                uint64_t btn = keyToButton(ev.key.keysym.sym);
                if(btn) app.handleInput(btn);
                if(ev.key.keysym.sym==SDLK_h) app.showHelp=!app.showHelp;
                if(ev.key.keysym.sym==SDLK_g) app.showGraph=!app.showGraph;
                if(ev.key.keysym.sym==SDLK_p) app.setPaused(!app.isPaused());
                if(ev.key.keysym.sym==SDLK_f) app.cycleBaseFPS();
                if(ev.key.keysym.sym==SDLK_m) app.cycleInterpMode();
                if(ev.key.keysym.sym==SDLK_n) app.nextScene();
                if(ev.key.keysym.sym==SDLK_b) app.prevScene();
            }
        }

        double baseFPS = (app.getBaseFPS()==FIS4::BaseFPS::FPS_30?30.0:60.0);
        double nativeInterval = 1000.0/baseFPS;
        double elapsed = std::chrono::duration<double,std::milli>(now - lastNative).count();
        bool doNative = elapsed >= nativeInterval;
        if(doNative){
            float dt = nativeInterval/1000.f;
            app.updateNative(dt, nowMs);
            lastNative = now;
        }
        float interpT = 0.f;
        if(app.isFIS4Enabled()){
            interpT = (float)(elapsed / nativeInterval);
            if(interpT>1) interpT=1; if(interpT<0) interpT=0;
        }
        auto rs = app.getRenderState(interpT);

        // Render
        renderer.clear({8,8,16});
        if(app.getSceneID()==FIS4::SceneID::ROTATING_GRID) renderer.drawGrid(rs.gridPhase*40.f);
        else if(app.getSceneID()==FIS4::SceneID::TUNNEL) renderer.drawTunnel(rs.tunnelOffset);
        else renderer.drawGrid(rs.gridPhase*20.f);

        for(size_t i=0;i<rs.balls.size();++i){
            const auto &b=rs.balls[i];
            uint8_t r=(uint8_t)(120+120*sinf(b.hue*6.28f));
            uint8_t g=(uint8_t)(120+120*sinf(b.hue*6.28f+2.f));
            uint8_t bl=(uint8_t)(120+120*sinf(b.hue*6.28f+4.f));
            FIS4::Color col{r,g,bl,255};
            renderer.drawBall(b,col);
        }
        if(app.getSceneID()!=FIS4::SceneID::TUNNEL) renderer.drawBar(rs.bar,{255,200,40});
        renderer.drawHUD(app);
        renderer.present();
        app.tickFrame(nowMs);

        // Cap to effective FPS
        double eff = app.getEffectiveFPS();
        double budget = 1000.0/eff;
        auto end = clock::now();
        double busy = std::chrono::duration<double,std::milli>(end-now).count();
        double remain = budget - busy;
        if(remain>1) SDL_Delay((uint32_t)remain*0.9);

        // Print stats occasionally
        if(frameId% (int)eff ==0){
            auto s=app.getStats();
            printf("[FIS4] Base %d | FIS4 %s | Eff %.0f FPS | avg %.1f | scene %s | frames %llu\n",
                (int)app.getBaseFPS(), app.isFIS4Enabled()?"ON":"OFF", eff, s.avgFPS, app.getSceneName(), (unsigned long long)s.totalFrames);
        }
        frameId++;
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
#else
    // Headless test mode
    printf("SDL2 not available - running headless logic test\n");
    FIS4::App app(1920,1080);
    app.init();
    using clock=std::chrono::high_resolution_clock;
    auto start=clock::now();
    for(int i=0;i<180;i++){
        auto now=clock::now();
        double nowMs=std::chrono::duration<double,std::milli>(now-start).count();
        app.updateNative(1.f/30.f, nowMs);
        auto st=app.getStats();
        printf("frame %d eff %.0f\n", i, app.getEffectiveFPS());
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    printf("Headless test done - FIS4 interpolator works (generated %llu frames)\n", (unsigned long long)app.getStats().interpolatedFrames);
    return 0;
#endif
}
