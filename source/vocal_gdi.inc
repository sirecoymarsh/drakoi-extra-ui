/* Present captions in the same transaction as every intersecting GDI update. */
typedef BOOL(WINAPI*VOCAL_BITBLT)(HDC,int,int,int,int,HDC,int,int,DWORD);
static VOCAL_BITBLT realBitBlt;
static HDC gdiStageDC;
static HBITMAP gdiStageBitmap,gdiOldBitmap;
static int gdiWidth,gdiHeight;
static CRITICAL_SECTION gdiGuard;
static DWORD gdiOwner;
static LONG gdiPresentCount,gdiPartialCount,gdiContendedCount;
static BOOL gdiPrepare(HDC source,int width,int height){
    BITMAPINFO bi;void*pixels;
    if(gdiStageDC&&gdiWidth==width&&gdiHeight==height)return TRUE;
    if(gdiStageDC){SelectObject(gdiStageDC,gdiOldBitmap);DeleteObject(gdiStageBitmap);DeleteDC(gdiStageDC);gdiStageDC=NULL;}
    gdiStageDC=CreateCompatibleDC(source);if(!gdiStageDC)return FALSE;
    ZeroMemory(&bi,sizeof(bi));bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=width;bi.bmiHeader.biHeight=-height;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;
    gdiStageBitmap=CreateDIBSection(source,&bi,DIB_RGB_COLORS,&pixels,NULL,0);if(!gdiStageBitmap){DeleteDC(gdiStageDC);gdiStageDC=NULL;return FALSE;}
    gdiOldBitmap=SelectObject(gdiStageDC,gdiStageBitmap);gdiWidth=width;gdiHeight=height;return TRUE;
}
static BOOL WINAPI gdiPresent(HDC dst,int x,int y,int width,int height,HDC src,int sx,int sy,DWORD operation){
    HWND window=WindowFromDC(dst);WCHAR text[256];double seconds=0;BOOL result;RECT client;DWORD thread=GetCurrentThreadId();LONG count;
    if(window!=gameWindow||!window)return realBitBlt(dst,x,y,width,height,src,sx,sy,operation);
    if(extra07_active())return TRUE;
    if(operation==SRCCOPY)extra07_probe_frame(window,src,sx,sy,width,height);
    if(operation!=SRCCOPY||width<=0||height<=0||width>4096||height>4096||y>=VOCAL_BAND_TOP+VOCAL_BAND_HEIGHT||y+height<=VOCAL_BAND_TOP||!GetClientRect(window,&client)||client.right<600||client.right>4096||GetMapMode(dst)!=MM_TEXT||GetMapMode(src)!=MM_TEXT)return realBitBlt(dst,x,y,width,height,src,sx,sy,operation);
    if(!TryEnterCriticalSection(&gdiGuard)){InterlockedIncrement(&gdiContendedCount);EnterCriticalSection(&gdiGuard);}
    /* A recursive call finishes before its outer transaction. Other threads
       wait; they must never erase a caption with an uncomposited late frame. */
    if(gdiOwner==thread){result=realBitBlt(dst,x,y,width,height,src,sx,sy,operation);LeaveCriticalSection(&gdiGuard);return result;}gdiOwner=thread;
    count=InterlockedIncrement(&gdiPresentCount);if(width<client.right||height<400)InterlockedIncrement(&gdiPartialCount);
    captionHotkey(window);
    if(!captionForDisplay(text,256,&seconds)||!gdiPrepare(src,width,height)||!realBitBlt(gdiStageDC,0,0,width,height,src,sx,sy,SRCCOPY)||!vocalPaint(gdiStageDC,text,-x,-y,client.right,FALSE))goto original;
    result=realBitBlt(dst,x,y,width,height,gdiStageDC,0,0,operation);
    if(result&&(InterlockedIncrement(&draws)%300)==1)logline("GDI DRAW %ld seconds=%.3f\r\n",draws,seconds);
    if(!result)goto original;goto done;
original:
    result=realBitBlt(dst,x,y,width,height,src,sx,sy,operation);
done:
    if(result&&x==0&&y==0&&width>=client.right&&height>=VOCAL_BAND_TOP+VOCAL_BAND_HEIGHT)InterlockedExchange(&gdiLastPresent,(LONG)GetTickCount());
    if(count==1||count%600==0)logline("GDI TRANSACTION frames=%ld partial=%ld waited=%ld size=%dx%d thread=%lu\r\n",count,gdiPartialCount,gdiContendedCount,width,height,thread);
    gdiOwner=0;LeaveCriticalSection(&gdiGuard);return result;
}
