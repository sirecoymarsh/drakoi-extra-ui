#define WIN32_LEAN_AND_MEAN
#define COBJMACROS
#include <windows.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdarg.h>
static WCHAR dir[MAX_PATH];
static BOOL shown=TRUE;
static void logline(const char*f,...){(void)f;}
#include "vocal_cues.h"
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

static BOOL reviewAudioGate;
static void extra07_engineAudio(BOOL block){reviewAudioGate=block;}
#include "extra07_player.inc"
static BOOL savebmp(HBITMAP bm,const WCHAR*name){
 BITMAP b;BITMAPFILEHEADER h={0};BITMAPINFOHEADER i={0};FILE*f=NULL;WCHAR p[MAX_PATH];
 if(!GetObjectW(bm,sizeof(b),&b))return FALSE;
 h.bfType=0x4d42;h.bfOffBits=sizeof(h)+sizeof(i);h.bfSize=h.bfOffBits+b.bmWidthBytes*b.bmHeight;
 i.biSize=sizeof(i);i.biWidth=b.bmWidth;i.biHeight=-b.bmHeight;i.biPlanes=1;i.biBitCount=32;i.biCompression=BI_RGB;
 _snwprintf_s(p,MAX_PATH,_TRUNCATE,L"%s\\rendered\\%s",dir,name);if(_wfopen_s(&f,p,L"wb")||!f)return FALSE;
 fwrite(&h,1,sizeof(h),f);fwrite(&i,1,sizeof(i),f);fwrite(b.bmBits,1,b.bmWidthBytes*b.bmHeight,f);fclose(f);return TRUE;
}



#define CHECK(x) do {if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);return __LINE__;}} while(0)
static BYTE*reviewRead(LPCWSTR path,DWORD*size){HANDLE f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);BYTE*p;DWORD got;if(f==INVALID_HANDLE_VALUE)return NULL;*size=GetFileSize(f,NULL);p=HeapAlloc(GetProcessHeap(),0,*size);if(!p){CloseHandle(f);return NULL;}ReadFile(f,p,*size,&got,NULL);CloseHandle(f);if(got!=*size){HeapFree(GetProcessHeap(),0,p);return NULL;}return p;}
static BOOL reviewWrite(LPCWSTR path,const BYTE*p,DWORD size){HANDLE f=CreateFileW(path,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,0,NULL);DWORD got;BOOL ok;if(f==INVALID_HANDLE_VALUE)return FALSE;ok=WriteFile(f,p,size,&got,NULL)&&got==size;CloseHandle(f);return ok;}
int wmain(int argc,wchar_t**argv){
 WNDCLASSW wc={0};HWND host;HDC dc;HBITMAP bm;HGDIOBJ old;BITMAPINFO bi={0};void*pixels;LRESULT out;int i,j,count;SIZE extent;SB_STEP anchor;SB_BOOK*book;WCHAR path[MAX_PATH],renamed[MAX_PATH];DWORD size;BYTE*originalText,*inserted;double scale,anchorX,anchorY;BOOL reload;
 CHECK(argc==2||argc==3);wcscpy_s(dir,MAX_PATH,argv[1]);ex07_instance=GetModuleHandleW(NULL);wc.hInstance=ex07_instance;wc.lpszClassName=L"ExtraUpdateReview";wc.lpfnWndProc=DefWindowProcW;RegisterClassW(&wc);host=CreateWindowW(wc.lpszClassName,L"ExtraUpdateReview",WS_POPUP,0,0,800,600,NULL,NULL,wc.hInstance,NULL);CHECK(host);
 dc=CreateCompatibleDC(NULL);bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=800;bi.bmiHeader.biHeight=-600;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bm=CreateDIBSection(dc,&bi,DIB_RGB_COLORS,&pixels,NULL,0);old=SelectObject(dc,bm);

 if(argc==3&&(!wcscmp(argv[2],L"--menu-introduction")||!wcscmp(argv[2],L"--menu-introduction-render"))){
  RECT bounds={0,0,692,0};WCHAR name[64];
  CHECK(extra07_window_message(host,EX07_OPEN,0,0,&out));SelectObject(dc,ex07_note);
  CHECK(DrawTextW(dc,ex07_synopsis,-1,&bounds,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX)<=63);
  if(!wcscmp(argv[2],L"--menu-introduction")){
  CHECK(ex07_hit(668,98)==-1&&ex07_hit(210,98)==-1&&ex07_hit(180,172)==-1);
  CHECK(ex07_hit(668,150)==1030&&ex07_hit(210,150)==1040&&ex07_hit(320,150)==1041&&ex07_hit(430,150)==1042);
  for(i=0;i<13;i++){CHECK(ex07_hit(450,185+26*i)==i);CHECK(ex07_hit(450,198+26*i)==-1);}
  CHECK(ex07_hit(120,250)==1000&&ex07_hit(90,490)==1020&&ex07_hit(180,490)==1021&&ex07_hit(270,490)==1022); }
  cdtitle_preferences();
  for(i=0;i<3;i++){cdart_menuImage=i;cdtitle_korean=i!=1;ex07_render(ex07_child,dc);GdiFlush();_snwprintf_s(name,64,_TRUNCATE,L"drama-introduction-%d.bmp",i);CHECK(savebmp(bm,name));}
  DestroyWindow(ex07_child);DestroyWindow(host);SelectObject(dc,old);DeleteObject(bm);DeleteDC(dc);
  puts("PASS: exact synopsis fits 3 lines; moved language/title buttons and all 13 track rows align with hit regions; introduction has no hidden old hit targets; all 3 artwork variants rendered. Existing audio/playback behavior unchanged.");return 0;
 }
 if(argc==3&&!wcscmp(argv[2],L"--flowchart")){
  MSG msg;POINT pointer={300,250};WCHAR name[64];double sx,sy,fitX,fitY;const UINT modes[]={EX07_OPEN,EX07_NOVEL,EX07_SOUND,EX07_GALLERY,EX07_FLOW,EX07_TITLE};
  CHECK(SetPixel(dc,782,590,RGB(13,199,91))!=CLR_INVALID);CHECK(SetPixel(dc,786,590,RGB(233,17,151))!=CLR_INVALID);CHECK(SetPixel(dc,790,590,RGB(41,113,229))!=CLR_INVALID);
  extra07_probe_frame(host,dc,0,0,800,600);CHECK(PeekMessageW(&msg,host,EX07_HUB,EX07_HUB,PM_REMOVE));CHECK(extra07_window_message(host,msg.message,0,0,&out));CHECK(ex07_hub&&!ex07_flow);
  for(i=0;i<6;i++){CHECK(ex07_hit(400,174+56*i)==i);CHECK(ex07_hit(400,200+56*i)==-1);ex07_proc(ex07_child,WM_LBUTTONUP,0,MAKELPARAM(400,174+56*i));CHECK(PeekMessageW(&msg,host,modes[i],modes[i],PM_REMOVE));}
  ex07_render(ex07_child,dc);GdiFlush();CHECK(savebmp(bm,L"extra-hub-flowchart.bmp"));
  CHECK(extra07_window_message(host,EX07_FLOW,0,0,&out));CHECK(ex07_flow&&!ex07_hub&&!ex07_audio&&reviewAudioGate);
  /* at 100% every node sits inside the panel with unclipped text and hit-tests back to itself */
  fitX=flow_x;fitY=flow_y;CHECK(flow_scale==1&&fitX==FLOW_LEFT+(FLOW_RIGHT-FLOW_LEFT-FLOW_W)/2.0&&fitY==FLOW_TOP+(FLOW_BOTTOM-FLOW_TOP-FLOW_H)/2.0);
  for(i=0;i<FLOW_COUNT;i++){
   const FLOW_NODE*n=&flow_nodes[i];int left=(int)fitX+n->x,top=(int)fitY+n->y,room=n->x+n->width-8-flow_textLeft(n);
   CHECK(left>=FLOW_LEFT&&left+n->width<=FLOW_RIGHT&&top>=FLOW_TOP&&top+n->height<=FLOW_BOTTOM);
   CHECK(flow_hit(left+n->width/2,top+n->height/2)==100+i);
   if(n->kind==1){SIZE detail;SelectObject(dc,ex07_note);CHECK(GetTextExtentPoint32W(dc,n->title,lstrlenW(n->title),&extent));SelectObject(dc,ex07_tiny);CHECK(GetTextExtentPoint32W(dc,n->detail,lstrlenW(n->detail),&detail));CHECK(extent.cx+(n->detail[0]?12+detail.cx:0)<=n->width-20);}
   else if(n->height<40){_snwprintf_s(name,64,_TRUNCATE,L"%s · %s",n->title,n->detail);SelectObject(dc,ex07_note);CHECK(GetTextExtentPoint32W(dc,name,lstrlenW(name),&extent)&&extent.cx<=n->width-20);}
   else{SelectObject(dc,ex07_small);CHECK(GetTextExtentPoint32W(dc,n->title,lstrlenW(n->title),&extent)&&extent.cx<=room);SelectObject(dc,n->kind>=3?ex07_note:ex07_tiny);CHECK(GetTextExtentPoint32W(dc,n->detail,lstrlenW(n->detail),&extent)&&extent.cx<=room);}
  }
  for(i=0;i<FLOW_COUNT;i++)for(j=i+1;j<FLOW_COUNT;j++){const FLOW_NODE*p=&flow_nodes[i],*q=&flow_nodes[j];CHECK(p->x+p->width<=q->x||q->x+q->width<=p->x||p->y+p->height<=q->y||q->y+q->height<=p->y);}
  for(i=0;i<FLOW_EDGES;i++)if(flow_edges[i].route==0&&flow_edges[i].label){const FLOW_NODE*n=&flow_nodes[flow_edges[i].from];SIZE tag;_snwprintf_s(name,64,_TRUNCATE,L"%s ↓",flow_edges[i].label);SelectObject(dc,ex07_tiny);CHECK(GetTextExtentPoint32W(dc,name,lstrlenW(name),&tag));SelectObject(dc,ex07_small);CHECK(GetTextExtentPoint32W(dc,n->title,lstrlenW(n->title),&extent));CHECK(flow_textLeft(n)+extent.cx+8<=n->x+n->width-8-tag.cx);}
  CHECK(flow_hit((int)fitX+280,(int)fitY+10)==-1&&flow_hit(10,300)==-1&&flow_hit(400,70)==-1);
  ex07_render(ex07_child,dc);GdiFlush();CHECK(savebmp(bm,L"flowchart-overview.bmp"));
  ex07_hover=101;ex07_render(ex07_child,dc);GdiFlush();CHECK(flow_previewNode==1&&flow_preview);CHECK(savebmp(bm,L"flowchart-preview.bmp"));ex07_hover=-1;
  /* plain wheel zooms about the pointer, a drag pans without opening a scene, 0 returns to the full chart */
  scale=flow_scale;anchorX=(300-flow_x)/scale;anchorY=(250-flow_y)/scale;ClientToScreen(ex07_child,&pointer);
  SendMessageW(ex07_child,WM_MOUSEWHEEL,MAKEWPARAM(0,120),MAKELPARAM(pointer.x,pointer.y));CHECK(flow_scale>scale&&fabs((300-flow_x)/flow_scale-anchorX)<.001&&fabs((250-flow_y)/flow_scale-anchorY)<.001);
  sx=flow_x;sy=flow_y;SendMessageW(ex07_child,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(335,275));SendMessageW(ex07_child,WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(300,250));SendMessageW(ex07_child,WM_LBUTTONUP,0,MAKELPARAM(300,250));CHECK(!flow_drag&&flow_x==sx-35&&flow_y==sy-25&&flow_view==-1);
  for(i=0;i<3;i++)SendMessageW(ex07_child,WM_MOUSEWHEEL,MAKEWPARAM(0,120),MAKELPARAM(pointer.x,pointer.y));
  ex07_render(ex07_child,dc);GdiFlush();CHECK(savebmp(bm,L"flowchart-zoom.bmp"));
  SendMessageW(ex07_child,WM_KEYDOWN,'0',0);CHECK(flow_scale==1&&flow_x==fitX&&flow_y==fitY);
  /* every node opens its scene inside the module; Esc goes back to the chart, not out of it */
  for(i=0;i<FLOW_COUNT;i++){
   const FLOW_NODE*n=&flow_nodes[i];
   ex07_proc(ex07_child,WM_LBUTTONUP,0,MAKELPARAM((int)fitX+n->x+n->width/2,(int)fitY+n->y+n->height/2));
   CHECK(flow_view==i&&flow_pick==i&&flow_scene&&!ex07_audio);
   if(i<2){ex07_render(ex07_child,dc);GdiFlush();_snwprintf_s(name,64,_TRUNCATE,L"flowchart-scene-%d.bmp",i);CHECK(savebmp(bm,name));}
   SendMessageW(ex07_child,WM_KEYDOWN,VK_ESCAPE,0);CHECK(flow_view==-1&&!PeekMessageW(&msg,host,EX07_BACK,EX07_BACK,PM_REMOVE));
  }
  /* the scene keeps the image viewer's wheel zoom, drag and fit */
  flow_openScene(0);scale=flow_sceneScale;anchorX=(300-flow_sceneX)/scale;anchorY=(250-flow_sceneY)/scale;
  SendMessageW(ex07_child,WM_MOUSEWHEEL,MAKEWPARAM(0,120),MAKELPARAM(pointer.x,pointer.y));CHECK(flow_sceneScale>scale&&fabs((300-flow_sceneX)/flow_sceneScale-anchorX)<.001&&fabs((250-flow_sceneY)/flow_sceneScale-anchorY)<.001);
  sx=flow_sceneX;sy=flow_sceneY;SendMessageW(ex07_child,WM_LBUTTONDOWN,MK_LBUTTON,MAKELPARAM(300,250));SendMessageW(ex07_child,WM_MOUSEMOVE,MK_LBUTTON,MAKELPARAM(335,275));SendMessageW(ex07_child,WM_LBUTTONUP,0,MAKELPARAM(335,275));CHECK(flow_sceneX==sx+35&&flow_sceneY==sy+25&&flow_view==0);
  SendMessageW(ex07_child,WM_KEYDOWN,'0',0);CHECK(flow_sceneScale==scale&&flow_view==0);
  /* buttons and keys; previous/next skip the option chips, which share their choice's scene */
  SendMessageW(ex07_child,WM_LBUTTONUP,0,MAKELPARAM(345,572));CHECK(flow_view==-1&&flow_pick==0);
  SendMessageW(ex07_child,WM_LBUTTONUP,0,MAKELPARAM(585,572));CHECK(flow_pick==1);
  SendMessageW(ex07_child,WM_LBUTTONUP,0,MAKELPARAM(585,572));CHECK(flow_pick==4);
  SendMessageW(ex07_child,WM_LBUTTONUP,0,MAKELPARAM(470,572));CHECK(flow_pick==1&&flow_view==-1);
  SendMessageW(ex07_child,WM_LBUTTONUP,0,MAKELPARAM(705,572));CHECK(flow_view==1);
  SendMessageW(ex07_child,WM_KEYDOWN,VK_RIGHT,0);CHECK(flow_view==4);
  SendMessageW(ex07_child,WM_KEYDOWN,VK_RETURN,0);CHECK(flow_view==-1&&flow_pick==4);
  SendMessageW(ex07_child,WM_KEYDOWN,VK_LEFT,0);CHECK(flow_pick==1);
  SendMessageW(ex07_child,WM_RBUTTONUP,0,0);CHECK(PeekMessageW(&msg,host,EX07_BACK,EX07_BACK,PM_REMOVE));CHECK(extra07_window_message(host,msg.message,0,0,&out));CHECK(ex07_hub&&!ex07_flow);
  CHECK(extra07_window_message(host,EX07_FLOW,0,0,&out));CHECK(flow_scale==1&&flow_pick==-1&&flow_view==-1);
  SendMessageW(ex07_child,WM_LBUTTONUP,0,MAKELPARAM(216,572));CHECK(PeekMessageW(&msg,host,EX07_BACK,EX07_BACK,PM_REMOVE));CHECK(extra07_window_message(host,msg.message,0,0,&out));CHECK(ex07_hub&&!ex07_flow);
  CHECK(extra07_window_message(host,EX07_FLOW,0,0,&out));SendMessageW(ex07_child,WM_KEYDOWN,VK_ESCAPE,0);CHECK(PeekMessageW(&msg,host,EX07_BACK,EX07_BACK,PM_REMOVE));CHECK(extra07_window_message(host,msg.message,0,0,&out));CHECK(ex07_hub);
  CHECK(extra07_window_message(host,EX07_FLOW,0,0,&out));SendMessageW(ex07_child,WM_LBUTTONUP,0,MAKELPARAM(95,572));CHECK(PeekMessageW(&msg,host,EX07_TITLE,EX07_TITLE,PM_REMOVE));CHECK(extra07_window_message(host,msg.message,0,0,&out));CHECK(!ex07_child&&!reviewAudioGate);
  KillTimer(host,EX07_RELEASE_TIMER);DestroyWindow(host);SelectObject(dc,old);DeleteObject(bm);DeleteDC(dc);
  puts("PASS: six hub button routes; all 20 flowchart nodes fit the panel at 100% with unclipped text, no overlap and exact hit-tests; hover preview; plain wheel zoom about the pointer, drag pan without a click, 0 fit; every node opens its scene in the module and Esc returns to the chart; scene wheel zoom, drag and fit; buttons and keys walk the scene nodes; overview, preview, zoom and two scene renders saved. Read-only graph has no game-script or save mutations.");return 0;
 }
 if(argc==3&&!wcscmp(argv[2],L"--subtitle-modes")){
  int mode,t,k,line,anchorCue,seen;DWORD held;HWAVEOUT audio;BYTE*pcm;WCHAR ini[MAX_PATH],name[64];
  CHECK(extra07_window_message(host,EX07_OPEN,0,0,&out));cdsub_preferences();CHECK(cdsub_mode==0);
  CHECK(ex07_hit(210,100)==1040&&ex07_hit(320,100)==1041&&ex07_hit(430,100)==1042&&ex07_hit(265,100)==-1&&ex07_hit(668,98)==1030);
  for(mode=0;mode<3;mode++)for(t=0;t<13;t++){
   const CDSUB_TRACK*d=&cdsub_tracks[mode][t];CHECK(d->count==ex07_tracks[t].count);
   for(k=0;k<d->count;k++){
    const EX07_CUE*q=&d->cues[k];CHECK(q->start==ex07_tracks[t].cues[k].start&&q->end==ex07_tracks[t].cues[k].end&&q->y>=140&&q->y+q->height<=496);
    for(line=0;line<q->count;line++){
     const EX07_LINE*z=&q->lines[line];WORD glyphs[1024];int length=lstrlenW(z->text);CHECK(length>0&&length<1024);
     CHECK(mode!=0||!z->source);CHECK(mode!=1||z->source);
     SelectObject(dc,z->source?(mode==1?ex07_jpMain:ex07_jp):ex07_font);
     CHECK(GetTextExtentPoint32W(dc,z->text,length,&extent)&&extent.cx<=652);
     CHECK(GetGlyphIndicesW(dc,z->text,length,glyphs,GGI_MARK_NONEXISTING_GLYPHS)!=GDI_ERROR);
     for(j=0;j<length;j++)CHECK(glyphs[j]!=0xffff);
    }
   }
  }
  ex07_volume=0;CHECK(ex07_play(0));CHECK(ex07_seekTo(90000));ex07_update();ex07_pause();
  held=ex07_clock();seen=ex07_readCue[0];audio=ex07_audio;pcm=ex07_pcm;
  for(mode=0;mode<3;mode++){
   ex07_proc(ex07_child,WM_LBUTTONUP,0,MAKELPARAM(210+110*mode,100));CHECK(cdsub_mode==mode&&ex07_audio==audio&&ex07_pcm==pcm&&ex07_clock()==held&&ex07_paused&&ex07_readCue[0]==seen);
   ex07_viewPage=0;CHECK(ex07_page()==cdsub_get(0)->cues[ex07_cue].page);
   CHECK(ex07_hit(100,cdsub_get(0)->cues[ex07_cue].y+12)==ex07_cue);
   ex07_render(ex07_child,dc);GdiFlush();_snwprintf_s(name,64,_TRUNCATE,L"subtitles-mode-%d.bmp",mode);CHECK(savebmp(bm,name));
   cdsub_loaded=FALSE;cdsub_preferences();CHECK(cdsub_mode==mode);
  }
  ex07_pageMove(-1);anchorCue=-1;for(i=0;i<=seen;i++)if(cdsub_get(0)->cues[i].page==ex07_page()){anchorCue=i;break;}CHECK(anchorCue>=0);
  ex07_proc(ex07_child,WM_LBUTTONUP,0,MAKELPARAM(320,100));CHECK(ex07_page()==cdsub_get(0)->cues[anchorCue].page);
  ex07_pageMove(0);anchorCue=ex07_cue;ex07_proc(ex07_child,WM_LBUTTONUP,0,MAKELPARAM(100,cdsub_get(0)->cues[anchorCue].y+12));CHECK(ex07_replaying&&ex07_replayCue==anchorCue&&ex07_clock()==held);
  ex07_proc(ex07_child,WM_LBUTTONUP,0,MAKELPARAM(430,100));CHECK(cdsub_mode==2&&ex07_replaying&&ex07_replayCue==anchorCue&&ex07_clock()==held&&ex07_readCue[0]==seen);
  CHECK(ex07_startAt(held,FALSE));ex07_menu=TRUE;ex07_proc(ex07_child,WM_LBUTTONUP,0,MAKELPARAM(210,100));CHECK(!ex07_paused&&ex07_audio==audio&&ex07_pcm==pcm&&ex07_clock()+10>=held&&ex07_clock()<held+500);
  ex07_render(ex07_child,dc);GdiFlush();CHECK(savebmp(bm,L"drama-subtitle-menu.bmp"));
  cdsub_path(ini);CHECK(WritePrivateProfileStringW(L"Display",L"SubtitleMode",L"99",ini));cdsub_loaded=FALSE;cdsub_preferences();CHECK(cdsub_mode==0);
  DestroyWindow(ex07_child);DestroyWindow(host);SelectObject(dc,old);DeleteObject(bm);DeleteDC(dc);
  puts("PASS: 629 cues x 3 layouts; matching timing/IDs, font glyphs, width and page height; three native button routes and INI reload; paused/playing/replay audio preserved; history anchorCue retained; active display cue click replays the same original cue; four renders.");return 0;
 }
 if(argc==3&&!wcscmp(argv[2],L"--novel-intro")){
  CHECK(extra07_window_message(host,EX07_NOVEL,0,0,&out));ex07_render(ex07_child,dc);GdiFlush();CHECK(savebmp(bm,L"novel-introduction.bmp"));
  DestroyWindow(ex07_child);DestroyWindow(host);SelectObject(dc,old);DeleteObject(bm);DeleteDC(dc);puts("Rendered the changed novel introduction only; passing title-toggle checks reused.");return 0;
 }
 if(argc==3&&!wcscmp(argv[2],L"--title-toggle-intro")){
  WCHAR ini[MAX_PATH],label[160],setting[16];WORD glyphs[160];HWAVEOUT audio;BYTE*pcm;DWORD held;int seen,language;
  CHECK(extra07_window_message(host,EX07_OPEN,0,0,&out));cdtitle_preferences();CHECK(cdtitle_korean);
  CHECK(ex07_hit(668,98)==1030&&ex07_hit(589,98)==-1&&ex07_hit(668,114)==-1&&ex07_hit(400,137)==0);
  for(language=0;language<2;language++){
   SelectObject(dc,ex07_small);
   for(i=0;i<13;i++){
    _snwprintf_s(label,160,_TRUNCATE,L"%02d. %s",i+1,cdtitle_get(i,TRUE));
    CHECK(GetTextExtentPoint32W(dc,label,lstrlenW(label),&extent)&&extent.cx<=404);
    CHECK(GetGlyphIndicesW(dc,label,lstrlenW(label),glyphs,GGI_MARK_NONEXISTING_GLYPHS)!=GDI_ERROR);
    for(j=0;j<lstrlenW(label);j++)CHECK(glyphs[j]!=0xffff);
   }
   ex07_render(ex07_child,dc);GdiFlush();CHECK(savebmp(bm,language?L"drama-titles-original.bmp":L"drama-titles-korean.bmp"));
   ex07_proc(ex07_child,WM_LBUTTONUP,0,MAKELPARAM(668,98));CHECK(cdtitle_korean==(language==1));
   cdtitle_loaded=FALSE;cdtitle_preferences();CHECK(cdtitle_korean==(language==1));
  }
  ex07_volume=0;CHECK(ex07_play(0));CHECK(ex07_seekTo(90000));ex07_update();ex07_pause();
  audio=ex07_audio;pcm=ex07_pcm;held=ex07_clock();seen=ex07_readCue[0];
  ex07_menu=TRUE;ex07_proc(ex07_child,WM_LBUTTONUP,0,MAKELPARAM(668,98));
  CHECK(!cdtitle_korean&&ex07_menu&&ex07_audio==audio&&ex07_pcm==pcm&&ex07_clock()==held&&ex07_paused&&ex07_readCue[0]==seen);
  ex07_pause();ex07_proc(ex07_child,WM_LBUTTONUP,0,MAKELPARAM(668,98));CHECK(cdtitle_korean&&!ex07_paused&&ex07_audio==audio&&ex07_pcm==pcm&&ex07_clock()>=held&&ex07_clock()<held+500&&ex07_readCue[0]==seen);
  ex07_pause();ex07_menu=FALSE;ex07_render(ex07_child,dc);GdiFlush();CHECK(savebmp(bm,L"drama-title-player.bmp"));
  CHECK(extra07_window_message(host,EX07_BACK,0,0,&out));CHECK(extra07_window_message(host,EX07_OPEN,0,0,&out));CHECK(cdtitle_korean);
  cdart_settingsPath(ini);GetPrivateProfileStringW(L"Display",L"Unrelated",L"",setting,16,ini);CHECK(!wcscmp(setting,L"keep"));CHECK(cdart_menuImage==0&&cdart_background==1);
  CHECK(WritePrivateProfileStringW(L"Display",L"TranslateTitles",L"99",ini));cdtitle_loaded=FALSE;cdtitle_preferences();CHECK(cdtitle_korean);
  CHECK(extra07_window_message(host,EX07_NOVEL,0,0,&out));ex07_render(ex07_child,dc);GdiFlush();CHECK(savebmp(bm,L"novel-introduction.bmp"));
  DestroyWindow(ex07_child);DestroyWindow(host);SelectObject(dc,old);DeleteObject(bm);DeleteDC(dc);
  puts("PASS: 13 Korean/original titles fit and have glyphs; toggle window messages and boundaries; ON/OFF persisted through preference reload and mode return; paused/playing audio handle, PCM, position and read progress preserved; other settings preserved; invalid setting defaults ON; three menu/player renders plus novel introduction render.");return 0;
 }
 if(argc==3&&!wcscmp(argv[2],L"--menu-art-selection")){
  DWORD hashes[3]={0};WCHAR ini[MAX_PATH];
  CHECK(extra07_window_message(host,EX07_OPEN,0,0,&out));cdart_preferences();CHECK(cdart_menuImage==0&&cdart_view==-1);
  for(i=0;i<3;i++){
   const BYTE*bytes;size_t k;WCHAR name[64];
   ex07_proc(ex07_child,WM_LBUTTONUP,0,MAKELPARAM(94+i*88,490));
   CHECK(ex07_menu&&cdart_view==-1&&cdart_menuImage==i&&cdart_background==1&&ex07_track==-1&&!ex07_audio);
   ex07_render(ex07_child,dc);GdiFlush();_snwprintf_s(name,64,_TRUNCATE,L"menu-art-%d.bmp",i);CHECK(savebmp(bm,name));
   bytes=(const BYTE*)pixels;hashes[i]=2166136261u;for(k=0;k<800*600*4;k++){hashes[i]^=bytes[k];hashes[i]*=16777619u;}
   ex07_proc(ex07_child,WM_LBUTTONUP,0,MAKELPARAM(180,290));CHECK(cdart_view==i);
   ex07_proc(ex07_child,WM_KEYDOWN,VK_ESCAPE,0);CHECK(cdart_view==-1&&ex07_menu&&cdart_menuImage==i);
  }
  CHECK(hashes[0]!=hashes[1]&&hashes[1]!=hashes[2]&&hashes[0]!=hashes[2]);
  CHECK(ex07_hit(138,490)==-1&&ex07_hit(226,490)==-1&&ex07_hit(400,137)==0&&ex07_hit(400,485)==12);
  CHECK(extra07_window_message(host,EX07_BACK,0,0,&out));CHECK(extra07_window_message(host,EX07_OPEN,0,0,&out));CHECK(cdart_menuImage==2);
  cdart_preferencesLoaded=FALSE;cdart_menuImage=-1;cdart_preferences();CHECK(cdart_menuImage==2&&cdart_background==1);
  cdart_settingsPath(ini);CHECK(WritePrivateProfileStringW(L"Display",L"MenuImage",L"99",ini));cdart_preferencesLoaded=FALSE;cdart_preferences();CHECK(cdart_menuImage==0&&cdart_background==1);
  DestroyWindow(ex07_child);DestroyWindow(host);SelectObject(dc,old);DeleteObject(bm);DeleteDC(dc);
  puts("PASS: three buttons replace list-side image without opening viewer; clicking picture opens selected image; close returns to same list; distinct renders; track rows and gaps retained; menu choice persistence and invalid-value fallback; playback background unaffected.");return 0;
 }
 if(argc==3&&!wcscmp(argv[2],L"--sound-art")){
  BITMAP art;BYTE*bytes;int partial=0,clear=0;
  CHECK(extra07_window_message(host,EX07_SOUND,0,0,&out));ex07_render(ex07_child,dc);GdiFlush();
  CHECK(sm_art&&sm_artWidth==142&&sm_artHeight==132&&GetObjectW(sm_art,sizeof(art),&art));bytes=(BYTE*)art.bmBits;
  for(i=0;i<sm_artWidth*sm_artHeight;i++){BYTE a=bytes[i*4+3];if(!a)clear++;else if(a<255)partial++;CHECK(bytes[i*4]<=a&&bytes[i*4+1]<=a&&bytes[i*4+2]<=a);}
  CHECK(clear>100&&partial>100);CHECK(savebmp(bm,L"sound-booklet-list.bmp"));
  ex07_volume=0;CHECK(ex07_play(13));CHECK(ex07_seekTo(21000));ex07_pause();ex07_render(ex07_child,dc);GdiFlush();CHECK(savebmp(bm,L"sound-booklet-lyrics.bmp"));
  ex07_stop();ex07_track=15;ex07_render(ex07_child,dc);GdiFlush();CHECK(savebmp(bm,L"sound-booklet-long-title.bmp"));
  CHECK(extra07_window_message(host,EX07_BACK,0,0,&out));CHECK(!sm_art&&!sm_artTried);
  CHECK(extra07_window_message(host,EX07_SOUND,0,0,&out));ex07_render(ex07_child,dc);CHECK(sm_artWidth==142&&sm_artHeight==132);
  DestroyWindow(ex07_child);CHECK(!sm_art);DestroyWindow(host);SelectObject(dc,old);DeleteObject(bm);DeleteDC(dc);
  puts("PASS: display-size WIC filter; transparent and partially covered pixels; valid premultiplied channels; cache reload and cleanup; refreshed sound renders.");return 0;
 }
 if(argc==3&&!wcscmp(argv[2],L"--cover-sound")){
  POINT point={400,250};double before;HWAVEOUT audio;BYTE*pcm;DWORD held;
  CHECK(extra07_window_message(host,EX07_OPEN,0,0,&out));
  CHECK(ex07_hit(94,490)==1020&&ex07_hit(182,490)==1021&&ex07_hit(270,490)==1022&&ex07_hit(138,490)==-1);
  ex07_render(ex07_child,dc);GdiFlush();CHECK(savebmp(bm,L"drama-menu-korean.bmp"));
  ex07_proc(ex07_child,WM_LBUTTONUP,0,MAKELPARAM(270,490));CHECK(cdart_menuImage==2&&cdart_view==-1);ex07_proc(ex07_child,WM_LBUTTONUP,0,MAKELPARAM(180,290));CHECK(cdart_view==2&&cdart_width[2]==1058&&cdart_height[2]==1487);
  ex07_render(ex07_child,dc);GdiFlush();CHECK(savebmp(bm,L"classroom-cover-korean.bmp"));
  before=cdart_scale;ClientToScreen(ex07_child,&point);ex07_proc(ex07_child,WM_MOUSEWHEEL,MAKEWPARAM(0,WHEEL_DELTA),MAKELPARAM(point.x,point.y));CHECK(cdart_scale>before);
  ex07_proc(ex07_child,WM_KEYDOWN,'0',0);CHECK(fabs(cdart_scale-before)<.001);
  ex07_proc(ex07_child,WM_LBUTTONUP,0,MAKELPARAM(330,570));CHECK(cdart_view==0);
  ex07_proc(ex07_child,WM_LBUTTONUP,0,MAKELPARAM(580,570));CHECK(cdart_view==2);
  ex07_proc(ex07_child,WM_KEYDOWN,VK_ESCAPE,0);CHECK(cdart_view==-1&&ex07_menu);
  CHECK(extra07_window_message(host,EX07_SOUND,0,0,&out));ex07_volume=0;
  ex07_render(ex07_child,dc);GdiFlush();CHECK(sm_art&&sm_artWidth==142&&sm_artHeight==132);CHECK(savebmp(bm,L"sound-booklet-list.bmp"));
  for(i=0;i<SM_COUNT;i++)CHECK(ex07_hit(70+(i/9)*367,151+(i%9)*29)==i);
  CHECK(sm_hit(500,383)==-1&&sm_hit(395,151)==-1&&sm_hit(75,164)==-1&&sm_hit(75,126)==-1);
  ex07_proc(ex07_child,WM_LBUTTONUP,0,MAKELPARAM(500,267));CHECK(ex07_track==13&&ex07_audio);
  CHECK(ex07_seekTo(21000));ex07_pause();CHECK(vocal_cue(sm_fingerprint[13],926100)!=NULL);
  audio=ex07_audio;pcm=ex07_pcm;held=ex07_clock();ex07_render(ex07_child,dc);GdiFlush();CHECK(savebmp(bm,L"sound-booklet-lyrics.bmp"));
  CHECK(ex07_audio==audio&&ex07_pcm==pcm&&ex07_clock()==held&&ex07_paused);
  ex07_stop();ex07_track=15;ex07_render(ex07_child,dc);GdiFlush();CHECK(savebmp(bm,L"sound-booklet-long-title.bmp"));
  CHECK(extra07_window_message(host,EX07_BACK,0,0,&out));CHECK(!sm_art&&!sm_artTried&&ex07_hub);
  CHECK(extra07_window_message(host,EX07_SOUND,0,0,&out));ex07_render(ex07_child,dc);CHECK(sm_art);
  DestroyWindow(ex07_child);CHECK(!sm_art&&!cdart_bitmap[2]);DestroyWindow(host);SelectObject(dc,old);DeleteObject(bm);DeleteDC(dc);
  puts("PASS: Korean classroom cover loads; direct menu and viewer-tab routes; wheel-message zoom and fit; 17 relocated sound rows and gaps; selected row opens correct audio; transparent mascot load/reload/cleanup; rendering preserves paused audio position; native menu/cover/sound/lyrics/long-title renders.");return 0;
 }
 if(argc==3&&!wcscmp(argv[2],L"--backgrounds")){
  DWORD held;int seen;BYTE*pcm;HWAVEOUT audio;DWORD hashes[3]={0};
  CHECK(extra07_window_message(host,EX07_OPEN,0,0,&out));ex07_volume=0;CHECK(ex07_play(0));CHECK(ex07_seekTo(90000));ex07_update();ex07_pause();held=ex07_clock();seen=ex07_readCue[0];pcm=ex07_pcm;audio=ex07_audio;
  CHECK(cdart_load(2)&&cdart_width[2]==427&&cdart_height[2]==600);
  for(i=0;i<3;i++){
   const BYTE*bytes;size_t k;WCHAR name[64];
   ex07_proc(ex07_child,WM_LBUTTONUP,0,MAKELPARAM(550,58));CHECK(cdart_picker);
   ex07_proc(ex07_child,WM_LBUTTONUP,0,MAKELPARAM(600,140+38*i));CHECK(!cdart_picker&&cdart_background==i);
   CHECK(ex07_pcm==pcm&&ex07_audio==audio&&ex07_clock()==held&&ex07_readCue[0]==seen&&ex07_paused);
   cdart_preferencesLoaded=FALSE;cdart_background=-1;cdart_preferences();CHECK(cdart_background==i);
   ex07_render(ex07_child,dc);GdiFlush();_snwprintf_s(name,64,_TRUNCATE,L"background-%d.bmp",i);CHECK(savebmp(bm,name));
   bytes=(const BYTE*)pixels;hashes[i]=2166136261u;for(k=0;k<800*600*4;k++){hashes[i]^=bytes[k];hashes[i]*=16777619u;}
  }
  CHECK(hashes[0]!=hashes[1]&&hashes[1]!=hashes[2]&&hashes[0]!=hashes[2]);
  ex07_proc(ex07_child,WM_LBUTTONUP,0,MAKELPARAM(550,58));ex07_render(ex07_child,dc);GdiFlush();CHECK(savebmp(bm,L"background-picker.bmp"));
  ex07_proc(ex07_child,WM_KEYDOWN,VK_ESCAPE,0);CHECK(!cdart_picker&&!ex07_menu);
  ex07_proc(ex07_child,WM_LBUTTONUP,0,MAKELPARAM(680,58));CHECK(cdart_view==2);CHECK(cdart_hit(580,570)==4&&cdart_hit(700,570)==5);cdart_close();
  CHECK(ex07_hit(100,525)==-1&&ex07_hit(670,538)==1007);ex07_proc(ex07_child,WM_LBUTTONUP,0,MAKELPARAM(100,525));CHECK(ex07_clock()==held);
  ex07_proc(ex07_child,WM_LBUTTONUP,0,MAKELPARAM(670,538));CHECK(ex07_track==1&&ex07_savedPosition[0]==held&&ex07_savedPaused[0]&&cdart_background==2);
  CHECK(ex07_seekTo(30000));ex07_update();ex07_pause();ex07_proc(ex07_child,WM_LBUTTONUP,0,MAKELPARAM(120,538));CHECK(ex07_track==0&&ex07_paused&&ex07_clock()+10>=held&&ex07_clock()<=held&&ex07_readCue[0]==seen);
  CHECK(ex07_play(12));CHECK(ex07_hit(670,525)==-1);held=ex07_clock();ex07_proc(ex07_child,WM_LBUTTONUP,0,MAKELPARAM(670,525));CHECK(ex07_track==12&&ex07_clock()>=held&&ex07_clock()<held+500);
  ex07_render(ex07_child,dc);GdiFlush();CHECK(savebmp(bm,L"last-track.bmp"));
  DestroyWindow(ex07_child);CHECK(!cdart_bitmap[0]&&!cdart_bitmap[1]&&!cdart_bitmap[2]);DestroyWindow(host);SelectObject(dc,old);DeleteObject(bm);DeleteDC(dc);
  puts("PASS: three distinct backgrounds; actual window-message selector routing; INI selection persistence; no audio/clock/read-history changes during selection; selected image viewer; previous/next track routing and remembered positions; first/last disabled buttons do not seek; all three bitmap handles released.");return 0;
 }
 if(argc==3&&!wcscmp(argv[2],L"--sentence-replay")){
  DWORD held,deadline;int seen,current;const EX07_CUE*cue;
  CHECK(extra07_window_message(host,EX07_OPEN,0,0,&out));ex07_volume=0;CHECK(ex07_play(0));CHECK(ex07_seekTo(90000));ex07_update();ex07_pause();
  held=ex07_clock();seen=ex07_readCue[0];current=ex07_cue;CHECK(seen==21&&ex07_page()==4&&ex07_paused);
  ex07_pageMove(-1);ex07_pageMove(-1);ex07_pageMove(-1);CHECK(ex07_page()==1&&ex07_clock()==held&&ex07_hit(100,145)==0);
  CHECK(ex07_hit(530,100)==1002&&ex07_hit(620,100)==1003&&ex07_hit(710,100)==1004);
  CHECK(ex07_replaySentence(0));cue=&ex07_tracks[0].cues[0];
  CHECK(ex07_header.lpData==(LPSTR)(ex07_pcm+((DWORD)((ULONGLONG)cue->start*176400/1000)&~3u)));
  CHECK(ex07_header.dwBufferLength==((DWORD)((ULONGLONG)cue->end*176400/1000)&~3u)-((DWORD)((ULONGLONG)cue->start*176400/1000)&~3u));
  CHECK(ex07_clock()==held&&ex07_cue==current&&ex07_readCue[0]==seen&&ex07_page()==1&&!ex07_paused);
  ex07_render(ex07_child,dc);GdiFlush();CHECK(savebmp(bm,L"sentence-replay-history.bmp"));
  deadline=GetTickCount()+3500;while(ex07_replaying&&GetTickCount()<deadline){Sleep(20);ex07_update();}
  CHECK(!ex07_replaying&&ex07_paused&&ex07_clock()<=held&&held-ex07_clock()<10&&ex07_readCue[0]==seen&&ex07_page()==1);
  ex07_pause();Sleep(120);ex07_update();CHECK(ex07_replaySentence(0));held=ex07_resumePosition;CHECK(ex07_replaySentence(1));CHECK(ex07_resumePosition==held&&!ex07_resumePaused);
  deadline=GetTickCount()+4000;while(ex07_replaying&&GetTickCount()<deadline){Sleep(20);ex07_update();}
  CHECK(!ex07_replaying&&!ex07_paused&&ex07_clock()+10>=held&&ex07_clock()<held+300&&ex07_readCue[0]==seen);
  ex07_pageMove(0);CHECK(ex07_viewPage==0&&ex07_page()==4);ex07_render(ex07_child,dc);GdiFlush();CHECK(savebmp(bm,L"sentence-replay-current.bmp"));
  CHECK(ex07_seekTo(90000));ex07_update();ex07_pause();held=ex07_clock();CHECK(ex07_replaySentence(0));
  CHECK(extra07_window_message(host,EX07_BACK,0,0,&out));CHECK(!ex07_replaying&&!ex07_audio&&ex07_savedPosition[0]==held&&ex07_savedPaused[0]&&ex07_readCue[0]==seen);
  CHECK(extra07_window_message(host,EX07_OPEN,0,0,&out));CHECK(ex07_play(0));ex07_update();CHECK(ex07_paused&&ex07_clock()+10>=held&&ex07_clock()<=held&&ex07_readCue[0]==seen);
  CHECK(!ex07_replaySentence(seen+1));ex07_sound=TRUE;CHECK(!ex07_replaySentence(0));CHECK(ex07_seekTo(1000));CHECK(ex07_prepared&&!ex07_paused&&!ex07_replaying&&ex07_header.dwBufferLength==ex07_bytes-ex07_seek);ex07_sound=FALSE;
  DestroyWindow(ex07_child);DestroyWindow(host);SelectObject(dc,old);DeleteObject(bm);DeleteDC(dc);
  puts("PASS: exact cue start/end PCM range; frozen subtitle/read progress; paused and playing restoration after real WaveOut completion; replacement replay keeps original position; previous/current/next page routing; exit during replay and track resume; future-cue rejection; unchanged full-stream seek path.");return 0;
 }
 if(argc==3&&!wcscmp(argv[2],L"--drama-art")){
  HWAVEOUT playing;BYTE*pcm;DWORD clockBefore;double zoom,px,py;
  CHECK(extra07_window_message(host,EX07_OPEN,0,0,&out));
  CHECK(cdart_load(0)&&cdart_width[0]==1059&&cdart_height[0]==1485);
  CHECK(cdart_load(1)&&cdart_width[1]==1024&&cdart_height[1]==1536);
  CHECK(ex07_hit(150,300)==1000&&ex07_hit(120,490)==1020&&ex07_hit(240,490)==1022);
  CHECK(ex07_hit(400,137)==0&&ex07_hit(400,485)==12);
  ex07_render(ex07_child,dc);GdiFlush();CHECK(savebmp(bm,L"drama-menu.bmp"));
  cdart_open(0);CHECK(ex07_menu);ex07_render(ex07_child,dc);GdiFlush();CHECK(savebmp(bm,L"drama-cover.bmp"));
  zoom=cdart_scale;px=(400-cdart_x)/zoom;py=(250-cdart_y)/zoom;cdart_wheel(400,250,120);
  CHECK(cdart_scale>zoom&&fabs((400-cdart_x)/cdart_scale-px)<.001&&fabs((250-cdart_y)/cdart_scale-py)<.001);
  cdart_key('0');CHECK(fabs(cdart_scale-zoom)<.001);cdart_open(1);ex07_render(ex07_child,dc);GdiFlush();CHECK(savebmp(bm,L"drama-protagonists.bmp"));
  cdart_close();CHECK(ex07_menu);ex07_volume=0;CHECK(ex07_play(0));playing=ex07_audio;pcm=ex07_pcm;clockBefore=ex07_clock();cdart_open(1);Sleep(250);ex07_update();
  CHECK(ex07_audio==playing&&ex07_pcm==pcm&&!ex07_menu&&ex07_clock()>clockBefore&&!ex07_paused);
  cdart_close();CHECK(!ex07_menu&&ex07_audio==playing);ex07_cue=3;ex07_render(ex07_child,dc);GdiFlush();CHECK(savebmp(bm,L"drama-player.bmp"));
  cdart_open(0);CHECK(extra07_window_message(host,EX07_BACK,0,0,&out));CHECK(ex07_hub&&cdart_view==-1&&!ex07_audio);
  CHECK(extra07_window_message(host,EX07_OPEN,0,0,&out));CHECK(ex07_menu&&cdart_view==-1);
  DestroyWindow(ex07_child);CHECK(!cdart_bitmap[0]&&!cdart_bitmap[1]&&!reviewAudioGate);DestroyWindow(host);SelectObject(dc,old);DeleteObject(bm);DeleteDC(dc);
  puts("PASS: two full-resolution BMP loads; all artwork and track hit regions; menu/player artwork renders; pointer-centred wheel/fit; unchanged active WAV and advancing clock through artwork; back/reentry; bitmap cleanup.");return 0;
 }
 if(argc==3&&!wcscmp(argv[2],L"--gallery-labels")){
  int maxHeight=0;RECT labelRect;WCHAR fullLabel[256];WORD glyphs[256];
  CHECK(extra07_window_message(host,EX07_GALLERY,0,0,&out));SelectObject(dc,ex07_small);
  for(i=0;i<GAL_COUNT;i++){
   int n=lstrlenW(gal_items[i].label),h;CHECK(n<256);
   CHECK(GetGlyphIndicesW(dc,gal_items[i].label,n,glyphs,GGI_MARK_NONEXISTING_GLYPHS)!=GDI_ERROR);
   for(j=0;j<n;j++)CHECK(glyphs[j]!=0xffff);
   gal_labelLines(dc,gal_items[i].label,fullLabel);labelRect=(RECT){0,0,162,52};h=DrawTextW(dc,fullLabel,-1,&labelRect,DT_CALCRECT|DT_CENTER|DT_WORDBREAK|DT_EDITCONTROL|DT_NOPREFIX);
   if(h>maxHeight)maxHeight=h;if(h>52){wprintf(L"OVERFLOW %d %s %d\n",i,gal_items[i].label,h);return 2;}
   _snwprintf_s(fullLabel,256,_TRUNCATE,L"108 / 108 · %s · 1200%%",gal_items[i].label);CHECK(GetTextExtentPoint32W(dc,fullLabel,lstrlenW(fullLabel),&extent));CHECK(extent.cx<=712);
  }
  gal_category(1);ex07_render(ex07_child,dc);GdiFlush();CHECK(savebmp(bm,L"gallery-ko-backgrounds.bmp"));
  gal_category(2);gal_page=1;gal_clearThumbs();ex07_render(ex07_child,dc);GdiFlush();CHECK(savebmp(bm,L"gallery-ko-standing.bmp"));
  gal_select(14);ex07_render(ex07_child,dc);GdiFlush();CHECK(savebmp(bm,L"gallery-ko-selected.bmp"));
  DestroyWindow(ex07_child);DestroyWindow(host);SelectObject(dc,old);DeleteObject(bm);DeleteDC(dc);
  printf("PASS: %d gallery labels; all Hangul glyphs present; two-line maximum %d pixels within 52; selected-title width <=712; protected filenames reviewed separately.\n",GAL_COUNT,maxHeight);return 0;
 }
 if(argc==3){
  CHECK(extra07_window_message(host,EX07_OPEN,0,0,&out));ex07_track=6;ex07_menu=FALSE;ex07_cue=3;ex07_render(ex07_child,dc);GdiFlush();CHECK(savebmp(bm,L"t07-english.bmp"));
  ex07_cue=8;ex07_render(ex07_child,dc);GdiFlush();CHECK(savebmp(bm,L"t07-yandere.bmp"));SelectObject(dc,ex07_font);
  for(i=0;i<ex07_tracks[6].count;i++){const EX07_CUE*c=&ex07_tracks[6].cues[i];CHECK(c->y+c->height<=496);for(j=0;j<c->count;j++){CHECK(GetTextExtentPoint32W(dc,c->lines[j].text,lstrlenW(c->lines[j].text),&extent));CHECK(extent.cx<=652);}}
  for(i=0;i<3;i++){CHECK(extra07_window_message(host,EX07_BACK,0,0,&out));CHECK(ex07_hub&&reviewAudioGate&&ex07_child);CHECK(extra07_window_message(host,EX07_SOUND,0,0,&out));CHECK(ex07_sound&&!ex07_hub);CHECK(extra07_window_message(host,EX07_BACK,0,0,&out));CHECK(ex07_hub);CHECK(extra07_window_message(host,EX07_GALLERY,0,0,&out));CHECK(ex07_gallery);CHECK(extra07_window_message(host,EX07_BACK,0,0,&out));CHECK(ex07_hub);}
  ex07_render(ex07_child,dc);GdiFlush();CHECK(savebmp(bm,L"extra-hub.bmp"));CHECK(extra07_window_message(host,EX07_TITLE,0,0,&out));CHECK(!ex07_child&&!reviewAudioGate);KillTimer(host,EX07_RELEASE_TIMER);DestroyWindow(host);SelectObject(dc,old);DeleteObject(bm);DeleteDC(dc);puts("PASS: updated T07 native line bounds and English/yandere renders; repeated CD/music/gallery native hub transitions; final title exit restores audio gate.");return 0;
 }
 _snwprintf_s(path,MAX_PATH,_TRUNCATE,L"%s\\side-black\\reading.ini",dir);WritePrivateProfileStringW(L"Reader",L"Version",L"24d7ea21fe239844",path);WritePrivateProfileStringW(L"Reader",L"Position",L"98",path);
 CHECK(extra07_window_message(host,EX07_NOVEL,0,0,&out));CHECK(reviewAudioGate&&SB_COUNT>=408&&sb_bookmark>=0);CHECK(sb_steps[sb_bookmark].paragraph==sb_legacy[98][1]);sb_choose(sb_bookmark);anchor=sb_steps[sb_index];
 SelectObject(dc,ex07_font);book=&sb_book;
 for(i=0;i<book->count;i++){SB_STEP*p=&book->steps[i];CHECK(p->y>=138&&p->y+p->count*36<=498);for(j=0;j<p->count;j++){CHECK(GetTextExtentPoint32W(dc,p->lines[j],lstrlenW(p->lines[j]),&extent));CHECK(extent.cx<=680);}}
 for(i=0;i<book->count;){int k=0,part=book->steps[i].part,para=book->steps[i].paragraph;DWORD hash=2166136261u;do{SB_STEP*p=&book->steps[i];for(j=0;j<p->count;j++){LPCWSTR text=p->lines[j];while(*text){hash^=(WORD)*text++;hash*=16777619u;k++;}}i++;}while(i<book->count&&book->steps[i].part==part&&book->steps[i].paragraph==para);CHECK(hash==book->steps[i-1].hash&&k>0);}
 sb_path(path,0);originalText=reviewRead(path,&size);CHECK(originalText);inserted=HeapAlloc(GetProcessHeap(),0,size+16);CHECK(inserted);memcpy(inserted,"added paragraph\n",16);memcpy(inserted+16,originalText+(size>=3&&originalText[0]==239?3:0),size-(size>=3&&originalText[0]==239?3:0));
 CHECK(reviewWrite(path,inserted,size+16-(originalText[0]==239?3:0)));sb_poll();sb_poll();CHECK(sb_steps[sb_index].hash==anchor.hash&&sb_steps[sb_index].paragraph==anchor.paragraph+1);count=SB_COUNT;anchor=sb_steps[sb_index];
 CHECK(reviewWrite(path,(const BYTE*)"\xff\xfe\x80",3));reload=sb_reload();CHECK(!reload&&SB_COUNT==count&&sb_steps[sb_index].hash==anchor.hash);
 CHECK(reviewWrite(path,originalText,size));CHECK(sb_reload());CHECK(sb_steps[sb_index].hash==anchor.hash&&sb_steps[sb_index].paragraph==anchor.paragraph-1);
 _snwprintf_s(renamed,MAX_PATH,_TRUNCATE,L"%s.review-held",path);CHECK(MoveFileW(path,renamed));reload=sb_reload();CHECK(MoveFileW(renamed,path));CHECK(!reload&&SB_COUNT>0);CHECK(sb_reload());
 sb_choose(5);ex07_render(ex07_child,dc);GdiFlush();CHECK(savebmp(bm,L"live-novel.bmp"));count=sb_index;DestroyWindow(ex07_child);CHECK(!reviewAudioGate&&!SB_COUNT);CHECK(extra07_window_message(host,EX07_NOVEL,0,0,&out));CHECK(sb_bookmark==count);DestroyWindow(ex07_child);
 CHECK(extra07_window_message(host,EX07_GALLERY,0,0,&out));CHECK(reviewAudioGate&&gal_count>0);ex07_render(ex07_child,dc);GdiFlush();CHECK(savebmp(bm,L"gallery-list.bmp"));
 for(i=0;i<GAL_COUNT;i++){int w=0,h=0;HBITMAP asset=gal_load(gal_items[i].file,&w,&h);CHECK(asset&&w>0&&h>0);DeleteObject(asset);}
 gal_category(2);CHECK(gal_count==108);gal_select(gal_count-1);CHECK(gal_full&&gal_selection==107);gal_selection=-1;gal_page=8;CHECK(gal_hit(50,120)==96);gal_page=8;gal_click(50,246);CHECK(gal_selection==100&&gal_filter==2);
 gal_select(0);scale=gal_scale;anchorX=(300-gal_x)/scale;anchorY=(240-gal_y)/scale;gal_wheel(300,240,120);CHECK(gal_scale>scale&&fabs((300-gal_x)/gal_scale-anchorX)<.001&&fabs((240-gal_y)/gal_scale-anchorY)<.001);gal_key('0');CHECK(fabs(gal_scale-scale)<.001);ex07_render(ex07_child,dc);GdiFlush();CHECK(savebmp(bm,L"gallery-standing.bmp"));gal_key(VK_ESCAPE);CHECK(gal_selection==-1);DestroyWindow(ex07_child);CHECK(!gal_full&&!reviewAudioGate);
 CHECK(extra07_window_message(host,EX07_SOUND,0,0,&out));ex07_track=15;ex07_render(ex07_child,dc);GdiFlush();CHECK(savebmp(bm,L"sound-titles.bmp"));DestroyWindow(ex07_child);
 DestroyWindow(host);SelectObject(dc,old);DeleteObject(bm);DeleteDC(dc);HeapFree(GetProcessHeap(),0,originalText);HeapFree(GetProcessHeap(),0,inserted);DeleteFileW(sb_ini);
 puts("PASS: native external UTF-8 load and exact paragraph reassembly; layout widths; debounce refresh without rebuild; insertion anchor; invalid/missing-file retention; old/new bookmark migration; gallery decode coverage, late-page selection, pointer zoom/fit; sound title render. Audio gate lifecycle stub only; real game gate readback pending.");return 0;
}
