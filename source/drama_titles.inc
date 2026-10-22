/* Display-only title translations. Original track metadata and file keys stay intact.
 * translator_model: Codex internal; checked against the existing track dialogue. */
static BOOL cdtitle_korean=TRUE,cdtitle_loaded;
static const WCHAR*cdtitle_ko[13]={
 L"용의 뒷이야기 / 잠자리 대화",
 L"슈퍼빌런, 슈퍼히로인을 만나다",
 L"용의 뒷이야기 / 은빛 악마",
 L"빙글빙글 도는 소녀",
 L"용의 뒷이야기 / 용살자",
 L"너에게 입맞춤을",
 L"용의 뒷이야기 / 열혈 엄마",
 L"너에게 죽음을",
 L"용의 뒷이야기 / 브륀힐데",
 L"용의 뒷이야기 / 가난한 이야기의 황금",
 L"슈퍼히로인 VS 슈퍼빌런",
 L"용의 뒷이야기 / 술래잡기",
 L"막이 오르다"
};
static void cdtitle_preferences(void){WCHAR path[MAX_PATH];UINT value;if(cdtitle_loaded)return;cdtitle_loaded=TRUE;cdart_settingsPath(path);value=GetPrivateProfileIntW(L"Display",L"TranslateTitles",1,path);cdtitle_korean=value==0?FALSE:TRUE;}
static LPCWSTR cdtitle_get(int n,BOOL brief){LPCWSTR title;const WCHAR*slash;cdtitle_preferences();if(n<0||n>=13)return L"드라마 CD";title=cdtitle_korean?cdtitle_ko[n]:ex07_tracks[n].title;if(brief&&(slash=wcschr(title,L'/'))!=NULL){title=slash+1;while(*title==L' ')title++;}return title;}
static void cdtitle_toggle(void){WCHAR path[MAX_PATH];BOOL saved;cdtitle_preferences();cdtitle_korean=!cdtitle_korean;cdart_settingsPath(path);saved=WritePrivateProfileStringW(L"Display",L"TranslateTitles",cdtitle_korean?L"1":L"0",path);logline("CD TITLE TRANSLATION %d saved=%d\r\n",cdtitle_korean,saved);}
