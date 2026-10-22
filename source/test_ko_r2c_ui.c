#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include "ko_r2c_ui.h"
#include "ko_ui_r2i.h"

static unsigned int g_checks = 0u;
static unsigned int g_failures = 0u;

static void fail_case(const char *name, const char *reason) {
    ++g_failures;
    fprintf(stderr, "FAIL [%s]: %s\n", name, reason);
}

static void expect_exact(const char *name, LPCWSTR source, LPCWSTR expected) {
    LPCWSTR actual;
    ++g_checks;
    actual = ko_r2c_find_exact_translation(source);
    if (!actual) {
        fail_case(name, "expected exact translation, got NULL");
    } else if (lstrcmpW(actual, expected) != 0) {
        fail_case(name, "exact translation mismatch");
    }
}

static void expect_no_exact(const char *name, LPCWSTR source) {
    ++g_checks;
    if (ko_r2c_find_exact_translation(source) != NULL) {
        fail_case(name, "unexpected exact translation");
    }
}

static void expect_dynamic(const char *name, LPCWSTR source, LPCWSTR expected) {
    WCHAR *actual;
    ++g_checks;
    actual = ko_r2c_make_dynamic_translation(source);
    if (!actual) {
        fail_case(name, "expected dynamic translation, got NULL");
        return;
    }
    if (lstrcmpW(actual, expected) != 0) {
        fail_case(name, "dynamic translation mismatch");
    }
    HeapFree(GetProcessHeap(), 0, actual);
}

static void expect_no_dynamic(const char *name, LPCWSTR source) {
    WCHAR *actual;
    ++g_checks;
    actual = ko_r2c_make_dynamic_translation(source);
    if (actual) {
        fail_case(name, "unexpected dynamic translation");
        HeapFree(GetProcessHeap(), 0, actual);
    }
}

int main(void) {
    WCHAR overlong[4098];
    SIZE_T i;

    /* Two newly discovered WS_VISIBLE static labels: exact and case-sensitive. */
    expect_exact("exact-SystemVersion", L"SystemVersion", L"시스템 버전");
    expect_exact("exact-ScriptVersion", L"ScriptVersion", L"스크립트 버전");
    expect_no_exact("exact-case-sensitive", L"systemVersion");
    expect_no_exact("exact-no-trailing-space", L"SystemVersion ");
    expect_no_exact("exact-unrelated", L"Version");
    expect_no_exact("exact-null", NULL);

    /* Positive cases mirror fully formatted runtime strings, never raw templates. */
    expect_dynamic(
        "screen-1920x1080",
        L"現在の画面サイズが1920×1080以下です。\n"
        L"フルスクリーンモードに切り替えますか？\n"
        L"（キャンセルで終了）",
        L"현재 화면 크기가 1920×1080 이하입니다.\n"
        L"전체 화면으로 전환하시겠습니까?\n"
        L"(취소하면 종료됩니다)"
    );
    expect_dynamic(
        "screen-minimum",
        L"現在の画面サイズが1×1以下です。\n"
        L"フルスクリーンモードに切り替えますか？\n"
        L"（キャンセルで終了）",
        L"현재 화면 크기가 1×1 이하입니다.\n"
        L"전체 화면으로 전환하시겠습니까?\n"
        L"(취소하면 종료됩니다)"
    );
    expect_dynamic(
        "refresh-normal",
        L" リフレッシュレート 60Hz",
        L" 주사율: 60Hz"
    );
    expect_dynamic(
        "refresh-signed-sentinel",
        L" リフレッシュレート -1Hz",
        L" 주사율: -1Hz"
    );
    expect_dynamic(
        "refresh-int32-max",
        L" リフレッシュレート 2147483647Hz",
        L" 주사율: 2147483647Hz"
    );
    expect_dynamic(
        "refresh-int32-min",
        L" リフレッシュレート -2147483648Hz",
        L" 주사율: -2147483648Hz"
    );
    expect_dynamic(
        "memory-normal",
        L"メモリ 空き容量: 123456KB ／ 総容量: 789012KB\r\n",
        L"메모리 여유 공간: 123456KB / 전체 용량: 789012KB\r\n"
    );
    expect_dynamic(
        "memory-uint64",
        L"メモリ 空き容量: 18446744073709551615KB ／ 総容量: 18446744073709551615KB\r\n",
        L"메모리 여유 공간: 18446744073709551615KB / 전체 용량: 18446744073709551615KB\r\n"
    );
    expect_dynamic(
        "drive-upper",
        L"システムドライブ:C:\\  空き容量: 1024 MB ／ 総容量: 2048 MB",
        L"시스템 드라이브: C:\\  여유 공간: 1024 MB / 전체 용량: 2048 MB"
    );
    expect_dynamic(
        "drive-lower",
        L"システムドライブ:d:  空き容量: 0 MB ／ 総容量: 999999 MB",
        L"시스템 드라이브: d:  여유 공간: 0 MB / 전체 용량: 999999 MB"
    );
    expect_dynamic(
        "resources-normal",
        L"リソース:システム 90％ ユーザー: 80％ ＧＤＩ: 70％",
        L"리소스: 시스템 90% 사용자: 80% GDI: 70%"
    );
    expect_dynamic(
        "resources-signed-sentinel",
        L"リソース:システム -1％ ユーザー: -1％ ＧＤＩ: -1％",
        L"리소스: 시스템 -1% 사용자: -1% GDI: -1%"
    );
    expect_dynamic(
        "playtime-zero",
        L"プレイ時間 00時間00分",
        L"플레이 시간 00시간 00분"
    );
    expect_dynamic(
        "playtime-long-hours",
        L"プレイ時間 123時間59分",
        L"플레이 시간 123시간 59분"
    );
    expect_dynamic(
        "file-ascii",
        L"ファイル script.pak が開けません。",
        L"다음 파일을 열 수 없습니다.\nscript.pak"
    );
    expect_dynamic(
        "file-korean-value",
        L"ファイル 저장.dat が開けません。",
        L"다음 파일을 열 수 없습니다.\n저장.dat"
    );
    expect_dynamic(
        "disc-ascii-values",
        L"「DISC1」の「DATA」をいれてください。",
        L"「DISC1」의 「DATA」를 넣어 주세요."
    );
    expect_dynamic(
        "disc-korean-values",
        L"「본편」の「두 번째 디스크」をいれてください。",
        L"「본편」의 「두 번째 디스크」를 넣어 주세요."
    );
    expect_dynamic(
        "branch-japanese-value",
        L"[竜姫]に分岐しますか？",
        L"[竜姫] 분기로 이동하시겠습니까?"
    );
    expect_dynamic(
        "branch-korean-value",
        L"[용희]に分岐しますか？",
        L"[용희] 분기로 이동하시겠습니까?"
    );

    /* Near misses demonstrate that no keyword/substr replacement is performed. */
    expect_no_dynamic(
        "screen-missing-width",
        L"現在の画面サイズが×1080以下です。\n"
        L"フルスクリーンモードに切り替えますか？\n"
        L"（キャンセルで終了）"
    );
    expect_no_dynamic(
        "screen-two-crosses",
        L"現在の画面サイズが1920×10×80以下です。\n"
        L"フルスクリーンモードに切り替えますか？\n"
        L"（キャンセルで終了）"
    );
    expect_no_dynamic(
        "screen-extra-tail",
        L"現在の画面サイズが1920×1080以下です。\n"
        L"フルスクリーンモードに切り替えますか？\n"
        L"（キャンセルで終了）!"
    );
    expect_no_dynamic("refresh-no-indent", L"リフレッシュレート 60Hz");
    expect_no_dynamic("refresh-two-indent", L"  リフレッシュレート 60Hz");
    expect_no_dynamic("refresh-raw-template", L" リフレッシュレート %dHz");
    expect_no_dynamic("refresh-int32-positive-overflow", L" リフレッシュレート 2147483648Hz");
    expect_no_dynamic("refresh-int32-negative-overflow", L" リフレッシュレート -2147483649Hz");
    expect_no_dynamic(
        "memory-lf-only",
        L"メモリ 空き容量: 1KB ／ 総容量: 2KB\n"
    );
    expect_no_dynamic(
        "memory-raw-template",
        L"メモリ 空き容量: %I64uKB ／ 総容量: %I64uKB\r\n"
    );
    expect_no_dynamic(
        "memory-negative",
        L"メモリ 空き容量: -1KB ／ 総容量: 2KB\r\n"
    );
    expect_no_dynamic(
        "memory-uint64-overflow",
        L"メモリ 空き容量: 18446744073709551616KB ／ 総容量: 2KB\r\n"
    );
    expect_no_dynamic(
        "drive-not-drive-shaped",
        L"システムドライブ:ROOT  空き容量: 1 MB ／ 総容量: 2 MB"
    );
    expect_no_dynamic(
        "drive-raw-template",
        L"システムドライブ:%s  空き容量: %I64u MB ／ 総容量: %I64u MB"
    );
    expect_no_dynamic(
        "drive-wrong-spacing",
        L"システムドライブ:C:\\ 空き容量: 1 MB ／ 総容量: 2 MB"
    );
    expect_no_dynamic(
        "drive-uint64-overflow",
        L"システムドライブ:C:\\  空き容量: 18446744073709551616 MB ／ 総容量: 2 MB"
    );
    expect_no_dynamic(
        "resources-ascii-percent",
        L"リソース:システム 90% ユーザー: 80% ＧＤＩ: 70%"
    );
    expect_no_dynamic(
        "resources-raw-template",
        L"リソース:システム %d％ ユーザー: %d％ ＧＤＩ: %d％"
    );
    expect_no_dynamic(
        "resources-missing-gdi",
        L"リソース:システム 90％ ユーザー: 80％"
    );
    expect_no_dynamic(
        "resources-int32-overflow",
        L"リソース:システム 2147483648％ ユーザー: 80％ ＧＤＩ: 70％"
    );
    expect_no_dynamic("playtime-one-digit-hour", L"プレイ時間 1時間01分");
    expect_no_dynamic("playtime-one-digit-minute", L"プレイ時間 01時間1分");
    expect_no_dynamic("playtime-minute-60", L"プレイ時間 01時間60分");
    expect_no_dynamic("playtime-raw-template", L"プレイ時間 %02d時間%02d分");
    expect_no_dynamic("file-empty", L"ファイル  が開けません。");
    expect_no_dynamic("file-raw-template", L"ファイル %s が開けません。");
    expect_no_dynamic("file-different-past-tense", L"ファイル data.pak が開けませんでした。");
    expect_no_dynamic("file-line-break", L"ファイル data\npak が開けません。");
    expect_no_dynamic("disc-empty-first", L"「」の「DATA」をいれてください。");
    expect_no_dynamic("disc-raw-template", L"「%s」の「%s」をいれてください。");
    expect_no_dynamic("disc-different-kanji", L"「DISC1」の「DATA」を入れてください。");
    expect_no_dynamic("branch-empty", L"[]に分岐しますか？");
    expect_no_dynamic("branch-raw-template", L"[%s]に分岐しますか？");
    expect_no_dynamic("branch-ascii-question", L"[竜姫]に分岐しますか?");
    expect_no_dynamic("dialogue-substring", L"リフレッシュレートの話をしよう。");
    expect_no_dynamic("dynamic-null", NULL);

    for (i = 0u; i < 4097u; ++i) overlong[i] = L'A';
    overlong[4097] = L'\0';
    expect_no_dynamic("bounded-overlong-source", overlong);

    {
        const WCHAR *source=L"Windows 11\r\nCPU info\r\n画面解像度: 4096×2160 32Bits リフレッシュレート 119Hz\r\nメモリ 空き容量: 123KB ／ 総容量: 456KB\r\nシステムドライブ:C:\\  空き容量: 10 MB ／ 総容量: 20 MB\r\n";
        const WCHAR *expected=L"Windows 11\r\nCPU info\r\n해상도: 4096×2160 32Bits 주사율: 119Hz\r\n메모리 여유: 123KB / 전체: 456KB\r\n시스템 드라이브: C:\\  여유: 10 MB / 전체: 20 MB\r\n";
        WCHAR *value=ko_r2i_system_info(source);
        if(!value || lstrcmpW(value,expected)!=0) { fprintf(stderr,"composite info mismatch\n"); return 1; }
        HeapFree(GetProcessHeap(),0,value);
        value=ko_r2i_system_info(L"画面解像度: の話をしよう。");
        if(value) { HeapFree(GetProcessHeap(),0,value); return 1; }
        printf("PASS: scoped composite system-info values and dialogue exclusion\n");
    }
    if (g_failures != 0u) {
        fprintf(stderr, "FAIL: %u of %u checks failed\n", g_failures, g_checks);
        return 1;
    }
    printf("PASS: %u exact/dynamic matcher checks\n", g_checks);
    return 0;
}