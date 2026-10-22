/* DirectDraw7 presentation used by the game's ordinary 2D scenes. */
#include <ddraw.h>
typedef HRESULT(WINAPI*DDCREATEEX)(GUID*,void**,REFIID,IUnknown*);
static DDCREATEEX realDDCreateEx;
static LONG ddBusy;
static HWND ddWindow;
static IDirectDraw7 *ddOwner;
static IDirectDrawSurface7 *ddScratch;
static DDSURFACEDESC2 ddScratchDesc;
static BOOL ddPresentLogged;
static unsigned ddErrors;
static const GUID vocalIID_DD7={0x15e65ec0,0x3b9c,0x11d2,{0xb9,0x2f,0x00,0x60,0x97,0x97,0xea,0x5b}};
typedef struct DD_SAVED {
    HDC dc;HBITMAP bitmap,oldBitmap;int width;RECT rect;
    IDirectDrawSurface7*surface;BOOL active;
} DD_SAVED;
static DD_SAVED ddFlipSaved;
static void ddFailure(const char*where,HRESULT hr){if(ddErrors++<8)logline("DDRAW %s failed %lx\r\n",where,hr);}
static BOOL ddPrimary(IDirectDrawSurface7*p){DDSCAPS2 caps;ZeroMemory(&caps,sizeof(caps));return SUCCEEDED(IDirectDrawSurface7_GetCaps(p,&caps))&&(caps.dwCaps&DDSCAPS_PRIMARYSURFACE);}
static BOOL ddBackupDC(DD_SAVED*s,HDC dc,int width){
    if(s->dc&&s->width==width)return TRUE;
    if(s->dc){SelectObject(s->dc,s->oldBitmap);DeleteObject(s->bitmap);DeleteDC(s->dc);ZeroMemory(s,sizeof(*s));}
    s->dc=CreateCompatibleDC(dc);if(!s->dc)return FALSE;
    {BITMAPINFO bi;void*pixels;ZeroMemory(&bi,sizeof(bi));bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=width;bi.bmiHeader.biHeight=-VOCAL_BAND_HEIGHT;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;s->bitmap=CreateDIBSection(dc,&bi,DIB_RGB_COLORS,&pixels,NULL,0);}
    if(!s->bitmap){DeleteDC(s->dc);s->dc=NULL;return FALSE;}
    s->oldBitmap=SelectObject(s->dc,s->bitmap);s->width=width;return TRUE;
}
/* Draw only into our private staging surface for windowed Blt. Fullscreen
   post-Flip drawing retains a lossless backup of the current front strip. */
static BOOL ddStamp(IDirectDrawSurface7*p,const RECT*source,DD_SAVED*save){
    DDSURFACEDESC2 desc;RECT area,band;HDC dc;WCHAR text[256];double seconds=0;HRESULT hr;BOOL painted=FALSE;
    if(!captionForDisplay(text,256,&seconds))return FALSE;
    ZeroMemory(&desc,sizeof(desc));desc.dwSize=sizeof(desc);
    if(FAILED(IDirectDrawSurface7_GetSurfaceDesc(p,&desc)))return FALSE;
    area=source?*source:(RECT){0,0,(LONG)desc.dwWidth,(LONG)desc.dwHeight};
    if(area.left<0||area.top<0||area.right>(LONG)desc.dwWidth||area.bottom>(LONG)desc.dwHeight||area.right-area.left<600||area.right-area.left>4096||area.bottom-area.top<400)return FALSE;
    band=(RECT){area.left,area.top+VOCAL_BAND_TOP,area.right,area.top+VOCAL_BAND_TOP+VOCAL_BAND_HEIGHT};
    hr=IDirectDrawSurface7_GetDC(p,&dc);if(FAILED(hr)){ddFailure("GetDC",hr);return FALSE;}
    if(save){
        if(!ddBackupDC(save,dc,band.right-band.left)||!BitBlt(save->dc,0,0,save->width,VOCAL_BAND_HEIGHT,dc,band.left,band.top,SRCCOPY))goto done;
        save->surface=p;save->rect=band;save->active=TRUE;
    }
    painted=vocalPaint(dc,text,area.left,area.top,area.right-area.left,FALSE);
    if(painted&&(InterlockedIncrement(&draws)%300)==1)logline("DDRAW DRAW %ld seconds=%.3f\r\n",draws,seconds);
done:
    IDirectDrawSurface7_ReleaseDC(p,dc);return painted;
}
static HRESULT ddRestore(DD_SAVED*s){
    HDC dc;HRESULT hr;if(!s->active||!s->surface)return DD_OK;
    hr=IDirectDrawSurface7_GetDC(s->surface,&dc);
    if(hr==DDERR_SURFACELOST){s->active=FALSE;s->surface=NULL;return DD_OK;}
    if(FAILED(hr)){ddFailure("restore GetDC",hr);return hr;}
    if(!BitBlt(dc,s->rect.left,s->rect.top,s->width,VOCAL_BAND_HEIGHT,s->dc,0,0,SRCCOPY))hr=E_FAIL;
    GdiFlush();IDirectDrawSurface7_ReleaseDC(s->surface,dc);
    if(SUCCEEDED(hr)){s->active=FALSE;s->surface=NULL;}
    return hr;
}
/* An acquired/released DC fences the preceding use of our staging surface.
   Busy surfaces simply fall back to the game's original presentation. */
static IDirectDrawSurface7*ddStage(IDirectDrawSurface7*src,const RECT*rect){
    DDSURFACEDESC2 desc,create;RECT area;HDC dc;WCHAR text[256];double seconds=0;HRESULT hr;
    if(!ddOwner||!captionForDisplay(text,256,&seconds))return NULL;
    ZeroMemory(&desc,sizeof(desc));desc.dwSize=sizeof(desc);
    if(FAILED(IDirectDrawSurface7_GetSurfaceDesc(src,&desc)))return NULL;
    area=rect?*rect:(RECT){0,0,(LONG)desc.dwWidth,(LONG)desc.dwHeight};
    if(area.left<0||area.top<0||area.right>(LONG)desc.dwWidth||area.bottom>(LONG)desc.dwHeight||area.right-area.left<600||area.right-area.left>4096||area.bottom-area.top<400)return NULL;
    if(ddScratch&&(ddScratchDesc.dwWidth!=(DWORD)(area.right-area.left)||ddScratchDesc.dwHeight!=(DWORD)(area.bottom-area.top)||memcmp(&ddScratchDesc.ddpfPixelFormat,&desc.ddpfPixelFormat,sizeof(DDPIXELFORMAT)))){IDirectDrawSurface7_Release(ddScratch);ddScratch=NULL;}
    if(!ddScratch){
        ZeroMemory(&create,sizeof(create));create.dwSize=sizeof(create);create.dwFlags=DDSD_CAPS|DDSD_WIDTH|DDSD_HEIGHT|DDSD_PIXELFORMAT;create.dwWidth=area.right-area.left;create.dwHeight=area.bottom-area.top;create.ddpfPixelFormat=desc.ddpfPixelFormat;create.ddsCaps.dwCaps=DDSCAPS_OFFSCREENPLAIN|DDSCAPS_SYSTEMMEMORY;
        hr=IDirectDraw7_CreateSurface(ddOwner,&create,&ddScratch,NULL);if(FAILED(hr)){ddFailure("staging surface",hr);return NULL;}ddScratchDesc=create;
    }
    hr=IDirectDrawSurface7_GetDC(ddScratch,&dc);if(FAILED(hr))return NULL;
    IDirectDrawSurface7_ReleaseDC(ddScratch,dc);
    hr=IDirectDrawSurface7_Blt(ddScratch,NULL,src,&area,DDBLT_WAIT,NULL);if(FAILED(hr))return NULL;
    return ddStamp(ddScratch,NULL,NULL)?ddScratch:NULL;
}
static BOOL ddEnter(IDirectDrawSurface7*p){return ddPrimary(p)&&!InterlockedCompareExchange(&ddBusy,1,0);}
static HRESULT WINAPI ddBlt(IDirectDrawSurface7*p,RECT*dst,IDirectDrawSurface7*src,RECT*rect,DWORD flags,DDBLTFX*fx){
    HRESULT(WINAPI*fn)(IDirectDrawSurface7*,RECT*,IDirectDrawSurface7*,RECT*,DWORD,DDBLTFX*)=(void*)original(p,5);HRESULT hr;IDirectDrawSurface7*staged=NULL;
    if(!ddEnter(p))return fn(p,dst,src,rect,flags,fx);
    hr=ddRestore(&ddFlipSaved);if(FAILED(hr)){InterlockedExchange(&ddBusy,0);return hr;}captionHotkey(ddWindow);
    if(!ddPresentLogged){ddPresentLogged=TRUE;logline("DDRAW PRESENT Blt flags=%lx fx=%p\r\n",flags,fx);}
    if(src&&src!=p&&!fx&&!(flags&~(DDBLT_WAIT|DDBLT_DONOTWAIT)))staged=ddStage(src,rect);
    hr=fn(p,dst,staged?staged:src,staged?NULL:rect,flags,fx);
    if(staged&&FAILED(hr))hr=fn(p,dst,src,rect,flags,fx);
    InterlockedExchange(&ddBusy,0);return hr;
}
static HRESULT WINAPI ddBltFast(IDirectDrawSurface7*p,DWORD x,DWORD y,IDirectDrawSurface7*src,RECT*rect,DWORD flags){
    HRESULT(WINAPI*fn)(IDirectDrawSurface7*,DWORD,DWORD,IDirectDrawSurface7*,RECT*,DWORD)=(void*)original(p,7);HRESULT hr;IDirectDrawSurface7*staged=NULL;
    if(!ddEnter(p))return fn(p,x,y,src,rect,flags);
    hr=ddRestore(&ddFlipSaved);if(FAILED(hr)){InterlockedExchange(&ddBusy,0);return hr;}captionHotkey(ddWindow);
    if(src&&src!=p&&!(flags&~(DDBLTFAST_WAIT|DDBLTFAST_DONOTWAIT)))staged=ddStage(src,rect);
    if(staged){
        HRESULT(WINAPI*blt)(IDirectDrawSurface7*,RECT*,IDirectDrawSurface7*,RECT*,DWORD,DDBLTFX*)=(void*)original(p,5);
        RECT dst={(LONG)x,(LONG)y,(LONG)(x+ddScratchDesc.dwWidth),(LONG)(y+ddScratchDesc.dwHeight)};
        DWORD bltFlags=(flags&DDBLTFAST_WAIT?DDBLT_WAIT:0)|(flags&DDBLTFAST_DONOTWAIT?DDBLT_DONOTWAIT:0);
        /* BltFast requires display memory; our system-memory staging surface
           uses the equivalent unclipped Blt path, with original fallback. */
        hr=blt(p,&dst,staged,NULL,bltFlags,NULL);if(FAILED(hr))hr=fn(p,x,y,src,rect,flags);
    }else hr=fn(p,x,y,src,rect,flags);
    InterlockedExchange(&ddBusy,0);return hr;
}
static HRESULT WINAPI ddFlip(IDirectDrawSurface7*p,IDirectDrawSurface7*target,DWORD flags){
    HRESULT(WINAPI*fn)(IDirectDrawSurface7*,IDirectDrawSurface7*,DWORD)=(void*)original(p,11);HRESULT hr;
    if(!ddEnter(p))return fn(p,target,flags);
    /* Restore the old front before its memory becomes the next back buffer.
       After Flip, p names the NEW front: never restore a pre-flip back pointer. */
    hr=ddRestore(&ddFlipSaved);if(FAILED(hr)){InterlockedExchange(&ddBusy,0);return hr;}captionHotkey(ddWindow);hr=fn(p,target,flags);
    if(SUCCEEDED(hr))ddStamp(p,NULL,&ddFlipSaved);
    InterlockedExchange(&ddBusy,0);return hr;
}
static ULONG WINAPI ddRelease(IDirectDrawSurface7*p){
    ULONG(WINAPI*fn)(IDirectDrawSurface7*)=(void*)original(p,2);ULONG n=fn(p);
    if(!n&&ddFlipSaved.surface==p){ddFlipSaved.active=FALSE;ddFlipSaved.surface=NULL;}
    return n;
}
static HRESULT WINAPI ddCreateSurface(IDirectDraw7*p,DDSURFACEDESC2*desc,IDirectDrawSurface7**out,IUnknown*outer){
    HRESULT(WINAPI*fn)(IDirectDraw7*,DDSURFACEDESC2*,IDirectDrawSurface7**,IUnknown*)=(void*)original(p,6);HRESULT hr=fn(p,desc,out,outer);
    if(SUCCEEDED(hr)&&out&&*out){method(*out,2,ddRelease);method(*out,5,ddBlt);method(*out,7,ddBltFast);method(*out,11,ddFlip);logline("DDRAW SURFACE %p caps=%lx\r\n",*out,desc?desc->ddsCaps.dwCaps:0);}
    return hr;
}
static HRESULT WINAPI ddSetCooperative(IDirectDraw7*p,HWND window,DWORD flags){
    HRESULT(WINAPI*fn)(IDirectDraw7*,HWND,DWORD)=(void*)original(p,20);HRESULT hr;
    hr=ddRestore(&ddFlipSaved);if(FAILED(hr))return hr;hr=fn(p,window,flags);
    if(SUCCEEDED(hr)){ddWindow=window;gameWindow=window;captionAttachWindow(window);logline("DDRAW WINDOW %p flags=%lx\r\n",window,flags);}
    return hr;
}
static HRESULT WINAPI ddCreateEx(GUID*g,void**out,REFIID iid,IUnknown*outer){
    HRESULT hr=realDDCreateEx(g,out,iid,outer);
    if(SUCCEEDED(hr)&&out&&*out&&!memcmp(iid,&vocalIID_DD7,sizeof(GUID))){
        if(ddScratch){IDirectDrawSurface7_Release(ddScratch);ddScratch=NULL;}
        if(ddOwner)IDirectDraw7_Release(ddOwner);
        ddOwner=(IDirectDraw7*)*out;IDirectDraw7_AddRef(ddOwner);
        method(*out,6,ddCreateSurface);method(*out,20,ddSetCooperative);logline("DDRAW CREATE %p\r\n",*out);
    }
    return hr;
}
