/* The game soundtrack shares PCM controls with the continuous CD player.
 * Cue timing reuses the accepted fingerprint-index track IDs and original frames.
 * The booklet mascot and paper/pink/cyan palette affect rendering only.
 */
#include <objbase.h>
#include <wincodec.h>
static HBITMAP gal_load(LPCWSTR file,int*w,int*h);
static void gal_blit(HDC dc,HBITMAP bm,int x,int y,int width,int height);
static HBITMAP sm_art;
static int sm_artWidth,sm_artHeight;
static BOOL sm_artTried;
static HBITMAP sm_paper;
static void sm_cleanup(void){if(sm_art)DeleteObject(sm_art);sm_art=NULL;sm_artTried=FALSE;sm_artWidth=sm_artHeight=0;if(sm_paper)DeleteObject(sm_paper);sm_paper=NULL;}
/* Filter premultiplied pixels once at display size; AlphaBlend alone uses
 * nearest-neighbour reduction and loses the fine printed outlines. */
static void sm_artScale(void){
    IWICImagingFactory*factory=NULL;IWICBitmap*source=NULL;IWICBitmapScaler*scaler=NULL;
    BITMAPINFO bi={0};void*pixels=NULL;HBITMAP scaled=NULL;HRESULT init,hr;int width=148,height;
    if(!sm_art||!sm_artWidth||!sm_artHeight)return;
    height=MulDiv(width,sm_artHeight,sm_artWidth);if(height>132){height=132;width=MulDiv(height,sm_artWidth,sm_artHeight);}
    init=CoInitializeEx(NULL,COINIT_APARTMENTTHREADED);if(FAILED(init)&&init!=RPC_E_CHANGED_MODE)return;
    hr=CoCreateInstance(&CLSID_WICImagingFactory,NULL,CLSCTX_INPROC_SERVER,&IID_IWICImagingFactory,(void**)&factory);if(FAILED(hr))goto done;
    hr=IWICImagingFactory_CreateBitmapFromHBITMAP(factory,sm_art,NULL,WICBitmapUsePremultipliedAlpha,&source);if(FAILED(hr))goto done;
    hr=IWICImagingFactory_CreateBitmapScaler(factory,&scaler);if(FAILED(hr))goto done;
    hr=IWICBitmapScaler_Initialize(scaler,(IWICBitmapSource*)source,(UINT)width,(UINT)height,WICBitmapInterpolationModeFant);if(FAILED(hr))goto done;
    bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=width;bi.bmiHeader.biHeight=-height;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;
    scaled=CreateDIBSection(NULL,&bi,DIB_RGB_COLORS,&pixels,NULL,0);if(!scaled)goto done;
    hr=IWICBitmapScaler_CopyPixels(scaler,NULL,(UINT)width*4,(UINT)(width*height*4),pixels);if(FAILED(hr)){DeleteObject(scaled);scaled=NULL;goto done;}
    DeleteObject(sm_art);sm_art=scaled;sm_artWidth=width;sm_artHeight=height;
done:if(scaler)IWICBitmapScaler_Release(scaler);if(source)IWICBitmap_Release(source);if(factory)IWICImagingFactory_Release(factory);if(SUCCEEDED(init))CoUninitialize();
}
static void sm_artRender(HDC dc){
    int width=148,height;
    if(!sm_artTried){sm_artTried=TRUE;sm_art=gal_load(L"../sound-mode/booklet-chibi.png",&sm_artWidth,&sm_artHeight);sm_artScale();}
    if(!sm_art||!sm_artWidth||!sm_artHeight)return;
    height=MulDiv(width,sm_artHeight,sm_artWidth);if(height>132){height=132;width=MulDiv(height,sm_artWidth,sm_artHeight);}
    gal_blit(dc,sm_art,608+(148-width)/2,1+(132-height)/2,width,height);
}
static void sm_button(HDC dc,RECT r,LPCWSTR label,BOOL active){
    ex07_plate(dc,r,12,active?RGB(246,160,192):RGB(222,210,244),active?RGB(160,40,90):RGB(176,156,218),active?RGB(232,104,148):RGB(255,255,255),active?RGB(186,58,104):RGB(246,242,252));
    ex07_text(dc,label,r,ex07_small,active?RGB(255,250,253):RGB(83,66,100),DT_CENTER|DT_VCENTER|DT_SINGLELINE);
}
/* The booklet's heart wand, as the playback position marker. */
static void sm_heart(HDC dc,int x,int y,COLORREF color){
    HBRUSH br=CreateSolidBrush(color);HPEN pen=CreatePen(PS_SOLID,1,color);HGDIOBJ oldBrush=SelectObject(dc,br),oldPen=SelectObject(dc,pen);POINT tip[3];
    tip[0].x=x-6;tip[0].y=y-1;tip[1].x=x+6;tip[1].y=y-1;tip[2].x=x;tip[2].y=y+7;
    Ellipse(dc,x-7,y-6,x+1,y+2);Ellipse(dc,x-1,y-6,x+7,y+2);Polygon(dc,tip,3);
    SelectObject(dc,oldPen);SelectObject(dc,oldBrush);DeleteObject(pen);DeleteObject(br);
}
static int sm_hit(int x,int y){
    int col,row,n;if(y<138||y>=399)return -1;
    col=x>=44&&x<389?0:x>=411&&x<756?1:-1;if(col<0)return -1;
    row=(y-138)/29;if((y-138)%29>=26)return -1;n=col*9+row;return n<SM_COUNT?n:-1;
}
static void sm_toggle(void){if(ex07_audio&&!sm_ended)ex07_pause();else ex07_play(ex07_track>=0?ex07_track:0);}
static void sm_click(int x,int y){
    int hit=sm_hit(x,y);
    if(hit>=0){ex07_play(hit);return;}
    if(y>=506&&y<=530&&x>=60&&x<=740&&ex07_audio){DWORD duration=(DWORD)((ULONGLONG)ex07_bytes*1000/176400);ex07_seekTo((DWORD)((ULONGLONG)(x-60)*duration/680));return;}
    if(y<554||y>590)return;
    if(x>=44&&x<148)PostMessageW(ex07_parent,EX07_TITLE,0,0);
    else if(x>=160&&x<274)PostMessageW(ex07_parent,EX07_BACK,0,0);
    else if(x>=286&&x<390)sm_toggle();
    else if(x>=402&&x<476){ex07_stop();ex07_log("MUSIC_STOP");}
    else if(x>=488&&x<620){sm_repeat=(sm_repeat+1)%3;ex07_log("MUSIC_REPEAT");}
    else if(x>=632&&x<=756){ex07_volume=max(0,min(100,ex07_volume+(x<694?-5:5)));ex07_setVolume();}
}
static void sm_render(HWND w,HDC dest){
    RECT r,client;WCHAR text[256],source[256],korean[256];int i;LPCWSTR cue=NULL;
    const EX07_TRACK*t=ex07_track>=0?&sm_tracks[ex07_track]:NULL;
    GetClientRect(w,&client);
    /* Booklet paper: lavender halftone with pink blooms, after the CD booklet's lyric pages. */
    if(!sm_paper)sm_paper=ex07_halftone(RGB(250,244,252),RGB(255,251,253),RGB(238,230,250),RGB(250,198,218),RGB(206,188,238));
    ex07_backdrop(sm_paper,RGB(249,246,244));
    r=(RECT){0,0,800,5};ex07_box(ex07_dc,r,RGB(182,67,112));
    r=(RECT){44,124,600,126};ex07_box(ex07_dc,r,RGB(214,204,236));
    r=(RECT){44,124,144,126};ex07_box(ex07_dc,r,RGB(202,106,145));
    r=(RECT){44,12,600,38};ex07_text(ex07_dc,L"DANCES & DRAGONS!",r,ex07_small,RGB(170,59,102),DT_SINGLELINE|DT_VCENTER);
    r=(RECT){48,38,600,84};ex07_logoTextAt(ex07_dc,L"사운드 모드",r,ex07_head,DT_LEFT);
    r=(RECT){600,4,758,134};ex07_plate(ex07_dc,r,14,RGB(250,190,212),RGB(214,98,140),RGB(255,255,255),RGB(253,240,246));
    sm_artRender(ex07_dc);
    if(t)_snwprintf_s(text,256,_TRUNCATE,L"%s  ·  %s",t->title,ex07_audio?(ex07_paused?L"일시정지":sm_ended?L"재생 완료":L"재생 중"):L"정지");
    else wcscpy_s(text,256,L"듣고 싶은 곡을 선택해 주세요.");
    r=(RECT){46,86,600,118};ex07_text(ex07_dc,text,r,ex07_small,RGB(98,84,120),DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);
    for(i=0;i<SM_COUNT;i++){
        int col=i/9,row=i%9,x=44+367*col;DWORD duration=sm_tracks[i].duration;BOOL current=i==ex07_track;RECT badge;
        r=(RECT){x,138+row*29,x+345,164+row*29};
        if(current)ex07_plate(ex07_dc,r,9,RGB(246,160,192),RGB(196,64,112),RGB(255,244,248),RGB(250,216,230));
        else if(i==ex07_hover)ex07_plate(ex07_dc,r,9,RGB(206,188,242),RGB(150,122,210),RGB(252,250,255),RGB(238,232,252));
        else ex07_plate(ex07_dc,r,9,RGB(232,224,246),RGB(204,192,230),RGB(255,255,255),RGB(250,247,253));
        badge=(RECT){x+6,r.top+4,x+34,r.bottom-4};ex07_plate(ex07_dc,badge,9,current?RGB(240,140,176):RGB(206,190,238),current?RGB(170,40,90):RGB(132,106,196),current?RGB(232,104,148):RGB(190,172,232),current?RGB(196,58,104):RGB(156,132,214));
        _snwprintf_s(text,256,_TRUNCATE,L"%02d",i+1);ex07_text(ex07_dc,text,badge,ex07_tiny,RGB(255,255,255),DT_CENTER|DT_VCENTER|DT_SINGLELINE);
        r.left=x+42;r.right=x+296;ex07_text(ex07_dc,sm_tracks[i].title,r,ex07_small,current?RGB(140,30,76):RGB(72,56,92),DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);
        r.left=x+294;r.right=x+337;_snwprintf_s(text,256,_TRUNCATE,L"%lu:%02lu",duration/60000,(duration/1000)%60);ex07_text(ex07_dc,text,r,ex07_small,RGB(124,110,148),DT_SINGLELINE|DT_RIGHT|DT_VCENTER);
    }
    r=(RECT){44,408,756,503};ex07_plate(ex07_dc,r,14,RGB(214,200,240),RGB(170,148,214),RGB(255,253,255),RGB(244,238,252));
    if(t&&ex07_audio&&shown)cue=vocal_cue(sm_fingerprint[ex07_track],(unsigned)((ULONGLONG)ex07_position*44100/1000));
    if(cue&&vocalSplit(cue,source,korean)){
        r=(RECT){52,412,748,446};ex07_text(ex07_dc,source,r,ex07_jp,RGB(98,88,124),DT_CENTER|DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);
        r=(RECT){52,447,748,489};ex07_text(ex07_dc,korean,r,ex07_font,RGB(170,40,90),DT_CENTER|DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);
    }else if(t&&sm_original[ex07_track]){
        r=(RECT){52,410,748,454};ex07_text(ex07_dc,sm_original[ex07_track],r,ex07_jp,RGB(98,88,124),DT_CENTER|DT_WORDBREAK);
        r=(RECT){52,458,748,503};ex07_text(ex07_dc,ex07_track==15?L"혹은 영웅은 어떻게 사랑하는 용을 살육하고, 그 이야기를 완성했는가":t->title,r,ex07_small,RGB(170,40,90),DT_CENTER|DT_WORDBREAK);
    }else{
        LPCWSTR hint=t?(ex07_audio?(shown?sm_kind[ex07_track]:L"가사 숨김 · F8"):L"재생 버튼을 누르면 처음부터 듣습니다."):L"보컬 · 배경음 · 반주 · 짧은 버전";
        r=(RECT){52,422,748,475};ex07_text(ex07_dc,hint,r,ex07_small,RGB(120,108,140),DT_SINGLELINE|DT_CENTER|DT_VCENTER);
    }
    r=(RECT){60,516,740,520};ex07_box(ex07_dc,r,RGB(222,214,238));
    if(t){DWORD duration=t->duration;r.right=60+(LONG)((ULONGLONG)min(ex07_position,duration)*680/max(duration,1));ex07_box(ex07_dc,r,RGB(214,74,124));sm_heart(ex07_dc,r.right,518,RGB(226,84,136));
        _snwprintf_s(text,256,_TRUNCATE,L"%02lu:%02lu / %02lu:%02lu",ex07_position/60000,(ex07_position/1000)%60,duration/60000,(duration/1000)%60);r=(RECT){60,526,740,549};ex07_text(ex07_dc,text,r,ex07_small,RGB(98,84,120),DT_CENTER|DT_SINGLELINE);
    }
    r=(RECT){44,554,148,590};sm_button(ex07_dc,r,L"타이틀로",FALSE);
    r=(RECT){160,554,274,590};sm_button(ex07_dc,r,L"엑스트라로",FALSE);
    r=(RECT){286,554,390,590};sm_button(ex07_dc,r,!ex07_audio||sm_ended?L"재생":ex07_paused?L"계속 재생":L"일시정지",TRUE);
    r=(RECT){402,554,476,590};sm_button(ex07_dc,r,L"정지",FALSE);
    r=(RECT){488,554,620,590};sm_button(ex07_dc,r,sm_repeat==1?L"한 곡 반복":sm_repeat==2?L"전체 반복":L"반복 꺼짐",sm_repeat!=0);
    _snwprintf_s(text,256,_TRUNCATE,L"− %d +",ex07_volume);r=(RECT){632,554,756,590};sm_button(ex07_dc,r,text,FALSE);
    SetStretchBltMode(dest,HALFTONE);StretchBlt(dest,0,0,client.right,client.bottom,ex07_dc,0,0,800,600,SRCCOPY);
}
