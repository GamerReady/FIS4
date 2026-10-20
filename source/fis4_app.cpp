#include "fis4_app.h"
#include <algorithm>

namespace FIS4 {

App::App(int w,int h): W(w), H(h), scene_(w,h) {}

void App::init(){
    interpolator_.setEnabled(true);
    interpolator_.setBaseFPS(BaseFPS::FPS_30);
    interpolator_.setMode(InterpMode::CLASSIC);
    interpolator_.setMultiplier(2);
    stats_={};
    ftHistory_.clear();
}

void App::handleInput(uint64_t buttons, float /*stickX*/, float /*stickY*/){
    // Map generic button bits (same as Orbis Pad but also works for PC keyboard)
    // Bit layout: 0=CROSS, 1=CIRCLE, 2=TRIANGLE, 3=SQUARE, 4=L1,5=R1,6=L2,7=R2,8=UP,9=DOWN,10=LEFT,11=RIGHT,12=OPTIONS,13=SHARE, etc.
    // We use simple masks:
    const uint64_t BTN_CROSS    = 1ULL<<0;
    const uint64_t BTN_CIRCLE   = 1ULL<<1;
    const uint64_t BTN_TRIANGLE = 1ULL<<2;
    const uint64_t BTN_SQUARE   = 1ULL<<3;
    const uint64_t BTN_UP       = 1ULL<<8;
    const uint64_t BTN_DOWN     = 1ULL<<9;
    const uint64_t BTN_LEFT     = 1ULL<<10;
    const uint64_t BTN_RIGHT    = 1ULL<<11;
    const uint64_t BTN_OPTIONS  = 1ULL<<12;

    if(buttons & BTN_CROSS){ toggleFIS4(); }
    if(buttons & BTN_SQUARE){ cycleInterpMode(); }
    if(buttons & BTN_TRIANGLE){ showGraph = !showGraph; }
    if(buttons & BTN_CIRCLE){ setPaused(!isPaused()); }
    if(buttons & BTN_UP){ // increase base FPS
        if(getBaseFPS()==BaseFPS::FPS_30) interpolator_.setBaseFPS(BaseFPS::FPS_60);
    }
    if(buttons & BTN_DOWN){
        if(getBaseFPS()==BaseFPS::FPS_60) interpolator_.setBaseFPS(BaseFPS::FPS_30);
    }
    if(buttons & BTN_LEFT){ prevScene(); }
    if(buttons & BTN_RIGHT){ nextScene(); }
    if(buttons & BTN_OPTIONS){ showHelp = !showHelp; }
}

void App::cycleBaseFPS(){
    if(interpolator_.getBaseFPS()==BaseFPS::FPS_30) interpolator_.setBaseFPS(BaseFPS::FPS_60);
    else interpolator_.setBaseFPS(BaseFPS::FPS_30);
}
void App::cycleInterpMode(){
    auto m = interpolator_.getMode();
    if(m==InterpMode::CLASSIC) m=InterpMode::AI;
    else if(m==InterpMode::AI) m=InterpMode::HYBRID;
    else m=InterpMode::CLASSIC;
    interpolator_.setMode(m);
}

void App::updateNative(float dt, double nowMs){
    if(paused_) return;
    scene_.update(dt, nowMs, stats_.nativeFrames);
    // Push primary object state into interpolator for demo (first ball is reference)
    auto pos = scene_.getPrimaryPos();
    auto vel = scene_.getPrimaryVel();
    ObjectState st{pos, vel, 0,1, stats_.nativeFrames, nowMs};
    interpolator_.pushFrame(st, nowMs);
    stats_.nativeFrames++;
}

DemoScene::RenderState App::getRenderState(float interpT){
    if(!interpolator_.isEnabled() || interpolator_.getMultiplier()==1) return scene_.getNativeState();
    // Interpolated state between prev and current
    return scene_.getInterpolated(interpT);
}

void App::tickFrame(double nowMs){
    double dt = lastFrameMs>0 ? (nowMs - lastFrameMs) : 16.0;
    lastFrameMs = nowMs;
    stats_.frameTimeMs = dt;
    stats_.totalFrames++;
    if(interpolator_.isEnabled()) stats_.interpolatedFrames++;
    else stats_.nativeFrames++;

    // FPS calc
    double fps = dt>0 ? 1000.0/dt : 0;
    stats_.instantFPS = fps;
    fpsHistory_.push_back(fps);
    if(fpsHistory_.size()>60) fpsHistory_.pop_front();
    double sum=0; for(auto v:fpsHistory_) sum+=v;
    stats_.avgFPS = fpsHistory_.empty()?0:sum/fpsHistory_.size();

    ftHistory_.push_back((float)dt);
    if(ftHistory_.size()>120) ftHistory_.pop_front();
}

} // namespace FIS4
