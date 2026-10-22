/* Main-story routes verified against installed script.pak; view only.
 * At 100% the whole chart fits the panel: choices carry their two options as
 * chips and the endings sit in the right column. The wheel still zooms about
 * the pointer, dragging pans and 0 returns to the full view. A node opens its
 * scene background for recall; nothing here touches the save or the script. */
typedef struct {int x,y,width,height,kind;LPCWSTR title,detail,scene;} FLOW_NODE;
typedef struct {int from,to,route;LPCWSTR label;} FLOW_EDGE;
static const FLOW_NODE flow_nodes[]={
 {  0,  0,262,50,0,L"이야기 시작",L"용과의 만남",L"059"},
 {  0, 59,262,50,0,L"첫 번째 선택",L"보리수 잎",L"041"},
 {276, 59,260,23,1,L"구해줘야 해……!",L"구출을 선택",L"041"},
 {276, 86,260,23,1,L"내가 뭘 할 수 있겠어",L"구출하지 않음",L"041"},
 {  0,118,262,50,0,L"두 번째 선택",L"데이트",L"067"},
 {276,118,260,23,1,L"어울려 준다",L"데이트를 선택",L"067"},
 {276,145,260,23,1,L"단호하게 돌아간다",L"데이트하지 않음",L"067"},
 {  0,177,262,50,2,L"용살자와 재회",L"구출 선택의 결과로 분기",L"037"},
 {550,177,188,50,3,L"엔딩 1",L"JABBERWOCKY",L"029"},
 {  0,236,262,50,0,L"세 번째 선택",L"위기에서의 결단",L"030"},
 {276,236,260,23,1,L"숨통을 끊는다",L"이야기가 계속된다",L"030"},
 {276,263,260,23,1,L"그건 위험하다",L"",L"030"},
 {550,236,188,50,3,L"엔딩 2",L"HERO LOST",L"027"},
 {  0,295,262,50,2,L"결전 이후",L"두 번째 선택의 결과로 분기",L"026"},
 {  0,354,262,50,0,L"마지막 선택",L"데이트를 했다면",L"045"},
 {550,295,188,50,3,L"엔딩 3",L"MOTE IN EYE",L"061"},
 {276,354,260,23,1,L"올바른 길을 간다",L"",L"045"},
 {276,381,260,23,1,L"틀린 길을 간다",L"",L"045"},
 {550,354,188,23,3,L"엔딩 3",L"MOTE IN EYE",L"061"},
 {550,381,188,23,4,L"엔딩 4",L"LOVE SONG",L"060"}
};
/* route 0: down the story column (a label becomes a tag on the source node),
 * 1: choice to its option chips, 2: option to ending, 3: branch to ending
 * across the option column with the condition written on the line. */
static const FLOW_EDGE flow_edges[]={
 {0,1,0,NULL},{1,4,0,NULL},{4,7,0,NULL},{7,9,0,L"구출함"},{9,13,0,NULL},{13,14,0,L"데이트함"},
 {1,2,1,NULL},{1,3,1,NULL},{4,5,1,NULL},{4,6,1,NULL},{9,10,1,NULL},{9,11,1,NULL},{14,16,1,NULL},{14,17,1,NULL},
 {11,12,2,NULL},{16,18,2,NULL},{17,19,2,NULL},
 {7,8,3,L"구출하지 않음"},{13,15,3,L"데이트하지 않음"}
};
#define FLOW_COUNT ((int)(sizeof(flow_nodes)/sizeof(flow_nodes[0])))
#define FLOW_EDGES ((int)(sizeof(flow_edges)/sizeof(flow_edges[0])))
/* Graph extent and the panel interior it is fitted into at 100%. */
#define FLOW_W 738
#define FLOW_H 404
#define FLOW_LEFT 28
#define FLOW_TOP 98
#define FLOW_RIGHT 772
#define FLOW_BOTTOM 514
static double flow_scale=1.0,flow_x=31,flow_y=104,flow_sceneScale=1.0,flow_sceneX,flow_sceneY;
static int flow_lastX,flow_lastY,flow_pressX,flow_pressY,flow_pick=-1,flow_view=-1,flow_sceneW,flow_sceneH,flow_previewNode=-1,flow_previewW,flow_previewH;
static int flow_thumbW[FLOW_COUNT],flow_thumbH[FLOW_COUNT];
static BOOL flow_drag,flow_moved,flow_thumbTried[FLOW_COUNT];
static HBITMAP flow_scene,flow_preview,flow_thumb[FLOW_COUNT];
static BOOL flow_area(int x,int y){return x>=FLOW_LEFT&&x<FLOW_RIGHT&&y>=FLOW_TOP&&y<FLOW_BOTTOM;}
/* Smaller than the panel: centred. Larger: the graph edge may not leave the panel edge. */
static void flow_clamp(void){
 double w=FLOW_W*flow_scale,h=FLOW_H*flow_scale;
 if(w<=FLOW_RIGHT-FLOW_LEFT-6)flow_x=FLOW_LEFT+(FLOW_RIGHT-FLOW_LEFT-w)/2;else flow_x=max(FLOW_RIGHT-3-w,min(FLOW_LEFT+3,flow_x));
 if(h<=FLOW_BOTTOM-FLOW_TOP-6)flow_y=FLOW_TOP+(FLOW_BOTTOM-FLOW_TOP-h)/2;else flow_y=max(FLOW_BOTTOM-3-h,min(FLOW_TOP+3,flow_y));
}
static void flow_sceneFit(void){
 double s;if(!flow_scene||flow_sceneW<=0||flow_sceneH<=0)return;
 s=min(736.0/flow_sceneW,408.0/flow_sceneH);flow_sceneScale=s;
 flow_sceneX=FLOW_LEFT+(FLOW_RIGHT-FLOW_LEFT-flow_sceneW*s)/2;flow_sceneY=FLOW_TOP+(FLOW_BOTTOM-FLOW_TOP-flow_sceneH*s)/2;
}
static void flow_closeScene(void){if(flow_scene)DeleteObject(flow_scene);flow_scene=NULL;flow_sceneW=flow_sceneH=0;flow_view=-1;flow_drag=FALSE;}
static void flow_cleanup(void){
 int i;flow_closeScene();
 for(i=0;i<FLOW_COUNT;i++){if(flow_thumb[i])DeleteObject(flow_thumb[i]);flow_thumb[i]=NULL;flow_thumbTried[i]=FALSE;flow_thumbW[i]=flow_thumbH[i]=0;}
 if(flow_preview)DeleteObject(flow_preview);flow_preview=NULL;flow_previewNode=-1;flow_previewW=flow_previewH=0;flow_pick=-1;
}
static void flow_reset(void){flow_closeScene();flow_pick=-1;flow_scale=1;flow_clamp();flow_drag=flow_moved=FALSE;}
static void flow_fit(void){if(flow_view>=0){flow_sceneFit();return;}flow_scale=1;flow_clamp();flow_drag=FALSE;}
static void flow_zoom(int x,int y,int delta){
 double previous,next;
 if(!delta||!flow_area(x,y))return;
 if(flow_view>=0){
  if(!flow_scene)return;previous=flow_sceneScale;next=max(.10,min(8.0,previous*pow(1.15,delta/120.0)));
  flow_sceneX=x-(x-flow_sceneX)*next/previous;flow_sceneY=y-(y-flow_sceneY)*next/previous;flow_sceneScale=next;return;
 }
 previous=flow_scale;next=max(1.0,min(2.5,previous*pow(1.15,delta/120.0)));
 flow_x=x-(x-flow_x)*next/previous;flow_y=y-(y-flow_y)*next/previous;flow_scale=next;flow_clamp();
}
static void flow_pan(int dx,int dy){if(flow_view>=0){flow_sceneX+=dx;flow_sceneY+=dy;return;}flow_x+=dx;flow_y+=dy;flow_clamp();}
static HBITMAP flow_thumbnail(int n){
 WCHAR file[32];
 if(n<0||n>=FLOW_COUNT)return NULL;
 if(!flow_thumbTried[n]){flow_thumbTried[n]=TRUE;_snwprintf_s(file,32,_TRUNCATE,L"%s-thumb.png",flow_nodes[n].scene);flow_thumb[n]=gal_load(file,&flow_thumbW[n],&flow_thumbH[n]);}
 return flow_thumb[n];
}
/* Small pictures keep their aspect: the source is centre-cropped to fill the slot. */
static void flow_cover(HDC dc,HBITMAP bm,int srcW,int srcH,int x,int y,int w,int h){
 HDC src;HGDIOBJ old;POINT origin;int cropX=0,cropY=0,cropW=srcW,cropH=srcH;
 if(!bm||srcW<=0||srcH<=0||w<=0||h<=0)return;
 if((LONGLONG)srcW*h>(LONGLONG)srcH*w){cropW=MulDiv(srcH,w,h);cropX=(srcW-cropW)/2;}else{cropH=MulDiv(srcW,h,w);cropY=(srcH-cropH)/2;}
 src=CreateCompatibleDC(dc);if(!src)return;old=SelectObject(src,bm);
 SetStretchBltMode(dc,HALFTONE);SetBrushOrgEx(dc,0,0,&origin);StretchBlt(dc,x,y,w,h,src,cropX,cropY,cropW,cropH,SRCCOPY);SetBrushOrgEx(dc,origin.x,origin.y,NULL);
 SelectObject(src,old);DeleteDC(src);
}
static void flow_previewLoad(int n){
 WCHAR file[32];
 if(n==flow_previewNode)return;
 if(flow_preview)DeleteObject(flow_preview);flow_preview=NULL;flow_previewW=flow_previewH=0;flow_previewNode=n;
 if(n<0||n>=FLOW_COUNT)return;
 _snwprintf_s(file,32,_TRUNCATE,L"%s.png",flow_nodes[n].scene);flow_preview=gal_load(file,&flow_previewW,&flow_previewH);
}
static void flow_openScene(int n){
 WCHAR file[32];
 if(n<0||n>=FLOW_COUNT)return;
 if(flow_scene)DeleteObject(flow_scene);flow_scene=NULL;flow_sceneW=flow_sceneH=0;
 flow_view=n;flow_pick=n;ex07_hover=-1;flow_drag=FALSE;
 _snwprintf_s(file,32,_TRUNCATE,L"%s.png",flow_nodes[n].scene);
 flow_scene=gal_load(file,&flow_sceneW,&flow_sceneH);flow_sceneFit();
 logline("FLOW SCENE %d file=%ls ok=%d\r\n",n,file,flow_scene!=NULL);
}
/* Bring a node chosen from the keyboard into view when zoomed in. */
static void flow_reveal(int n){
 const FLOW_NODE*node=&flow_nodes[n];double left=flow_x+node->x*flow_scale,top=flow_y+node->y*flow_scale;
 if(left>=FLOW_LEFT&&left+node->width*flow_scale<=FLOW_RIGHT&&top>=FLOW_TOP&&top+node->height*flow_scale<=FLOW_BOTTOM)return;
 flow_x=(FLOW_LEFT+FLOW_RIGHT)/2.0-(node->x+node->width/2.0)*flow_scale;flow_y=(FLOW_TOP+FLOW_BOTTOM)/2.0-(node->y+node->height/2.0)*flow_scale;flow_clamp();
}
/* Previous/next walk the scene nodes; option chips share their choice's scene. */
static void flow_step(int delta){
 int n=flow_pick<0?(delta>0?-1:FLOW_COUNT):flow_pick;
 do{n+=delta;}while(n>=0&&n<FLOW_COUNT&&flow_nodes[n].kind==1);
 if(n<0||n>=FLOW_COUNT)return;
 if(flow_view>=0)flow_openScene(n);else{flow_pick=n;flow_reveal(n);}
}
static int flow_node(int x,int y){
 int i;double graphX,graphY;
 if(!flow_area(x,y))return -1;
 graphX=(x-flow_x)/flow_scale;graphY=(y-flow_y)/flow_scale;
 for(i=0;i<FLOW_COUNT;i++){const FLOW_NODE*n=&flow_nodes[i];if(graphX>=n->x&&graphX<n->x+n->width&&graphY>=n->y&&graphY<n->y+n->height)return i;}
 return -1;
}
static int flow_hit(int x,int y){
 int n;
 if(y>=554&&y<590&&x>=44&&x<756){
  if(x<148)return 0;if(x>=160&&x<274)return 1;if(x>=286&&x<406)return 2;
  if(x>=418&&x<522)return 3;if(x>=534&&x<638)return 4;return x>=650?5:-1;}
 n=flow_view>=0?-1:flow_node(x,y);
 return n>=0?100+n:-1;
}
static void flow_click(int x,int y){
 int hit=flow_hit(x,y);
 if(hit>=100){flow_openScene(hit-100);return;}
 switch(hit){
  case 0:PostMessageW(ex07_parent,EX07_TITLE,0,0);break;
  case 1:PostMessageW(ex07_parent,EX07_BACK,0,0);break;
  case 2:if(flow_view>=0)flow_closeScene();else flow_fit();break;
  case 3:flow_step(-1);break;
  case 4:flow_step(1);break;
  case 5:if(flow_view>=0)flow_fit();else flow_openScene(flow_pick<0?0:flow_pick);break;
  default:break;}
}
/* Left edge of the text beside a node's thumbnail; the review harness measures against it. */
static int flow_textLeft(const FLOW_NODE*n){return n->x+6+(n->kind>=3?48:64)+8;}
static int flow_previewTarget(void){return ex07_hover>=100&&ex07_hover<100+FLOW_COUNT?ex07_hover-100:flow_pick;}
static void flow_backdrop(LPCWSTR line){
 RECT r;
 ex07_skyDraw();
 r=(RECT){0,14,800,58};ex07_logoText(ex07_dc,L"플로우 차트",r,ex07_head);
 r=(RECT){0,62,800,88};ex07_text(ex07_dc,line,r,ex07_note,RGB(60,104,132),DT_CENTER|DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);
}
static void flow_footer(LPCWSTR text){RECT r={44,520,756,548};ex07_text(ex07_dc,text,r,ex07_note,RGB(60,104,132),DT_CENTER|DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);}
static void flow_buttons(LPCWSTR third,LPCWSTR sixth){
 RECT r;
 r=(RECT){44,554,148,590};ex07_button(ex07_dc,r,L"타이틀로",FALSE);
 r=(RECT){160,554,274,590};ex07_button(ex07_dc,r,L"엑스트라로",FALSE);
 r=(RECT){286,554,406,590};ex07_button(ex07_dc,r,third,FALSE);
 r=(RECT){418,554,522,590};ex07_button(ex07_dc,r,L"이전 장면",FALSE);
 r=(RECT){534,554,638,590};ex07_button(ex07_dc,r,L"다음 장면",FALSE);
 r=(RECT){650,554,756,590};ex07_button(ex07_dc,r,sixth,FALSE);
}
/* The drawing below runs in graph coordinates under the zoom transform. */
static void flow_legend(void){
 static const LPCWSTR names[4]={L"이야기",L"선택지",L"결과 분기",L"엔딩"};
 static const COLORREF rims[4]={RGB(60,190,236),RGB(150,214,236),RGB(232,178,76),RGB(226,128,154)};
 RECT r;int i;
 for(i=0;i<4;i++){int left=290+(i%2)*120,top=6+(i/2)*22;r=(RECT){left,top+3,left+18,top+15};ex07_plate(ex07_dc,r,4,rims[i],rims[i],RGB(255,255,255),RGB(232,242,246));r=(RECT){left+24,top,left+118,top+18};ex07_text(ex07_dc,names[i],r,ex07_tiny,RGB(60,104,132),DT_SINGLELINE|DT_VCENTER);}
}
static void flow_arrow(int x,int y,int dx,int dy){MoveToEx(ex07_dc,x-4*dx-3*dy,y-4*dy-3*dx,NULL);LineTo(ex07_dc,x,y);LineTo(ex07_dc,x-4*dx+3*dy,y-4*dy+3*dx);}
static void flow_edge(const FLOW_EDGE*e){
 const FLOW_NODE*a=&flow_nodes[e->from],*b=&flow_nodes[e->to];int ax,ay,by,fork;
 if(e->route==0){ax=a->x+a->width/2;ay=a->y+a->height;by=b->y;MoveToEx(ex07_dc,ax,ay,NULL);LineTo(ex07_dc,ax,by);flow_arrow(ax,by,0,1);return;}
 ax=a->x+a->width;ay=a->y+a->height/2;
 if(e->route==1){fork=(ax+b->x)/2;by=b->y+b->height/2;MoveToEx(ex07_dc,ax,ay,NULL);LineTo(ex07_dc,fork,ay);LineTo(ex07_dc,fork,by);LineTo(ex07_dc,b->x,by);flow_arrow(b->x,by,1,0);return;}
 MoveToEx(ex07_dc,ax,ay,NULL);LineTo(ex07_dc,b->x,ay);flow_arrow(b->x,ay,1,0);
}
static void flow_edgeLabel(const FLOW_EDGE*e){
 const FLOW_NODE*a=&flow_nodes[e->from];RECT r;SIZE size;int middle,line;WCHAR tag[64];
 if(!e->label)return;
 if(e->route==0){_snwprintf_s(tag,64,_TRUNCATE,L"%s ↓",e->label);r=(RECT){a->x+a->width-100,a->y+4,a->x+a->width-8,a->y+24};ex07_text(ex07_dc,tag,r,ex07_tiny,RGB(24,118,150),DT_SINGLELINE|DT_VCENTER|DT_RIGHT);return;}
 middle=(a->x+a->width+flow_nodes[e->to].x)/2;line=a->y+a->height/2;
 SelectObject(ex07_dc,ex07_tiny);if(!GetTextExtentPoint32W(ex07_dc,e->label,lstrlenW(e->label),&size))size.cx=100;
 r=(RECT){middle-size.cx/2-8,line-10,middle+size.cx/2+8,line+10};ex07_plate(ex07_dc,r,8,RGB(238,160,178),RGB(176,70,96),RGB(255,250,251),RGB(250,236,240));
 ex07_text(ex07_dc,e->label,r,ex07_tiny,RGB(150,60,90),DT_CENTER|DT_VCENTER|DT_SINGLELINE);
}
static void flow_nodeRender(int i){
 static const COLORREF colors[5][8]={
  {RGB(96,210,242),RGB(26,110,144),RGB(240,248,251),RGB(214,232,240),RGB(176,240,254),RGB(54,160,196),RGB(250,254,255),RGB(224,244,250)},
  {RGB(170,222,240),RGB(96,164,194),RGB(255,255,255),RGB(238,246,250),RGB(190,240,254),RGB(60,166,200),RGB(255,255,255),RGB(226,246,252)},
  {RGB(244,204,120),RGB(186,132,40),RGB(254,249,236),RGB(240,228,200),RGB(255,226,160),RGB(214,160,60),RGB(255,252,242),RGB(248,236,210)},
  {RGB(238,160,178),RGB(176,70,96),RGB(252,240,244),RGB(238,216,224),RGB(255,214,224),RGB(214,96,124),RGB(255,246,248),RGB(244,224,232)},
  {RGB(130,212,160),RGB(40,140,96),RGB(240,251,244),RGB(212,238,222),RGB(176,240,200),RGB(60,170,120),RGB(248,255,250),RGB(222,246,232)}};
 const FLOW_NODE*n=&flow_nodes[i];const COLORREF*c=colors[n->kind];RECT r={n->x,n->y,n->x+n->width,n->y+n->height};SIZE size;WCHAR line[96];int left;
 if((ex07_hover==100+i)||(flow_pick==i))c+=4;
 ex07_plate(ex07_dc,r,n->kind==1?8:10,c[0],c[1],c[2],c[3]);
 if(n->kind==1){
  r=(RECT){n->x+10,n->y,n->x+n->width-10,n->y+n->height};ex07_text(ex07_dc,n->title,r,ex07_note,RGB(22,46,62),DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);
  SelectObject(ex07_dc,ex07_note);
  if(n->detail[0]&&GetTextExtentPoint32W(ex07_dc,n->title,lstrlenW(n->title),&size)){r.left=min(r.right,r.left+size.cx+12);ex07_text(ex07_dc,n->detail,r,ex07_tiny,RGB(96,120,138),DT_SINGLELINE|DT_VCENTER|DT_RIGHT|DT_END_ELLIPSIS);}
  return;
 }
 if(n->height<40){_snwprintf_s(line,96,_TRUNCATE,L"%s · %s",n->title,n->detail);r=(RECT){n->x+10,n->y,n->x+n->width-10,n->y+n->height};ex07_text(ex07_dc,line,r,ex07_note,RGB(22,46,62),DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);return;}
 left=flow_textLeft(n);
 r=(RECT){n->x+6,n->y+5,left-8,n->y+45};ex07_box(ex07_dc,r,RGB(196,214,224));
 r=(RECT){left,n->y+3,n->x+n->width-8,n->y+25};ex07_text(ex07_dc,n->title,r,ex07_small,RGB(22,46,62),DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);
 r=(RECT){left,n->y+25,n->x+n->width-8,n->y+45};ex07_text(ex07_dc,n->detail,r,n->kind>=3?ex07_note:ex07_tiny,RGB(58,96,122),DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS);
}
/* The empty corner above the endings shows the pointed or chosen node's scene. */
static void flow_previewFrame(void){
 RECT r={550,0,738,143};int n=flow_previewTarget();
 ex07_plate(ex07_dc,r,10,RGB(150,232,250),RGB(40,140,176),n>=0?RGB(28,46,60):RGB(244,250,252),n>=0?RGB(14,24,34):RGB(226,240,246));
 if(n<0){r=(RECT){562,50,726,94};ex07_text(ex07_dc,L"노드에 마우스를 올리면\n장면이 여기에 보입니다",r,ex07_tiny,RGB(96,140,166),DT_CENTER|DT_WORDBREAK);return;}
 r=(RECT){550,146,738,168};ex07_text(ex07_dc,flow_nodes[n].title,r,ex07_note,RGB(22,46,62),DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
}
/* Pictures are drawn after the transform is reset, at the transformed positions. */
static int flow_device(double origin,int graph){return (int)(origin+graph*flow_scale+.5);}
static void flow_nodeThumb(int i){
 const FLOW_NODE*n=&flow_nodes[i];HBITMAP bm;int left;
 if(n->kind==1||n->height<40)return;bm=flow_thumbnail(i);if(!bm)return;left=flow_textLeft(n)-8;
 flow_cover(ex07_dc,bm,flow_thumbW[i],flow_thumbH[i],flow_device(flow_x,n->x+6),flow_device(flow_y,n->y+5),flow_device(flow_x,left)-flow_device(flow_x,n->x+6),flow_device(flow_y,n->y+45)-flow_device(flow_y,n->y+5));
}
static void flow_previewImage(void){
 int n=flow_previewTarget();
 if(n<0)return;flow_previewLoad(n);
 if(flow_preview)flow_cover(ex07_dc,flow_preview,flow_previewW,flow_previewH,flow_device(flow_x,554),flow_device(flow_y,4),flow_device(flow_x,734)-flow_device(flow_x,554),flow_device(flow_y,139)-flow_device(flow_y,4));
}
static void flow_sceneRender(HWND window,HDC dest){
 RECT r,client;const FLOW_NODE*n=&flow_nodes[flow_view];WCHAR line[200];
 GetClientRect(window,&client);
 if(n->detail[0])_snwprintf_s(line,200,_TRUNCATE,L"%s · %s",n->title,n->detail);else _snwprintf_s(line,200,_TRUNCATE,L"%s",n->title);
 flow_backdrop(line);
 r=(RECT){24,94,776,518};ex07_panel(ex07_dc,r,TRUE);
 SaveDC(ex07_dc);IntersectClipRect(ex07_dc,FLOW_LEFT,FLOW_TOP,FLOW_RIGHT,FLOW_BOTTOM);
 if(flow_scene)gal_blit(ex07_dc,flow_scene,(int)flow_sceneX,(int)flow_sceneY,(int)(flow_sceneW*flow_sceneScale),(int)(flow_sceneH*flow_sceneScale));
 else{r=(RECT){FLOW_LEFT,FLOW_TOP,FLOW_RIGHT,FLOW_BOTTOM};ex07_text(ex07_dc,L"장면 그림을 읽지 못했습니다.",r,ex07_font,RGB(255,210,190),DT_CENTER|DT_VCENTER|DT_SINGLELINE);}
 RestoreDC(ex07_dc,-1);
 _snwprintf_s(line,200,_TRUNCATE,L"%d%% · 휠: 확대·축소 · 드래그: 이동 · 0: 맞춤 · Esc: 차트로",(int)(flow_sceneScale*100+.5));flow_footer(line);
 flow_buttons(L"차트로",L"화면 맞춤");
 SetStretchBltMode(dest,HALFTONE);StretchBlt(dest,0,0,client.right,client.bottom,ex07_dc,0,0,800,600,SRCCOPY);
}
static void flow_render(HWND window,HDC dest){
 RECT r,client;XFORM transform;HPEN pen;HGDIOBJ oldPen;int i;WCHAR line[200];
 if(flow_view>=0){flow_sceneRender(window,dest);return;}
 GetClientRect(window,&client);flow_backdrop(L"본편 주요 선택지 네 곳과 엔딩 네 종류의 연결");
 r=(RECT){24,94,776,518};ex07_panel(ex07_dc,r,FALSE);
 SaveDC(ex07_dc);IntersectClipRect(ex07_dc,FLOW_LEFT,FLOW_TOP,FLOW_RIGHT,FLOW_BOTTOM);
 SetGraphicsMode(ex07_dc,GM_ADVANCED);ZeroMemory(&transform,sizeof(transform));transform.eM11=transform.eM22=(FLOAT)flow_scale;transform.eDx=(FLOAT)flow_x;transform.eDy=(FLOAT)flow_y;SetWorldTransform(ex07_dc,&transform);
 flow_legend();
 pen=CreatePen(PS_SOLID,2,RGB(72,150,182));oldPen=SelectObject(ex07_dc,pen);
 for(i=0;i<FLOW_EDGES;i++)flow_edge(&flow_edges[i]);
 SelectObject(ex07_dc,oldPen);DeleteObject(pen);
 for(i=0;i<FLOW_COUNT;i++)flow_nodeRender(i);
 for(i=0;i<FLOW_EDGES;i++)flow_edgeLabel(&flow_edges[i]);
 flow_previewFrame();
 ModifyWorldTransform(ex07_dc,NULL,MWT_IDENTITY);
 for(i=0;i<FLOW_COUNT;i++)flow_nodeThumb(i);
 flow_previewImage();
 RestoreDC(ex07_dc,-1);
 _snwprintf_s(line,200,_TRUNCATE,L"%d%% · 휠: 확대·축소 · 드래그: 이동 · 0: 전체 보기 · 클릭: 장면 보기",(int)(flow_scale*100+.5));flow_footer(line);
 flow_buttons(L"전체 보기",L"장면 보기");
 SetStretchBltMode(dest,HALFTONE);StretchBlt(dest,0,0,client.right,client.bottom,ex07_dc,0,0,800,600,SRCCOPY);
}
