#include "demo_scene.h"

namespace FIS4 {

void Ball::update(float dt, int boundsW, int boundsH) {
    pos = pos + vel * dt;
    if (pos.x - radius < 0 || pos.x + radius > boundsW) { vel.x = -vel.x; pos.x = pos.x < radius ? radius : (pos.x > boundsW - radius ? boundsW - radius : pos.x); }
    if (pos.y - radius < 0 || pos.y + radius > boundsH) { vel.y = -vel.y; pos.y = pos.y < radius ? radius : (pos.y > boundsH - radius ? boundsH - radius : pos.y); }
    trailPhase += dt * 8.f;
}
ObjectState Ball::toState(double ts, uint64_t fid) const { ObjectState s; s.pos=pos; s.vel=vel; s.timestampMs=ts; s.frameId=fid; return s; }

void RotatingBar::update(float dt) { angle += angularVel * dt; if(angle >= 360.f) angle -= 360.f; }
ObjectState RotatingBar::toState(double ts, uint64_t fid) const { ObjectState s; s.pos=center; s.vel={angularVel,0}; s.angle=angle; s.timestampMs=ts; s.frameId=fid; return s; }

DemoScene::DemoScene(int w,int h): W(w), H(h) { reset(); }

void DemoScene::reset() {
    native_ = {}; prevNative_ = {}; hasPrev=false;
    if(scene==SceneID::BOUNCING_BALLS){
        native_.balls.resize(5);
        // deterministic positions/velocities to make FPS differences obvious
        native_.balls[0] = {{200,200},{520,380},34, 0.f,0};
        native_.balls[1] = {{600,400},{-480,520},28, 0.2f,1};
        native_.balls[2] = {{1000,300},{380,-460},22, 0.4f,2};
        native_.balls[3] = {{1400,700},{-620,-340},40, 0.6f,3};
        native_.balls[4] = {{900,800},{500,600},18, 0.8f,4};
        native_.bar.center={(float)W*0.5f,(float)H*0.5f};
        native_.bar.length=400; native_.bar.angularVel=45;
    } else if(scene==SceneID::ROTATING_GRID){
        native_.bar.center={(float)W*0.5f,(float)H*0.5f};
        native_.bar.length=600; native_.bar.angularVel=60;
        native_.gridPhase=0;
        native_.balls.resize(1);
        native_.balls[0]={{(float)W*0.5f,(float)H*0.5f},{600,0},20,0,0};
    } else if(scene==SceneID::TUNNEL){
        native_.tunnelOffset={0,0};
        native_.balls.resize(3);
        native_.balls[0]={{(float)W*0.5f,200},{700,0},26,0,0};
        native_.balls[1]={{(float)W*0.5f,500},{-700,0},30,0.3f,1};
        native_.balls[2]={{(float)W*0.5f,800},{700,0},22,0.6f,2};
    } else { // STRESS - many balls
        native_.balls.resize(12);
        for(int i=0;i<12;i++){
            float x = 100 + (i*140)% (W-200);
            float y = 100 + (i*90) % (H-200);
            float vx = 300 + (i*70)%400; if(i%2) vx=-vx;
            float vy = 320 + (i*60)%380; if(i%3==0) vy=-vy;
            native_.balls[i]={{x,y},{vx,vy}, 14 + (i%4)*4, (i/12.f), (float)i};
        }
        native_.bar.center={(float)W*0.5f,(float)H*0.5f};
        native_.bar.angularVel=90;
    }
}

void DemoScene::nextScene(){ scene = (SceneID)(((int)scene+1)%(int)SceneID::COUNT); reset(); }
void DemoScene::prevScene(){ int v=(int)scene-1; if(v<0) v=(int)SceneID::COUNT-1; scene=(SceneID)v; reset(); }
const char* DemoScene::getSceneName() const {
    switch(scene){
        case SceneID::BOUNCING_BALLS: return "BOUNCING BALLS (Motion Clarity Test)";
        case SceneID::ROTATING_GRID:  return "ROTATING GRID (Judder Test)";
        case SceneID::TUNNEL:         return "TUNNEL RUNNER (Parallax Test)";
        case SceneID::STRESS:         return "STRESS 12 BALLS (Throughput Test)";
        default: return "UNKNOWN";
    }
}

void DemoScene::update(float dt, double nowMs, uint64_t frameId){
    (void)nowMs; (void)frameId;
    prevNative_ = native_;
    hasPrev = true;
    switch(scene){
        case SceneID::BOUNCING_BALLS: updateBouncingBalls(dt); break;
        case SceneID::ROTATING_GRID: updateRotatingGrid(dt); break;
        case SceneID::TUNNEL: updateTunnel(dt); break;
        case SceneID::STRESS: updateStress(dt); break;
        default: break;
    }
    // generic bar update
    if(scene!=SceneID::TUNNEL) native_.bar.update(dt);
}

void DemoScene::updateBouncingBalls(float dt){ for(auto &b: native_.balls) b.update(dt,W,H); }
void DemoScene::updateRotatingGrid(float dt){
    native_.gridPhase += dt*2.f;
    // ball orbits center to expose stutter
    float t = native_.gridPhase;
    native_.balls[0].pos = {(float)W*0.5f + cosf(t*1.2f)*500.f, (float)H*0.5f + sinf(t*1.7f)*300.f };
    native_.balls[0].vel = {-sinf(t*1.2f)*500.f*1.2f, cosf(t*1.7f)*300.f*1.7f};
}
void DemoScene::updateTunnel(float dt){
    native_.tunnelOffset.x += dt*200.f;
    native_.tunnelOffset.y += dt*120.f;
    if(native_.tunnelOffset.x>200) native_.tunnelOffset.x-=200;
    if(native_.tunnelOffset.y>200) native_.tunnelOffset.y-=200;
    for(auto &b: native_.balls) b.update(dt,W,H);
}
void DemoScene::updateStress(float dt){ for(auto &b: native_.balls) b.update(dt,W,H); native_.gridPhase+=dt; }

DemoScene::RenderState DemoScene::getInterpolated(float t) const {
    if(!hasPrev || t<=0.f) return native_;
    if(t>=1.f) return native_;
    RenderState out;
    out.bar = native_.bar;
    // Interpolate bar angle
    float dAngle = native_.bar.angle - prevNative_.bar.angle;
    while(dAngle>180) dAngle-=360; while(dAngle<-180) dAngle+=360;
    out.bar.angle = prevNative_.bar.angle + dAngle*t;
    // Interpolate balls
    size_t n = native_.balls.size();
    out.balls.resize(n);
    for(size_t i=0;i<n;i++){
        const auto &a = prevNative_.balls[i];
        const auto &b = native_.balls[i];
        Ball m;
        m.pos = a.pos.lerp(b.pos, t);
        m.vel = a.vel.lerp(b.vel, t);
        m.radius = a.radius + (b.radius-a.radius)*t;
        m.hue = b.hue;
        m.trailPhase = b.trailPhase;
        out.balls[i]=m;
    }
    out.tunnelOffset = prevNative_.tunnelOffset.lerp(native_.tunnelOffset, t);
    out.gridPhase = prevNative_.gridPhase + (native_.gridPhase - prevNative_.gridPhase)*t;
    return out;
}

Vec2 DemoScene::getPrimaryPos() const { return native_.balls.empty()? Vec2{W*0.5f,H*0.5f} : native_.balls[0].pos; }
Vec2 DemoScene::getPrimaryVel() const { return native_.balls.empty()? Vec2{0,0} : native_.balls[0].vel; }

} // namespace FIS4
