/* Reviewed Side Black text: paragraph accumulation and an independent bookmark. */
#include "novel_loader.inc"
static BOOL sb_menu=TRUE,sb_saved=TRUE;
static int sb_index=-1,sb_bookmark=-1;
static HBITMAP sb_cover;
static WCHAR sb_ini[MAX_PATH];
static void sb_putNumber(LPCWSTR key,DWORD number){WCHAR text[32];_snwprintf_s(text,32,_TRUNCATE,L"%lu",number);if(!WritePrivateProfileStringW(L"Reader",key,text,sb_ini))sb_saved=FALSE;}
static void sb_save(void){
 const SB_STEP*s;if(sb_index<0||sb_index>=SB_COUNT)return;s=&sb_steps[sb_index];sb_saved=WritePrivateProfileStringW(L"Reader",L"Version",L"external-v1",sb_ini);
 sb_putNumber(L"Position",(DWORD)sb_index);sb_putNumber(L"Part",(DWORD)s->part);sb_putNumber(L"Paragraph",(DWORD)s->paragraph);sb_putNumber(L"Offset",(DWORD)s->offset);sb_putNumber(L"Hash",s->hash);if(sb_saved)sb_bookmark=sb_index;
}
static BOOL sb_reload(void){
 SB_BOOK candidate;SB_STEP anchor={0};int current=sb_index>=0?sb_index:sb_bookmark;BOOL anchored=current>=0&&current<SB_COUNT;
 if(anchored)anchor=sb_steps[current];
 if(!sb_loadBook(&candidate)){wcscpy_s(sb_notice,160,L"원고를 읽지 못했습니다. UTF-8로 저장한 뒤 F5를 눌러 주세요.");logline("NOVEL REFRESH_FAILED retained=%d\r\n",SB_COUNT);return FALSE;}
 sb_freeBook(&sb_book);sb_book=candidate;
 if(anchored){sb_bookmark=sb_anchor(anchor.part,anchor.paragraph,anchor.offset,anchor.hash);if(sb_index>=0){sb_index=sb_bookmark;sb_save();}}
 sb_notice[0]=0;logline("NOVEL LOADED steps=%d paragraphs=%d,%d\r\n",SB_COUNT,sb_book.paras[0],sb_book.paras[1]);return TRUE;
}
static void sb_poll(void){SB_STAMP now[2];sb_stamp(now);if(sb_sameStamp(now,sb_seen))return;if(!sb_sameStamp(now,sb_pending)){memcpy(sb_pending,now,sizeof(now));return;}memcpy(sb_seen,now,sizeof(now));sb_reload();}
static void sb_open(void){
 WCHAR version[64],path[MAX_PATH];int index;sb_menu=TRUE;sb_index=-1;sb_bookmark=-1;sb_saved=TRUE;
 _snwprintf_s(sb_ini,MAX_PATH,_TRUNCATE,L"%s\\side-black\\reading.ini",dir);sb_reload();sb_stamp(sb_seen);memcpy(sb_pending,sb_seen,sizeof(sb_seen));
 GetPrivateProfileStringW(L"Reader",L"Version",L"",version,64,sb_ini);index=(int)GetPrivateProfileIntW(L"Reader",L"Position",(UINT)-1,sb_ini);
 if(!wcscmp(version,L"external-v1"))sb_bookmark=sb_anchor((int)GetPrivateProfileIntW(L"Reader",L"Part",0,sb_ini),(int)GetPrivateProfileIntW(L"Reader",L"Paragraph",0,sb_ini),(int)GetPrivateProfileIntW(L"Reader",L"Offset",0,sb_ini),GetPrivateProfileIntW(L"Reader",L"Hash",0,sb_ini));
 else if(!wcscmp(version,L"24d7ea21fe239844")&&index>=0&&index<(int)(sizeof(sb_legacy)/sizeof(sb_legacy[0])))sb_bookmark=sb_anchor(sb_legacy[index][0],sb_legacy[index][1],0,0);
 _snwprintf_s(path,MAX_PATH,_TRUNCATE,L"%s\\side-black\\cover.bmp",dir);sb_cover=(HBITMAP)LoadImageW(NULL,path,IMAGE_BITMAP,0,0,LR_LOADFROMFILE|LR_CREATEDIBSECTION);
}
static void sb_cleanup(void){sb_freeBook(&sb_book);if(sb_cover){DeleteObject(sb_cover);sb_cover=NULL;}}
/* The relay source frames FLASH with square glyphs. Keep the source data,
   but represent its typography as a centered flash cue in the reader. */
static BOOL sb_decoration(int index){
    const SB_STEP*s=&sb_steps[index];int i;const WCHAR*p;BOOL square=FALSE;
    for(i=0;i<s->count;i++)for(p=s->lines[i];*p;p++){if(*p==L'□')square=TRUE;else if(*p!=L' '&&*p!=L'　')return FALSE;}
    return square;
}
static void sb_stripFrame(LPCWSTR source,WCHAR*out,int capacity){int n=0;for(;*source&&n<capacity-1;source++)if(*source!=L'□')out[n++]=*source;out[n]=0;}
static void sb_choose(int index){if(index<0||index>=SB_COUNT)return;while(index<SB_COUNT&&sb_decoration(index))index++;if(index>=SB_COUNT)return;sb_index=index;sb_menu=FALSE;sb_save();}
static void sb_move(int delta){
    int next;if(sb_menu||sb_index<0||delta==0)return;
    next=sb_index+delta;while(next>=0&&next<SB_COUNT&&sb_decoration(next))next+=delta>0?1:-1;
    if(next>=SB_COUNT){sb_menu=TRUE;return;}
    sb_choose(max(0,next));
}
static int sb_hit(int x,int y){
    if(y>=554&&y<590){if(x>=44&&x<148)return 10;if(x>=160&&x<274)return 11;
        if(!sb_menu){if(x>=286&&x<390)return 12;if(x>=402&&x<550)return 13;if(x>=562&&x<756)return 14;}}
    if(sb_menu&&SB_COUNT&&x>=352&&x<746){if(y>=162&&y<214)return 0;if(y>=234&&y<286)return 1;if(y>=306&&y<358&&sb_bookmark>=0)return 2;}
    if(!sb_menu&&x>=44&&x<756&&y>=118&&y<510)return 14;
    return -1;
}
static void sb_click(int x,int y){
    int hit=sb_hit(x,y);
    if(hit==10)PostMessageW(ex07_parent,EX07_TITLE,0,0);
    else if(hit==11)PostMessageW(ex07_parent,EX07_BACK,0,0);
    else if(hit==12)sb_menu=TRUE;
    else if(hit==13)sb_move(-1);
    else if(hit==14)sb_move(1);
    else if((hit==0||hit==1)&&SB_COUNT)sb_choose(sb_first[hit]);
    else if(hit==2)sb_choose(sb_bookmark);
}
static void sb_key(WPARAM key){
    if(key==VK_F5){sb_reload();sb_stamp(sb_seen);return;}
    if(key==VK_ESCAPE){if(sb_menu)PostMessageW(ex07_parent,EX07_BACK,0,0);else sb_menu=TRUE;return;}
    if(sb_menu){if(key==VK_RETURN||key==VK_SPACE)sb_choose(sb_bookmark>=0?sb_bookmark:0);return;}
    if(key==VK_SPACE||key==VK_RETURN||key==VK_RIGHT||key==VK_NEXT)sb_move(1);
    else if(key==VK_LEFT||key==VK_PRIOR)sb_move(-1);
    else if(key==VK_HOME)sb_choose(sb_first[sb_steps[sb_index].part]);
    else if(key==VK_END)sb_choose(sb_end[sb_steps[sb_index].part]-1);
}
static void sb_drawCover(HDC dc){
    HDC src;HGDIOBJ old;BITMAP bm;int width,height,x,y;
    if(!sb_cover||!GetObjectW(sb_cover,sizeof(bm),&bm)||bm.bmWidth<=0||bm.bmHeight<=0)return;
    width=254;height=MulDiv(width,bm.bmHeight,bm.bmWidth);
    if(height>342){height=342;width=MulDiv(height,bm.bmWidth,bm.bmHeight);}
    x=54+(254-width)/2;y=142+(342-height)/2;src=CreateCompatibleDC(dc);if(!src)return;
    old=SelectObject(src,sb_cover);SetStretchBltMode(dc,HALFTONE);SetBrushOrgEx(dc,0,0,NULL);
    StretchBlt(dc,x,y,width,height,src,0,0,bm.bmWidth,bm.bmHeight,SRCCOPY);SelectObject(src,old);DeleteDC(src);
}
static void sb_render(HWND w,HDC dest){
    RECT client,r;WCHAR buf[160];int i,j,start;const SB_STEP*c;
    GetClientRect(w,&client);ex07_skyDraw();
    r=(RECT){0,30,800,80};ex07_logoText(ex07_dc,L"우로보로스의 사랑",r,ex07_head);
    if(sb_menu){
        r=(RECT){54,88,748,116};ex07_text(ex07_dc,L"읽을 편을 선택해 주세요. 읽던 위치는 자동으로 기억합니다.",r,ex07_small,RGB(52,86,108),DT_CENTER|DT_SINGLELINE);
        r=(RECT){48,136,314,490};ex07_panel(ex07_dc,r,TRUE);sb_drawCover(ex07_dc);
        /* Hit rows stay at 352..746 / 162,234,306 + 52; the lenses sit inside them. */
        r=(RECT){352,164,746,212};ex07_lens(ex07_dc,r,L"상편 · 처음부터",ex07_hover==0,TRUE);
        r=(RECT){352,236,746,284};ex07_lens(ex07_dc,r,L"하편 · 처음부터",ex07_hover==1,TRUE);
        r=(RECT){352,308,746,356};ex07_lens(ex07_dc,r,L"이어서 읽기",ex07_hover==2,sb_bookmark>=0);
        if(sb_bookmark>=0){c=&sb_steps[sb_bookmark];_snwprintf_s(buf,160,_TRUNCATE,L"%s · %d / %d 페이지",c->part?L"하편":L"상편",c->page+1,sb_pages[c->part]);}
        else wcscpy_s(buf,160,L"아직 읽기 기록이 없습니다.");
        r=(RECT){352,368,746,397};ex07_text(ex07_dc,buf,r,ex07_small,RGB(52,86,108),DT_CENTER|DT_SINGLELINE);
        r=(RECT){344,404,754,520};ex07_panel(ex07_dc,r,FALSE);
        r=(RECT){354,414,744,516};ex07_text(ex07_dc,L"하가네야 진이 쓴 《드라†코이》 외전.\n2006년 《더 스니커》 4월호에 처음 실렸다.\n2007년 12월 코믹마켓 73에서\n니트로플러스 세트 특전 책자로 재록됐다.",r,ex07_small,RGB(22,46,62),DT_WORDBREAK);
    }else if(sb_index>=0&&sb_index<SB_COUNT){
        c=&sb_steps[sb_index];_snwprintf_s(buf,160,_TRUNCATE,L"%s · %d / %d 페이지",c->part?L"하편":L"상편",c->page+1,sb_pages[c->part]);
        r=(RECT){56,89,746,115};ex07_text(ex07_dc,buf,r,ex07_small,RGB(52,86,108),DT_SINGLELINE);
        r=(RECT){40,120,760,514};ex07_panel(ex07_dc,r,FALSE);
        start=sb_index;while(start>0&&sb_steps[start-1].part==c->part&&sb_steps[start-1].page==c->page)start--;
        for(i=start;i<=sb_index;i++){const SB_STEP*p=&sb_steps[i];if(sb_decoration(i))continue;for(j=0;j<p->count;j++){
            BOOL flash=wcschr(p->lines[j],L'□')!=NULL;WCHAR clean[1024];LPCWSTR text=p->lines[j];
            if(flash){sb_stripFrame(text,clean,1024);text=clean;}
            r=(RECT){64,p->y+j*36,744,p->y+j*36+34};
            ex07_text(ex07_dc,text,r,flash?ex07_head:ex07_font,flash?(i==sb_index?RGB(196,40,72):RGB(214,150,166)):(i==sb_index?RGB(10,30,44):RGB(112,130,142)),DT_SINGLELINE|(flash?DT_CENTER:0));
        }}
        r=(RECT){60,529,740,550};ex07_text(ex07_dc,sb_saved?L"클릭 / Space: 다음 문단   ·   ←: 이전 문단   ·   위치 자동 저장":L"읽던 위치를 저장하지 못했습니다.",r,ex07_small,RGB(60,104,132),DT_CENTER|DT_SINGLELINE);
        r=(RECT){286,554,390,590};ex07_button(ex07_dc,r,L"목차",ex07_hover==12);
        r=(RECT){402,554,550,590};ex07_button(ex07_dc,r,L"이전",ex07_hover==13);
        r=(RECT){562,554,756,590};ex07_button(ex07_dc,r,sb_index==SB_COUNT-1?L"목차로":sb_index==sb_end[0]-1?L"하편으로":L"다음",ex07_hover==14);
    }
    r=(RECT){54,sb_menu?527:500,748,sb_menu?552:525};ex07_text(ex07_dc,sb_notice,r,ex07_small,RGB(170,40,72),DT_CENTER|DT_SINGLELINE);
    r=(RECT){44,554,148,590};ex07_button(ex07_dc,r,L"타이틀로",ex07_hover==10);
    r=(RECT){160,554,274,590};ex07_button(ex07_dc,r,L"엑스트라로",ex07_hover==11);
    SetStretchBltMode(dest,HALFTONE);StretchBlt(dest,0,0,client.right,client.bottom,ex07_dc,0,0,800,600,SRCCOPY);
}
