#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <string>
#include "graphics.h"

#ifdef FIS4_PS4
#include "log.h"

Scene2D::Scene2D(int w, int h, int pixelDepth)
{
    this->width = w;
    this->height = h;
    this->depth = pixelDepth;
    this->frameBufferSize = this->width * this->height * this->depth;
}

Scene2D::~Scene2D() {
    if (videoMem) deallocateVideoMem();
}

bool Scene2D::Init(size_t memSize, int numFrameBuffers)
{
    int rc;
    this->video = sceVideoOutOpen(ORBIS_VIDEO_USER_MAIN, ORBIS_VIDEO_OUT_BUS_MAIN, 0, 0);
    this->videoMem = NULL;
    if (this->video < 0) {
        DEBUGLOG << "Failed to open video handle: " << std::string(strerror(errno));
        return false;
    }
#ifdef GRAPHICS_USES_FONT
    rc = sceSysmoduleLoadModule(ORBIS_SYSMODULE_FREETYPE_OL);
    if (rc < 0) { DEBUGLOG << "Failed to load freetype"; return false; }
    rc = FT_Init_FreeType(&this->ftLib);
    if (rc != 0) { DEBUGLOG << "Failed to init freetype"; return false; }
#endif
    if(!initFlipQueue()) { DEBUGLOG << "Failed to init flip queue"; return false; }
    if(!allocateVideoMem(memSize, 0x200000)) { DEBUGLOG << "Failed to alloc video mem"; return false; }
    if(!allocateFrameBuffers(numFrameBuffers)) { DEBUGLOG << "Failed to alloc framebuffers"; return false; }
    sceVideoOutSetFlipRate(this->video, 0);
    return true;
}

bool Scene2D::initFlipQueue() { int rc = sceKernelCreateEqueue(&flipQueue, "homebrew flip queue"); if(rc < 0) return false; sceVideoOutAddFlipEvent(flipQueue, this->video, 0); return true; }
bool Scene2D::allocateFrameBuffers(int num) {
    this->frameBuffers = new char*[num];
    for(int i=0;i<num;i++) this->frameBuffers[i] = this->allocateDisplayMem(frameBufferSize);
    sceVideoOutSetBufferAttribute(&this->attr, 0x80000000, 1, 0, this->width, this->height, this->width);
    return (sceVideoOutRegisterBuffers(this->video, 0, (void **)this->frameBuffers, num, &this->attr) == 0);
}
char *Scene2D::allocateDisplayMem(size_t size) { char *ptr = (char *)videoMemSP; videoMemSP += size; return ptr; }
bool Scene2D::allocateVideoMem(size_t size, int alignment) {
    int rc; this->directMemAllocationSize = (size + alignment - 1) / alignment * alignment;
    rc = sceKernelAllocateDirectMemory(0, sceKernelGetDirectMemorySize(), this->directMemAllocationSize, alignment, 3, &this->directMemOff);
    if(rc < 0) { this->directMemAllocationSize = 0; return false; }
    rc = sceKernelMapDirectMemory(&this->videoMem, this->directMemAllocationSize, 0x33, 0, this->directMemOff, alignment);
    if(rc < 0) { sceKernelReleaseDirectMemory(this->directMemOff, this->directMemAllocationSize); this->directMemOff=0; this->directMemAllocationSize=0; return false; }
    this->videoMemSP = (uintptr_t)this->videoMem; return true;
}
void Scene2D::deallocateVideoMem() { sceKernelReleaseDirectMemory(this->directMemOff, this->directMemAllocationSize); this->videoMem=0; this->videoMemSP=0; this->directMemOff=0; this->directMemAllocationSize=0; delete this->frameBuffers; this->frameBuffers=0; }
void Scene2D::SetActiveFrameBuffer(int index) { this->activeFrameBufferIdx = index; }
void Scene2D::SubmitFlip(int frameID) { sceVideoOutSubmitFlip(this->video, this->activeFrameBufferIdx, ORBIS_VIDEO_OUT_FLIP_VSYNC, frameID); }
void Scene2D::FrameWait(int frameID) {
    if(this->video == 0) return;
    for(;;) { OrbisVideoOutFlipStatus flipStatus; sceVideoOutGetFlipStatus(video, &flipStatus); if(flipStatus.flipArg == frameID) break; OrbisKernelEvent evt; int count; if(sceKernelWaitEqueue(this->flipQueue, &evt, 1, &count, 0) != 0) break; }
}
void Scene2D::FrameBufferSwap() { this->activeFrameBufferIdx = (this->activeFrameBufferIdx + 1) % 2; }
void Scene2D::FrameBufferClear() { Color blank = { 8, 8, 16 }; FrameBufferFill(blank); }
void Scene2D::FrameBufferFill(Color color) { DrawRectangle(0, 0, this->width, this->height, color); }
void Scene2D::DrawPixel(int x, int y, Color color) {
    if (x < 0 || x >= width || y < 0 || y >= height) return;
    int pixel = (y * this->width) + x;
    uint32_t encodedColor = 0xFF000000 | (color.r << 16) | (color.g << 8) | color.b;
    // framebuffer is XRGB 32-bit on PS4
    ((uint32_t *)this->frameBuffers[this->activeFrameBufferIdx])[pixel] = encodedColor;
}
void Scene2D::DrawRectangle(int x, int y, int w, int h, Color color) {
    int x0 = x < 0 ? 0 : x; int y0 = y < 0 ? 0 : y; int x1 = x + w > width ? width : x + w; int y1 = y + h > height ? height : y + h;
    for(int yp=y0; yp<y1; ++yp) for(int xp=x0; xp<x1; ++xp) DrawPixel(xp, yp, color);
}
void Scene2D::DrawCircle(int cx, int cy, int r, Color color) {
    // Midpoint circle filled
    for(int y=-r; y<=r; ++y) for(int x=-r; x<=r; ++x) if(x*x + y*y <= r*r) DrawPixel(cx+x, cy+y, color);
}
void Scene2D::DrawLine(int x0, int y0, int x1, int y1, Color color) {
    int dx = abs(x1-x0), sx = x0<x1?1:-1; int dy = -abs(y1-y0), sy = y0<y1?1:-1; int err = dx+dy;
    while(true){ DrawPixel(x0,y0,color); if(x0==x1 && y0==y1) break; int e2=2*err; if(e2>=dy){ err+=dy; x0+=sx;} if(e2<=dx){ err+=dx; y0+=sy; } }
}
void Scene2D::DrawRectOutline(int x, int y, int w, int h, Color color, int thickness) {
    DrawRectangle(x, y, w, thickness, color);
    DrawRectangle(x, y+h-thickness, w, thickness, color);
    DrawRectangle(x, y, thickness, h, color);
    DrawRectangle(x+w-thickness, y, thickness, h, color);
}
#ifdef GRAPHICS_USES_FONT
bool Scene2D::InitFont(FT_Face *face, const char *fontPath, int fontSize){ int rc=FT_New_Face(this->ftLib, fontPath,0,face); if(rc<0) return false; rc=FT_Set_Pixel_Sizes(*face,0,fontSize); return rc>=0; }
void Scene2D::DrawText(char *txt, FT_Face face, int startX, int startY, Color bgColor, Color fgColor){
    (void)bgColor; int xOffset=0, yOffset=0; FT_GlyphSlot slot=face->glyph;
    for(size_t n=0;n<strlen(txt);n++){
        FT_UInt gi=FT_Get_Char_Index(face, txt[n]); if(FT_Load_Glyph(face, gi, FT_LOAD_DEFAULT)) continue; if(FT_Render_Glyph(slot, ft_render_mode_normal)) continue;
        if(txt[n]=='\n'){ xOffset=0; yOffset+=slot->bitmap.rows + 4; continue; }
        for(int yPos=0;yPos<(int)slot->bitmap.rows;yPos++) for(int xPos=0;xPos<(int)slot->bitmap.width;xPos++){
            char pix=slot->bitmap.buffer[yPos*slot->bitmap.width+xPos]; int x=startX+xPos+xOffset+slot->bitmap_left; int y=startY+yPos+yOffset-slot->bitmap_top;
            if(x<0||y<0||x>=width||y>=height) continue; uint8_t r=(pix*fgColor.r)/255,g=(pix*fgColor.g)/255,b=(pix*fgColor.b)/255; Color c={r,g,b}; DrawPixel(x,y,c);
        }
        xOffset += slot->advance.x >> 6;
    }
}
void Scene2D::DrawTextContainer(char *txt, FT_Face f,int sx,int sy,int mw,int mh,Color bg,Color fg){ (void)mw;(void)mh; DrawText(txt,f,sx,sy,bg,fg); }
#endif

#endif // FIS4_PS4
