/* Main-story routes verified against installed script.pak; view only. */
typedef struct {int x,y,width,height,kind;LPCWSTR title,detail;} FLOW_NODE;
typedef struct {int from,to;LPCWSTR label;int x,y,width;} FLOW_EDGE;
static const FLOW_NODE flow_nodes[]={
 {240,0,220,64,0,L"이야기 시작",L"용과의 만남"},
 {225,108,250,64,0,L"첫 번째 선택",L"보리수 잎"},
 {0,222,330,68,1,L"구해줘야 해……!",L"구출을 선택"},
 {370,222,330,68,1,L"내가 뭘 할 수 있겠어",L"구출하지 않음"},
 {225,340,250,64,0,L"두 번째 선택",L"데이트"},
 {0,454,330,68,1,L"어울려 준다",L"데이트를 선택"},
 {370,454,330,68,1,L"단호하게 돌아간다",L"데이트하지 않음"},
 {225,572,250,76,2,L"용살자와 재회",L"구출 선택의 결과로 분기"},
 {480,728,220,76,3,L"엔딩 1",L"JABBERWOCKY"},
 {0,728,420,64,0,L"세 번째 선택",L"위기에서의 결단"},
 {0,854,330,68,1,L"숨통을 끊는다",L"이야기가 계속된다"},
 {370,854,330,68,1,L"그건 위험하다",L""},
 {480,978,220,76,3,L"엔딩 2",L"HERO LOST"},
 {0,1008,420,76,2,L"결전 이후",L"두 번째 선택의 결과로 분기"},
 {0,1190,420,64,0,L"마지막 선택",L"데이트를 했다면"},
 {480,1190,220,76,3,L"엔딩 3",L"MOTE IN EYE"},
 {0,1314,330,68,1,L"올바른 길을 간다",L""},
 {370,1314,330,68,1,L"틀린 길을 간다",L""},
 {0,1460,330,76,3,L"엔딩 3",L"MOTE IN EYE"},
 {370,1460,330,76,4,L"엔딩 4",L"LOVE SONG"}
};
static const FLOW_EDGE flow_edges[]={
 {0,1,NULL,0,0,0},{1,2,NULL,0,0,0},{1,3,NULL,0,0,0},
 {2,4,NULL,0,0,0},{3,4,NULL,0,0,0},{4,5,NULL,0,0,0},{4,6,NULL,0,0,0},
 {5,7,NULL,0,0,0},{6,7,NULL,0,0,0},
 {7,8,L"구출하지 않음",486,678,208},{7,9,L"구출함",80,678,150},
 {9,10,NULL,0,0,0},{9,11,NULL,0,0,0},{10,13,NULL,0,0,0},{11,12,NULL,0,0,0},
 {13,14,L"데이트함",80,1122,150},{13,15,L"데이트하지 않음",470,1122,230},
 {14,16,NULL,0,0,0},{14,17,NULL,0,0,0},{16,18,NULL,0,0,0},{17,19,NULL,0,0,0}
};
static double flow_scale=1.0,flow_x=50.0,flow_y=138.0;
static int flow_stop,flow_lastX,flow_lastY;
static BOOL flow_drag,flow_moved;
static const int flow_stops[]={0,330,664,986,1176};
static BOOL flow_area(int x,int y){return x>=44&&x<756&&y>=126&&y<500;}
static void flow_reset(void){flow_scale=1;flow_x=50;flow_y=138;flow_stop=0;flow_drag=flow_moved=FALSE;}
static void flow_fit(void){flow_scale=350.0/1536;flow_x=400-350*flow_scale;flow_y=138;flow_drag=FALSE;}
static void flow_zoom(int x,int y,int delta){double previous=flow_scale;if(!flow_area(x,y))return;flow_scale=max(.20,min(2.0,previous*pow(1.15,delta/120.0)));flow_x=x-(x-flow_x)*flow_scale/previous;flow_y=y-(y-flow_y)*flow_scale/previous;}
static void flow_focus(int delta){flow_stop=max(0,min((int)(sizeof(flow_stops)/sizeof(flow_stops[0]))-1,flow_stop+delta));flow_scale=1;flow_x=50;flow_y=138-flow_stops[flow_stop];}
static int flow_hit(int x,int y){int n;if(y<554||y>=590||x<44||x>=756)return -1;if(x<148)return 0;if(x>=160&&x<274)return 1;if(x>=286&&x<406)return 2;if(x>=418&&x<522)return 3;if(x>=534&&x<638)return 4;n=x>=650?5:-1;return n;}
static void flow_click(int x,int y){switch(flow_hit(x,y)){case 0:PostMessageW(ex07_parent,EX07_TITLE,0,0);break;case 1:PostMessageW(ex07_parent,EX07_BACK,0,0);break;case 2:flow_fit();break;case 3:flow_focus(-1);break;case 4:flow_focus(1);break;case 5:flow_reset();break;}}
static void flow_render(HWND window,HDC dest){
 RECT r,client;XFORM transform;HPEN pen;HGDIOBJ oldPen;size_t i;WCHAR caption[160];
 GetClientRect(window,&client);r=(RECT){0,0,800,600};ex07_box(ex07_dc,r,RGB(19,30,43));
 r=(RECT){52,37,748,80};ex07_text(ex07_dc,L"플로우 차트",r,ex07_head,RGB(244,232,205),DT_SINGLELINE|DT_VCENTER);
 r=(RECT){54,84,748,114};ex07_text(ex07_dc,L"본편 주요 선택지 네 곳과 엔딩 네 종류의 연결",r,ex07_small,RGB(180,196,208),DT_SINGLELINE);
 r=(RECT){44,126,756,500};ex07_box(ex07_dc,r,RGB(12,21,31));SaveDC(ex07_dc);IntersectClipRect(ex07_dc,r.left,r.top,r.right,r.bottom);
 SetGraphicsMode(ex07_dc,GM_ADVANCED);ZeroMemory(&transform,sizeof(transform));transform.eM11=transform.eM22=(FLOAT)flow_scale;transform.eDx=(FLOAT)flow_x;transform.eDy=(FLOAT)flow_y;SetWorldTransform(ex07_dc,&transform);
 pen=CreatePen(PS_SOLID,2,RGB(125,157,174));oldPen=SelectObject(ex07_dc,pen);
 for(i=0;i<sizeof(flow_edges)/sizeof(flow_edges[0]);i++){
  const FLOW_EDGE*e=&flow_edges[i];const FLOW_NODE*a=&flow_nodes[e->from],*b=&flow_nodes[e->to];int ax=a->x+a->width/2,ay=a->y+a->height,bx=b->x+b->width/2,by=b->y,mid=(ay+by)/2;
  MoveToEx(ex07_dc,ax,ay,NULL);LineTo(ex07_dc,ax,mid);LineTo(ex07_dc,bx,mid);LineTo(ex07_dc,bx,by);MoveToEx(ex07_dc,bx-6,by-9,NULL);LineTo(ex07_dc,bx,by);LineTo(ex07_dc,bx+6,by-9);
  if(e->label){r=(RECT){e->x,e->y,e->x+e->width,e->y+26};ex07_box(ex07_dc,r,RGB(12,21,31));ex07_text(ex07_dc,e->label,r,ex07_small,RGB(229,207,151),DT_CENTER|DT_SINGLELINE);}
 }
 for(i=0;i<sizeof(flow_nodes)/sizeof(flow_nodes[0]);i++){
  const FLOW_NODE*n=&flow_nodes[i];COLORREF fill=n->kind==4?RGB(55,88,80):n->kind==3?RGB(75,48,61):n->kind==2?RGB(79,68,43):n->kind==1?RGB(34,68,86):RGB(34,47,63);
  r=(RECT){n->x,n->y,n->x+n->width,n->y+n->height};ex07_box(ex07_dc,r,fill);
  r.left+=8;r.right-=8;r.top+=8;r.bottom=r.top+32;ex07_text(ex07_dc,n->title,r,ex07_font,RGB(244,236,218),DT_CENTER|DT_SINGLELINE);
  r.top=n->y+42;r.bottom=n->y+n->height;ex07_text(ex07_dc,n->detail,r,ex07_small,RGB(194,210,219),DT_CENTER|DT_SINGLELINE);
 }
 SelectObject(ex07_dc,oldPen);DeleteObject(pen);RestoreDC(ex07_dc,-1);
 _snwprintf_s(caption,160,_TRUNCATE,L"%d%% · 휠: 확대·축소 · 드래그: 이동 · 0: 전체 보기",(int)(flow_scale*100));r=(RECT){44,510,756,542};ex07_text(ex07_dc,caption,r,ex07_small,RGB(182,199,211),DT_CENTER|DT_SINGLELINE|DT_VCENTER);
 r=(RECT){44,554,148,590};ex07_button(ex07_dc,r,L"타이틀로",FALSE);r=(RECT){160,554,274,590};ex07_button(ex07_dc,r,L"엑스트라로",FALSE);
 r=(RECT){286,554,406,590};ex07_button(ex07_dc,r,L"전체 보기",FALSE);r=(RECT){418,554,522,590};ex07_button(ex07_dc,r,L"이전 분기",FALSE);
 r=(RECT){534,554,638,590};ex07_button(ex07_dc,r,L"다음 분기",FALSE);r=(RECT){650,554,756,590};ex07_button(ex07_dc,r,L"처음으로",FALSE);
 SetStretchBltMode(dest,HALFTONE);StretchBlt(dest,0,0,client.right,client.bottom,ex07_dc,0,0,800,600,SRCCOPY);
}
