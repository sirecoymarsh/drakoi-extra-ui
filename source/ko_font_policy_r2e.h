#ifndef DRAKOI_KO_FONT_POLICY_R2E_H
#define DRAKOI_KO_FONT_POLICY_R2E_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <limits.h>
#include <string.h>

/*
 * R2e backlog-only metric repair.
 *
 * Preserve R2c's accepted font behavior for every request except the exact
 * backlog.ini tuple: face "Malgun Gothic", height -17, weight 600. Only that
 * tuple becomes a positive 17-pixel cell-height request. In particular, the
 * main dialogue request (-21/500) and choice/default-weight requests retain
 * their original R2c heights, avoiding R2d's all-text shrink side effect.
 */
static BOOL ko_r2e_is_explicit_malgun_gothic(const LOGFONTA *input) {
    static const CHAR expected[] = "Malgun Gothic";
    unsigned int i;
    if (!input) return FALSE;
    for (i = 0u; i < (unsigned int)sizeof(expected); ++i) {
        if (input->lfFaceName[i] != expected[i]) return FALSE;
    }
    return TRUE;
}

static BOOL ko_r2e_prepare_logfont(const LOGFONTA *input, LOGFONTA *output) {
    if (!input || !output) return FALSE;
    memcpy(output, input, sizeof(*output));
    if (output->lfHeight == -17 && output->lfWeight == 600 &&
        ko_r2e_is_explicit_malgun_gothic(input)) {
        output->lfHeight = 17;
    }
    output->lfCharSet = HANGUL_CHARSET;
    ZeroMemory(output->lfFaceName, LF_FACESIZE);
    lstrcpynA(output->lfFaceName, "Malgun Gothic", LF_FACESIZE);
    return TRUE;
}

#endif