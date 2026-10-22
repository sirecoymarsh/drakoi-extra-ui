#ifndef KO_UI_R2I_H
#define KO_UI_R2I_H
#include <windows.h>
#include <wchar.h>
/* Codex internal, Japanese-original UI context. Only the complete, identified
 * system information control is eligible; never apply these tokens to prose. */
static WCHAR *ko_r2i_system_info(LPCWSTR source) {
    static const struct { LPCWSTR ja; LPCWSTR ko; } pairs[] = {
        {L"画面解像度: ", L"해상도: "},
        {L" リフレッシュレート ", L" 주사율: "},
        {L"メモリ 空き容量: ", L"메모리 여유: "},
        {L"KB ／ 総容量: ", L"KB / 전체: "},
        {L"システムドライブ:", L"시스템 드라이브: "},
        {L"  空き容量: ", L"  여유: "},
        {L" MB ／ 総容量: ", L" MB / 전체: "},
        {L"リソース:システム ", L"리소스: 시스템 "},
        {L"％ ユーザー: ", L"% 사용자: "},
        {L"％ ＧＤＩ: ", L"% GDI: "},
        {L"％", L"%"}
    };
    SIZE_T len, pos = 0, used = 0, i;
    WCHAR *out;
    if (!source) return NULL;
    for (len=0; len<=4096 && source[len]; ++len) {}
    if (len>4096 || !wcsstr(source,L"\r\n画面解像度: ") ||
        !wcsstr(source,L"\r\nメモリ 空き容量: ") ||
        !wcsstr(source,L"\r\nシステムドライブ:")) return NULL;
    out=(WCHAR *)HeapAlloc(GetProcessHeap(),0,8192*sizeof(WCHAR));
    if (!out) return NULL;
    while(pos<len) {
        BOOL matched=FALSE;
        for(i=0;i<sizeof(pairs)/sizeof(pairs[0]);++i) {
            SIZE_T a=wcslen(pairs[i].ja), b=wcslen(pairs[i].ko);
            if(a<=len-pos && wmemcmp(source+pos,pairs[i].ja,a)==0) {
                if(used+b>=8192) { HeapFree(GetProcessHeap(),0,out); return NULL; }
                memcpy(out+used,pairs[i].ko,b*sizeof(WCHAR)); used+=b; pos+=a;
                matched=TRUE; break;
            }
        }
        if(!matched) { if(used+1>=8192) { HeapFree(GetProcessHeap(),0,out); return NULL; } out[used++]=source[pos++]; }
    }
    out[used]=0;
    return out;
}
#endif
