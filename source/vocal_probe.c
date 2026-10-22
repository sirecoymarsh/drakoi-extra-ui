#define WIN32_LEAN_AND_MEAN
#define COBJMACROS
#define DIRECTSOUND_VERSION 0x0800
#define DIRECTDRAW_VERSION 0x0700
#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>
#include <stdio.h>
#include <stdarg.h>
#include "vocal_sync.h"
/* Review candidate: source-position subtitles. No diagnostic lyric placeholders. */
static WCHAR dir[MAX_PATH];
static CRITICAL_SECTION guard;
static HANDLE logFile=INVALID_HANDLE_VALUE;
static LONG draws;
static volatile LONG gdiLastPresent;
static BOOL extra07_active(void);
static BOOL extra07_window_message(HWND,UINT,WPARAM,LPARAM,LRESULT*);
static void extra07_probe_frame(HWND,HDC,int,int,int,int);
static BOOL shown=TRUE;
static VS_INDEX fingerprints;
typedef struct BUF {
    LPDIRECTSOUNDBUFFER p;VS_RING ring;
    DWORD rate,lockOffset;BOOL supported,lockKnown,volumeKnown;LONG desiredVolume;
} BUF;
static BUF buffers[128];
typedef HRESULT(WINAPI*DSCREATE)(LPCGUID,LPDIRECTSOUND8*,LPUNKNOWN);
static DSCREATE realDS;
typedef struct METHOD {void **table;unsigned slot;void *old;} METHOD;
static METHOD methods[128];static unsigned methodCount;
static void logline(const char*fmt,...){
    char b[512];DWORD written;int n;va_list args;va_start(args,fmt);
    n=_vsnprintf_s(b,sizeof(b),_TRUNCATE,fmt,args);va_end(args);
    if(n>0&&logFile!=INVALID_HANDLE_VALUE)WriteFile(logFile,b,(DWORD)n,&written,NULL);
}
static void*original(void*p,unsigned slot){
    void**table=*(void***)p;void*result;unsigned i;
    EnterCriticalSection(&guard);result=table[slot];
    for(i=0;i<methodCount;i++)if(methods[i].table==table&&methods[i].slot==slot){result=methods[i].old;break;}
    LeaveCriticalSection(&guard);return result;
}
static void method(void*p,unsigned slot,void*replacement){
    void**table=*(void***)p;unsigned i;DWORD old,unused;
    EnterCriticalSection(&guard);
    for(i=0;i<methodCount;i++)if(methods[i].table==table&&methods[i].slot==slot)break;
    if(i==methodCount&&methodCount<128&&VirtualProtect(table+slot,sizeof(void*),PAGE_READWRITE,&old)){
        methods[i].table=table;methods[i].slot=slot;methods[i].old=table[slot];methodCount++;
        InterlockedExchangePointer(table+slot,replacement);VirtualProtect(table+slot,sizeof(void*),old,&unused);
    }LeaveCriticalSection(&guard);
}
static BUF*findbuf(LPDIRECTSOUNDBUFFER p){unsigned i;for(i=0;i<128;i++)if(buffers[i].p==p)return buffers+i;return NULL;}
static ULONG WINAPI brelease(LPDIRECTSOUNDBUFFER p){
    ULONG(WINAPI*fn)(LPDIRECTSOUNDBUFFER)=(void*)original(p,2);ULONG n;BUF*b;
    EnterCriticalSection(&guard);b=findbuf(p);n=fn(p);
    if(!n&&b)ZeroMemory(b,sizeof(*b));
    LeaveCriticalSection(&guard);return n;
}
static HRESULT WINAPI block(LPDIRECTSOUNDBUFFER p,DWORD pos,DWORD count,LPVOID*p1,LPDWORD n1,LPVOID*p2,LPDWORD n2,DWORD flags){
    HRESULT(WINAPI*fn)(LPDIRECTSOUNDBUFFER,DWORD,DWORD,LPVOID*,LPDWORD,LPVOID*,LPDWORD,DWORD)=(void*)original(p,11);
    HRESULT r;BUF*b;
    EnterCriticalSection(&guard);r=fn(p,pos,count,p1,n1,p2,n2,flags);b=findbuf(p);
    if(SUCCEEDED(r)&&b&&b->supported){
        b->lockKnown=!(flags&DSBLOCK_FROMWRITECURSOR)&&!(pos%4);b->lockOffset=(flags&DSBLOCK_ENTIREBUFFER)?0:pos;
        if(!b->lockKnown||b->lockOffset>=b->ring.bytes)ZeroMemory(b->ring.spans,sizeof(b->ring.spans));
        else{
            vs_invalidate(&b->ring,b->lockOffset,n1?*n1:0);
            if(p2&&*p2&&n2)vs_invalidate(&b->ring,0,*n2);
        }
    }LeaveCriticalSection(&guard);return r;
}
static HRESULT WINAPI bunlock(LPDIRECTSOUNDBUFFER p,LPVOID p1,DWORD n1,LPVOID p2,DWORD n2){
    HRESULT(WINAPI*fn)(LPDIRECTSOUNDBUFFER,LPVOID,DWORD,LPVOID,DWORD)=(void*)original(p,19);
    VS_MATCH m1[VS_MATCH_MAX],m2[VS_MATCH_MAX];unsigned c1=0,c2=0;HRESULT r;BUF*b;
    EnterCriticalSection(&guard);b=findbuf(p);
    if(b&&b->supported&&b->lockKnown){
        if(p1&&!(n1%4))c1=vs_match(&fingerprints,(const int16_t*)p1,n1/4,m1);
        if(p2&&!(n2%4))c2=vs_match(&fingerprints,(const int16_t*)p2,n2/4,m2);
    }
    r=fn(p,p1,n1,p2,n2);
    if(SUCCEEDED(r)&&b&&b->supported&&b->lockKnown){
        vs_commit(&b->ring,b->lockOffset,n1,m1,c1);
        if(p2&&n2)vs_commit(&b->ring,0,n2,m2,c2);
    }
    if(b)b->lockKnown=FALSE;
    LeaveCriticalSection(&guard);return r;
}
static HRESULT WINAPI bposition(LPDIRECTSOUNDBUFFER p,DWORD pos){
    HRESULT(WINAPI*fn)(LPDIRECTSOUNDBUFFER,DWORD)=(void*)original(p,13);
    /* Cursor reposition does not alter the PCM already committed to the ring. */
    return fn(p,pos);
}
static HRESULT WINAPI brestore(LPDIRECTSOUNDBUFFER p){
    HRESULT(WINAPI*fn)(LPDIRECTSOUNDBUFFER)=(void*)original(p,20);HRESULT r;BUF*b;
    EnterCriticalSection(&guard);r=fn(p);b=findbuf(p);
    if(SUCCEEDED(r)&&b)ZeroMemory(b->ring.spans,sizeof(b->ring.spans));
    LeaveCriticalSection(&guard);return r;
}
static BOOL engineAudioBlocked;
static HRESULT WINAPI bvolume(LPDIRECTSOUNDBUFFER p,LONG volume){
 HRESULT(WINAPI*fn)(LPDIRECTSOUNDBUFFER,LONG)=(void*)original(p,15);HRESULT result;BUF*b;
 EnterCriticalSection(&guard);b=findbuf(p);if(b&&b->supported){b->desiredVolume=volume;b->volumeKnown=TRUE;if(engineAudioBlocked)volume=DSBVOLUME_MIN;}result=fn(p,volume);LeaveCriticalSection(&guard);return result;
}
static void extra07_engineAudio(BOOL blockAudio){
 unsigned i;EnterCriticalSection(&guard);engineAudioBlocked=blockAudio;
 for(i=0;i<128;i++){BUF*b=buffers+i;HRESULT result;LONG value;if(!b->p||!b->supported)continue;
 if(blockAudio&&!b->volumeKnown&&SUCCEEDED(IDirectSoundBuffer_GetVolume(b->p,&value))){b->desiredVolume=value;b->volumeKnown=TRUE;}
 if(b->volumeKnown){HRESULT(WINAPI*fn)(LPDIRECTSOUNDBUFFER,LONG)=(void*)original(b->p,15);result=fn(b->p,blockAudio?DSBVOLUME_MIN:b->desiredVolume);value=1;IDirectSoundBuffer_GetVolume(b->p,&value);logline("EXTRA AUDIO_GATE block=%d result=%lx readback=%ld desired=%ld\r\n",blockAudio,(unsigned long)result,value,b->desiredVolume);}
 }LeaveCriticalSection(&guard);
}
static HRESULT WINAPI createbuf(LPDIRECTSOUND8 p,LPCDSBUFFERDESC desc,LPDIRECTSOUNDBUFFER*out,LPUNKNOWN outer){
    HRESULT(WINAPI*fn)(LPDIRECTSOUND8,LPCDSBUFFERDESC,LPDIRECTSOUNDBUFFER*,LPUNKNOWN)=(void*)original(p,3);
    HRESULT r=fn(p,desc,out,outer);unsigned i;
    if(FAILED(r)||!out||!*out)return r;
    EnterCriticalSection(&guard);
    for(i=0;i<128;i++)if(!buffers[i].p){
        BUF*b=buffers+i;const WAVEFORMATEX*w=desc->lpwfxFormat;
        ZeroMemory(b,sizeof(*b));b->p=*out;b->ring.bytes=desc->dwBufferBytes;
        b->rate=w?w->nAvgBytesPerSec:0;
        b->supported=w&&w->wFormatTag==WAVE_FORMAT_PCM&&w->nChannels==2&&w->nSamplesPerSec==44100&&w->wBitsPerSample==16&&w->nBlockAlign==4&&w->nAvgBytesPerSec==176400&&desc->dwBufferBytes==352800;
        method(*out,15,bvolume);
        if(b->supported&&SUCCEEDED(IDirectSoundBuffer_GetVolume(*out,&b->desiredVolume))){b->volumeKnown=TRUE;if(engineAudioBlocked){HRESULT(WINAPI*vf)(LPDIRECTSOUNDBUFFER,LONG)=(void*)original(*out,15);vf(*out,DSBVOLUME_MIN);}}
        method(*out,2,brelease);method(*out,11,block);method(*out,13,bposition);method(*out,19,bunlock);method(*out,20,brestore);
        logline("BUFFER %p bytes=%lu supported=%d\r\n",*out,b->ring.bytes,b->supported);break;
    }LeaveCriticalSection(&guard);return r;
}
static HRESULT WINAPI dscreate(LPCGUID g,LPDIRECTSOUND8*out,LPUNKNOWN outer){
    HRESULT r=realDS(g,out,outer);if(SUCCEEDED(r)&&out&&*out)method(*out,3,createbuf);return r;
}
#include "vocal_cues.h"
/* All surviving PCM candidates must select the same caption. */
static BOOL current_caption(WCHAR*text,unsigned capacity,double*seconds){
    unsigned i,k,n;DWORD cursor,status;VS_MATCH at[VS_MATCH_MAX];LPCWSTR agreed=NULL;BOOL ok=FALSE;
    text[0]=0;EnterCriticalSection(&guard);
    for(i=0;i<128;i++){
        BUF*b=buffers+i;LPCWSTR candidate=NULL;BOOL equal=TRUE;
        if(!b->p||!b->supported)continue;
        if(FAILED(IDirectSoundBuffer_GetStatus(b->p,&status))||!(status&DSBSTATUS_PLAYING)||(status&DSBSTATUS_BUFFERLOST))continue;
        if(FAILED(IDirectSoundBuffer_GetCurrentPosition(b->p,&cursor,NULL)))continue;
        n=vs_at(&fingerprints,&b->ring,cursor,at);if(!n)continue;
        for(k=0;k<n;k++){
            LPCWSTR found=vocal_cue(at[k].track,at[k].frame);
            if(!found||!*found){equal=FALSE;continue;}
            if(!candidate)candidate=found;else if(lstrcmpW(candidate,found)){equal=FALSE;}
        }
        if(!equal){if(candidate){ok=FALSE;break;}continue;}
        /* A crossfade with two different sung lines is intentionally blank. */
        if(agreed&&lstrcmpW(agreed,candidate)){ok=FALSE;break;}
        agreed=candidate;*seconds=(double)at[0].frame/44100.0;ok=TRUE;
    }
    if(ok&&agreed)lstrcpynW(text,agreed,capacity);
    LeaveCriticalSection(&guard);return ok;
}
/* Shared input/display selection for both the 2D and 3D render paths. */
static BOOL captionKeyWasDown;
static HWND captionHookWindow;
static WNDPROC captionOldProc;
static UINT_PTR captionTimer;
static ULONGLONG captionNoticeUntil;
static void captionHotkey(HWND window){
    if(window==captionHookWindow)return;
    BOOL down=GetForegroundWindow()==window&&(GetAsyncKeyState(VK_F8)&0x8000);
    if(down&&!captionKeyWasDown){shown=!shown;captionNoticeUntil=GetTickCount64()+1200;logline("F8 captions=%d\r\n",shown);}
    captionKeyWasDown=down;
}
static BOOL captionForDisplay(WCHAR*text,unsigned capacity,double*seconds){
    if(captionNoticeUntil&&GetTickCount64()<captionNoticeUntil){lstrcpynW(text,shown?L"보컬 자막 표시":L"보컬 자막 숨김",capacity);*seconds=0;return TRUE;}
    return shown&&current_caption(text,capacity,seconds);
}
/* The game stops repainting while waiting for dialogue input. A UI-thread
   timer requests fresh presentation; no scene buffers or audio clocks change. */
static void CALLBACK captionTick(HWND window,UINT message,UINT_PTR id,DWORD tick){
    (void)message;(void)id;(void)tick;
    if(!extra07_active()&&IsWindowVisible(window)&&!IsIconic(window)&&(DWORD)(GetTickCount()-(DWORD)InterlockedCompareExchange(&gdiLastPresent,0,0))>=40)InvalidateRect(window,NULL,FALSE);
}
static LRESULT CALLBACK captionWindowProc(HWND window,UINT message,WPARAM w,LPARAM l){
    LRESULT handled;if(extra07_window_message(window,message,w,l,&handled))return handled;
    if(message==WM_KEYDOWN&&w==VK_F8){
        if(!(l&(1L<<30))){shown=!shown;captionNoticeUntil=GetTickCount64()+1600;logline("F8 captions=%d\r\n",shown);InvalidateRect(window,NULL,FALSE);}
        return 0;
    }
    if(message==WM_KEYUP&&w==VK_F8)return 0;
    if(message==WM_NCDESTROY){
        WNDPROC previous=captionOldProc;
        if(captionTimer)KillTimer(window,captionTimer);
        SetWindowLongPtrW(window,GWLP_WNDPROC,(LONG_PTR)previous);
        captionHookWindow=NULL;captionOldProc=NULL;captionTimer=0;
        return CallWindowProcW(previous,window,message,w,l);
    }
    return CallWindowProcW(captionOldProc,window,message,w,l);
}
static void captionAttachWindow(HWND window){
    WNDPROC previous;
    if(!window||captionHookWindow==window)return;
    if(captionHookWindow){
        /* Preserve any later subclass instead of breaking its call chain. */
        if((WNDPROC)GetWindowLongPtrW(captionHookWindow,GWLP_WNDPROC)!=captionWindowProc){logline("CAPTION window transfer deferred: another subclass owns window\r\n");return;}
        if(captionTimer)KillTimer(captionHookWindow,captionTimer);
        SetWindowLongPtrW(captionHookWindow,GWLP_WNDPROC,(LONG_PTR)captionOldProc);
        captionHookWindow=NULL;captionOldProc=NULL;captionTimer=0;
    }
    SetLastError(0);previous=(WNDPROC)SetWindowLongPtrW(window,GWLP_WNDPROC,(LONG_PTR)captionWindowProc);
    if(!previous)return;
    captionOldProc=previous;captionHookWindow=window;
    captionTimer=SetTimer(window,(UINT_PTR)captionTick,50,captionTick);
    logline("CAPTION WINDOW timer=%p\r\n",(void*)captionTimer);
}
static BOOL replace(void**slot,void*fn){DWORD old,again;if(!VirtualProtect(slot,sizeof(void*),PAGE_READWRITE,&old))return FALSE;InterlockedExchangePointer(slot,fn);return VirtualProtect(slot,sizeof(void*),old,&again);}
#define VOCAL_FONT_PX 26
#define VOCAL_FONT_WEIGHT 700
#define VOCAL_OUTLINE 2
#define VOCAL_BAND_TOP 4
#define VOCAL_BAND_HEIGHT 74
#define VOCAL_SOURCE_PX 22
#define VOCAL_SOURCE_TOP 6
#define VOCAL_KOREAN_TOP 40
#define VOCAL_TEXT_COLOR RGB(255,232,112)
#include "vocal_text.inc"
#include "vocal_d3d8.inc"
#include "vocal_ddraw.inc"
#define D3DCOLORVALUE_DEFINED
#include "extra07_player.inc"
#include "vocal_gdi.inc"
void vocal_init(HINSTANCE instance){
    BYTE*base;WCHAR path[MAX_PATH],enabled[8],*last;DWORD length;FILE*f=NULL;
    /* Opt in from this DLL's directory, never from the process working directory.
       Extra and the sound room remain available when lyric display is disabled. */
    length=GetModuleFileNameW(instance,dir,MAX_PATH);
    if(!length||length>=MAX_PATH)return;
    last=wcsrchr(dir,L'\\');if(!last)return;*last=0;
    if(_snwprintf_s(path,MAX_PATH,_TRUNCATE,L"%s\\vocal_mod.ini",dir)<0)return;
    shown=GetPrivateProfileStringW(L"VocalSubtitles",L"Enabled",L"0",enabled,8,path)==1&&enabled[0]==L'1';
    InitializeCriticalSection(&guard);InitializeCriticalSection(&gdiGuard);ex07_instance=instance;
    base=(BYTE*)GetModuleHandleW(L"system.dll");
    _snwprintf_s(path,MAX_PATH,_TRUNCATE,L"%s\\vocal-review.log",dir);logFile=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_ALWAYS,0,NULL);
    _snwprintf_s(path,MAX_PATH,_TRUNCATE,L"%s\\vocal_fingerprints.bin",dir);
    if(_wfopen_s(&f,path,L"rb")||!f||!vs_load(&fingerprints,f)){if(f)fclose(f);logline("Fingerprint data unavailable; captions disabled.\r\n");return;}fclose(f);
    if(!base){logline("system.dll unavailable; captions disabled.\r\n");return;}
    logline("INIT tracks=%u anchors=%u\r\n",fingerprints.tracks,fingerprints.count);
    realBitBlt=(VOCAL_BITBLT)*(void**)(base+0xb902c);
    replace((void**)(base+0xb902c),gdiPresent);
    realDDCreateEx=(DDCREATEEX)*(void**)(base+0xb9018);
    replace((void**)(base+0xb9018),ddCreateEx);
    realDS=(DSCREATE)*(void**)(base+0xb9020);realGetProc=(void*)*(void**)(base+0xb90cc);
    replace((void**)(base+0xb9020),dscreate);replace((void**)(base+0xb90cc),getProc);
}


