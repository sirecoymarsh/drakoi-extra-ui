/* Bilingual caption layout shared by GDI, DirectDraw and the D3D glyph mask. */
static HFONT vocalSourceFont,vocalKoreanFont;
static BOOL vocalTextFonts(void){
    if(!vocalSourceFont)vocalSourceFont=CreateFontW(-VOCAL_SOURCE_PX,0,0,0,VOCAL_FONT_WEIGHT,FALSE,FALSE,FALSE,DEFAULT_CHARSET,0,0,ANTIALIASED_QUALITY,0,L"Yu Gothic");
    if(!vocalKoreanFont)vocalKoreanFont=CreateFontW(-VOCAL_FONT_PX,0,0,0,VOCAL_FONT_WEIGHT,FALSE,FALSE,FALSE,HANGUL_CHARSET,0,0,ANTIALIASED_QUALITY,0,L"Malgun Gothic");
    return vocalSourceFont&&vocalKoreanFont;
}
static BOOL vocalSplit(LPCWSTR text,WCHAR*source,WCHAR*korean){
    LPCWSTR split=wcschr(text,L'\n');unsigned count;
    source[0]=0;korean[0]=0;
    if(!split){if(lstrlenW(text)>=256)return FALSE;lstrcpyW(korean,text);return TRUE;}
    count=(unsigned)(split-text);
    if(!count||count>=256||!split[1]||lstrlenW(split+1)>=256||wcschr(split+1,L'\n'))return FALSE;
    memcpy(source,text,count*sizeof(WCHAR));source[count]=0;lstrcpyW(korean,split+1);return TRUE;
}
static BOOL vocalMeasureLine(HDC dc,LPCWSTR text,HFONT font,int width,int maxHeight){
    SIZE extent;if(!*text)return TRUE;
    if(!SelectObject(dc,font))return FALSE;
    return GetTextExtentPoint32W(dc,text,lstrlenW(text),&extent)&&extent.cx<=width-40&&extent.cy<=maxHeight;
}
static BOOL vocalPaintLine(HDC dc,LPCWSTR text,HFONT font,const RECT*line,COLORREF color,BOOL mask){
    int dx,dy;if(!*text)return TRUE;SelectObject(dc,font);
    if(!mask){
        SetTextColor(dc,RGB(10,10,10));
        for(dy=-VOCAL_OUTLINE;dy<=VOCAL_OUTLINE;dy++)for(dx=-VOCAL_OUTLINE;dx<=VOCAL_OUTLINE;dx++)if(dx||dy){RECT outline=*line;OffsetRect(&outline,dx,dy);DrawTextW(dc,text,-1,&outline,DT_CENTER|DT_SINGLELINE|DT_NOPREFIX);}
    }
    SetTextColor(dc,mask?RGB(255,255,255):color);
    {RECT copy=*line;return DrawTextW(dc,text,-1,&copy,DT_CENTER|DT_SINGLELINE|DT_NOPREFIX)>0;}
}
static BOOL vocalPaint(HDC dc,LPCWSTR text,int left,int top,int width,BOOL mask){
    WCHAR source[256],korean[256];RECT first,second;int saved;BOOL ok=FALSE;
    if(width<600||width>4096||!vocalSplit(text,source,korean)||!vocalTextFonts())return FALSE;
    saved=SaveDC(dc);if(!saved)return FALSE;
    if(!vocalMeasureLine(dc,source,vocalSourceFont,width,32)||!vocalMeasureLine(dc,korean,vocalKoreanFont,width,36))goto done;
    first=(RECT){left,top+VOCAL_SOURCE_TOP,left+width,top+VOCAL_KOREAN_TOP};
    second=(RECT){left,top+(*source?VOCAL_KOREAN_TOP:VOCAL_SOURCE_TOP),left+width,top+VOCAL_BAND_TOP+VOCAL_BAND_HEIGHT};
    SetBkMode(dc,TRANSPARENT);IntersectClipRect(dc,left,top+VOCAL_BAND_TOP,left+width,top+VOCAL_BAND_TOP+VOCAL_BAND_HEIGHT);
    ok=vocalPaintLine(dc,source,vocalSourceFont,&first,RGB(255,255,255),mask)&&vocalPaintLine(dc,korean,vocalKoreanFont,&second,VOCAL_TEXT_COLOR,mask);
    GdiFlush();
done:
    RestoreDC(dc,saved);return ok;
}
