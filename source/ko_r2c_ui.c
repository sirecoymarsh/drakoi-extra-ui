#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#include "ko_r2c_ui.h"

/*
 * Japanese-original-only semantic translations authored by Codex internal.
 * The matchers intentionally accept only the exact runtime shapes emitted by
 * the verified Japanese system.dll format strings.  Captured values are copied
 * verbatim; no dialogue or unrelated text is rewritten by substring search.
 */

#define KO_R2C_MAX_SOURCE_CHARS 4096u
#define KO_R2C_MAX_OUTPUT_CHARS 8192u

typedef struct KO_R2C_SPAN {
    LPCWSTR text;
    SIZE_T length;
} KO_R2C_SPAN;

static KO_R2C_SPAN make_span(LPCWSTR text, SIZE_T length) {
    KO_R2C_SPAN value;
    value.text = text;
    value.length = length;
    return value;
}

static BOOL bounded_length(LPCWSTR text, SIZE_T *length) {
    SIZE_T i;
    if (!text || !length) return FALSE;
    for (i = 0; i <= KO_R2C_MAX_SOURCE_CHARS; ++i) {
        if (text[i] == L'\0') {
            *length = i;
            return TRUE;
        }
    }
    return FALSE;
}

static SIZE_T literal_length(LPCWSTR text) {
    return (SIZE_T)lstrlenW(text);
}

static BOOL span_has_no_line_break_or_percent(KO_R2C_SPAN value) {
    SIZE_T i;
    if (!value.text || value.length == 0u) return FALSE;
    for (i = 0; i < value.length; ++i) {
        if (value.text[i] == L'\r' || value.text[i] == L'\n' || value.text[i] == L'%') {
            return FALSE;
        }
    }
    return TRUE;
}

static BOOL is_ascii_unsigned(KO_R2C_SPAN value, SIZE_T min_digits, SIZE_T max_digits) {
    SIZE_T i;
    if (!value.text || value.length < min_digits || value.length > max_digits) return FALSE;
    for (i = 0; i < value.length; ++i) {
        if (value.text[i] < L'0' || value.text[i] > L'9') return FALSE;
    }
    return TRUE;
}

static BOOL is_ascii_uint64(KO_R2C_SPAN value) {
    static const WCHAR maximum[] = L"18446744073709551615";
    SIZE_T i;
    if (!is_ascii_unsigned(value, 1u, 20u)) return FALSE;
    if (value.length < 20u) return TRUE;
    for (i = 0u; i < 20u; ++i) {
        if (value.text[i] < maximum[i]) return TRUE;
        if (value.text[i] > maximum[i]) return FALSE;
    }
    return TRUE;
}

static BOOL is_ascii_signed_int(KO_R2C_SPAN value) {
    BOOL negative = FALSE;
    SIZE_T start = 0u;
    SIZE_T i;
    ULONGLONG magnitude = 0u;
    ULONGLONG limit;
    if (!value.text || value.length == 0u || value.length > 11u) return FALSE;
    if (value.text[0] == L'-') {
        negative = TRUE;
        start = 1u;
    }
    if (start == value.length || value.length - start > 10u) return FALSE;
    limit = negative ? 2147483648ULL : 2147483647ULL;
    for (i = start; i < value.length; ++i) {
        unsigned int digit;
        if (value.text[i] < L'0' || value.text[i] > L'9') return FALSE;
        digit = (unsigned int)(value.text[i] - L'0');
        if (magnitude > (limit - digit) / 10u) return FALSE;
        magnitude = magnitude * 10u + digit;
    }
    return TRUE;
}

static BOOL is_minute(KO_R2C_SPAN value) {
    unsigned int minute;
    if (!is_ascii_unsigned(value, 2u, 2u)) return FALSE;
    minute = (unsigned int)(value.text[0] - L'0') * 10u
           + (unsigned int)(value.text[1] - L'0');
    return minute <= 59u;
}

static BOOL is_system_drive(KO_R2C_SPAN value) {
    WCHAR first;
    if (!span_has_no_line_break_or_percent(value) || value.length < 2u || value.length > 64u) {
        return FALSE;
    }
    first = value.text[0];
    if (!((first >= L'A' && first <= L'Z') || (first >= L'a' && first <= L'z'))) return FALSE;
    return value.text[1] == L':';
}

static BOOL span_equals_at(LPCWSTR source, SIZE_T source_length, SIZE_T offset,
                           LPCWSTR literal, SIZE_T literal_len) {
    if (!source || !literal || offset > source_length || literal_len > source_length - offset) {
        return FALSE;
    }
    return memcmp(source + offset, literal, literal_len * sizeof(WCHAR)) == 0;
}

static BOOL match_outer(LPCWSTR source, SIZE_T source_length,
                        LPCWSTR prefix, LPCWSTR suffix, KO_R2C_SPAN *middle) {
    SIZE_T prefix_len;
    SIZE_T suffix_len;
    if (!source || !prefix || !suffix || !middle) return FALSE;
    prefix_len = literal_length(prefix);
    suffix_len = literal_length(suffix);
    if (source_length < prefix_len + suffix_len) return FALSE;
    if (!span_equals_at(source, source_length, 0u, prefix, prefix_len)) return FALSE;
    if (!span_equals_at(source, source_length, source_length - suffix_len, suffix, suffix_len)) {
        return FALSE;
    }
    middle->text = source + prefix_len;
    middle->length = source_length - prefix_len - suffix_len;
    return TRUE;
}

static BOOL find_literal(LPCWSTR source, SIZE_T source_length, SIZE_T start,
                         LPCWSTR literal, SIZE_T *position) {
    SIZE_T literal_len;
    SIZE_T i;
    if (!source || !literal || !position || start > source_length) return FALSE;
    literal_len = literal_length(literal);
    if (literal_len == 0u || literal_len > source_length - start) return FALSE;
    for (i = start; i <= source_length - literal_len; ++i) {
        if (span_equals_at(source, source_length, i, literal, literal_len)) {
            *position = i;
            return TRUE;
        }
    }
    return FALSE;
}

static KO_R2C_SPAN literal_span(LPCWSTR text) {
    KO_R2C_SPAN value;
    value.text = text;
    value.length = literal_length(text);
    return value;
}

static WCHAR *make_output(const KO_R2C_SPAN *pieces, SIZE_T piece_count) {
    SIZE_T total = 0u;
    SIZE_T i;
    SIZE_T cursor = 0u;
    WCHAR *output;
    if (!pieces || piece_count == 0u) return NULL;
    for (i = 0; i < piece_count; ++i) {
        if (!pieces[i].text || pieces[i].length > KO_R2C_MAX_OUTPUT_CHARS - total) return NULL;
        total += pieces[i].length;
    }
    output = (WCHAR *)HeapAlloc(GetProcessHeap(), 0, (total + 1u) * sizeof(WCHAR));
    if (!output) return NULL;
    for (i = 0; i < piece_count; ++i) {
        if (pieces[i].length != 0u) {
            memcpy(output + cursor, pieces[i].text, pieces[i].length * sizeof(WCHAR));
            cursor += pieces[i].length;
        }
    }
    output[cursor] = L'\0';
    return output;
}

static WCHAR *translate_screen_size(LPCWSTR source, SIZE_T source_length) {
    static const WCHAR prefix[] = L"現在の画面サイズが";
    static const WCHAR suffix[] = L"以下です。\nフルスクリーンモードに切り替えますか？\n（キャンセルで終了）";
    static const WCHAR separator[] = L"×";
    KO_R2C_SPAN middle;
    KO_R2C_SPAN width;
    KO_R2C_SPAN height;
    KO_R2C_SPAN pieces[5];
    SIZE_T cross;
    if (!match_outer(source, source_length, prefix, suffix, &middle)) return NULL;
    if (!find_literal(middle.text, middle.length, 0u, separator, &cross)) return NULL;
    width = make_span(middle.text, cross);
    height = make_span(middle.text + cross + 1u, middle.length - cross - 1u);
    if (!is_ascii_unsigned(width, 1u, 6u) || !is_ascii_unsigned(height, 1u, 6u)) return NULL;
    pieces[0] = literal_span(L"현재 화면 크기가 ");
    pieces[1] = width;
    pieces[2] = literal_span(L"×");
    pieces[3] = height;
    pieces[4] = literal_span(L" 이하입니다.\n전체 화면으로 전환하시겠습니까?\n(취소하면 종료됩니다)");
    return make_output(pieces, 5u);
}

static WCHAR *translate_refresh_rate(LPCWSTR source, SIZE_T source_length) {
    KO_R2C_SPAN rate;
    KO_R2C_SPAN pieces[3];
    if (!match_outer(source, source_length, L" リフレッシュレート ", L"Hz", &rate)) return NULL;
    if (!is_ascii_signed_int(rate)) return NULL;
    pieces[0] = literal_span(L" 주사율: ");
    pieces[1] = rate;
    pieces[2] = literal_span(L"Hz");
    return make_output(pieces, 3u);
}

static WCHAR *translate_memory(LPCWSTR source, SIZE_T source_length) {
    static const WCHAR prefix[] = L"メモリ 空き容量: ";
    static const WCHAR separator[] = L"KB ／ 総容量: ";
    static const WCHAR suffix[] = L"KB\r\n";
    KO_R2C_SPAN free_space;
    KO_R2C_SPAN total_space;
    KO_R2C_SPAN pieces[5];
    SIZE_T separator_pos;
    SIZE_T prefix_len = literal_length(prefix);
    SIZE_T separator_len = literal_length(separator);
    SIZE_T suffix_len = literal_length(suffix);
    if (source_length < prefix_len + separator_len + suffix_len) return NULL;
    if (!span_equals_at(source, source_length, 0u, prefix, prefix_len)) return NULL;
    if (!span_equals_at(source, source_length, source_length - suffix_len, suffix, suffix_len)) return NULL;
    if (!find_literal(source, source_length - suffix_len, prefix_len, separator, &separator_pos)) return NULL;
    free_space = make_span(source + prefix_len, separator_pos - prefix_len);
    total_space = make_span(source + separator_pos + separator_len,
                            source_length - suffix_len - separator_pos - separator_len);
    if (!is_ascii_uint64(free_space) || !is_ascii_uint64(total_space)) {
        return NULL;
    }
    pieces[0] = literal_span(L"메모리 여유 공간: ");
    pieces[1] = free_space;
    pieces[2] = literal_span(L"KB / 전체 용량: ");
    pieces[3] = total_space;
    pieces[4] = literal_span(L"KB\r\n");
    return make_output(pieces, 5u);
}

static WCHAR *translate_system_drive(LPCWSTR source, SIZE_T source_length) {
    static const WCHAR prefix[] = L"システムドライブ:";
    static const WCHAR separator1[] = L"  空き容量: ";
    static const WCHAR separator2[] = L" MB ／ 総容量: ";
    static const WCHAR suffix[] = L" MB";
    KO_R2C_SPAN drive;
    KO_R2C_SPAN free_space;
    KO_R2C_SPAN total_space;
    KO_R2C_SPAN pieces[7];
    SIZE_T p1;
    SIZE_T p2;
    SIZE_T prefix_len = literal_length(prefix);
    SIZE_T separator1_len = literal_length(separator1);
    SIZE_T separator2_len = literal_length(separator2);
    SIZE_T suffix_len = literal_length(suffix);
    if (source_length < prefix_len + separator1_len + separator2_len + suffix_len) return NULL;
    if (!span_equals_at(source, source_length, 0u, prefix, prefix_len)) return NULL;
    if (!span_equals_at(source, source_length, source_length - suffix_len, suffix, suffix_len)) return NULL;
    if (!find_literal(source, source_length - suffix_len, prefix_len, separator1, &p1)) return NULL;
    if (!find_literal(source, source_length - suffix_len, p1 + separator1_len, separator2, &p2)) return NULL;
    drive = make_span(source + prefix_len, p1 - prefix_len);
    free_space = make_span(source + p1 + separator1_len,
                           p2 - p1 - separator1_len);
    total_space = make_span(source + p2 + separator2_len,
                            source_length - suffix_len - p2 - separator2_len);
    if (!is_system_drive(drive)
            || !is_ascii_uint64(free_space)
            || !is_ascii_uint64(total_space)) {
        return NULL;
    }
    pieces[0] = literal_span(L"시스템 드라이브: ");
    pieces[1] = drive;
    pieces[2] = literal_span(L"  여유 공간: ");
    pieces[3] = free_space;
    pieces[4] = literal_span(L" MB / 전체 용량: ");
    pieces[5] = total_space;
    pieces[6] = literal_span(L" MB");
    return make_output(pieces, 7u);
}

static WCHAR *translate_resources(LPCWSTR source, SIZE_T source_length) {
    static const WCHAR prefix[] = L"リソース:システム ";
    static const WCHAR separator1[] = L"％ ユーザー: ";
    static const WCHAR separator2[] = L"％ ＧＤＩ: ";
    static const WCHAR suffix[] = L"％";
    KO_R2C_SPAN system_value;
    KO_R2C_SPAN user_value;
    KO_R2C_SPAN gdi_value;
    KO_R2C_SPAN pieces[7];
    SIZE_T p1;
    SIZE_T p2;
    SIZE_T prefix_len = literal_length(prefix);
    SIZE_T separator1_len = literal_length(separator1);
    SIZE_T separator2_len = literal_length(separator2);
    SIZE_T suffix_len = literal_length(suffix);
    if (source_length < prefix_len + separator1_len + separator2_len + suffix_len) return NULL;
    if (!span_equals_at(source, source_length, 0u, prefix, prefix_len)) return NULL;
    if (!span_equals_at(source, source_length, source_length - suffix_len, suffix, suffix_len)) return NULL;
    if (!find_literal(source, source_length - suffix_len, prefix_len, separator1, &p1)) return NULL;
    if (!find_literal(source, source_length - suffix_len, p1 + separator1_len, separator2, &p2)) return NULL;
    system_value = make_span(source + prefix_len, p1 - prefix_len);
    user_value = make_span(source + p1 + separator1_len, p2 - p1 - separator1_len);
    gdi_value = make_span(source + p2 + separator2_len,
                          source_length - suffix_len - p2 - separator2_len);
    if (!is_ascii_signed_int(system_value)
            || !is_ascii_signed_int(user_value)
            || !is_ascii_signed_int(gdi_value)) {
        return NULL;
    }
    pieces[0] = literal_span(L"리소스: 시스템 ");
    pieces[1] = system_value;
    pieces[2] = literal_span(L"% 사용자: ");
    pieces[3] = user_value;
    pieces[4] = literal_span(L"% GDI: ");
    pieces[5] = gdi_value;
    pieces[6] = literal_span(L"%");
    return make_output(pieces, 7u);
}

static WCHAR *translate_play_time(LPCWSTR source, SIZE_T source_length) {
    static const WCHAR prefix[] = L"プレイ時間 ";
    static const WCHAR separator[] = L"時間";
    static const WCHAR suffix[] = L"分";
    KO_R2C_SPAN hours;
    KO_R2C_SPAN minutes;
    KO_R2C_SPAN pieces[5];
    SIZE_T p;
    SIZE_T prefix_len = literal_length(prefix);
    SIZE_T separator_len = literal_length(separator);
    SIZE_T suffix_len = literal_length(suffix);
    if (source_length < prefix_len + separator_len + suffix_len) return NULL;
    if (!span_equals_at(source, source_length, 0u, prefix, prefix_len)) return NULL;
    if (!span_equals_at(source, source_length, source_length - suffix_len, suffix, suffix_len)) return NULL;
    if (!find_literal(source, source_length - suffix_len, prefix_len, separator, &p)) return NULL;
    hours = make_span(source + prefix_len, p - prefix_len);
    minutes = make_span(source + p + separator_len,
                        source_length - suffix_len - p - separator_len);
    if (!is_ascii_unsigned(hours, 2u, 10u) || !is_minute(minutes)) return NULL;
    pieces[0] = literal_span(L"플레이 시간 ");
    pieces[1] = hours;
    pieces[2] = literal_span(L"시간 ");
    pieces[3] = minutes;
    pieces[4] = literal_span(L"분");
    return make_output(pieces, 5u);
}

static WCHAR *translate_file_open_error(LPCWSTR source, SIZE_T source_length) {
    KO_R2C_SPAN file_name;
    KO_R2C_SPAN pieces[2];
    if (!match_outer(source, source_length, L"ファイル ", L" が開けません。", &file_name)) return NULL;
    if (!span_has_no_line_break_or_percent(file_name) || file_name.length > 2048u) return NULL;
    pieces[0] = literal_span(L"다음 파일을 열 수 없습니다.\n");
    pieces[1] = file_name;
    return make_output(pieces, 2u);
}

static WCHAR *translate_disc_insert(LPCWSTR source, SIZE_T source_length) {
    static const WCHAR prefix[] = L"「";
    static const WCHAR separator[] = L"」の「";
    static const WCHAR suffix[] = L"」をいれてください。";
    KO_R2C_SPAN first;
    KO_R2C_SPAN second;
    KO_R2C_SPAN pieces[5];
    SIZE_T p;
    SIZE_T prefix_len = literal_length(prefix);
    SIZE_T separator_len = literal_length(separator);
    SIZE_T suffix_len = literal_length(suffix);
    if (source_length < prefix_len + separator_len + suffix_len) return NULL;
    if (!span_equals_at(source, source_length, 0u, prefix, prefix_len)) return NULL;
    if (!span_equals_at(source, source_length, source_length - suffix_len, suffix, suffix_len)) return NULL;
    if (!find_literal(source, source_length - suffix_len, prefix_len, separator, &p)) return NULL;
    first = make_span(source + prefix_len, p - prefix_len);
    second = make_span(source + p + separator_len,
                       source_length - suffix_len - p - separator_len);
    if (!span_has_no_line_break_or_percent(first) || first.length > 1024u
            || !span_has_no_line_break_or_percent(second) || second.length > 1024u) {
        return NULL;
    }
    pieces[0] = literal_span(L"「");
    pieces[1] = first;
    pieces[2] = literal_span(L"」의 「");
    pieces[3] = second;
    pieces[4] = literal_span(L"」를 넣어 주세요.");
    return make_output(pieces, 5u);
}

static WCHAR *translate_branch_prompt(LPCWSTR source, SIZE_T source_length) {
    KO_R2C_SPAN branch;
    KO_R2C_SPAN pieces[3];
    if (!match_outer(source, source_length, L"[", L"]に分岐しますか？", &branch)) return NULL;
    if (!span_has_no_line_break_or_percent(branch) || branch.length > 512u) return NULL;
    pieces[0] = literal_span(L"[");
    pieces[1] = branch;
    pieces[2] = literal_span(L"] 분기로 이동하시겠습니까?");
    return make_output(pieces, 3u);
}

LPCWSTR ko_r2c_find_exact_translation(LPCWSTR source) {
    if (!source) return NULL;
    if (lstrcmpW(source, L"SystemVersion") == 0) return L"시스템 버전";
    if (lstrcmpW(source, L"ScriptVersion") == 0) return L"스크립트 버전";
    return NULL;
}

WCHAR *ko_r2c_make_dynamic_translation(LPCWSTR source) {
    SIZE_T source_length;
    WCHAR *translated;
    if (!bounded_length(source, &source_length)) return NULL;

    translated = translate_screen_size(source, source_length);
    if (translated) return translated;
    translated = translate_refresh_rate(source, source_length);
    if (translated) return translated;
    translated = translate_memory(source, source_length);
    if (translated) return translated;
    translated = translate_system_drive(source, source_length);
    if (translated) return translated;
    translated = translate_resources(source, source_length);
    if (translated) return translated;
    translated = translate_play_time(source, source_length);
    if (translated) return translated;
    translated = translate_file_open_error(source, source_length);
    if (translated) return translated;
    translated = translate_disc_insert(source, source_length);
    if (translated) return translated;
    return translate_branch_prompt(source, source_length);
}
