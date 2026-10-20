#include "fis4_renderer.h"
#include <cstdio>
#include <cmath>
#include <cstring>

namespace FIS4 {

#ifdef FIS4_PS4

Color Renderer::hsv2rgb(float h){
    h = fmodf(h,1.f); if(h<0) h+=1;
    float r=0,g=0,b=0; int i=(int)(h*6); float f=h*6 - i; float q=1-f;
    switch(i%6){ case 0: r=1; g=f; b=0; break; case 1: r=q; g=1; b=0; break; case 2: r=0; g=1; b=f; break; case 3: r=0; g=q; b=1; break; case 4: r=f; g=0; b=1; break; case 5: r=1; g=0; b=q; break; }
    return {(uint8_t)(r*255),(uint8_t)(g*255),(uint8_t)(b*255)};
}
void Renderer::drawBall(const Ball& b, Color col){ gfx.DrawCircle((int)b.pos.x,(int)b.pos.y,(int)b.radius,col); // inner highlight
    Color hi={255,255,255}; gfx.DrawCircle((int)(b.pos.x - b.radius*0.3f),(int)(b.pos.y - b.radius*0.3f),(int)(b.radius*0.25f),hi);
    // velocity vector
    Color velc={255,255,0}; gfx.DrawLine((int)b.pos.x,(int)b.pos.y,(int)(b.pos.x + b.vel.x*0.04f),(int)(b.pos.y + b.vel.y*0.04f),velc);
}
void Renderer::drawBar(const RotatingBar& bar, Color col){
    float rad = bar.angle * 3.14159f/180.f; float half=bar.length*0.5f;
    float x0 = bar.center.x - cosf(rad)*half; float y0 = bar.center.y - sinf(rad)*half;
    float x1 = bar.center.x + cosf(rad)*half; float y1 = bar.center.y + sinf(rad)*half;
    // thick line via multiple lines
    for(int t=-2; t<=2; ++t){ float nx = -sinf(rad)*t, ny = cosf(rad)*t; gfx.DrawLine((int)(x0+nx),(int)(y0+ny),(int)(x1+nx),(int)(y1+ny),col); }
    gfx.DrawCircle((int)bar.center.x,(int)bar.center.y,6,{255,255,255});
}
void Renderer::drawGrid(float phase){
    int step=80; Color c1={20,30,60}, c2={30,40,80};
    for(int x= (int)phase%step; x<1920; x+=step) gfx.DrawLine(x,0,x,1080, (x/step)%2?c1:c2);
    for(int y= (int)(phase*0.7f)%step; y<1080; y+=step) gfx.DrawLine(0,y,1920,y, (y/step)%2?c1:c2);
}
void Renderer::drawTunnel(Vec2 off){
    for(int i=0;i<10;i++){ int s=200; Color col={(uint8_t)(30+i*10),(uint8_t)(50+i*8),(uint8_t)(80+i*12)}; gfx.DrawRectOutline((int)(off.x + s*i*0.5f),(int)(off.y + s*i*0.3f),1920 - i*60,1080 - i*60,col,2); }
}
void Renderer::drawTextLine(int x,int y, const char* txt, Color fg, Color bg){
    (void)bg; (void)fg; (void)x; (void)y; (void)txt;
    // TODO: font rendering via FT if enabled. For now we could use simple rectangle text placeholder.
    // We keep it minimal: draw a small indicator rect to prove HUD is alive.
}

void Renderer::drawHUD(App& app){
    // Top bar
    Color topBg={0,0,0}; topBg.r=0; topBg.g=0; topBg.b=0;
    gfx.DrawRectangle(0,0,1920,140,{12,12,20});
    // FIS4 status pill
    bool on = app.isFIS4Enabled();
    Color pill = on ? Color{0,220,120} : Color{220,40,40};
    gfx.DrawRectangle(20,20, 360, 50, pill);
    // Show FPS mode bar
    bool is30 = app.getBaseFPS()==BaseFPS::FPS_30;
    Color fpsBg={40,40,60}; gfx.DrawRectangle(400,20,420,50,fpsBg);
    Color fpsActive={80,160,255}; if(is30) gfx.DrawRectangle(400,20,210,50,fpsActive); else gfx.DrawRectangle(610,20,210,50,fpsActive);
    // Scene indicator
    gfx.DrawRectangle(20,90, 800, 30, {30,30,50});
    // Bottom controls
    gfx.DrawRectangle(0,980,1920,100,{12,12,20});
    // Graph area if enabled
    if(app.showGraph){
        gfx.DrawRectangle(1300,160, 580, 200, {18,18,28});
        gfx.DrawRectOutline(1300,160,580,200,{60,60,80},1);
        auto &hist = app.getFrameTimeHistory();
        int idx=0; for(float ft: hist){ int h = (int)(ft*4); if(h>180) h=180; int x=1300+ idx* (580/(int)hist.size()); int y=360 - h; gfx.DrawLine(x,360,x,y,{0,220,120}); idx++; }
        // target lines
        float targetMs = app.isFIS4Enabled() ? (1000.0/app.getEffectiveFPS()) : (1000.0/(app.getBaseFPS()==BaseFPS::FPS_30?30:60));
        int ty = 360 - (int)(targetMs*4); gfx.DrawLine(1300,ty,1880,ty,{255,200,0});
    }
}

#else // FIS4_PS4 not defined => PC

Color Renderer::hsv2rgb(float h){ h=fmodf(h,1.f); if(h<0)h+=1; float r=0,g=0,b=0; int i=(int)(h*6); float f=h*6-i; float q=1-f; switch(i%6){case 0:r=1;g=f;b=0;break;case1:r=q;g=1;b=0;break;case2:r=0;g=1;b=f;break;case3:r=0;g=q;b=1;break;case4:r=f;g=0;b=1;break;case5:r=1;g=0;b=q;break;} return {(uint8_t)(r*255),(uint8_t)(g*255),(uint8_t)(b*255),255}; }

#ifdef FIS4_SDL
void Renderer::clear(Color c){ SDL_SetRenderDrawColor(sdlRen, c.r,c.g,c.b,c.a); SDL_RenderClear(sdlRen); }
void Renderer::drawCircleSDL(int cx,int cy,int r){ for(int y=-r;y<=r;y++) for(int x=-r;x<=r;x++) if(x*x+y*y<=r*r) SDL_RenderDrawPoint(sdlRen,cx+x,cy+y); }
void Renderer::drawLineSDL(int x0,int y0,int x1,int y1){ SDL_RenderDrawLine(sdlRen,x0,y0,x1,y1); }
void Renderer::drawRectSDL(int x,int y,int w,int h){ SDL_Rect rc{x,y,w,h}; SDL_RenderDrawRect(sdlRen,&rc); }

void Renderer::drawBall(const Ball& b, Color col){
    SDL_SetRenderDrawColor(sdlRen,col.r,col.g,col.b,col.a);
    drawCircleSDL((int)b.pos.x,(int)b.pos.y,(int)b.radius);
    SDL_SetRenderDrawColor(sdlRen,255,255,255,200);
    drawCircleSDL((int)(b.pos.x - b.radius*0.3f),(int)(b.pos.y - b.radius*0.3f),(int)(b.radius*0.25f));
    SDL_SetRenderDrawColor(sdlRen,255,255,0,255);
    SDL_RenderDrawLine(sdlRen,(int)b.pos.x,(int)b.pos.y,(int)(b.pos.x+b.vel.x*0.04f),(int)(b.pos.y+b.vel.y*0.04f));
}
void Renderer::drawBar(const RotatingBar& bar, Color col){
    float rad=bar.angle*3.14159f/180.f; float half=bar.length*0.5f;
    SDL_SetRenderDrawColor(sdlRen,col.r,col.g,col.b,col.a);
    float x0=bar.center.x - cosf(rad)*half, y0=bar.center.y - sinf(rad)*half;
    float x1=bar.center.x + cosf(rad)*half, y1=bar.center.y + sinf(rad)*half;
    for(int t=-2;t<=2;t++){ float nx=-sinf(rad)*t, ny=cosf(rad)*t; SDL_RenderDrawLine(sdlRen,(int)(x0+nx),(int)(y0+ny),(int)(x1+nx),(int)(y1+ny)); }
    SDL_SetRenderDrawColor(sdlRen,255,255,255,255); drawCircleSDL((int)bar.center.x,(int)bar.center.y,6);
}
void Renderer::drawGrid(float phase){ int step=80; for(int x=(int)phase%step;x<W;x+=step){ SDL_SetRenderDrawColor(sdlRen, (x/step)%2?30:20, (x/step)%2?40:30, (x/step)%2?80:60,255); SDL_RenderDrawLine(sdlRen,x,0,x,H); } for(int y=(int)(phase*0.7f)%step;y<H;y+=step){ SDL_SetRenderDrawColor(sdlRen, (y/step)%2?30:20, (y/step)%2?40:30, (y/step)%2?80:60,255); SDL_RenderDrawLine(sdlRen,0,y,W,y); } }
void Renderer::drawTunnel(Vec2 off){ for(int i=0;i<10;i++){ SDL_SetRenderDrawColor(sdlRen,30+i*10,50+i*8,80+i*12,255); int pad=i*30; SDL_Rect rc{(int)off.x+pad/2,(int)off.y+pad/2, W-pad, H-pad}; SDL_RenderDrawRect(sdlRen,&rc); } }

void Renderer::drawHUD(App& app){
    // Top bar bg
    SDL_SetRenderDrawColor(sdlRen,12,12,20,255); SDL_Rect top{0,0,W,140}; SDL_RenderFillRect(sdlRen,&top);
    // FIS4 pill
    bool on=app.isFIS4Enabled(); SDL_SetRenderDrawColor(sdlRen, on?0:220,on?220:40,on?120:40,255); SDL_Rect pill{20,20,360,50}; SDL_RenderFillRect(sdlRen,&pill);
    // FPS toggle
    SDL_SetRenderDrawColor(sdlRen,40,40,60,255); SDL_Rect fpsBg{400,20,420,50}; SDL_RenderFillRect(sdlRen,&fpsBg);
    SDL_SetRenderDrawColor(sdlRen,80,160,255,255); bool is30=app.getBaseFPS()==BaseFPS::FPS_30; SDL_Rect fpsAct{ is30?400:610,20,210,50}; SDL_RenderFillRect(sdlRen,&fpsAct);
    SDL_SetRenderDrawColor(sdlRen,30,30,50,255); SDL_Rect sceneBg{20,90,800,30}; SDL_RenderFillRect(sdlRen,&sceneBg);
    // bottom
    SDL_SetRenderDrawColor(sdlRen,12,12,20,255); SDL_Rect bot{0,H-100,W,100}; SDL_RenderFillRect(sdlRen,&bot);
    if(app.showGraph){
        SDL_SetRenderDrawColor(sdlRen,18,18,28,255); SDL_Rect gr{1300,160,580,200}; SDL_RenderFillRect(sdlRen,&gr);
        SDL_SetRenderDrawColor(sdlRen,60,60,80,255); SDL_RenderDrawRect(sdlRen,&gr);
        auto &hist=app.getFrameTimeHistory();
        int n=hist.size(); if(n>0){ int idx=0; for(float ft: hist){ int h=(int)(ft*4); if(h>180) h=180; int x=1300+ idx*580/(n>1?n:1); int y=360-h; SDL_SetRenderDrawColor(sdlRen,0,220,120,255); SDL_RenderDrawLine(sdlRen,x,360,x,y); idx++; } }
        float targetMs = app.isFIS4Enabled() ? (1000.0/app.getEffectiveFPS()) : (1000.0/(app.getBaseFPS()==BaseFPS::FPS_30?30:60));
        int ty=360-(int)(targetMs*4); SDL_SetRenderDrawColor(sdlRen,255,200,0,255); SDL_RenderDrawLine(sdlRen,1300,ty,1880,ty);
    }
}
#else
// headless stubs already in header
#endif

#endif

} // namespace FIS4
