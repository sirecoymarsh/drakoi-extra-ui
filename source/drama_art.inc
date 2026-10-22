/* Three lossless images, shared by the CD list, player and close-up view.
 * Opening the artwork never changes the audio stream or subtitle clock. */
static HBITMAP cdart_bitmap[3];
static int cdart_width[3],cdart_height[3],cdart_view=-1,cdart_lastX,cdart_lastY,cdart_background=1,cdart_menuImage;
static BOOL cdart_tried[3],cdart_drag,cdart_picker,cdart_preferencesLoaded;
static double cdart_scale=1.0,cdart_x,cdart_y;
static const WCHAR*cdart_names[]={L"책자 표지",L"두 주인공",L"교실 표지"};
static void cdart_settingsPath(WCHAR path[MAX_PATH]){_snwprintf_s(path,MAX_PATH,_TRUNCATE,L"%s\\extra07\\player.ini",dir);}
static void cdart_preferences(void){WCHAR path[MAX_PATH];UINT n;if(cdart_preferencesLoaded)return;cdart_preferencesLoaded=TRUE;cdart_settingsPath(path);n=GetPrivateProfileIntW(L"Display",L"Background",1,path);cdart_background=n<3?(int)n:1;n=GetPrivateProfileIntW(L"Display",L"MenuImage",0,path);cdart_menuImage=n<3?(int)n:0;}
static void cdart_selectBackground(int n){WCHAR path[MAX_PATH],value[8];BOOL saved;if(n<0||n>=3)return;cdart_preferencesLoaded=TRUE;cdart_background=n;cdart_picker=FALSE;cdart_settingsPath(path);_snwprintf_s(value,8,_TRUNCATE,L"%d",n);saved=WritePrivateProfileStringW(L"Display",L"Background",value,path);logline("CD BACKGROUND %d saved=%d\r\n",n,saved);}
static void cdart_selectMenuImage(int n){WCHAR path[MAX_PATH],value[8];BOOL saved;if(n<0||n>=3)return;cdart_preferences();cdart_menuImage=n;cdart_settingsPath(path);_snwprintf_s(value,8,_TRUNCATE,L"%d",n);saved=WritePrivateProfileStringW(L"Display",L"MenuImage",value,path);logline("CD MENU IMAGE %d saved=%d\r\n",n,saved);}
static int cdart_pickerHit(int x,int y){int n;if(x<502||x>=746||y<124||y>=238)return -1;n=(y-124)/38;return n<3&&(y-124)%38<34?n:-1;}
static void cdart_pickerRender(HDC dc){int i;RECT r={498,120,750,242};ex07_panel(dc,r,FALSE);for(i=0;i<3;i++){r=(RECT){502,124+i*38,746,158+i*38};ex07_button(dc,r,cdart_names[i],cdart_background==i);}}
static BOOL cdart_load(int n){
 WCHAR path[MAX_PATH];BITMAP b;static const WCHAR*files[]={L"cover.bmp",L"protagonists.bmp",L"classroom.bmp"};
 if(n<0||n>2)return FALSE;
 if(!cdart_tried[n]){cdart_tried[n]=TRUE;_snwprintf_s(path,MAX_PATH,_TRUNCATE,L"%s\\extra07\\%s",dir,files[n]);
  cdart_bitmap[n]=(HBITMAP)LoadImageW(NULL,path,IMAGE_BITMAP,0,0,LR_LOADFROMFILE|LR_CREATEDIBSECTION);
  if(cdart_bitmap[n]&&GetObjectW(cdart_bitmap[n],sizeof(b),&b)){cdart_width[n]=b.bmWidth;cdart_height[n]=b.bmHeight;}
 }
 return cdart_bitmap[n]&&cdart_width[n]>0&&cdart_height[n]>0;
}
static void cdart_blit(HDC dc,int n,int x,int y,int width,int height,BYTE opacity){
 HDC src;HGDIOBJ old;BLENDFUNCTION blend={AC_SRC_OVER,0,255,0};
 if(!cdart_load(n)||width<=0||height<=0)return;src=CreateCompatibleDC(dc);if(!src)return;old=SelectObject(src,cdart_bitmap[n]);
 if(opacity==255){SetStretchBltMode(dc,HALFTONE);SetBrushOrgEx(dc,0,0,NULL);StretchBlt(dc,x,y,width,height,src,0,0,cdart_width[n],cdart_height[n],SRCCOPY);}
 else{blend.SourceConstantAlpha=opacity;AlphaBlend(dc,x,y,width,height,src,0,0,cdart_width[n],cdart_height[n],blend);}
 SelectObject(src,old);DeleteDC(src);
}
static void cdart_inset(HDC dc,RECT area,int n,BYTE opacity){
 int width,height;if(!cdart_load(n))return;width=area.right-area.left;height=MulDiv(width,cdart_height[n],cdart_width[n]);
 if(height>area.bottom-area.top){height=area.bottom-area.top;width=MulDiv(height,cdart_width[n],cdart_height[n]);}
 cdart_blit(dc,n,area.left+(area.right-area.left-width)/2,area.top+(area.bottom-area.top-height)/2,width,height,opacity);
}
static BOOL cdart_area(int x,int y){return x>=44&&x<756&&y>=80&&y<500;}
static void cdart_fit(void){
 if(!cdart_load(cdart_view))return;cdart_scale=min(712.0/cdart_width[cdart_view],420.0/cdart_height[cdart_view]);
 cdart_x=44+(712-cdart_width[cdart_view]*cdart_scale)/2;cdart_y=80+(420-cdart_height[cdart_view]*cdart_scale)/2;
}
static void cdart_open(int n){if(n<0||n>=3)return;cdart_view=n;cdart_drag=FALSE;cdart_picker=FALSE;ex07_hover=-1;cdart_fit();logline("CD ART OPEN %d\r\n",n);}
static void cdart_close(void){cdart_view=-1;cdart_drag=FALSE;cdart_picker=FALSE;ex07_hover=-1;ReleaseCapture();}
static void cdart_cleanup(void){int i;cdart_close();for(i=0;i<3;i++){if(cdart_bitmap[i])DeleteObject(cdart_bitmap[i]);cdart_bitmap[i]=NULL;cdart_tried[i]=FALSE;cdart_width[i]=cdart_height[i]=0;}}
static void cdart_wheel(int x,int y,int delta){
 double before,after,ratio;if(!cdart_area(x,y)||!delta||!cdart_load(cdart_view))return;
 before=cdart_scale;after=max(0.10,min(12.0,before*pow(1.2,(double)delta/WHEEL_DELTA)));ratio=after/before;
 cdart_x=x-(x-cdart_x)*ratio;cdart_y=y-(y-cdart_y)*ratio;cdart_scale=after;
}
static int cdart_hit(int x,int y){int n;if(y<554||y>=590||x<44||x>=756)return -1;n=(x-44)/121;return n<6&&(x-44)%121<107?n:-1;}
static void cdart_click(int x,int y){
 switch(cdart_hit(x,y)){case 0:cdart_close();break;case 1:cdart_fit();break;case 2:cdart_open(0);break;case 3:cdart_open(1);break;case 4:cdart_open(2);break;case 5:ex07_pause();break;}
}
static void cdart_key(WPARAM key){
 if(key==VK_ESCAPE)cdart_close();else if(key=='0'||key==VK_HOME)cdart_fit();else if(key==VK_LEFT)cdart_open((cdart_view+2)%3);else if(key==VK_RIGHT)cdart_open((cdart_view+1)%3);else if(key==VK_SPACE)ex07_pause();
}
static void cdart_render(HWND w,HDC dest){
 RECT r,client;WCHAR caption[160];int i;const WCHAR*buttons[]={L"돌아가기",L"화면 맞춤",L"책자 표지",L"주인공",L"교실 표지",ex07_paused?L"계속 재생":L"일시정지"};
 GetClientRect(w,&client);ex07_skyDraw();
 r=(RECT){44,20,756,66};_snwprintf_s(caption,160,_TRUNCATE,L"드라마 CD · %s",cdart_names[cdart_view]);ex07_logoText(ex07_dc,caption,r,ex07_head);
 r=(RECT){40,76,760,504};ex07_panel(ex07_dc,r,TRUE);r=(RECT){44,80,756,500};SaveDC(ex07_dc);IntersectClipRect(ex07_dc,r.left,r.top,r.right,r.bottom);
 if(cdart_load(cdart_view))cdart_blit(ex07_dc,cdart_view,(int)cdart_x,(int)cdart_y,(int)(cdart_width[cdart_view]*cdart_scale),(int)(cdart_height[cdart_view]*cdart_scale),255);
 else ex07_text(ex07_dc,L"이미지를 읽지 못했습니다.",r,ex07_font,RGB(255,210,190),DT_CENTER|DT_VCENTER|DT_SINGLELINE);RestoreDC(ex07_dc,-1);
 _snwprintf_s(caption,160,_TRUNCATE,L"%d%% · 휠: 확대·축소 · 드래그: 이동 · 0: 화면 맞춤 · Esc: 돌아가기",(int)(cdart_scale*100));
 r=(RECT){44,512,756,540};ex07_text(ex07_dc,caption,r,ex07_small,RGB(60,104,132),DT_CENTER|DT_SINGLELINE|DT_VCENTER);
 for(i=0;i<6;i++){r=(RECT){44+121*i,554,151+121*i,590};ex07_button(ex07_dc,r,buttons[i],(i>=2&&i<=4&&cdart_view==i-2)||(i==5&&ex07_paused));}
 SetStretchBltMode(dest,HALFTONE);StretchBlt(dest,0,0,client.right,client.bottom,ex07_dc,0,0,800,600,SRCCOPY);
}
