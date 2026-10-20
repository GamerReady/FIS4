/*
 * FIS4 - Frame Interpolation System for PS4
 * PS4 Entry Point (OpenOrbis)
 * 
 * Shows moving objects (balls, rotating bars, grid)
 * Allows switching between 30/60 native FPS and toggling FIS4 x2 interpolation
 * Demonstrates how AI frame interpolation will double FPS in future
 */

#include <orbis/libkernel.h>
#include <orbis/VideoOut.h>
#include <orbis/Pad.h>
#include <orbis/SystemService.h>
#include <orbis/UserService.h>
#include <stdint.h>
#include <string.h>
#include <chrono>
#include <thread>

#define GRAPHICS_USES_FONT
#include "common/graphics.h"
#include "common/log.h"
#include "fis4_app.h"
#include "fis4_renderer.h"

// PS4 specific pad handling helpers
static uint64_t mapOrbisPadToFIS4(const OrbisPadData* pad) {
    uint64_t out=0;
    if(!pad) return 0;
    // Orbis buttons are bitmask; map to FIS4 generic
    // Reference: ORBIS_PAD_BUTTON_CROSS, etc.
    if(pad->buttons & ORBIS_PAD_BUTTON_CROSS)    out |= 1ULL<<0;
    if(pad->buttons & ORBIS_PAD_BUTTON_CIRCLE)   out |= 1ULL<<1;
    if(pad->buttons & ORBIS_PAD_BUTTON_TRIANGLE) out |= 1ULL<<2;
    if(pad->buttons & ORBIS_PAD_BUTTON_SQUARE)   out |= 1ULL<<3;
    if(pad->buttons & ORBIS_PAD_BUTTON_L1)       out |= 1ULL<<4;
    if(pad->buttons & ORBIS_PAD_BUTTON_R1)       out |= 1ULL<<5;
    if(pad->buttons & ORBIS_PAD_BUTTON_L2)       out |= 1ULL<<6;
    if(pad->buttons & ORBIS_PAD_BUTTON_R2)       out |= 1ULL<<7;
    if(pad->buttons & ORBIS_PAD_BUTTON_UP)       out |= 1ULL<<8;
    if(pad->buttons & ORBIS_PAD_BUTTON_DOWN)     out |= 1ULL<<9;
    if(pad->buttons & ORBIS_PAD_BUTTON_LEFT)     out |= 1ULL<<10;
    if(pad->buttons & ORBIS_PAD_BUTTON_RIGHT)    out |= 1ULL<<11;
    if(pad->buttons & ORBIS_PAD_BUTTON_OPTIONS)   out |= 1ULL<<12;
    if(pad->buttons & ORBIS_PAD_BUTTON_TOUCH_PAD) out |= 1ULL<<13;
    return out;
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    // Init system service
    int rc = sceSystemServiceInit();
    if(rc < 0) DEBUGLOG << "SystemService init failed";

    // Init pad
    OrbisPadConfig *padConf = nullptr;
    scePadInit();
    int padHandle = scePadOpen(ORBIS_USER_SERVICE_USER_ID_SYSTEM, ORBIS_PAD_PORT_TYPE_STANDARD, 0, nullptr);
    if(padHandle < 0) DEBUGLOG << "Failed to open pad";

    // Init graphics: 1920x1080, 4 bytes per pixel (XRGB)
    Scene2D scene(1920, 1080, 4);
    if(!scene.Init(0xC000000, 2)) {
        DEBUGLOG << "Failed to init Scene2D";
        return -1;
    }
    scene.SetActiveFrameBuffer(0);

    // FIS4 App core
    FIS4::App app(1920,1080);
    app.init();
    FIS4::Renderer renderer(scene);

    // Timing
    using clock = std::chrono::high_resolution_clock;
    auto startTime = clock::now();
    auto lastNativeTick = startTime;
    uint64_t frameId = 0;
    uint64_t nativeFrameId = 0;
    bool wasPressed[16] = {false};

    // Font (optional)
    FT_Face fontFace;
#ifdef GRAPHICS_USES_FONT
    // Try to init font if available at /app0/assets/font.ttf
    // If fails, we continue without text (we draw rect HUD instead)
    bool hasFont = scene.InitFont(&fontFace, "/app0/assets/Gontserrat.ttf", 24);
    (void)hasFont;
#endif

    DEBUGLOG << "FIS4 PS4 Lab started - Base 30 FPS, FIS4 x2 ON";

    // Main loop
    bool running = true;
    while(running) {
        auto now = clock::now();
        double nowMs = std::chrono::duration<double, std::milli>(now - startTime).count();

        // --- Input polling ---
        OrbisPadData padData;
        if(scePadReadState(padHandle, &padData) == 0) {
            // Edge detection for buttons that should trigger once per press
            uint64_t mapped = mapOrbisPadToFIS4(&padData);
            // For simplicity, we feed all buttons to handleInput which internally handles toggles.
            // But to avoid rapid toggle, we detect rising edge for toggle buttons.
            static uint64_t prevMapped = 0;
            uint64_t pressed = mapped & ~prevMapped;
            if(pressed) {
                // Check for exit combo: L1+R1+Options -> exit
                if((mapped & ((1ULL<<4)|(1ULL<<5)|(1ULL<<12))) == ((1ULL<<4)|(1ULL<<5)|(1ULL<<12))) {
                    running = false;
                } else {
                    app.handleInput(pressed);
                }
            }
            prevMapped = mapped;

            // Left stick for future control
            float lx = padData.leftStick.x / 128.f;
            float ly = padData.leftStick.y / 128.f;
            (void)lx; (void)ly;
        }

        // --- Determine target native tick rate ---
        double baseFPS = (app.getBaseFPS() == FIS4::BaseFPS::FPS_30 ? 30.0 : 60.0);
        double nativeIntervalMs = 1000.0 / baseFPS;
        double elapsedMs = std::chrono::duration<double, std::milli>(now - lastNativeTick).count();

        bool doNativeTick = elapsedMs >= nativeIntervalMs;
        if(doNativeTick) {
            float dt = (float)(nativeIntervalMs / 1000.0);
            app.updateNative(dt, nowMs);
            lastNativeTick = now;
            nativeFrameId++;
        }

        // --- Render ---
        // Interpolation factor t = how far we are between native ticks (0..1)
        float interpT = 0.f;
        if(app.isFIS4Enabled() && baseFPS < 120) {
            double sinceTick = std::chrono::duration<double, std::milli>(now - lastNativeTick).count();
            // But lastNativeTick was just updated; better compute from previous tick
            // Instead use elapsed since last tick / interval
            interpT = (float)(elapsedMs / nativeIntervalMs);
            if(interpT > 1.f) interpT = 1.f;
            if(interpT < 0.f) interpT = 0.f;
        }

        // Choose render state
        auto renderState = app.getRenderState(interpT);

        // Clear
        renderer.clear({8,8,16});

        // Draw scene dependent background
        if(app.getSceneID() == FIS4::SceneID::ROTATING_GRID) {
            renderer.drawGrid(renderState.gridPhase * 40.f);
        } else if(app.getSceneID() == FIS4::SceneID::TUNNEL) {
            renderer.drawTunnel(renderState.tunnelOffset);
        } else {
            // subtle grid
            renderer.drawGrid(renderState.gridPhase * 20.f);
        }

        // Draw objects
        // Colors derived from hue for distinct motion trails
        for(size_t i=0;i<renderState.balls.size();++i){
            const auto &b = renderState.balls[i];
            // Distinct colors per ball
            uint8_t r = (uint8_t)(120 + 120*sinf(b.hue*6.28f));
            uint8_t g = (uint8_t)(120 + 120*sinf(b.hue*6.28f + 2.f));
            uint8_t bl= (uint8_t)(120 + 120*sinf(b.hue*6.28f + 4.f));
            Color col{ r,g,bl };
            // If interpolated, add trail / ghost highlight
            if(app.isFIS4Enabled() && doNativeTick==false) {
                // interpolated frame: slightly dimmer to visualize generated frames
                // we will draw a faint ghost of native vs interpolated
                col.r = col.r * 0.9; col.g = col.g * 0.9; col.b = col.b * 0.95;
            }
            renderer.drawBall(b, col);
        }
        // Rotating bar
        if(app.getSceneID() != FIS4::SceneID::TUNNEL) {
            renderer.drawBar(renderState.bar, {255,200,40});
        }

        // HUD
        renderer.drawHUD(app);

        // Present
        renderer.present((int)frameId);
        frameId++;

        // Stats
        app.tickFrame(nowMs);

        // Frame pacing: aim for displayed FPS (effective FPS)
        double effectiveFPS = app.getEffectiveFPS();
        double frameBudgetMs = 1000.0 / effectiveFPS;
        // If FIS4 OFF, budget = native; if ON, budget = native*2
        // We already paced native ticks; now throttle display
        // Use vsync via FrameWait, but also add small yield to avoid busy loop
        auto frameEnd = clock::now();
        double busyMs = std::chrono::duration<double, std::milli>(frameEnd - now).count();
        double remainMs = frameBudgetMs - busyMs;
        if(remainMs > 1.0) {
            // Sleep in small increments; sceKernelUsleep if available else std::this_thread
            sceKernelUsleep((uint32_t)(remainMs * 900)); // slight undersleep to let vsync catch
        }
    }

    // Cleanup
    scePadClose(padHandle);
    sceSystemServiceTerm();
    return 0;
}
