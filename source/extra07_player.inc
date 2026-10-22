/* Continuous drama playback hosted inside the existing game window.
 * Sentence replay freezes the main clock and uses only the selected cue range.
 */
#include <windowsx.h>
#define EX07_OPEN (WM_APP+0x71)
#define EX07_TITLE (WM_APP+0x72)
#define EX07_SOUND (WM_APP+0x73)
#define EX07_BACK (WM_APP+0x74)
#define EX07_NOVEL (WM_APP+0x75)
#define EX07_GALLERY (WM_APP+0x76)
#define EX07_HUB (WM_APP+0x77)
#define EX07_FLOW (WM_APP+0x78)
#define EX07_RELEASE_TIMER 0x7e09
typedef struct {LPCWSTR text;int source;} EX07_LINE;
typedef struct {DWORD start,end;int page,y,height,count;const EX07_LINE*lines;} EX07_CUE;
typedef struct {LPCWSTR title,file;DWORD duration;int count,pages;const EX07_CUE*cues;} EX07_TRACK;
#include "extra07_data.h"
#include "sound_data.h"
static BOOL ex07_sound,ex07_novel,ex07_gallery,ex07_hub,ex07_flow,sm_ended;
static int sm_repeat;
static LPARAM ex07_exitPoint;
static HWND ex07_child,ex07_parent;
static LONG ex07_latched;
static LONG_PTR ex07_parentStyle;
static HINSTANCE ex07_instance;
static HWAVEOUT ex07_audio;
static WAVEHDR ex07_header;
static BYTE *ex07_file,*ex07_pcm;
static DWORD ex07_bytes,ex07_seek,ex07_position;
static BOOL ex07_prepared,ex07_paused,ex07_menu=TRUE;
static int ex07_track=-1,ex07_cue=-1,ex07_hover=-1,ex07_volume=75;
static BOOL ex07_replaying,ex07_resumePaused,ex07_savedPaused[13];
static DWORD ex07_resumePosition,ex07_savedPosition[13];
static int ex07_replayCue=-1,ex07_viewPage;
static int ex07_readCue[13]={-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1};
static HFONT ex07_font,ex07_small,ex07_head,ex07_jp,ex07_jpMain,ex07_note,ex07_logo,ex07_tiny;
static const WCHAR ex07_synopsis[]=L"같은 반 소녀에게 살의를 고백받은 소년과, 그를 쓰러뜨리려는 ‘정의의 히로인’.\n악의 대수령과 영웅이라는 운명 속에서 두 사람의 기묘한 사랑싸움이 시작된다.\n본편의 용공주와 어머니도 함께하는, 사랑과 살의가 뒤얽힌 《드라†코이》의 드라마 CD 외전.";
static HDC ex07_dc;
static HBITMAP ex07_bitmap,ex07_oldBitmap,ex07_sky;
#include "drama_subtitles.inc"
static BOOL extra07_active(void){return ex07_child!=NULL;}
static void ex07_log(const char*event){logline("EXTRA07 %s track=%d position=%lu cue=%d paused=%d\r\n",event,ex07_track+1,ex07_position,ex07_cue+1,ex07_paused);}
static DWORD ex07_clock(void){
    MMTIME t;DWORD bytes=0;ZeroMemory(&t,sizeof(t));t.wType=TIME_BYTES;
    if(ex07_replaying)return ex07_resumePosition;
    if(!ex07_audio||!ex07_prepared)return ex07_position;
    if(waveOutGetPosition(ex07_audio,&t,sizeof(t))!=MMSYSERR_NOERROR)return ex07_position;
    if(t.wType==TIME_BYTES)bytes=t.u.cb;else if(t.wType==TIME_SAMPLES)bytes=t.u.sample*4;else if(t.wType==TIME_MS)bytes=(DWORD)((ULONGLONG)t.u.ms*176400/1000);
    bytes=min(bytes,ex07_bytes-ex07_seek);return (DWORD)((ULONGLONG)(ex07_seek+bytes)*1000/176400);
}
static void ex07_remember(void){if(!ex07_sound&&ex07_track>=0&&ex07_track<13&&ex07_audio&&ex07_prepared){ex07_savedPosition[ex07_track]=ex07_clock();ex07_savedPaused[ex07_track]=ex07_replaying?ex07_resumePaused:ex07_paused;}}
static void ex07_stop(void){
    ex07_remember();ex07_replaying=FALSE;ex07_replayCue=-1;ex07_viewPage=0;
    if(ex07_audio){waveOutReset(ex07_audio);if(ex07_prepared)waveOutUnprepareHeader(ex07_audio,&ex07_header,sizeof(ex07_header));waveOutClose(ex07_audio);}
    ex07_audio=NULL;ex07_prepared=FALSE;ex07_paused=FALSE;sm_ended=FALSE;
    if(ex07_file)HeapFree(GetProcessHeap(),0,ex07_file);ex07_file=NULL;ex07_pcm=NULL;ex07_bytes=0;ex07_seek=0;ex07_position=0;ex07_cue=-1;
}
static void ex07_setVolume(void){DWORD v=(DWORD)(ex07_volume*65535/100);if(ex07_audio)waveOutSetVolume(ex07_audio,v|(v<<16));}
static BOOL ex07_queue(DWORD offset,DWORD length,BOOL paused){
    if(!ex07_audio||!ex07_pcm||offset>=ex07_bytes||!length||length>ex07_bytes-offset)return FALSE;
    waveOutReset(ex07_audio);if(ex07_prepared)waveOutUnprepareHeader(ex07_audio,&ex07_header,sizeof(ex07_header));ex07_prepared=FALSE;
    ZeroMemory(&ex07_header,sizeof(ex07_header));ex07_header.lpData=(LPSTR)(ex07_pcm+offset);ex07_header.dwBufferLength=length;
    if(waveOutPrepareHeader(ex07_audio,&ex07_header,sizeof(ex07_header))!=MMSYSERR_NOERROR)return FALSE;
    ex07_prepared=TRUE;ex07_paused=paused;if(paused)waveOutPause(ex07_audio);else waveOutRestart(ex07_audio);
    if(waveOutWrite(ex07_audio,&ex07_header,sizeof(ex07_header))!=MMSYSERR_NOERROR)return FALSE;
    return TRUE;
}
static BOOL ex07_startAt(DWORD ms,BOOL paused){
    DWORD offset;if(!ex07_audio||!ex07_pcm||ex07_bytes<4)return FALSE;
    offset=(DWORD)min((ULONGLONG)(ms/10)*1764,(ULONGLONG)ex07_bytes-4);offset&=~3u;
    ex07_replaying=FALSE;ex07_replayCue=-1;sm_ended=FALSE;ex07_seek=offset;ex07_position=(DWORD)((ULONGLONG)offset*1000/176400);ex07_cue=-1;
    if(!ex07_queue(offset,ex07_bytes-offset,paused))return FALSE;
    ex07_log("SEEK");return TRUE;
}
static BOOL ex07_seekTo(DWORD ms){return ex07_startAt(ms,FALSE);}
static BOOL ex07_replaySentence(int n){
    const EX07_CUE*c;DWORD first,last;
    if(ex07_sound||ex07_track<0||!ex07_audio||!ex07_pcm||ex07_bytes<4||n<0||n>ex07_readCue[ex07_track])return FALSE;
    c=&ex07_tracks[ex07_track].cues[n];first=(DWORD)min((ULONGLONG)c->start*176400/1000,(ULONGLONG)ex07_bytes-4);first&=~3u;
    last=(DWORD)min((ULONGLONG)c->end*176400/1000,(ULONGLONG)ex07_bytes);last&=~3u;if(last<=first)return FALSE;
    if(!ex07_replaying){ex07_resumePosition=ex07_clock();ex07_resumePaused=ex07_paused;}
    ex07_replaying=TRUE;ex07_replayCue=n;ex07_position=ex07_resumePosition;
    if(!ex07_queue(first,last-first,FALSE)){ex07_startAt(ex07_resumePosition,ex07_resumePaused);return FALSE;}
    ex07_log("SENTENCE_REPLAY");return TRUE;
}
static int ex07_page(void){if(ex07_viewPage>0)return ex07_viewPage;if(ex07_track>=0&&ex07_cue>=0)return cdsub_get(ex07_track)->cues[ex07_cue].page;return 1;}
static void ex07_pageMove(int delta){
    int last;if(ex07_track<0)return;last=ex07_readCue[ex07_track];if(last<0)return;
    if(!delta){ex07_viewPage=0;return;}ex07_viewPage=max(1,min(cdsub_get(ex07_track)->cues[last].page,ex07_page()+delta));ex07_hover=-1;
}
static BOOL ex07_play(int n){
    WCHAR path[MAX_PATH];HANDLE file;DWORD size,got,pos;WAVEFORMATEX fmt;BOOL format=FALSE;int i;
    if(n<0||n>=(ex07_sound?SM_COUNT:13))return FALSE;ex07_stop();ex07_track=n;
    _snwprintf_s(path,MAX_PATH,_TRUNCATE,L"%s\\extra07\\%s",dir,(ex07_sound?sm_tracks[n].file:ex07_tracks[n].file));
    file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE)goto fail;size=GetFileSize(file,NULL);
    if(size<44||size>160000000){CloseHandle(file);goto fail;}
    ex07_file=HeapAlloc(GetProcessHeap(),0,size);if(!ex07_file){CloseHandle(file);goto fail;}
    if(!ReadFile(file,ex07_file,size,&got,NULL)||got!=size){CloseHandle(file);goto fail;}CloseHandle(file);
    if(memcmp(ex07_file,"RIFF",4)||memcmp(ex07_file+8,"WAVE",4))goto fail;
    ZeroMemory(&fmt,sizeof(fmt));pos=12;
    while(pos+8<=size){DWORD length;memcpy(&length,ex07_file+pos+4,4);if(length>size-pos-8)goto fail;
        if(!memcmp(ex07_file+pos,"fmt ",4)&&length>=16){memcpy(&fmt,ex07_file+pos+8,16);format=TRUE;}
        if(!memcmp(ex07_file+pos,"data",4)){ex07_pcm=ex07_file+pos+8;ex07_bytes=length;}
        pos+=8+length+(length&1);
    }
    if(!format||fmt.wFormatTag!=1||fmt.nChannels!=2||fmt.nSamplesPerSec!=44100||fmt.wBitsPerSample!=16||!ex07_pcm||(ex07_bytes&3))goto fail;
    if(waveOutOpen(&ex07_audio,WAVE_MAPPER,&fmt,0,0,CALLBACK_NULL)!=MMSYSERR_NOERROR){ex07_audio=NULL;goto fail;}
    ex07_setVolume();ex07_menu=FALSE;if(!ex07_startAt(ex07_sound?0:ex07_savedPosition[n],ex07_sound?FALSE:ex07_savedPaused[n]))goto fail;
    if(!ex07_sound){for(i=0;i<ex07_tracks[n].count;i++){if(ex07_tracks[n].cues[i].start>ex07_position)break;ex07_cue=i;}ex07_readCue[n]=max(ex07_readCue[n],ex07_cue);}
    ex07_log("PLAY");InvalidateRect(ex07_child,NULL,FALSE);return TRUE;
fail:
    ex07_stop();ex07_track=-1;ex07_menu=TRUE;
    MessageBoxW(ex07_child,ex07_sound?L"음악 파일을 열지 못했습니다. 사운드 모드 파일을 확인해 주세요.":L"음성을 열지 못했습니다. extra07 폴더의 음성 파일을 확인해 주세요.",ex07_sound?L"사운드 모드":L"드라마 CD",MB_OK|MB_ICONERROR);return FALSE;
}
static BOOL ex07_trackStep(int delta){int n;if(ex07_sound||ex07_track<0)return FALSE;n=ex07_track+delta;if(n<0||n>=13)return FALSE;return ex07_play(n);}
static void ex07_update(void){
    int i,last=-1;DWORD p;if(ex07_track<0)return;
    if(ex07_replaying){if(ex07_prepared&&!ex07_paused&&(ex07_header.dwFlags&WHDR_DONE)){ex07_startAt(ex07_resumePosition,ex07_resumePaused);ex07_log("SENTENCE_RETURN");}else return;}
    p=ex07_clock();ex07_position=p;
    if(ex07_sound){if(ex07_prepared&&!ex07_paused&&!sm_ended&&(ex07_header.dwFlags&WHDR_DONE)){sm_ended=TRUE;ex07_log("MUSIC_END");if(sm_repeat==1)ex07_play(ex07_track);else if(sm_repeat==2)ex07_play((ex07_track+1)%SM_COUNT);}return;}
    for(i=0;i<ex07_tracks[ex07_track].count;i++){if(ex07_tracks[ex07_track].cues[i].start>p)break;last=i;}
    if(last!=ex07_cue){ex07_cue=last;ex07_log("CUE");}
    ex07_readCue[ex07_track]=max(ex07_readCue[ex07_track],last);
}
static void ex07_pause(void){
    if(!ex07_audio)return;
    if(ex07_paused)waveOutRestart(ex07_audio);else waveOutPause(ex07_audio);
    ex07_paused=!ex07_paused;ex07_update();ex07_log(ex07_paused?"PAUSE":"RESUME");
}
static void ex07_text(HDC dc,LPCWSTR s,RECT r,HFONT f,COLORREF color,UINT flags){SelectObject(dc,f);SetTextColor(dc,color);SetBkMode(dc,TRANSPARENT);DrawTextW(dc,s,-1,&r,flags|DT_NOPREFIX);}
static void ex07_box(HDC dc,RECT r,COLORREF c){HBRUSH b=CreateSolidBrush(c);FillRect(dc,&r,b);DeleteObject(b);}
/* Title-screen vocabulary: a pale halftone sky with rose blooms, and the
 * cyan-rimmed lens plate with a dragon fin the original entry buttons use.
 * Plates clip through paths, so they also draw correctly under a zoom transform. */
static COLORREF ex07_mix(COLORREF a,COLORREF b,int permille){int r=GetRValue(a)+(GetRValue(b)-GetRValue(a))*permille/1000,g=GetGValue(a)+(GetGValue(b)-GetGValue(a))*permille/1000,s=GetBValue(a)+(GetBValue(b)-GetBValue(a))*permille/1000;return RGB(r,g,s);}
static void ex07_gradient(HDC dc,RECT r,COLORREF top,COLORREF bottom){
    TRIVERTEX v[2];GRADIENT_RECT g;g.UpperLeft=0;g.LowerRight=1;
    v[0].x=r.left;v[0].y=r.top;v[0].Red=(COLOR16)(GetRValue(top)<<8);v[0].Green=(COLOR16)(GetGValue(top)<<8);v[0].Blue=(COLOR16)(GetBValue(top)<<8);v[0].Alpha=0;
    v[1].x=r.right;v[1].y=r.bottom;v[1].Red=(COLOR16)(GetRValue(bottom)<<8);v[1].Green=(COLOR16)(GetGValue(bottom)<<8);v[1].Blue=(COLOR16)(GetBValue(bottom)<<8);v[1].Alpha=0;
    GradientFill(dc,v,2,&g,1,GRADIENT_FILL_RECT_V);
}
static void ex07_roundClip(HDC dc,RECT r,int radius){BeginPath(dc);RoundRect(dc,r.left,r.top,r.right,r.bottom,radius*2,radius*2);EndPath(dc);SelectClipPath(dc,RGN_AND);}
static void ex07_plate(HDC dc,RECT r,int radius,COLORREF rimTop,COLORREF rimBottom,COLORREF faceTop,COLORREF faceBottom){
    RECT in,gloss;int inset=radius>=16?3:2;
    if(radius<2||r.right-r.left<2*radius||r.bottom-r.top<2*inset+2){ex07_gradient(dc,r,faceTop,faceBottom);return;}
    SaveDC(dc);ex07_roundClip(dc,r,radius);ex07_gradient(dc,r,rimTop,rimBottom);RestoreDC(dc,-1);
    in=(RECT){r.left+inset,r.top+inset,r.right-inset,r.bottom-inset};
    SaveDC(dc);ex07_roundClip(dc,in,radius-inset);ex07_gradient(dc,in,faceTop,faceBottom);
    gloss=in;gloss.bottom=in.top+(in.bottom-in.top)*2/5;
    if(gloss.bottom>gloss.top&&in.bottom-in.top<=80)ex07_gradient(dc,gloss,ex07_mix(faceTop,RGB(255,255,255),420),faceTop);
    RestoreDC(dc,-1);
}
static void ex07_fin(HDC dc,int x,int y,int height,BOOL hot){
    static const int fx[11]={0,-27,-6,-44,-9,-48,-9,-41,-5,-20,0},fy[11]={1,-7,9,5,18,20,27,34,36,45,43};
    POINT p[11];HPEN pen;HBRUSH br;HGDIOBJ oldPen,oldBrush;int i,half=height/2;
    pen=CreatePen(PS_SOLID,1,RGB(0,52,72));oldPen=SelectObject(dc,pen);
    br=CreateSolidBrush(hot?RGB(120,220,250):RGB(40,180,232));oldBrush=SelectObject(dc,br);
    for(i=0;i<11;i++){p[i].x=x+fx[i];p[i].y=y+fy[i]*height/44;}
    Polygon(dc,p,11);SelectObject(dc,oldBrush);DeleteObject(br);
    br=CreateSolidBrush(hot?RGB(200,242,254):RGB(140,222,250));oldBrush=SelectObject(dc,br);
    for(i=0;i<11;i++){p[i].x=x+fx[i]*5/9;p[i].y=y+half+(fy[i]*height/44-half)*7/10;}
    Polygon(dc,p,11);SelectObject(dc,oldBrush);DeleteObject(br);SelectObject(dc,oldPen);DeleteObject(pen);
}
static void ex07_logoTextAt(HDC dc,LPCWSTR s,RECT r,HFONT f,UINT align){
    static const int ox[8]={-1,0,1,1,1,0,-1,-1},oy[8]={-1,-1,-1,0,1,1,1,0};
    RECT q;int i,k;UINT flags=align|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS;
    for(k=4;k>=2;k--)for(i=0;i<8;i++){q=r;OffsetRect(&q,ox[i]*k,oy[i]*k);ex07_text(dc,s,q,f,k==4?RGB(255,255,255):RGB(70,196,238),flags);}
    ex07_text(dc,s,r,f,RGB(228,68,90),flags);
}
static void ex07_logoText(HDC dc,LPCWSTR s,RECT r,HFONT f){ex07_logoTextAt(dc,s,r,f,DT_CENTER);}
/* Halftone paper: a vertical wash, soft blooms in the corners and dots that grow toward the edges. */
static HBITMAP ex07_halftone(COLORREF top,COLORREF middle,COLORREF bottom,COLORREF bloom,COLORREF dot){
    static const int bx[4]={62,740,88,714},by[4]={56,60,534,546},brad[4]={300,300,330,330};
    BITMAPINFO bi;DWORD*px=NULL;HDC probe;HBITMAP bm;int x,y,i;
    ZeroMemory(&bi,sizeof(bi));bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=800;bi.bmiHeader.biHeight=-600;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;
    probe=GetDC(NULL);if(!probe)return NULL;
    bm=CreateDIBSection(probe,&bi,DIB_RGB_COLORS,(void**)&px,NULL,0);ReleaseDC(NULL,probe);
    if(!bm||!px){if(bm)DeleteObject(bm);return NULL;}
    for(y=0;y<600;y++)for(x=0;x<800;x++){
        double r,g,b,k,rose=0,cov,rad,shade;int dx,dy,d2,cell,ex,ey;COLORREF from=y<270?top:middle,to=y<270?middle:bottom;
        k=y<270?y/269.0:(y-270)/329.0;
        r=GetRValue(from)+(GetRValue(to)-GetRValue(from))*k;g=GetGValue(from)+(GetGValue(to)-GetGValue(from))*k;b=GetBValue(from)+(GetBValue(to)-GetBValue(from))*k;
        for(i=0;i<4;i++){dx=x-bx[i];dy=y-by[i];d2=dx*dx+dy*dy;if(d2<brad[i]*brad[i]){k=1.0-(double)d2/((double)brad[i]*brad[i]);rose+=k*k*0.62;}}
        if(rose>0.60)rose=0.60;
        if(rose>0){r+=(GetRValue(bloom)-r)*rose;g+=(GetGValue(bloom)-g)*rose;b+=(GetBValue(bloom)-b)*rose;}
        ex=x-400;ey=y-300;shade=(double)(ex*ex)/160000.0+(double)(ey*ey)/90000.0;if(shade>1.0)shade=1.0;
        cell=((y/12)&1)?6:0;dx=(((x-cell)%12)+12)%12-6;dy=y%12-6;d2=dx*dx+dy*dy;
        rad=1.05+2.15*shade;cov=rad*rad-(double)d2;
        if(cov>0){if(cov>1.0)cov=1.0;cov*=0.46;r+=(GetRValue(dot)-r)*cov;g+=(GetGValue(dot)-g)*cov;b+=(GetBValue(dot)-b)*cov;}
        px[y*800+x]=((DWORD)(int)(r+0.5)<<16)|((DWORD)(int)(g+0.5)<<8)|(DWORD)(int)(b+0.5);
    }
    return bm;
}
static void ex07_backdrop(HBITMAP bm,COLORREF fallback){
    RECT r={0,0,800,600};HDC src=bm?CreateCompatibleDC(ex07_dc):NULL;HGDIOBJ old;
    if(src){old=SelectObject(src,bm);BitBlt(ex07_dc,0,0,800,600,src,0,0,SRCCOPY);SelectObject(src,old);DeleteDC(src);}
    else ex07_box(ex07_dc,r,fallback);
}
static void ex07_skyDraw(void){
    if(!ex07_sky)ex07_sky=ex07_halftone(RGB(203,233,244),RGB(247,252,253),RGB(214,234,244),RGB(246,186,196),RGB(150,206,230));
    ex07_backdrop(ex07_sky,RGB(226,240,247));
}
/* Content panels: light for reading, dark as a frame for pictures. */
static void ex07_panel(HDC dc,RECT r,BOOL dark){ex07_plate(dc,r,12,RGB(150,232,250),RGB(40,140,176),dark?RGB(28,46,60):RGB(250,253,254),dark?RGB(14,24,34):RGB(222,238,246));}
/* The title screen's entry button: a fin on the left of a cyan-rimmed lens. */
static void ex07_lens(HDC dc,RECT r,LPCWSTR text,BOOL hot,BOOL enabled){
    int h=r.bottom-r.top;RECT label={r.left+52,r.top,r.right-10,r.bottom};
    ex07_fin(dc,r.left+16,r.top,h,hot&&enabled);
    if(enabled)ex07_plate(dc,r,h/2,hot?RGB(150,232,252):RGB(52,186,236),hot?RGB(30,140,186):RGB(0,112,156),hot?RGB(220,240,240):RGB(178,204,202),hot?RGB(164,200,202):RGB(124,156,156));
    else ex07_plate(dc,r,h/2,RGB(176,194,200),RGB(126,144,152),RGB(214,220,222),RGB(180,190,194));
    ex07_text(dc,text,label,ex07_small,!enabled?RGB(118,132,140):hot?RGB(0,30,44):RGB(0,43,58),DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
}
static void ex07_button(HDC dc,RECT r,LPCWSTR text,BOOL selected){
    RECT shadow;int h=r.bottom-r.top,radius=h/2<12?h/2:12;
    ex07_plate(dc,r,radius,selected?RGB(150,234,252):RGB(44,132,166),selected?RGB(42,128,160):RGB(18,62,84),selected?RGB(74,138,166):RGB(44,76,94),selected?RGB(36,88,114):RGB(23,43,57));
    shadow=r;OffsetRect(&shadow,1,1);ex07_text(dc,text,shadow,ex07_small,RGB(12,24,34),DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    ex07_text(dc,text,r,ex07_small,selected?RGB(255,232,171):RGB(230,236,238),DT_CENTER|DT_VCENTER|DT_SINGLELINE);
}
#include "sound_player.inc"
#include "novel_player.inc"
#include "gallery_player.inc"
#include "drama_art.inc"
#include "drama_titles.inc"
#include "flowchart.inc"
static int ex07_hubHit(int x,int y){int n;if(x<190||x>=610||y<150||y>=478)return -1;n=(y-150)/56;return n<6&&(y-150)%56<48?n:-1;}
static void ex07_hubRender(HWND w,HDC dest){
    static const WCHAR*labels[]={L"드라마 CD · 은빛 악마와 금빛 도깨비",L"우로보로스의 사랑",L"사운드 모드",L"추가 CG 감상",L"플로우 차트",L"타이틀로"};
    RECT r,client,plate;int i;
    GetClientRect(w,&client);ex07_skyDraw();
    r=(RECT){0,34,800,102};ex07_logoText(ex07_dc,L"엑스트라",r,ex07_logo);
    r=(RECT){0,104,800,130};ex07_text(ex07_dc,L"D r a † K o I   E X T R A",r,ex07_note,RGB(74,120,148),DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    /* Rows stay at 190..610 / 150+56i; the lens and its fin only dress them. */
    for(i=0;i<6;i++){plate=(RECT){192,152+56*i,608,196+56*i};ex07_lens(ex07_dc,plate,labels[i],ex07_hover==i,TRUE);}
    r=(RECT){0,556,800,559};ex07_box(ex07_dc,r,RGB(104,186,214));
    r=(RECT){0,559,800,600};ex07_box(ex07_dc,r,RGB(20,34,48));
    ex07_text(ex07_dc,L"원하는 항목을 고르세요 · Esc 키를 누르면 타이틀로 돌아갑니다",r,ex07_small,RGB(214,232,241),DT_CENTER|DT_VCENTER|DT_SINGLELINE);
    SetStretchBltMode(dest,HALFTONE);StretchBlt(dest,0,0,client.right,client.bottom,ex07_dc,0,0,800,600,SRCCOPY);
}
static void ex07_switchMode(UINT mode){
 cdart_close();if(ex07_flow)flow_cleanup();if(ex07_novel)sb_cleanup();if(ex07_gallery)gal_cleanup();if(ex07_sound)sm_cleanup();KillTimer(ex07_child,1);KillTimer(ex07_child,2);ex07_stop();ex07_track=-1;ex07_hover=-1;ex07_menu=TRUE;
 ex07_hub=mode==EX07_HUB;ex07_novel=mode==EX07_NOVEL;ex07_sound=mode==EX07_SOUND;ex07_gallery=mode==EX07_GALLERY;ex07_flow=mode==EX07_FLOW;
 if(ex07_novel){sb_open();SetTimer(ex07_child,2,750,NULL);}else if(ex07_gallery)gal_open();else if(ex07_flow)flow_reset();else if(!ex07_hub)SetTimer(ex07_child,1,33,NULL);
 SetFocus(ex07_child);InvalidateRect(ex07_child,NULL,FALSE);logline("EXTRA MODE %u\r\n",mode);
}
static void ex07_render(HWND w,HDC dest){
    RECT client,r;int i,j,page=1;WCHAR buf[160];const CDSUB_TRACK*t=ex07_track>=0&&!ex07_sound?cdsub_get(ex07_track):NULL;
    GetClientRect(w,&client);
    if(!ex07_dc){ex07_dc=CreateCompatibleDC(dest);ex07_bitmap=CreateCompatibleBitmap(dest,800,600);ex07_oldBitmap=SelectObject(ex07_dc,ex07_bitmap);}
    if(ex07_hub){ex07_hubRender(w,dest);return;}
    if(ex07_flow){flow_render(w,dest);return;}
    if(ex07_gallery){gal_render(w,dest);return;}
    if(ex07_novel){sb_render(w,dest);return;}
    if(ex07_sound){sm_render(w,dest);return;}
    if(cdart_view>=0){cdart_render(w,dest);return;}
    ex07_skyDraw();cdart_preferences();cdtitle_preferences();
    if(ex07_menu){r=(RECT){0,18,800,62};ex07_logoText(ex07_dc,L"은빛 악마와 금빛 도깨비",r,ex07_head);}
    else{r=(RECT){56,37,490,80};ex07_logoTextAt(ex07_dc,cdtitle_get(ex07_track,FALSE),r,ex07_head,DT_LEFT);}
    if(!ex07_menu){r=(RECT){502,42,618,76};ex07_button(ex07_dc,r,L"배경 선택",cdart_picker);r=(RECT){630,42,746,76};ex07_button(ex07_dc,r,L"그림 보기",FALSE);}
    cdsub_preferences();for(i=0;i<3;i++){int y=ex07_menu?133:84;r=(RECT){160+110*i,y,260+110*i,y+34};ex07_button(ex07_dc,r,cdsub_names[i],cdsub_mode==i);}
    if(ex07_menu){
        r=(RECT){54,64,746,127};ex07_text(ex07_dc,ex07_synopsis,r,ex07_note,RGB(52,86,108),DT_WORDBREAK);
        r=(RECT){54,133,148,167};ex07_text(ex07_dc,L"자막",r,ex07_small,RGB(22,46,62),DT_VCENTER|DT_SINGLELINE);
        r=(RECT){590,133,746,167};ex07_button(ex07_dc,r,cdtitle_korean?L"제목 번역 ON":L"제목 번역 OFF",cdtitle_korean);
        r=(RECT){48,169,316,472};ex07_panel(ex07_dc,r,TRUE);
        r=(RECT){54,173,310,468};cdart_inset(ex07_dc,r,cdart_menuImage,255);
        r=(RECT){54,476,134,508};ex07_button(ex07_dc,r,L"책자 표지",cdart_menuImage==0);
        r=(RECT){142,476,222,508};ex07_button(ex07_dc,r,L"주인공",cdart_menuImage==1);
        r=(RECT){230,476,310,508};ex07_button(ex07_dc,r,L"교실 표지",cdart_menuImage==2);
        r=(RECT){324,169,752,514};ex07_panel(ex07_dc,r,FALSE);
        for(i=0;i<13;i++){LPCWSTR title=cdtitle_get(i,TRUE);r=(RECT){330,173+i*26,746,198+i*26};_snwprintf_s(buf,160,_TRUNCATE,L"%02d. %s",i+1,title);
            if(i==ex07_hover||i==ex07_track)ex07_plate(ex07_dc,r,8,RGB(176,240,254),RGB(54,160,196),RGB(250,254,255),RGB(214,240,250));else if(i&1)ex07_box(ex07_dc,r,RGB(234,244,249));
            r.left+=12;ex07_text(ex07_dc,buf,r,ex07_small,RGB(22,46,62),DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);}
    }else if(t){
        page=ex07_page();
        _snwprintf_s(buf,160,_TRUNCATE,L"%02d / %02d",page,t->pages);r=(RECT){56,84,148,118};ex07_text(ex07_dc,buf,r,ex07_small,RGB(22,46,62),DT_SINGLELINE|DT_VCENTER);
        r=(RECT){502,84,576,118};ex07_button(ex07_dc,r,L"이전 쪽",FALSE);r=(RECT){587,84,661,118};ex07_button(ex07_dc,r,L"현재",ex07_viewPage==0);r=(RECT){672,84,746,118};ex07_button(ex07_dc,r,L"다음 쪽",FALSE);
        r=(RECT){44,124,756,512};ex07_panel(ex07_dc,r,FALSE);
        SaveDC(ex07_dc);IntersectClipRect(ex07_dc,48,128,752,508);r=(RECT){348,128,752,508};cdart_inset(ex07_dc,r,cdart_background,44);RestoreDC(ex07_dc,-1);
        if(ex07_cue<0&&ex07_readCue[ex07_track]<0){r=(RECT){70,170,730,218};ex07_text(ex07_dc,L"재생 중…",r,ex07_font,RGB(60,104,132),DT_SINGLELINE);}
        for(i=0;i<=ex07_readCue[ex07_track];i++){const EX07_CUE*c=&t->cues[i];int yy;BOOL active=ex07_replaying?i==ex07_replayCue:i==ex07_cue;if(c->page!=page)continue;
            if(i==ex07_hover){r=(RECT){60,c->y,740,c->y+c->height};ex07_plate(ex07_dc,r,8,RGB(176,240,254),RGB(54,160,196),RGB(250,254,255),RGB(214,240,250));}
            yy=c->y+5;
            for(j=0;j<c->count;j++){int pitch=cdsub_pitch(c->lines[j].source);BOOL smallSource=c->lines[j].source&&cdsub_mode==2;r=(RECT){78,yy,730,yy+pitch};ex07_text(ex07_dc,c->lines[j].text,r,c->lines[j].source?(cdsub_mode==1?ex07_jpMain:ex07_jp):ex07_font,smallSource?RGB(96,120,138):(active?RGB(196,40,72):RGB(22,46,62)),DT_SINGLELINE);yy+=pitch;}
        }
    }
    r=(RECT){60,516,740,520};ex07_box(ex07_dc,r,RGB(190,214,226));
    if(t){DWORD duration=(DWORD)((ULONGLONG)ex07_bytes*1000/176400);r.right=60+(LONG)((ULONGLONG)min(ex07_position,duration)*680/max(duration,1));ex07_box(ex07_dc,r,RGB(52,186,236));
        _snwprintf_s(buf,160,_TRUNCATE,L"%02lu:%02lu / %02lu:%02lu",ex07_position/60000,(ex07_position/1000)%60,duration/60000,(duration/1000)%60);r=(RECT){200,526,600,549};ex07_text(ex07_dc,buf,r,ex07_small,RGB(22,46,62),DT_CENTER|DT_SINGLELINE);
        if(!ex07_menu){r=(RECT){60,524,196,550};ex07_button(ex07_dc,r,L"이전 트랙",FALSE);if(ex07_track==0){ex07_plate(ex07_dc,r,r.bottom-r.top>=24?12:(r.bottom-r.top)/2,RGB(30,58,74),RGB(18,36,48),RGB(30,48,60),RGB(22,36,47));ex07_text(ex07_dc,L"이전 트랙",r,ex07_small,RGB(100,117,129),DT_CENTER|DT_VCENTER|DT_SINGLELINE);}r=(RECT){604,524,740,550};ex07_button(ex07_dc,r,L"다음 트랙",FALSE);if(ex07_track==12){ex07_plate(ex07_dc,r,r.bottom-r.top>=24?12:(r.bottom-r.top)/2,RGB(30,58,74),RGB(18,36,48),RGB(30,48,60),RGB(22,36,47));ex07_text(ex07_dc,L"다음 트랙",r,ex07_small,RGB(100,117,129),DT_CENTER|DT_VCENTER|DT_SINGLELINE);}}
    }
    r=(RECT){44,554,148,590};ex07_button(ex07_dc,r,L"타이틀로",FALSE);
    r=(RECT){160,554,274,590};ex07_button(ex07_dc,r,L"엑스트라로",FALSE);
    r=(RECT){286,554,414,590};ex07_button(ex07_dc,r,ex07_menu?L"재생 화면":L"파일 목록",FALSE);
    r=(RECT){426,554,530,590};ex07_button(ex07_dc,r,ex07_paused?L"계속 재생":L"일시정지",ex07_paused);
    r=(RECT){542,554,640,590};ex07_button(ex07_dc,r,L"처음부터",FALSE);
    _snwprintf_s(buf,160,_TRUNCATE,L"− %d +",ex07_volume);r=(RECT){652,554,756,590};ex07_button(ex07_dc,r,buf,FALSE);
    if(cdart_picker)cdart_pickerRender(ex07_dc);
    SetStretchBltMode(dest,HALFTONE);StretchBlt(dest,0,0,client.right,client.bottom,ex07_dc,0,0,800,600,SRCCOPY);
}
static void ex07_point(HWND w,LPARAM l,int*x,int*y){RECT r;GetClientRect(w,&r);*x=GET_X_LPARAM(l)*800/max(r.right,1);*y=GET_Y_LPARAM(l)*600/max(r.bottom,1);}
static int ex07_hit(int x,int y){
 int i;if(ex07_hub)return ex07_hubHit(x,y);if(ex07_flow)return flow_hit(x,y);if(ex07_gallery)return gal_hit(x,y);if(ex07_novel)return sb_hit(x,y);if(ex07_sound)return sm_hit(x,y);if(cdart_view>=0)return cdart_hit(x,y);if(cdart_picker){i=cdart_pickerHit(x,y);return i>=0?1010+i:-1;}if(x<54||x>746)return -1;
 if(y>=(ex07_menu?133:84)&&y<(ex07_menu?167:118)&&x>=160&&x<480){i=(x-160)/110;if((x-160)%110<100)return 1040+i;}
 if(ex07_menu){if(x>=590&&x<746&&y>=133&&y<167)return 1030;if(x<310){if(y>=173&&y<468)return 1000;if(y>=476&&y<508){if(x<134)return 1020;if(x>=142&&x<222)return 1021;if(x>=230)return 1022;}}if(x<330)return -1;i=(y-173)/26;return y>=173&&i<13&&y<173+i*26+25?i:-1;}
 if(y>=42&&y<76){if(x>=630)return 1001;if(x>=502&&x<618)return 1005;}
 if(y>=524&&y<550){if(x>=60&&x<196&&ex07_track>0)return 1006;if(x>=604&&x<740&&ex07_track<12)return 1007;}
 if(y>=84&&y<118){if(x>=502&&x<576)return 1002;if(x>=587&&x<661)return 1003;if(x>=672)return 1004;}
 if(ex07_track<0)return -1;for(i=0;i<=ex07_readCue[ex07_track];i++){const EX07_CUE*c=&cdsub_get(ex07_track)->cues[i];if(c->page==ex07_page()&&y>=c->y&&y<c->y+c->height)return i;}return -1;
}
static LRESULT CALLBACK ex07_proc(HWND w,UINT m,WPARAM wp,LPARAM lp){
    int x,y,hit;PAINTSTRUCT ps;HDC dc;
    if(m==WM_ERASEBKGND)return 1;
    if(m==WM_PAINT){dc=BeginPaint(w,&ps);ex07_render(w,dc);EndPaint(w,&ps);return 0;}
    if(m==WM_TIMER){if(ex07_novel)sb_poll();else ex07_update();InvalidateRect(w,NULL,FALSE);return 0;}
    if(ex07_flow){
        if(m==WM_MOUSEWHEEL){POINT p={GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};RECT r;if(GET_KEYSTATE_WPARAM(wp)&MK_CONTROL)return DefWindowProcW(w,m,wp,lp);ScreenToClient(w,&p);GetClientRect(w,&r);flow_zoom(p.x*800/max(r.right,1),p.y*600/max(r.bottom,1),GET_WHEEL_DELTA_WPARAM(wp));InvalidateRect(w,NULL,FALSE);return 0;}
        if(m==WM_LBUTTONDOWN){ex07_point(w,lp,&x,&y);if(flow_area(x,y)){flow_drag=TRUE;flow_moved=FALSE;flow_lastX=flow_pressX=x;flow_lastY=flow_pressY=y;SetCapture(w);}return 0;}
        if(m==WM_CAPTURECHANGED)flow_drag=FALSE;
        if(m==WM_MOUSEMOVE&&flow_drag){ex07_point(w,lp,&x,&y);flow_pan(x-flow_lastX,y-flow_lastY);if((x-flow_pressX)*(x-flow_pressX)+(y-flow_pressY)*(y-flow_pressY)>9)flow_moved=TRUE;flow_lastX=x;flow_lastY=y;InvalidateRect(w,NULL,FALSE);return 0;}
        if(m==WM_LBUTTONUP){BOOL dragged=flow_moved;ex07_point(w,lp,&x,&y);flow_drag=flow_moved=FALSE;ReleaseCapture();if(!dragged)flow_click(x,y);InvalidateRect(w,NULL,FALSE);return 0;}
        if(m==WM_RBUTTONUP){if(flow_view>=0){flow_closeScene();InvalidateRect(w,NULL,FALSE);}else PostMessageW(ex07_parent,EX07_BACK,0,0);return 0;}
        if(m==WM_KEYDOWN){if(wp==VK_ESCAPE){if(flow_view>=0)flow_closeScene();else PostMessageW(ex07_parent,EX07_BACK,0,0);}else if(wp=='0'||wp==VK_HOME)flow_fit();else if(wp==VK_NEXT||wp==VK_RIGHT)flow_step(1);else if(wp==VK_PRIOR||wp==VK_LEFT)flow_step(-1);else if(wp==VK_DOWN)flow_pan(0,-80);else if(wp==VK_UP)flow_pan(0,80);else if(wp==VK_RETURN||wp==VK_SPACE){if(flow_view>=0)flow_closeScene();else flow_openScene(flow_pick<0?0:flow_pick);}InvalidateRect(w,NULL,FALSE);return 0;}
    }
    if(cdart_picker&&m==WM_KEYDOWN&&wp==VK_ESCAPE){cdart_picker=FALSE;InvalidateRect(w,NULL,FALSE);return 0;}
    if(cdart_view>=0){
        if(m==WM_MOUSEWHEEL){POINT p={GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};RECT r;if(GET_KEYSTATE_WPARAM(wp)&MK_CONTROL)return DefWindowProcW(w,m,wp,lp);ScreenToClient(w,&p);GetClientRect(w,&r);cdart_wheel(p.x*800/max(r.right,1),p.y*600/max(r.bottom,1),GET_WHEEL_DELTA_WPARAM(wp));InvalidateRect(w,NULL,FALSE);return 0;}
        if(m==WM_LBUTTONDOWN){ex07_point(w,lp,&x,&y);if(cdart_area(x,y)){cdart_drag=TRUE;cdart_lastX=x;cdart_lastY=y;SetCapture(w);}return 0;}
        if(m==WM_CAPTURECHANGED)cdart_drag=FALSE;
        if(m==WM_MOUSEMOVE&&cdart_drag){ex07_point(w,lp,&x,&y);cdart_x+=x-cdart_lastX;cdart_y+=y-cdart_lastY;cdart_lastX=x;cdart_lastY=y;InvalidateRect(w,NULL,FALSE);return 0;}
        if(m==WM_LBUTTONUP){ex07_point(w,lp,&x,&y);if(cdart_drag){cdart_drag=FALSE;ReleaseCapture();}else cdart_click(x,y);InvalidateRect(w,NULL,FALSE);return 0;}
        if(m==WM_RBUTTONUP){cdart_close();InvalidateRect(w,NULL,FALSE);return 0;}
        if(m==WM_KEYDOWN){if(!(lp&0x40000000L))cdart_key(wp);InvalidateRect(w,NULL,FALSE);return 0;}
    }
    if(ex07_gallery&&m==WM_MOUSEWHEEL){POINT p={GET_X_LPARAM(lp),GET_Y_LPARAM(lp)};RECT r;if(GET_KEYSTATE_WPARAM(wp)&MK_CONTROL)return DefWindowProcW(w,m,wp,lp);ScreenToClient(w,&p);GetClientRect(w,&r);gal_wheel(p.x*800/max(r.right,1),p.y*600/max(r.bottom,1),GET_WHEEL_DELTA_WPARAM(wp));InvalidateRect(w,NULL,FALSE);return 0;}
    if(ex07_gallery&&m==WM_LBUTTONDOWN){ex07_point(w,lp,&x,&y);if(gal_selection>=0&&gal_area(x,y)){gal_drag=TRUE;gal_moved=FALSE;gal_lastX=x;gal_lastY=y;SetCapture(w);}return 0;}
    if(m==WM_CAPTURECHANGED)gal_drag=FALSE;
    if(m==WM_MOUSEMOVE&&ex07_gallery&&gal_drag){ex07_point(w,lp,&x,&y);if(x!=gal_lastX||y!=gal_lastY){gal_x+=x-gal_lastX;gal_y+=y-gal_lastY;gal_lastX=x;gal_lastY=y;gal_moved=TRUE;InvalidateRect(w,NULL,FALSE);}return 0;}
    if(m==WM_RBUTTONUP&&ex07_gallery){gal_key(VK_ESCAPE);InvalidateRect(w,NULL,FALSE);return 0;}
    if(m==WM_KEYDOWN&&ex07_hub){if(wp==VK_ESCAPE)PostMessageW(ex07_parent,EX07_TITLE,0,0);return 0;}
    if(m==WM_KEYDOWN&&ex07_gallery){if(!(lp&0x40000000L))gal_key(wp);InvalidateRect(w,NULL,FALSE);return 0;}
    if(m==WM_MOUSEMOVE){ex07_point(w,lp,&x,&y);hit=ex07_hit(x,y);if(hit!=ex07_hover){ex07_hover=hit;InvalidateRect(w,NULL,FALSE);}SetCursor(LoadCursorW(NULL,hit>=0?MAKEINTRESOURCEW(32649):MAKEINTRESOURCEW(32512)));return 0;}
    if(m==WM_RBUTTONUP&&ex07_novel){sb_menu=TRUE;InvalidateRect(w,NULL,FALSE);return 0;}
    if(m==WM_LBUTTONUP){ex07_point(w,lp,&x,&y);hit=ex07_hit(x,y);
        if(ex07_hub){static const UINT modes[]={EX07_OPEN,EX07_NOVEL,EX07_SOUND,EX07_GALLERY,EX07_FLOW,EX07_TITLE};if(hit>=0&&hit<6)PostMessageW(ex07_parent,modes[hit],0,0);return 0;}
        if(ex07_gallery){BOOL dragged=gal_moved;gal_drag=FALSE;gal_moved=FALSE;ReleaseCapture();if(!dragged)gal_click(x,y);InvalidateRect(w,NULL,FALSE);return 0;}
        if(ex07_novel){sb_click(x,y);InvalidateRect(w,NULL,FALSE);return 0;}
        if(ex07_sound){sm_click(x,y);ex07_update();InvalidateRect(w,NULL,FALSE);return 0;}
        if(cdart_picker){int choice=cdart_pickerHit(x,y);cdart_picker=FALSE;if(choice>=0)cdart_selectBackground(choice);InvalidateRect(w,NULL,FALSE);return 0;}
        if(hit>=1040&&hit<=1042){cdsub_select(hit-1040);InvalidateRect(w,NULL,FALSE);return 0;}
        if(ex07_menu&&hit==1030){cdtitle_toggle();InvalidateRect(w,NULL,FALSE);return 0;}
        if(ex07_menu&&hit>=1020&&hit<=1022){cdart_selectMenuImage(hit-1020);InvalidateRect(w,NULL,FALSE);return 0;}
        if(hit==1000||hit==1001){cdart_open(ex07_menu?cdart_menuImage:cdart_background);InvalidateRect(w,NULL,FALSE);return 0;}
        if(hit>=1002&&hit<=1004){ex07_pageMove(hit-1003);InvalidateRect(w,NULL,FALSE);return 0;}
        if(hit==1005){cdart_picker=TRUE;ex07_hover=-1;InvalidateRect(w,NULL,FALSE);return 0;}
        if(hit==1006||hit==1007){ex07_trackStep(hit==1006?-1:1);ex07_update();InvalidateRect(w,NULL,FALSE);return 0;}
        if(y>=554&&y<=590){if(x>=44&&x<148)PostMessageW(ex07_parent,EX07_TITLE,0,0);else if(x>=160&&x<274)PostMessageW(ex07_parent,EX07_BACK,0,0);else if(x>=286&&x<414){if(ex07_track>=0)ex07_menu=!ex07_menu;}else if(x>=426&&x<530)ex07_pause();else if(x>=542&&x<640)ex07_seekTo(0);else if(x>=652&&x<=756){ex07_volume=max(0,min(100,ex07_volume+(x<704?-5:5)));ex07_setVolume();}}
        else if(y>=506&&y<524&&x>=60&&x<=740&&ex07_track>=0){DWORD dur=(DWORD)((ULONGLONG)ex07_bytes*1000/176400);ex07_seekTo((DWORD)((ULONGLONG)(x-60)*dur/680));}
        else if(hit>=0){if(ex07_menu)ex07_play(hit);else ex07_replaySentence(hit);}
        ex07_update();InvalidateRect(w,NULL,FALSE);return 0;
    }
    if(m==WM_KEYDOWN&&ex07_novel){if(!(lp&0x40000000L))sb_key(wp);InvalidateRect(w,NULL,FALSE);return 0;}
    if(m==WM_KEYDOWN){if(wp==VK_SPACE){if(!(lp&0x40000000L)){if(ex07_sound)sm_toggle();else ex07_pause();}}else if(wp==VK_ESCAPE)ex07_menu=TRUE;else if(wp==VK_F8)SendMessageW(ex07_parent,m,wp,lp);InvalidateRect(w,NULL,FALSE);return 0;}
    if(m==WM_DESTROY){extra07_engineAudio(FALSE);flow_cleanup();if(ex07_gallery)gal_cleanup();KillTimer(w,2);if(ex07_novel)sb_cleanup();KillTimer(w,1);ex07_stop();cdart_cleanup();sm_cleanup();ex07_child=NULL;ex07_track=-1;ex07_menu=TRUE;if(ex07_sky){DeleteObject(ex07_sky);ex07_sky=NULL;}if(ex07_dc){SelectObject(ex07_dc,ex07_oldBitmap);DeleteObject(ex07_bitmap);DeleteDC(ex07_dc);ex07_dc=NULL;}return 0;}
    return DefWindowProcW(w,m,wp,lp);
}
static BOOL extra07_window_message(HWND w,UINT m,WPARAM wp,LPARAM lp,LRESULT*out){
    (void)wp;(void)lp;
    if(m==EX07_OPEN||m==EX07_SOUND||m==EX07_NOVEL||m==EX07_GALLERY||m==EX07_FLOW||m==EX07_HUB){WNDCLASSW wc;RECT r;if(ex07_child){ex07_switchMode(m);*out=0;return TRUE;}ex07_flow=m==EX07_FLOW;if(ex07_flow)flow_reset();ZeroMemory(&wc,sizeof(wc));wc.lpfnWndProc=ex07_proc;wc.hInstance=ex07_instance;wc.lpszClassName=L"DraKoiContinuousExtra07";wc.hCursor=LoadCursorW(NULL,MAKEINTRESOURCEW(32512));RegisterClassW(&wc);
        if(!ex07_font){ex07_font=CreateFontW(-24,0,0,0,500,0,0,0,HANGUL_CHARSET,0,0,ANTIALIASED_QUALITY,0,L"Malgun Gothic");ex07_small=CreateFontW(-18,0,0,0,500,0,0,0,HANGUL_CHARSET,0,0,ANTIALIASED_QUALITY,0,L"Malgun Gothic");ex07_head=CreateFontW(-26,0,0,0,700,0,0,0,HANGUL_CHARSET,0,0,ANTIALIASED_QUALITY,0,L"Malgun Gothic");ex07_jp=CreateFontW(-18,0,0,0,500,0,0,0,SHIFTJIS_CHARSET,0,0,ANTIALIASED_QUALITY,0,L"Yu Gothic");ex07_jpMain=CreateFontW(-24,0,0,0,500,0,0,0,SHIFTJIS_CHARSET,0,0,ANTIALIASED_QUALITY,0,L"Yu Gothic");ex07_note=CreateFontW(-16,0,0,0,500,0,0,0,HANGUL_CHARSET,0,0,ANTIALIASED_QUALITY,0,L"Malgun Gothic");ex07_logo=CreateFontW(-52,0,0,0,700,0,0,0,HANGUL_CHARSET,0,0,ANTIALIASED_QUALITY,0,L"Malgun Gothic");ex07_tiny=CreateFontW(-13,0,0,0,500,0,0,0,HANGUL_CHARSET,0,0,ANTIALIASED_QUALITY,0,L"Malgun Gothic");}
        extra07_engineAudio(TRUE);ex07_hub=m==EX07_HUB;ex07_gallery=(m==EX07_GALLERY);if(ex07_gallery)gal_open();ex07_sound=(m==EX07_SOUND);ex07_novel=(m==EX07_NOVEL);if(ex07_novel)sb_open();sm_ended=FALSE;ex07_track=-1;ex07_parent=w;ex07_parentStyle=GetWindowLongPtrW(w,GWL_STYLE);SetWindowLongPtrW(w,GWL_STYLE,ex07_parentStyle|WS_CLIPCHILDREN);GetClientRect(w,&r);ex07_menu=TRUE;ex07_hover=-1;
        ex07_child=CreateWindowExW(0,wc.lpszClassName,ex07_hub?L"엑스트라":ex07_flow?L"플로우 차트":ex07_gallery?L"추가 CG 감상":ex07_novel?L"우로보로스의 사랑":ex07_sound?L"사운드 모드":L"드라마 CD",WS_CHILD|WS_VISIBLE|WS_TABSTOP,0,0,r.right,r.bottom,w,NULL,ex07_instance,NULL);if(ex07_child){if(ex07_novel)SetTimer(ex07_child,2,750,NULL);else if(!ex07_gallery&&!ex07_flow&&!ex07_hub)SetTimer(ex07_child,1,33,NULL);SetFocus(ex07_child);ex07_log("OPEN");}else{extra07_engineAudio(FALSE);if(ex07_gallery)gal_cleanup();if(ex07_novel)sb_cleanup();SetWindowLongPtrW(w,GWL_STYLE,ex07_parentStyle);InterlockedExchange(&ex07_latched,0);ex07_log("OPEN_FAILED");}*out=0;return TRUE;
    }
    if(m==EX07_BACK&&ex07_child){ex07_switchMode(EX07_HUB);*out=0;return TRUE;}
    if(m==EX07_TITLE||m==EX07_BACK){RECT r;int x=m==EX07_TITLE?116:217;if(ex07_child)DestroyWindow(ex07_child);SetWindowLongPtrW(w,GWL_STYLE,ex07_parentStyle);SetFocus(w);GetClientRect(w,&r);ex07_exitPoint=MAKELPARAM(x*r.right/800,572*r.bottom/600);
        /* Keep the native choice press across a frame; the original script
           remains the owner of title/Extra transitions. */
        SendMessageW(w,WM_MOUSEMOVE,0,ex07_exitPoint);SendMessageW(w,WM_LBUTTONDOWN,MK_LBUTTON,ex07_exitPoint);SetTimer(w,EX07_RELEASE_TIMER,100,NULL);InvalidateRect(w,NULL,FALSE);*out=0;return TRUE;
    }
    if(m==WM_TIMER&&wp==EX07_RELEASE_TIMER){KillTimer(w,EX07_RELEASE_TIMER);SendMessageW(w,WM_LBUTTONUP,0,ex07_exitPoint);*out=0;return TRUE;}
    if(m==WM_SIZE&&ex07_child){RECT r;GetClientRect(w,&r);MoveWindow(ex07_child,0,0,r.right,r.bottom,TRUE);}
    return FALSE;
}
static void extra07_probe_frame(HWND window,HDC source,int sx,int sy,int width,int height){
    BOOL marker;COLORREF type;if(width!=800||height!=600||ex07_child)return;type=GetPixel(source,sx+790,sy+590);
    marker=GetPixel(source,sx+782,sy+590)==RGB(13,199,91)&&GetPixel(source,sx+786,sy+590)==RGB(233,17,151)&&(type==RGB(41,113,229)||type==RGB(43,115,231)||type==RGB(45,117,233)||type==RGB(47,119,235));
    if(marker){if(!InterlockedCompareExchange(&ex07_latched,1,0))PostMessageW(window,type==RGB(47,119,235)?EX07_GALLERY:type==RGB(45,117,233)?EX07_NOVEL:type==RGB(43,115,231)?EX07_SOUND:EX07_HUB,0,0);}else InterlockedExchange(&ex07_latched,0);
}
