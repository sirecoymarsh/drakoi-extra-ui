#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "ko_font_policy_r2e.h"

static unsigned int g_checks = 0u;
static unsigned int g_failures = 0u;

static void fail_case(const char *name, const char *reason) {
    ++g_failures;
    fprintf(stderr, "FAIL [%s]: %s\n", name, reason);
}

static void check_true(const char *name, BOOL value, const char *reason) {
    ++g_checks;
    if (!value) fail_case(name, reason);
}

static void init_logfont(LOGFONTA *lf, LONG height, LPCSTR face) {
    ZeroMemory(lf, sizeof(*lf));
    lf->lfHeight = height;
    lf->lfWidth = 3;
    lf->lfEscapement = 17;
    lf->lfOrientation = 19;
    lf->lfWeight = 600;
    lf->lfItalic = 1;
    lf->lfUnderline = 1;
    lf->lfStrikeOut = 1;
    lf->lfCharSet = SHIFTJIS_CHARSET;
    lf->lfOutPrecision = OUT_TT_PRECIS;
    lf->lfClipPrecision = CLIP_STROKE_PRECIS;
    lf->lfQuality = ANTIALIASED_QUALITY;
    lf->lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
    if (face) lstrcpynA(lf->lfFaceName, face, LF_FACESIZE);
}

static void init_metric_logfont(LOGFONTA *lf, LONG height, LPCSTR face) {
    ZeroMemory(lf, sizeof(*lf));
    lf->lfHeight = height;
    lf->lfWeight = 600;
    lf->lfCharSet = SHIFTJIS_CHARSET;
    if (face) lstrcpynA(lf->lfFaceName, face, LF_FACESIZE);
}

static BOOL expected_transform(const LOGFONTA *input, LONG expected_height) {
    LOGFONTA actual;
    LOGFONTA expected;
    if (!ko_r2e_prepare_logfont(input, &actual)) return FALSE;
    memcpy(&expected, input, sizeof(expected));
    expected.lfHeight = expected_height;
    expected.lfCharSet = HANGUL_CHARSET;
    ZeroMemory(expected.lfFaceName, LF_FACESIZE);
    lstrcpynA(expected.lfFaceName, "Malgun Gothic", LF_FACESIZE);
    return memcmp(&actual, &expected, sizeof(actual)) == 0;
}

static BOOL measure_prepared(const LOGFONTA *input, LONG *prepared_height,
                             LONG *tm_height, LONG *hangul_width,
                             LONG *hangul_height) {
    static const WCHAR text[] = L"가";
    LOGFONTA prepared;
    HFONT font;
    HDC dc;
    HGDIOBJ old;
    TEXTMETRICW tm;
    SIZE size;
    BOOL ok = FALSE;
    if (!ko_r2e_prepare_logfont(input, &prepared)) return FALSE;
    font = CreateFontIndirectA(&prepared);
    if (!font) return FALSE;
    dc = CreateCompatibleDC(NULL);
    if (!dc) {
        DeleteObject(font);
        return FALSE;
    }
    old = SelectObject(dc, font);
    if (old && GetTextMetricsW(dc, &tm) &&
        GetTextExtentPoint32W(dc, text, 1, &size)) {
        *prepared_height = prepared.lfHeight;
        *tm_height = tm.tmHeight;
        *hangul_width = size.cx;
        *hangul_height = size.cy;
        ok = TRUE;
    }
    if (old) SelectObject(dc, old);
    DeleteDC(dc);
    DeleteObject(font);
    return ok;
}

int main(void) {
    LOGFONTA input;
    LOGFONTA output;
    LONG prepared_height = 0;
    LONG tm_height = 0;
    LONG hangul_width = 0;
    LONG hangul_height = 0;
    LONG unmarked_prepared_height = 0;
    LONG unmarked_tm_height = 0;
    LONG unmarked_hangul_width = 0;
    LONG unmarked_hangul_height = 0;

    check_true("null-input", !ko_r2e_prepare_logfont(NULL, &output),
               "NULL input must be rejected");
    init_logfont(&input, -17, "Malgun Gothic");
    check_true("null-output", !ko_r2e_prepare_logfont(&input, NULL),
               "NULL output must be rejected");
    check_true("marked-negative-exact-delta", expected_transform(&input, 17),
               "only height, charset, and existing R2c face substitution may differ");

    init_logfont(&input, -21, "Malgun Gothic");
    input.lfWeight = 500;
    check_true("main-dialogue-height-weight-preserved", expected_transform(&input, -21),
               "main dialogue -21/500 must retain the accepted R2c height");

    init_logfont(&input, -17, "Malgun Gothic");
    input.lfWeight = 400;
    check_true("choice-default-weight-preserved", expected_transform(&input, -17),
               "non-backlog -17 requests with default weight must retain R2c height");

    init_logfont(&input, 17, "Malgun Gothic");
    check_true("marked-positive-unchanged-height", expected_transform(&input, 17),
               "positive configured height must remain positive");

    init_logfont(&input, -17, "MS PGothic");
    check_true("japanese-face-height-unchanged", expected_transform(&input, -17),
               "dialogue/Japanese face requests must keep the R2c height");

    init_logfont(&input, -17, "malgun gothic");
    check_true("case-near-miss", expected_transform(&input, -17),
               "only the explicit configured spelling may trigger the fix");
    init_logfont(&input, -17, "Malgun Gothic ");
    check_true("suffix-near-miss", expected_transform(&input, -17),
               "a suffixed face name must not trigger the fix");
    init_logfont(&input, -17, "Malgun Gothi");
    check_true("prefix-near-miss", expected_transform(&input, -17),
               "a truncated face name must not trigger the fix");
    init_logfont(&input, -17, "");
    check_true("empty-face", expected_transform(&input, -17),
               "an empty face request must keep the R2c height");
    init_logfont(&input, LONG_MIN, "Malgun Gothic");
    check_true("long-min-no-overflow", expected_transform(&input, LONG_MIN),
               "LONG_MIN cannot be represented as a positive LONG");

    init_metric_logfont(&input, -17, "Malgun Gothic");
    check_true("configured-gdi-measurement",
               measure_prepared(&input, &prepared_height, &tm_height,
                                &hangul_width, &hangul_height),
               "configured Malgun Gothic measurement failed");
    check_true("configured-restores-17px",
               prepared_height == 17 && tm_height == 17 &&
               hangul_width > 0 && hangul_height == 17,
               "configured -17 must become a 17px cell and retain Hangul");

    init_metric_logfont(&input, -17, "MS PGothic");
    check_true("unmarked-gdi-measurement",
               measure_prepared(&input, &unmarked_prepared_height,
                                &unmarked_tm_height, &unmarked_hangul_width,
                                &unmarked_hangul_height),
               "unmarked request measurement failed");
    check_true("unmarked-r2c-regression",
               unmarked_prepared_height == -17 && unmarked_tm_height == 23 &&
               unmarked_hangul_width > 0 && unmarked_hangul_height == 23,
               "unmarked requests must retain R2c's negative Malgun metric");

    if (g_failures != 0u) {
        fprintf(stderr, "FAIL: %u of %u font-policy checks failed\n",
                g_failures, g_checks);
        return 1;
    }
    printf("PASS: %u font-policy checks; configured -17=>%ld, tm=%ld, Hangul=%ldx%ld; "
           "unmarked -17=>%ld, tm=%ld, Hangul=%ldx%ld\n",
           g_checks, prepared_height, tm_height, hangul_width, hangul_height,
           unmarked_prepared_height, unmarked_tm_height,
           unmarked_hangul_width, unmarked_hangul_height);
    return 0;
}