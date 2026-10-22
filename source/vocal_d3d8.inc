/* Native Direct3D8 text overlay. The texture contains only glyph alpha.
 * Wine interface headers are build-only, with their license headers retained. */
#ifndef __MSABI_LONG
#define __MSABI_LONG(x) x##L
#endif
#include "d3d8.h"
#define CAP_W 1024
#define CAP_H 128
static IDirect3DTexture8 *captionTexture;
static IDirect3DDevice8 *captionDevice;
static HDC captionDC;
static HBITMAP captionBitmap;
static DWORD *captionPixels;
static WCHAR previousCaption[256];
static HWND gameWindow;
static BOOL renderingCaption;
static DWORD renderThread;
static FARPROC (WINAPI *realGetProc)(HMODULE,LPCSTR);
static IDirect3D8* (WINAPI *realD3DCreate)(UINT);
typedef struct CAP_VERTEX{float x,y,z,rhw;DWORD color;float u,v;} CAP_VERTEX;

static void releaseCaptionTexture(void){IDirect3DTexture8 *t=captionTexture;captionTexture=NULL;captionDevice=NULL;previousCaption[0]=0;if(t)IDirect3DTexture8_Release(t);}
static BOOL updateCaptionTexture(IDirect3DDevice8 *device,LPCWSTR text){
    D3DLOCKED_RECT lock;unsigned x,y;BYTE mask[CAP_W*CAP_H];HRESULT hr;BOOL bilingual=wcschr(text,L'\n')!=NULL;
    if(captionTexture&&captionDevice!=device)releaseCaptionTexture();
    if(!captionTexture){hr=IDirect3DDevice8_CreateTexture(device,CAP_W,CAP_H,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&captionTexture);if(FAILED(hr)){logline("TEXTURE ERROR %lx\r\n",hr);return FALSE;}captionDevice=device;}
    if(lstrcmpW(text,previousCaption)==0)return TRUE;
    if(!captionDC){
        BITMAPINFO bi;HDC dc;HBITMAP bitmap;DWORD*pixels=NULL;
        ZeroMemory(&bi,sizeof(bi));bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=CAP_W;bi.bmiHeader.biHeight=-CAP_H;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;
        dc=CreateCompatibleDC(NULL);if(!dc)return FALSE;
        bitmap=CreateDIBSection(dc,&bi,DIB_RGB_COLORS,(void**)&pixels,NULL,0);
        if(!bitmap||!pixels){if(bitmap)DeleteObject(bitmap);DeleteDC(dc);return FALSE;}
        SelectObject(dc,bitmap);captionDC=dc;captionBitmap=bitmap;captionPixels=pixels;
    }
    ZeroMemory(captionPixels,CAP_W*CAP_H*4);
    if(!vocalPaint(captionDC,text,0,-VOCAL_BAND_TOP,CAP_W,TRUE))return FALSE;
    for(y=0;y<CAP_H;y++)for(x=0;x<CAP_W;x++)mask[y*CAP_W+x]=(BYTE)(captionPixels[y*CAP_W+x]&255);
    hr=IDirect3DTexture8_LockRect(captionTexture,0,&lock,NULL,0);if(FAILED(hr))return FALSE;
    for(y=0;y<CAP_H;y++){
        DWORD*dst=(DWORD*)((BYTE*)lock.pBits+y*lock.Pitch);
        unsigned fgGreen=bilingual&&y<VOCAL_KOREAN_TOP-VOCAL_BAND_TOP?255:232;
        unsigned fgBlue=bilingual&&y<VOCAL_KOREAN_TOP-VOCAL_BAND_TOP?255:112;
        for(x=0;x<CAP_W;x++){
            unsigned alpha=0,inner=mask[y*CAP_W+x],red,green,blue;int dx,dy;
            for(dy=-VOCAL_OUTLINE;dy<=VOCAL_OUTLINE;dy++)for(dx=-VOCAL_OUTLINE;dx<=VOCAL_OUTLINE;dx++){
                int xx=(int)x+dx,yy=(int)y+dy;unsigned coverage;if(xx<0||yy<0||xx>=CAP_W||yy>=CAP_H)continue;
                coverage=mask[yy*CAP_W+xx];if(coverage>alpha)alpha=coverage;
            }
            red=alpha?(inner*255+(alpha-inner)*10)/alpha:0;green=alpha?(inner*fgGreen+(alpha-inner)*10)/alpha:0;blue=alpha?(inner*fgBlue+(alpha-inner)*10)/alpha:0;
            dst[x]=(alpha<<24)|(red<<16)|(green<<8)|blue;
        }
    }
    IDirect3DTexture8_UnlockRect(captionTexture,0);lstrcpynW(previousCaption,text,256);return TRUE;
}
static void renderCaption(IDirect3DDevice8 *device){
    DWORD token;D3DVIEWPORT8 vp;IDirect3DSurface8 *rt=NULL,*back=NULL;HRESULT hr;WCHAR text[256];CAP_VERTEX v[4];double seconds=0;BOOL active=FALSE;float left,top;
    captionHotkey(gameWindow);
    if(!IsWindowEnabled(gameWindow)||IsIconic(gameWindow))return;
    if(!captionForDisplay(text,256,&seconds))return;
    if(FAILED(IDirect3DDevice8_GetRenderTarget(device,&rt)))return;
    if(FAILED(IDirect3DDevice8_GetBackBuffer(device,0,D3DBACKBUFFER_TYPE_MONO,&back))){IDirect3DSurface8_Release(rt);return;}
    active=rt==back;IDirect3DSurface8_Release(rt);IDirect3DSurface8_Release(back);if(!active)return;
    if(FAILED(IDirect3DDevice8_GetViewport(device,&vp))||vp.Width<600||vp.Height<400)return;
    if(!updateCaptionTexture(device,text))return;
    if(FAILED(IDirect3DDevice8_CreateStateBlock(device,D3DSBT_ALL,&token)))return;
    IDirect3DDevice8_SetVertexShader(device,D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1);IDirect3DDevice8_SetPixelShader(device,0);
    IDirect3DDevice8_SetRenderState(device,D3DRS_ZENABLE,FALSE);IDirect3DDevice8_SetRenderState(device,D3DRS_ZWRITEENABLE,FALSE);IDirect3DDevice8_SetRenderState(device,D3DRS_LIGHTING,FALSE);IDirect3DDevice8_SetRenderState(device,D3DRS_FOGENABLE,FALSE);IDirect3DDevice8_SetRenderState(device,D3DRS_ALPHATESTENABLE,FALSE);IDirect3DDevice8_SetRenderState(device,D3DRS_CULLMODE,D3DCULL_NONE);IDirect3DDevice8_SetRenderState(device,D3DRS_ALPHABLENDENABLE,TRUE);IDirect3DDevice8_SetRenderState(device,D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);IDirect3DDevice8_SetRenderState(device,D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
    IDirect3DDevice8_SetTexture(device,0,(IDirect3DBaseTexture8*)captionTexture);IDirect3DDevice8_SetTextureStageState(device,0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);IDirect3DDevice8_SetTextureStageState(device,0,D3DTSS_COLORARG1,D3DTA_TEXTURE);IDirect3DDevice8_SetTextureStageState(device,0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);IDirect3DDevice8_SetTextureStageState(device,0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);IDirect3DDevice8_SetTextureStageState(device,0,D3DTSS_MINFILTER,D3DTEXF_POINT);IDirect3DDevice8_SetTextureStageState(device,0,D3DTSS_MAGFILTER,D3DTEXF_POINT);IDirect3DDevice8_SetTextureStageState(device,0,D3DTSS_ADDRESSU,D3DTADDRESS_CLAMP);IDirect3DDevice8_SetTextureStageState(device,0,D3DTSS_ADDRESSV,D3DTADDRESS_CLAMP);IDirect3DDevice8_SetTextureStageState(device,1,D3DTSS_COLOROP,D3DTOP_DISABLE);IDirect3DDevice8_SetTextureStageState(device,1,D3DTSS_ALPHAOP,D3DTOP_DISABLE);
    IDirect3DDevice8_SetRenderState(device,D3DRS_STENCILENABLE,FALSE);
    IDirect3DDevice8_SetRenderState(device,D3DRS_BLENDOP,D3DBLENDOP_ADD);
    IDirect3DDevice8_SetRenderState(device,D3DRS_COLORWRITEENABLE,15);
    IDirect3DDevice8_SetRenderState(device,D3DRS_FILLMODE,D3DFILL_SOLID);
    IDirect3DDevice8_SetRenderState(device,D3DRS_WRAP0,0);
    IDirect3DDevice8_SetTextureStageState(device,0,D3DTSS_TEXCOORDINDEX,0);
    IDirect3DDevice8_SetTextureStageState(device,0,D3DTSS_RESULTARG,D3DTA_CURRENT);
    IDirect3DDevice8_SetTextureStageState(device,0,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_DISABLE);
    IDirect3DDevice8_SetTextureStageState(device,0,D3DTSS_MIPFILTER,D3DTEXF_NONE);
    left=(float)vp.X+((float)vp.Width-CAP_W)/2-.5f;top=(float)vp.Y+VOCAL_BAND_TOP-.5f;
    v[0]=(CAP_VERTEX){left,top,0,1,0xffffffff,0,0};v[1]=(CAP_VERTEX){left+CAP_W,top,0,1,0xffffffff,1,0};v[2]=(CAP_VERTEX){left,top+CAP_H,0,1,0xffffffff,0,1};v[3]=(CAP_VERTEX){left+CAP_W,top+CAP_H,0,1,0xffffffff,1,1};
    hr=IDirect3DDevice8_DrawPrimitiveUP(device,D3DPT_TRIANGLESTRIP,2,v,sizeof(v[0]));IDirect3DDevice8_ApplyStateBlock(device,token);IDirect3DDevice8_DeleteStateBlock(device,token);
    if((InterlockedIncrement(&draws)%300)==1)logline("D3D DRAW %ld seconds=%.3f hr=%lx viewport=%lu,%lu\r\n",draws,seconds,hr,vp.Width,vp.Height);
}
static HRESULT WINAPI d3dEndScene(IDirect3DDevice8*p){HRESULT(WINAPI*fn)(IDirect3DDevice8*)=(void*)original(p,35);if(GetCurrentThreadId()==renderThread&&!renderingCaption){renderingCaption=TRUE;renderCaption(p);renderingCaption=FALSE;}return fn(p);}
static HRESULT WINAPI d3dReset(IDirect3DDevice8*p,D3DPRESENT_PARAMETERS*params){HRESULT(WINAPI*fn)(IDirect3DDevice8*,D3DPRESENT_PARAMETERS*)=(void*)original(p,14);releaseCaptionTexture();return fn(p,params);}
static ULONG WINAPI d3dRelease(IDirect3DDevice8*p){ULONG(WINAPI*fn)(IDirect3DDevice8*)=(void*)original(p,2);if(captionDevice==p&&!renderingCaption)releaseCaptionTexture();return fn(p);}
static HRESULT WINAPI d3dDevice(IDirect3D8*p,UINT adapter,D3DDEVTYPE type,HWND focus,DWORD flags,D3DPRESENT_PARAMETERS*params,IDirect3DDevice8**out){HRESULT(WINAPI*fn)(IDirect3D8*,UINT,D3DDEVTYPE,HWND,DWORD,D3DPRESENT_PARAMETERS*,IDirect3DDevice8**)=(void*)original(p,15);HRESULT r=fn(p,adapter,type,focus,flags,params,out);if(SUCCEEDED(r)&&out&&*out){gameWindow=focus;captionAttachWindow(focus);renderThread=GetCurrentThreadId();method(*out,35,d3dEndScene);method(*out,14,d3dReset);method(*out,2,d3dRelease);logline("D3D DEVICE %p hwnd=%p\r\n",*out,focus);}return r;}
static IDirect3D8* WINAPI d3dCreate(UINT sdk){IDirect3D8*p=realD3DCreate(sdk);if(p){method(p,15,d3dDevice);logline("D3D CREATE %p\r\n",p);}return p;}
static FARPROC WINAPI getProc(HMODULE m,LPCSTR name){FARPROC result=realGetProc(m,name);WCHAR path[MAX_PATH],*base;if(result&&GetModuleFileNameW(m,path,MAX_PATH)){base=wcsrchr(path,L'\\');base=base?base+1:path;if(lstrcmpiW(base,L"D3D8.DLL")==0&&(IS_INTRESOURCE(name)?(ULONG_PTR)name==1:lstrcmpA(name,"Direct3DCreate8")==0)){realD3DCreate=(void*)result;return(FARPROC)d3dCreate;}}return result;}
