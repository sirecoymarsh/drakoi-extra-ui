/* Native gallery: source PNGs loaded on demand through Windows Imaging Component. */
#include <objbase.h>
#include <wincodec.h>
#include <math.h>
typedef struct {int category;LPCWSTR file,label;} GAL_ITEM;
#include "gallery_data.h"
static const WCHAR*gal_categories[]={L"일러스트",L"배경",L"스탠딩",L"컷인"};
static int gal_filter,gal_page,gal_selection=-1,gal_ids[GAL_COUNT],gal_count,gal_w,gal_h,gal_back;
static HBITMAP gal_full,gal_thumbs[12];
static BOOL gal_drag,gal_moved;
static int gal_lastX,gal_lastY;
static double gal_scale=1.0,gal_x,gal_y;
static HBITMAP gal_load(LPCWSTR file,int*w,int*h){
 WCHAR path[MAX_PATH];IWICImagingFactory*factory=NULL;IWICBitmapDecoder*decoder=NULL;IWICBitmapFrameDecode*frame=NULL;IWICFormatConverter*converter=NULL;
 HRESULT init,hr;UINT width=0,height=0;HBITMAP bm=NULL;BITMAPINFO bi={0};void*pixels=NULL;
 _snwprintf_s(path,MAX_PATH,_TRUNCATE,L"%s\\extra-gallery\\%s",dir,file);
 init=CoInitializeEx(NULL,COINIT_APARTMENTTHREADED);if(FAILED(init)&&init!=RPC_E_CHANGED_MODE)return NULL;
 hr=CoCreateInstance(&CLSID_WICImagingFactory,NULL,CLSCTX_INPROC_SERVER,&IID_IWICImagingFactory,(void**)&factory);
 if(FAILED(hr))goto done;
 hr=IWICImagingFactory_CreateDecoderFromFilename(factory,path,NULL,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&decoder);if(FAILED(hr))goto done;
 hr=IWICBitmapDecoder_GetFrame(decoder,0,&frame);if(FAILED(hr))goto done;
 hr=IWICBitmapFrameDecode_GetSize(frame,&width,&height);if(FAILED(hr)||!width||!height||width>8192||height>8192||width*height>32000000)goto done;
 hr=IWICImagingFactory_CreateFormatConverter(factory,&converter);if(FAILED(hr))goto done;
 hr=IWICFormatConverter_Initialize(converter,(IWICBitmapSource*)frame,&GUID_WICPixelFormat32bppPBGRA,WICBitmapDitherTypeNone,NULL,0,WICBitmapPaletteTypeCustom);if(FAILED(hr))goto done;
 bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=(LONG)width;bi.bmiHeader.biHeight=-(LONG)height;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;
 bm=CreateDIBSection(NULL,&bi,DIB_RGB_COLORS,&pixels,NULL,0);if(!bm)goto done;
 hr=IWICFormatConverter_CopyPixels(converter,NULL,width*4,width*height*4,pixels);if(FAILED(hr)){DeleteObject(bm);bm=NULL;goto done;}*w=(int)width;*h=(int)height;
done:if(converter)IWICFormatConverter_Release(converter);if(frame)IWICBitmapFrameDecode_Release(frame);if(decoder)IWICBitmapDecoder_Release(decoder);if(factory)IWICImagingFactory_Release(factory);if(SUCCEEDED(init))CoUninitialize();return bm;
}
static void gal_clearThumbs(void){int i;for(i=0;i<12;i++)if(gal_thumbs[i]){DeleteObject(gal_thumbs[i]);gal_thumbs[i]=NULL;}}
static void gal_cleanup(void){gal_clearThumbs();if(gal_full){DeleteObject(gal_full);gal_full=NULL;}gal_drag=FALSE;}
static void gal_category(int cat){int i;gal_cleanup();gal_filter=cat;gal_page=0;gal_selection=-1;gal_count=0;for(i=0;i<GAL_COUNT;i++)if(gal_items[i].category==cat)gal_ids[gal_count++]=i;}
static void gal_open(void){gal_back=0;gal_category(0);}
static void gal_fit(void){if(!gal_full)return;gal_scale=min(712.0/gal_w,390.0/gal_h);gal_x=44+(712-gal_w*gal_scale)/2;gal_y=114+(390-gal_h*gal_scale)/2;}
static void gal_select(int pos){const GAL_ITEM*item;if(pos<0||pos>=gal_count)return;gal_selection=pos;gal_page=pos/12;if(gal_full)DeleteObject(gal_full);gal_full=NULL;gal_w=gal_h=0;item=&gal_items[gal_ids[pos]];gal_full=gal_load(item->file,&gal_w,&gal_h);gal_fit();logline("GALLERY LOAD id=%d size=%d,%d ok=%d\r\n",gal_ids[pos],gal_w,gal_h,gal_full!=NULL);}
static void gal_change(int delta){if(gal_selection>=0)gal_select(max(0,min(gal_count-1,gal_selection+delta)));else{int page=max(0,min(max(0,(gal_count-1)/12),gal_page+delta));if(page!=gal_page){gal_page=page;gal_clearThumbs();}}}
static BOOL gal_area(int x,int y){return x>=44&&x<756&&y>=114&&y<504;}
static int gal_hit(int x,int y){int col,row,n;if(y>=74&&y<104&&x>=44&&x<756)return 1000+(x-44)/178;if(y>=554&&y<590){if(x>=44&&x<148)return 2000;if(x>=160&&x<274)return 2001;if(x>=286&&x<390)return 2002;if(x>=402&&x<506)return 2003;if(x>=518&&x<630)return 2004;if(x>=642&&x<756)return 2005;}if(gal_selection<0&&x>=44&&x<756&&y>=114&&y<492){col=(x-44)/178;row=(y-114)/126;n=gal_page*12+row*4+col;if(n<gal_count)return n;}return -1;}
static void gal_click(int x,int y){int hit=gal_hit(x,y);if(hit>=1000&&hit<1004)gal_category(hit-1000);else if(hit==2000)PostMessageW(ex07_parent,EX07_TITLE,0,0);else if(hit==2001)PostMessageW(ex07_parent,EX07_BACK,0,0);else if(hit==2002){gal_selection=-1;gal_clearThumbs();}else if(hit==2003)gal_fit();else if(hit==2004)gal_change(-1);else if(hit==2005)gal_change(1);else if(hit>=0&&hit<GAL_COUNT)gal_select(hit);}
static void gal_wheel(int x,int y,int delta){double before,after,ratio;if(!gal_area(x,y)||!delta)return;if(gal_selection<0){gal_change(delta>0?-1:1);return;}if(!gal_full)return;before=gal_scale;after=max(0.10,min(12.0,before*pow(1.2,(double)delta/WHEEL_DELTA)));ratio=after/before;gal_x=x-(x-gal_x)*ratio;gal_y=y-(y-gal_y)*ratio;gal_scale=after;}
static void gal_key(WPARAM key){if(key==VK_ESCAPE){if(gal_selection>=0){gal_selection=-1;gal_clearThumbs();}else PostMessageW(ex07_parent,EX07_BACK,0,0);}else if(key==VK_LEFT||key==VK_PRIOR)gal_change(-1);else if(key==VK_RIGHT||key==VK_NEXT)gal_change(1);else if(key==VK_HOME||key=='0')gal_fit();else if(key=='B')gal_back=(gal_back+1)%3;}
static void gal_blit(HDC dc,HBITMAP bm,int x,int y,int width,int height){HDC src;HGDIOBJ old;BITMAP b;BLENDFUNCTION blend={AC_SRC_OVER,0,255,AC_SRC_ALPHA};if(!bm||width<=0||height<=0)return;if(!GetObjectW(bm,sizeof(b),&b))return;src=CreateCompatibleDC(dc);old=SelectObject(src,bm);AlphaBlend(dc,x,y,width,height,src,0,0,b.bmWidth,b.bmHeight,blend);SelectObject(src,old);DeleteDC(src);}
static void gal_background(HDC dc,RECT area){int x,y;COLORREF base=gal_back==1?RGB(240,240,240):RGB(25,33,42);ex07_box(dc,area,base);if(gal_back==0)for(y=area.top;y<area.bottom;y+=20)for(x=area.left;x<area.right;x+=20)if(((x-area.left)/20+(y-area.top)/20)%2){RECT r={x,y,min(x+20,area.right),min(y+20,area.bottom)};ex07_box(dc,r,RGB(36,45,55));}}
/* Wrap visible Korean labels at word/separator boundaries only. */
static void gal_labelLines(HDC dc,LPCWSTR text,WCHAR out[256]){
 int n=lstrlenW(text),i,best=0x7fffffff;SIZE whole;WCHAR left[256],right[256];
 wcsncpy_s(out,256,text,_TRUNCATE);SelectObject(dc,ex07_small);
 if(!GetTextExtentPoint32W(dc,text,n,&whole)||whole.cx<=162)return;
 for(i=1;i<n-1;i++){int a=i,b=i,score;SIZE l,r;if(text[i]!=L' ')continue;
  while(a>0&&(text[a-1]==L' '||text[a-1]==L'·'))a--;
  while(b<n&&(text[b]==L' '||text[b]==L'·'))b++;
  if(!a||b==n)continue;wcsncpy_s(left,256,text,a);wcsncpy_s(right,256,text+b,_TRUNCATE);
  if(!GetTextExtentPoint32W(dc,left,a,&l)||!GetTextExtentPoint32W(dc,right,n-b,&r)||l.cx>162||r.cx>162)continue;
  score=max(l.cx,r.cx);if(score<best){best=score;_snwprintf_s(out,256,_TRUNCATE,L"%s\n%s",left,right);}
 }
}
static void gal_render(HWND w,HDC dest){RECT client,r;WCHAR label[256];int i;GetClientRect(w,&client);r=(RECT){0,0,800,600};ex07_box(ex07_dc,r,RGB(19,30,43));r=(RECT){44,24,756,66};ex07_text(ex07_dc,L"추가 CG 감상",r,ex07_head,RGB(244,232,205),DT_SINGLELINE|DT_VCENTER);
 for(i=0;i<4;i++){int k,n=0;for(k=0;k<GAL_COUNT;k++)if(gal_items[k].category==i)n++;_snwprintf_s(label,256,_TRUNCATE,L"%s · %d",gal_categories[i],n);r=(RECT){44+i*178,74,214+i*178,104};ex07_button(ex07_dc,r,label,gal_filter==i);}
 if(gal_selection<0){
  for(i=0;i<12;i++){int pos=gal_page*12+i,id,tw=0,th=0,x=44+(i%4)*178,y=114+(i/4)*126;WCHAR file[64];if(pos>=gal_count)break;id=gal_ids[pos];r=(RECT){x,y,x+170,y+120};ex07_box(ex07_dc,r,ex07_hover==pos?RGB(49,73,88):RGB(29,46,59));if(!gal_thumbs[i]){_snwprintf_s(file,64,_TRUNCATE,L"%03d-thumb.png",id);gal_thumbs[i]=gal_load(file,&tw,&th);}if(gal_thumbs[i]){BITMAP b;int dw,dh;GetObjectW(gal_thumbs[i],sizeof(b),&b);dh=min(62,b.bmHeight);dw=MulDiv(b.bmWidth,dh,b.bmHeight);gal_blit(ex07_dc,gal_thumbs[i],x+2+(166-dw)/2,y+3+(62-dh)/2,dw,dh);}r=(RECT){x+4,y+68,x+166,y+120};gal_labelLines(ex07_dc,gal_items[id].label,label);ex07_text(ex07_dc,label,r,ex07_small,RGB(222,232,236),DT_CENTER|DT_WORDBREAK|DT_EDITCONTROL);}
  _snwprintf_s(label,256,_TRUNCATE,L"%d / %d 페이지 · 그림을 눌러 크게 보기",gal_page+1,max(1,(gal_count+11)/12));
 }else{
  r=(RECT){44,114,756,504};gal_background(ex07_dc,r);SaveDC(ex07_dc);IntersectClipRect(ex07_dc,r.left,r.top,r.right,r.bottom);
  if(gal_full)gal_blit(ex07_dc,gal_full,(int)gal_x,(int)gal_y,(int)(gal_w*gal_scale),(int)(gal_h*gal_scale));else ex07_text(ex07_dc,L"이미지를 읽지 못했습니다.",r,ex07_font,RGB(255,210,190),DT_CENTER|DT_VCENTER|DT_SINGLELINE);RestoreDC(ex07_dc,-1);
  _snwprintf_s(label,256,_TRUNCATE,L"%d / %d · %s · %d%%",gal_selection+1,gal_count,gal_items[gal_ids[gal_selection]].label,(int)(gal_scale*100));
 }
 r=(RECT){44,507,756,530};ex07_text(ex07_dc,label,r,ex07_small,RGB(212,221,230),DT_CENTER|DT_SINGLELINE|DT_END_ELLIPSIS);r=(RECT){44,531,756,551};ex07_text(ex07_dc,L"휠: 확대·축소 / 목록 넘기기   ·   드래그: 이동   ·   0: 맞춤   ·   B: 배경",r,ex07_small,RGB(169,190,205),DT_CENTER|DT_SINGLELINE);
 r=(RECT){44,554,148,590};ex07_button(ex07_dc,r,L"타이틀로",FALSE);r=(RECT){160,554,274,590};ex07_button(ex07_dc,r,L"엑스트라로",FALSE);r=(RECT){286,554,390,590};ex07_button(ex07_dc,r,L"목록",FALSE);r=(RECT){402,554,506,590};ex07_button(ex07_dc,r,L"화면 맞춤",FALSE);r=(RECT){518,554,630,590};ex07_button(ex07_dc,r,L"이전",FALSE);r=(RECT){642,554,756,590};ex07_button(ex07_dc,r,L"다음",FALSE);
 SetStretchBltMode(dest,HALFTONE);StretchBlt(dest,0,0,client.right,client.bottom,ex07_dc,0,0,800,600,SRCCOPY);
}
