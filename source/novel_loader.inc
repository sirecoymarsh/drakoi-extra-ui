/* External UTF-8 manuscripts, transactional refresh, and paragraph bookmarks. */
typedef struct {int part,section,page,y,count,paragraph,offset;DWORD hash;WCHAR*lines[10];} SB_STEP;
typedef struct {SB_STEP*steps;int count,first[2],end[2],pages[2],paras[2];} SB_BOOK;
typedef struct {DWORD size;FILETIME time;BOOL exists;} SB_STAMP;
static SB_BOOK sb_book;
#define SB_COUNT (sb_book.count)
#define sb_steps (sb_book.steps)
#define sb_first (sb_book.first)
#define sb_end (sb_book.end)
#define sb_pages (sb_book.pages)
static SB_STAMP sb_seen[2],sb_pending[2];
static WCHAR sb_notice[160];
#include "legacy_bookmarks.h"
static void sb_freeBook(SB_BOOK*b){int i,j;for(i=0;i<b->count;i++)for(j=0;j<b->steps[i].count;j++)HeapFree(GetProcessHeap(),0,b->steps[i].lines[j]);if(b->steps)HeapFree(GetProcessHeap(),0,b->steps);ZeroMemory(b,sizeof(*b));}
static void sb_path(WCHAR*path,int part){_snwprintf_s(path,MAX_PATH,_TRUNCATE,L"%s\\side-black\\%s.txt",dir,part?L"하편":L"상편");}
static void sb_stamp(SB_STAMP*s){int p;for(p=0;p<2;p++){WIN32_FILE_ATTRIBUTE_DATA a;WCHAR path[MAX_PATH];sb_path(path,p);ZeroMemory(s+p,sizeof(*s));if(GetFileAttributesExW(path,GetFileExInfoStandard,&a)&&!a.nFileSizeHigh){s[p].exists=TRUE;s[p].size=a.nFileSizeLow;s[p].time=a.ftLastWriteTime;}}}
static BOOL sb_sameStamp(const SB_STAMP*a,const SB_STAMP*b){return a[0].exists==b[0].exists&&a[1].exists==b[1].exists&&a[0].size==b[0].size&&a[1].size==b[1].size&&!CompareFileTime(&a[0].time,&b[0].time)&&!CompareFileTime(&a[1].time,&b[1].time);}
static DWORD sb_hash(LPCWSTR s,int n){DWORD h=2166136261u;int i;for(i=0;i<n;i++){h^=(WORD)s[i];h*=16777619u;}return h;}
static BOOL sb_readPart(SB_BOOK*b,int part,HDC dc){
 WCHAR path[MAX_PATH],*text=NULL,*p;BYTE*bytes=NULL;HANDLE f;DWORD size,got;int chars,npara=0,page=0,rows=0;BOOL ok=FALSE;
 sb_path(path,part);f=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
 if(f==INVALID_HANDLE_VALUE)return FALSE;size=GetFileSize(f,NULL);
 if(!size||size>2097152){CloseHandle(f);return FALSE;}
 bytes=HeapAlloc(GetProcessHeap(),0,size);if(!bytes){CloseHandle(f);return FALSE;}
 if(!ReadFile(f,bytes,size,&got,NULL)||got!=size){CloseHandle(f);goto done;}CloseHandle(f);
 {DWORD off=(size>=3&&bytes[0]==239&&bytes[1]==187&&bytes[2]==191)?3:0;
 chars=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,(const char*)bytes+off,(int)(size-off),NULL,0);if(chars<=0)goto done;
 text=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,((SIZE_T)chars+1)*sizeof(WCHAR));if(!text)goto done;
 if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,(const char*)bytes+off,(int)(size-off),text,chars))goto done;}
 for(p=text;p<text+chars;p++)if(*p==0||(*p<32&&*p!=L'\r'&&*p!=L'\n'&&*p!=L'\t'))goto done;else if(*p==L'\t')*p=L' ';
 b->first[part]=b->count;
 for(p=text;*p;){
  WCHAR*end=p;int n,offset=0,nonspace=0;DWORD hash;SB_STEP*s=NULL;
  while(*end&&*end!=L'\r'&&*end!=L'\n')end++;n=(int)(end-p);
  {int k;for(k=0;k<n;k++)if(p[k]!=L' '&&p[k]!=L'　')nonspace++;}
  if(n>32768)goto done;
  if(nonspace){
   int fits;SIZE z;hash=sb_hash(p,n);
   /* A short paragraph stays together if the remaining page cannot contain it. */
   if(rows){int at=0,need=0;while(at<n&&need<=10){if(!GetTextExtentExPointW(dc,p+at,n-at,680,&fits,NULL,&z)||fits<1)goto done;if(fits<n-at){int k;for(k=fits-1;k>fits/2;k--)if(p[at+k]==L' '){fits=k+1;break;}}at+=fits;need++;}if(rows+need>10){rows=0;page++;}}
   while(offset<n){
    int take,k;WCHAR*line;
    if(!s||s->count==10){if(rows>=10){rows=0;page++;}if(b->count>=20000)goto done;s=&b->steps[b->count++];s->part=part;s->page=page;s->y=138+rows*36;s->paragraph=npara;s->offset=offset;s->hash=hash;}
    if(!GetTextExtentExPointW(dc,p+offset,n-offset,680,&take,NULL,&z)||take<1)goto done;
    if(take<n-offset){for(k=take-1;k>take/2;k--)if(p[offset+k]==L' '){take=k+1;break;}if(p[offset+take-1]>=0xd800&&p[offset+take-1]<=0xdbff)take--;if(take<1)goto done;}
    line=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,((SIZE_T)take+1)*sizeof(WCHAR));if(!line)goto done;memcpy(line,p+offset,(SIZE_T)take*sizeof(WCHAR));s->lines[s->count++]=line;offset+=take;rows++;
   }
   rows++;if(rows>=10){rows=0;page++;}npara++;
  }
  p=end;while(*p==L'\r'||*p==L'\n')p++;
 }
 b->paras[part]=npara;b->end[part]=b->count;b->pages[part]=b->count>b->first[part]?b->steps[b->count-1].page+1:0;ok=npara>0;
done:if(text)HeapFree(GetProcessHeap(),0,text);if(bytes)HeapFree(GetProcessHeap(),0,bytes);return ok;
}
static BOOL sb_loadBook(SB_BOOK*b){HDC dc;HGDIOBJ old;BOOL ok;SB_STAMP before[2],after[2];ZeroMemory(b,sizeof(*b));sb_stamp(before);b->steps=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,20000*sizeof(SB_STEP));if(!b->steps)return FALSE;dc=CreateCompatibleDC(NULL);if(!dc){sb_freeBook(b);return FALSE;}old=SelectObject(dc,ex07_font);ok=sb_readPart(b,0,dc)&&sb_readPart(b,1,dc);SelectObject(dc,old);DeleteDC(dc);sb_stamp(after);if(!ok||!sb_sameStamp(before,after)){sb_freeBook(b);return FALSE;}return TRUE;}
static int sb_anchor(int part,int paragraph,int offset,DWORD hash){int i,best=-1,distance=0x7fffffff;if(!SB_COUNT||part<0||part>1)return -1;for(i=sb_first[part];i<sb_end[part];i++){SB_STEP*s=&sb_steps[i];int d=abs(s->paragraph-paragraph);if(s->hash==hash&&d<distance){distance=d;best=i;}}if(best<0){paragraph=min(paragraph,sb_book.paras[part]-1);for(i=sb_first[part];i<sb_end[part];i++)if(sb_steps[i].paragraph==paragraph){best=i;break;}}if(best<0)return sb_first[part];while(best+1<sb_end[part]&&sb_steps[best+1].paragraph==sb_steps[best].paragraph&&sb_steps[best+1].offset<=offset)best++;return best;}
