#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <string.h>
#include "ko_map.h"
#include "ko_r2c_ui.h"
#include "ko_ui_r2i.h"
#include "ko_font_policy_r2e.h"

/* Verified x86 IAT RVAs in the hash-bound Japanese system.dll. */
#define SYSTEM_TEXTOUTA_IAT_RVA              0xB9054
#define SYSTEM_GETTEXTEXTENTA_IAT_RVA        0xB906C
#define SYSTEM_CREATEFONTA_IAT_RVA           0xB9070
#define SYSTEM_DIALOGBOXPARAMA_IAT_RVA        0xB92CC
#define SYSTEM_CREATEDIALOGPARAMA_IAT_RVA     0xB92DC
#define SYSTEM_SENDDLGITEMMESSAGEA_IAT_RVA    0xB930C
#define SYSTEM_MESSAGEBOXA_IAT_RVA            0xB9320
#define SYSTEM_SETWINDOWTEXTA_IAT_RVA         0xB9334
#define SYSTEM_SETDLGITEMTEXTA_IAT_RVA        0xB9338
#define SYSTEM_SENDMESSAGEA_IAT_RVA           0xB9344

typedef struct KO_UI_ENTRY {
    LPCWSTR source_ja;
    LPCWSTR translation_ko;
} KO_UI_ENTRY;

/* Japanese-original-only semantic translations authored by Codex internal. */
static const KO_UI_ENTRY g_ui_map[] = {
    { L"製品情報", L"제품 정보" },
    { L"PNG: サポートされていないフォーマットです。", L"PNG: 지원하지 않는 형식입니다." },
    { L"ＣＧファイルフォーマット", L"CG 파일 형식" },
    { L"ファイル書込に失敗しました。", L"파일을 쓰지 못했습니다." },
    { L"ファイルエラー", L"파일 오류" },
    { L"サムネール未表示", L"미리 보기 없음" },
    { L"CGモード", L"CG 모드" },
    { L"バージョン", L"버전" },
    { L"テスト音声", L"음성 테스트" },
    { L"音声", L"음성" },
    { L"章表示", L"장 표시" },
    { L"サウンド", L"사운드" },
    { L"メッセージ", L"대사" },
    { L"システム", L"시스템" },
    { L"その他", L"기타" },
    { L"ウェーブ リバーブ", L"웨이브 리버브" },
    { L"パラメトリック イコライザ", L"파라메트릭 이퀄라이저" },
    { L"ガーグル", L"가글" },
    { L"フランジ", L"플랜저" },
    { L"環境リバーブ", L"환경 리버브" },
    { L"エコー", L"에코" },
    { L"ディストーション", L"디스토션" },
    { L"コンプレッション", L"컴프레서" },
    { L"コーラス", L"코러스" },
    { L"確認", L"확인" },
    { L"ご確認ください", L"확인해 주세요" },
    { L"確認してください", L"확인해 주세요" },
    { L"警告", L"경고" },
    { L"Discを入れ替えてください", L"디스크를 바꿔 넣어 주세요" },
    { L"分岐", L"분기" },
    { L"終了します。よろしいですか？", L"종료하시겠습니까?" },
    { L"タイトル画面に戻ります。よろしいですか？", L"타이틀 화면으로 돌아가시겠습니까?" },
    { L"選択肢に戻ります。よろしいですか？", L"선택지로 돌아가시겠습니까?" },
    { L"現在の画面サイズが%d×%d以下です。\nフルスクリーンモードに切り替えますか？\n（キャンセルで終了）", L"현재 화면 크기가 %d×%d 이하입니다.\n전체 화면으로 전환하시겠습니까?\n(취소하면 종료됩니다)" },
    { L"ムービーをスキップしますか？", L"영상을 건너뛰시겠습니까?" },
    { L"  スキップ中", L"  건너뛰는 중" },
    { L"タイトル", L"타이틀" },
    { L"シリアルが間違っています", L"시리얼이 올바르지 않습니다" },
    { L"シリアルが間違っています。もう一度入力してください。", L"시리얼이 올바르지 않습니다. 다시 입력해 주세요." },
    { L"製品シリアルを入力して下さい", L"제품 시리얼을 입력해 주세요" },
    { L"オーディオデバイスが変更されました", L"오디오 장치가 변경되었습니다" },
    { L"オーディオデバイスが変更されました。正常動作が保証されませんので終了します。\nよろしいですか？", L"오디오 장치가 변경되어 정상 작동을 보장할 수 없습니다.\n게임을 종료하시겠습니까?" },
    { L"Windowが作成できませんでした。", L"창을 만들지 못했습니다." },
    { L"DirectX8.1以降がインストールされていません。\nDirectXをインストール後、実行してください", L"DirectX 8.1 이상이 설치되어 있지 않습니다.\nDirectX를 설치한 뒤 실행해 주세요." },
    { L"スクリプトファイルを指定して下さい", L"스크립트 파일을 지정해 주세요" },
    { L"DirectDrawが取得できませんでした。", L"DirectDraw를 가져오지 못했습니다." },
    { L"DirectSoundをエミュレーション動作で使用します。", L"DirectSound를 에뮬레이션 모드로 사용합니다." },
    { L"デバイス能力が取得できませんでした。", L"장치 기능 정보를 가져오지 못했습니다." },
    { L"プライマリバッファが取得できませんでした。", L"기본 버퍼를 가져오지 못했습니다." },
    { L"DirectSound が初期化できませんでした。", L"DirectSound를 초기화하지 못했습니다." },
    { L"設定", L"설정" },
    { L"IVideoWindowの取得に失敗しました。", L"IVideoWindow를 가져오지 못했습니다." },
    { L"MediaEventExの取得に失敗しました。", L"MediaEventEx를 가져오지 못했습니다." },
    { L"IMediaControlの取得に失敗しました。", L"IMediaControl을 가져오지 못했습니다." },
    { L"Render作成に失敗しました。", L"렌더러를 만들지 못했습니다." },
    { L"データを上書きしますが、よろしいでしょうか？", L"데이터를 덮어쓰시겠습니까?" },
    { L"ロードするデータを選択してください。", L"불러올 데이터를 선택해 주세요." },
    { L"セーブするデータを選択してください。", L"저장할 데이터를 선택해 주세요." },
    { L"閉じる(&C)", L"닫기(&C)" },
    { L"ロード", L"불러오기" },
    { L"セーブ", L"저장" },
    { L"ショートカット", L"단축키" },
    { L"バージョン情報", L"버전 정보" },
    { L"サウンド設定", L"사운드 설정" },
    { L"テキスト設定", L"텍스트 설정" },
    { L"基本設定", L"기본 설정" },
    { L"不明なエラーです。DirectX のバージョンやインストール場所などを確認してみてください。", L"알 수 없는 오류입니다. DirectX 버전과 설치 위치 등을 확인해 주세요." },
    { L"製品シリアルチェック", L"제품 시리얼 확인" },
    { L"製品シリアルを入力して下さい。(半角英数）", L"제품 시리얼을 입력해 주세요. (영문·숫자)" },
    { L"名前を入力してください", L"이름을 입력해 주세요." },
    { L"デフォルト", L"기본값" },
    { L"名前", L"이름" },
    { L"名", L"이름" },
    { L"姓", L"성" },
    { L"システム情報", L"시스템 정보" },
    { L"ユーザー登録ページへ", L"사용자 등록 페이지로" }
};

static int decode_text(LPCSTR src, int count, WCHAR **out_text) {
    int need;
    WCHAR *buf;
    int i;
    if (!src || count < 0 || !out_text) return 0;
    *out_text = NULL;
    if (count == 0) return 0;
    need = MultiByteToWideChar(932, 0, src, count, NULL, 0);
    if (need <= 0) return 0;
    buf = (WCHAR *)HeapAlloc(GetProcessHeap(), 0, (need + 1) * sizeof(WCHAR));
    if (!buf) return 0;
    if (MultiByteToWideChar(932, 0, src, count, buf, need) != need) {
        HeapFree(GetProcessHeap(), 0, buf);
        return 0;
    }
    buf[need] = 0;
    for (i = 0; i < need; ++i) {
        unsigned int ch = (unsigned int)buf[i];
        if (ch >= 0xE000 && ch < 0xE000 + (unsigned int)g_ko_map_count) {
            WCHAR mapped = g_ko_map[ch - 0xE000];
            if (mapped != 0) buf[i] = mapped;
        }
    }
    *out_text = buf;
    return need;
}

static BOOL decode_z(LPCSTR src, WCHAR **out_text) {
    int count;
    WCHAR *empty;
    if (!out_text) return FALSE;
    *out_text = NULL;
    if (!src) return TRUE;
    count = lstrlenA(src);
    if (count == 0) {
        empty = (WCHAR *)HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR));
        if (!empty) return FALSE;
        empty[0] = 0;
        *out_text = empty;
        return TRUE;
    }
    return decode_text(src, count, out_text) > 0;
}

static void free_wide(WCHAR *text) {
    if (text) HeapFree(GetProcessHeap(), 0, text);
}

static LPCWSTR find_ui_translation(LPCWSTR source) {
    unsigned int i;
    if (!source) return NULL;
    for (i = 0; i < (unsigned int)(sizeof(g_ui_map) / sizeof(g_ui_map[0])); ++i) {
        if (lstrcmpW(source, g_ui_map[i].source_ja) == 0) return g_ui_map[i].translation_ko;
    }
    return NULL;
}

static LPCWSTR resolve_ui_translation(LPCWSTR source, WCHAR **owned_translation) {
    LPCWSTR fixed;
    if (!owned_translation) return NULL;
    *owned_translation = NULL;
    fixed = find_ui_translation(source);
    if (!fixed) fixed = ko_r2c_find_exact_translation(source);
    if (fixed) return fixed;
    *owned_translation = ko_r2c_make_dynamic_translation(source);
    return *owned_translation;
}

static BOOL WINAPI HookTextOutA(HDC hdc, int x, int y, LPCSTR text, int count) {
    WCHAR *wide = NULL;
    WCHAR *owned_translation = NULL;
    LPCWSTR localized;
    int wide_count;
    BOOL result;
    if (count == 0) return TextOutW(hdc, x, y, L"", 0);
    wide_count = decode_text(text, count, &wide);
    if (wide_count <= 0 || !wide) return FALSE;
    localized = resolve_ui_translation(wide, &owned_translation);
    if (localized) {
        result = TextOutW(hdc, x, y, localized, lstrlenW(localized));
    } else {
        result = TextOutW(hdc, x, y, wide, wide_count);
    }
    free_wide(owned_translation);
    free_wide(wide);
    return result;
}

static BOOL WINAPI HookGetTextExtentPoint32A(HDC hdc, LPCSTR text, int count, LPSIZE size) {
    WCHAR *wide = NULL;
    WCHAR *owned_translation = NULL;
    LPCWSTR localized;
    int wide_count;
    BOOL result;
    if (count == 0) return GetTextExtentPoint32W(hdc, L"", 0, size);
    wide_count = decode_text(text, count, &wide);
    if (wide_count <= 0 || !wide) return FALSE;
    localized = resolve_ui_translation(wide, &owned_translation);
    if (localized) {
        result = GetTextExtentPoint32W(hdc, localized, lstrlenW(localized), size);
    } else {
        result = GetTextExtentPoint32W(hdc, wide, wide_count, size);
    }
    free_wide(owned_translation);
    free_wide(wide);
    return result;
}

static HFONT WINAPI HookCreateFontIndirectA(const LOGFONTA *input) {
    LOGFONTA lf;
    if (!ko_r2e_prepare_logfont(input, &lf)) return NULL;
    return CreateFontIndirectA(&lf);
}

static int WINAPI HookMessageBoxA(HWND window, LPCSTR text, LPCSTR caption, UINT type) {
    WCHAR *wide_text = NULL;
    WCHAR *wide_caption = NULL;
    WCHAR *owned_text = NULL;
    WCHAR *owned_caption = NULL;
    LPCWSTR ko_text;
    LPCWSTR ko_caption;
    int result;
    if (!decode_z(text, &wide_text) || !decode_z(caption, &wide_caption)) {
        free_wide(wide_text);
        free_wide(wide_caption);
        return MessageBoxA(window, text, caption, type);
    }
    ko_text = resolve_ui_translation(wide_text, &owned_text);
    ko_caption = resolve_ui_translation(wide_caption, &owned_caption);
    result = MessageBoxW(window, ko_text ? ko_text : wide_text,
                         ko_caption ? ko_caption : wide_caption, type);
    free_wide(owned_text);
    free_wide(owned_caption);
    free_wide(wide_text);
    free_wide(wide_caption);
    return result;
}

static BOOL WINAPI HookSetWindowTextA(HWND window, LPCSTR text) {
    WCHAR *wide = NULL;
    WCHAR *owned_translation = NULL;
    LPCWSTR localized;
    BOOL result;
    if (!decode_z(text, &wide)) return SetWindowTextA(window, text);
    /* Dialog 114, EDIT 1070 receives the complete assembled information. */
    if (GetDlgCtrlID(window)==1070) owned_translation=ko_r2i_system_info(wide);
    localized=owned_translation;
    if (!localized) localized = resolve_ui_translation(wide, &owned_translation);
    result = SetWindowTextW(window, localized ? localized : wide);
    free_wide(owned_translation);
    free_wide(wide);
    return result;
}

static BOOL WINAPI HookSetDlgItemTextA(HWND dialog, int item, LPCSTR text) {
    WCHAR *wide = NULL;
    WCHAR *owned_translation = NULL;
    LPCWSTR localized;
    BOOL result;
    if (!decode_z(text, &wide)) return SetDlgItemTextA(dialog, item, text);
    localized = resolve_ui_translation(wide, &owned_translation);
    result = SetDlgItemTextW(dialog, item, localized ? localized : wide);
    free_wide(owned_translation);
    free_wide(wide);
    return result;
}

/*
 * Dialog templates are UTF-16 even when the caller uses DialogBoxParamA or
 * CreateDialogParamA. On a Japanese ACP process, the ANSI entry points turn
 * Korean template captions into literal question marks before the dialog proc
 * can see them. Route the two system.dll imports through their Unicode peers;
 * integer resource identifiers and DLGPROC callbacks are ABI-compatible.
 */
static LPCWSTR resource_name_w(LPCSTR name, WCHAR **owned_name) {
    if (!owned_name) return NULL;
    *owned_name = NULL;
    if (IS_INTRESOURCE(name)) return (LPCWSTR)name;
    if (!decode_z(name, owned_name)) return NULL;
    return *owned_name;
}

static INT_PTR WINAPI HookDialogBoxParamA(HINSTANCE instance, LPCSTR template_name,
                                          HWND parent, DLGPROC dialog_proc,
                                          LPARAM init_param) {
    WCHAR *owned_name = NULL;
    LPCWSTR wide_name = resource_name_w(template_name, &owned_name);
    INT_PTR result;
    if (!wide_name) return DialogBoxParamA(instance, template_name, parent, dialog_proc, init_param);
    result = DialogBoxParamW(instance, wide_name, parent, dialog_proc, init_param);
    free_wide(owned_name);
    return result;
}

static HWND WINAPI HookCreateDialogParamA(HINSTANCE instance, LPCSTR template_name,
                                          HWND parent, DLGPROC dialog_proc,
                                          LPARAM init_param) {
    WCHAR *owned_name = NULL;
    LPCWSTR wide_name = resource_name_w(template_name, &owned_name);
    HWND result;
    if (!wide_name) return CreateDialogParamA(instance, template_name, parent, dialog_proc, init_param);
    result = CreateDialogParamW(instance, wide_name, parent, dialog_proc, init_param);
    free_wide(owned_name);
    return result;
}

/*
 * NitroSystem inserts the five configuration page captions with
 * TCM_INSERTITEMA after creating the Unicode dialog. Translate those exact
 * Japanese-source labels and send the ABI-equivalent TCITEMW structure.
 * Other messages retain their original ANSI semantics.
 */
static BOOL is_tab_control(HWND window) {
    CHAR class_name[32];
    int length;
    if (!window) return FALSE;
    length = GetClassNameA(window, class_name, (int)(sizeof(class_name) / sizeof(class_name[0])));
    if (length <= 0) return FALSE;
    return lstrcmpiA(class_name, "SysTabControl32") == 0;
}

static LRESULT send_tab_message_w(HWND window, UINT message, WPARAM w_param,
                                  LPARAM l_param, BOOL *handled) {
    const TCITEMA *item_a;
    WCHAR *wide = NULL;
    WCHAR *owned_translation = NULL;
    LPCWSTR localized;
    TCITEMW item_w;
    LRESULT result;
    *handled = FALSE;
    if (!is_tab_control(window)) return 0;
    if ((message != TCM_INSERTITEMA && message != TCM_SETITEMA) || l_param == 0) return 0;
    item_a = (const TCITEMA *)l_param;
    if (!(item_a->mask & TCIF_TEXT) || !item_a->pszText) return 0;
    if (!decode_z(item_a->pszText, &wide)) return 0;
    localized = resolve_ui_translation(wide, &owned_translation);
    ZeroMemory(&item_w, sizeof(item_w));
    item_w.mask = item_a->mask;
    item_w.dwState = item_a->dwState;
    item_w.dwStateMask = item_a->dwStateMask;
    item_w.pszText = (LPWSTR)(localized ? localized : wide);
    item_w.cchTextMax = item_a->cchTextMax;
    item_w.iImage = item_a->iImage;
    item_w.lParam = item_a->lParam;
    result = SendMessageW(window,
                          message == TCM_INSERTITEMA ? TCM_INSERTITEMW : TCM_SETITEMW,
                          w_param, (LPARAM)&item_w);
    free_wide(owned_translation);
    free_wide(wide);
    *handled = TRUE;
    return result;
}

static LRESULT WINAPI HookSendMessageA(HWND window, UINT message, WPARAM w_param, LPARAM l_param) {
    BOOL handled = FALSE;
    LRESULT result = send_tab_message_w(window, message, w_param, l_param, &handled);
    return handled ? result : SendMessageA(window, message, w_param, l_param);
}

static LRESULT WINAPI HookSendDlgItemMessageA(HWND dialog, int item, UINT message,
                                              WPARAM w_param, LPARAM l_param) {
    HWND control = GetDlgItem(dialog, item);
    BOOL handled = FALSE;
    LRESULT result = send_tab_message_w(control, message, w_param, l_param, &handled);
    return handled ? result : SendDlgItemMessageA(dialog, item, message, w_param, l_param);
}


/* Preserve every unrelated window's ANSI class/procedure semantics. */
static HWND WINAPI HookCreateWindowExA(DWORD ex, LPCSTR cls, LPCSTR title,
    DWORD style, int x, int y, int width, int height, HWND parent, HMENU menu,
    HINSTANCE instance, LPVOID param) {
    if (cls && !IS_INTRESOURCE(cls) && lstrcmpiA(cls,"BUTTON")==0 && title) {
        WCHAR *wide=NULL;
        if (decode_z(title,&wide)) {
            BOOL close_button = lstrcmpW(wide,L"閉じる(&C)")==0;
            free_wide(wide);
            if (close_button) return CreateWindowExW(ex,L"BUTTON",L"닫기(&C)",
                style,x,y,width,height,parent,menu,instance,param);
        }
    }
    return CreateWindowExA(ex,cls,title,style,x,y,width,height,parent,menu,instance,param);
}

static BOOL patch_pointer(BYTE *address, FARPROC replacement) {
    DWORD old_protect;
    DWORD restore_protect;
    if (!VirtualProtect(address, sizeof(FARPROC), PAGE_READWRITE, &old_protect)) return FALSE;
    *(FARPROC *)address = replacement;
    FlushInstructionCache(GetCurrentProcess(), address, sizeof(FARPROC));
    return VirtualProtect(address, sizeof(FARPROC), old_protect, &restore_protect);
}

static BOOL install_hooks(void) {
    BYTE *base = (BYTE *)GetModuleHandleA("system.dll");
    if (!base) return FALSE;
    if (!patch_pointer(base + 0xB9388, (FARPROC)HookCreateWindowExA)) return FALSE;
    if (!patch_pointer(base + SYSTEM_TEXTOUTA_IAT_RVA, (FARPROC)HookTextOutA)) return FALSE;
    if (!patch_pointer(base + SYSTEM_GETTEXTEXTENTA_IAT_RVA, (FARPROC)HookGetTextExtentPoint32A)) return FALSE;
    if (!patch_pointer(base + SYSTEM_CREATEFONTA_IAT_RVA, (FARPROC)HookCreateFontIndirectA)) return FALSE;
    if (!patch_pointer(base + SYSTEM_DIALOGBOXPARAMA_IAT_RVA, (FARPROC)HookDialogBoxParamA)) return FALSE;
    if (!patch_pointer(base + SYSTEM_CREATEDIALOGPARAMA_IAT_RVA, (FARPROC)HookCreateDialogParamA)) return FALSE;
    if (!patch_pointer(base + SYSTEM_SENDDLGITEMMESSAGEA_IAT_RVA, (FARPROC)HookSendDlgItemMessageA)) return FALSE;
    if (!patch_pointer(base + SYSTEM_MESSAGEBOXA_IAT_RVA, (FARPROC)HookMessageBoxA)) return FALSE;
    if (!patch_pointer(base + SYSTEM_SETWINDOWTEXTA_IAT_RVA, (FARPROC)HookSetWindowTextA)) return FALSE;
    if (!patch_pointer(base + SYSTEM_SETDLGITEMTEXTA_IAT_RVA, (FARPROC)HookSetDlgItemTextA)) return FALSE;
    if (!patch_pointer(base + SYSTEM_SENDMESSAGEA_IAT_RVA, (FARPROC)HookSendMessageA)) return FALSE;
    return TRUE;
}

void vocal_init(HINSTANCE instance);

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved) {
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(instance);
        if (!install_hooks()) return FALSE;
        vocal_init(instance);
        return TRUE;
    }
    return TRUE;
}
