/* The three display layouts share the original cue IDs and audio timing. */
typedef struct {int count,pages;const EX07_CUE*cues;} CDSUB_TRACK;
#include "drama_subtitles_data.h"
static int cdsub_mode;
static BOOL cdsub_loaded;
static const WCHAR*cdsub_names[]={L"한국어",L"일본어",L"동시 표시"};
static void cdsub_path(WCHAR path[MAX_PATH]){_snwprintf_s(path,MAX_PATH,_TRUNCATE,L"%s\\extra07\\player.ini",dir);}
static void cdsub_preferences(void){WCHAR path[MAX_PATH];UINT n;if(cdsub_loaded)return;cdsub_loaded=TRUE;cdsub_path(path);n=GetPrivateProfileIntW(L"Display",L"SubtitleMode",0,path);cdsub_mode=n<3?(int)n:0;}
static const CDSUB_TRACK*cdsub_get(int n){cdsub_preferences();return n>=0&&n<13?&cdsub_tracks[cdsub_mode][n]:NULL;}
static int cdsub_pitch(int source){return source&&cdsub_mode==2?25:33;}
static void cdsub_select(int n){
 WCHAR path[MAX_PATH],value[8];int anchor=-1,i;const CDSUB_TRACK*t;BOOL saved;
 if(n<0||n>2)return;t=cdsub_get(ex07_track);
 if(t&&ex07_viewPage>0){for(i=0;i<=ex07_readCue[ex07_track];i++){if(t->cues[i].page==ex07_viewPage){anchor=i;break;}}}
 cdsub_mode=n;ex07_viewPage=anchor>=0?cdsub_get(ex07_track)->cues[anchor].page:0;ex07_hover=-1;
 cdsub_path(path);_snwprintf_s(value,8,_TRUNCATE,L"%d",n);saved=WritePrivateProfileStringW(L"Display",L"SubtitleMode",value,path);logline("CD SUBTITLE MODE %d saved=%d\r\n",n,saved);
}
