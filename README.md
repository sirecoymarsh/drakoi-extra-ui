# DraKoi Extra UI

《드라†코이》 한국어 패치의 엑스트라 UI 디자인 작업용 **비공개** 저장소입니다. 실제 게임 엔진과 음원은 포함하지 않습니다.

## 현재 요청

엑스트라의 단순한 네모 패널을 게임 타이틀 화면과 어울리는 디자인으로 다듬습니다. 먼저 `docs/ui-reference/title-layout.png`와 현재 화면들을 확인하세요. 기존 재생·자막·메뉴 이동 기능은 유지합니다. **게임 실행 폴더에 파일을 복사하거나 적용하기 전에는 반드시 사용자에게 확인받으세요.**

## 코드와 자료

| 경로 | 역할 |
| --- | --- |
| `source/extra07_player.inc` | 엑스트라 허브, 드라마 CD 선택/재생, 공통 버튼 및 입력 처리 |
| `source/drama_art.inc`, `source/drama_titles.inc` | 표지/배경 선택, 그림 보기, 원제/번역 제목 |
| `source/drama_subtitles.inc`, `source/drama_subtitles_data.h` | 한국어/일본어/동시 자막과 629개 대사 |
| `source/sound_player.inc`, `novel_player.inc`, `gallery_player.inc`, `flowchart.inc` | 사운드, 소설, 갤러리, 플로차트 |
| `source/vocal_probe.c` | 게임에 연결되는 DLL 진입 코드 |
| `build/ko_hook_r2e.obj`, `build/ko_r2c_ui.obj` | 현재 검증된 x86 링크 입력. UI 디자인 작업에서 임의 교체하지 않습니다. |
| `native_update_review.c` | 실제 GDI 렌더러와 입력 처리를 사용하는 Windows 검수 프로그램 |
| `extra07/`, `sound-mode/`, `extra-gallery/`, `side-black/` | 화면 렌더링에 필요한 그림과 현재 게임 원고. 개인 설정/읽기 위치는 제외했습니다. |
| `docs/ui-reference/` | 원래 타이틀 디자인과 이미 검수한 현재 화면의 참고 이미지 |
| `docs/upload-manifest.json` | 업로드 기준 파일 해시와 검증 기록 |

소설·자막·가사 번역은 기존 Codex internal 작업물을 그대로 보존했습니다. 이번 요청은 디자인 수정이며, 대사/번역을 새로 작성하는 작업은 아닙니다. 일본어 드라마 자막은 기존 음성 전사 초안이며 공식 대본과 새로 대조한 것은 아닙니다.

## 빌드

Windows, Visual Studio C++ x86 도구와 Windows SDK가 필요합니다. `vswhere`로 설치 경로를 찾으며, 준비된 x86 개발자 명령 프롬프트도 사용할 수 있습니다.

```powershell
.\build.bat
```

결과는 `build/ko.dll`입니다. 필요한 임시 폴더는 자동 생성됩니다. 빌드 명령은 게임 실행 폴더에 설치하지 않습니다. Linux 클라우드에서는 코드를 수정할 수 있지만 이 Windows GDI DLL을 그대로 실행하거나 MSVC 검수를 완료했다고 주장할 수 없습니다. Windows 환경에서 빌드/렌더 검증을 수행하세요. GitHub Actions 실행은 이 저장소에 추가하지 않았습니다.

## 화면 검수

음원 없이 드라마 CD 목록의 세 가지 표지 배치를 렌더링합니다.

```powershell
.\review_update.bat --menu-introduction-render
```

결과는 `rendered/drama-introduction-0.bmp`부터 `-2.bmp`입니다. `--menu-introduction`은 현재 배치의 클릭 영역까지 확인하므로 좌표를 바꾸면 해당 기대값도 의미에 맞게 갱신하세요. `--flowchart`, `--subtitle-modes`, `--gallery-labels` 등 다른 검수 모드는 변경한 기능에 필요한 경우에만 실행합니다. 기존 검증 결과를 재사용하고, 전체 검수를 무조건 반복하지 않습니다. 음원을 제외했으므로 오디오 재생 검수는 게임이 있는 PC에서 별도로 해야 합니다.

## 로컬 연결

- 소스: `P:\AI\Codex\projects\drakoi-drama-cd\extra-live-text-gallery-20261022`
- 게임 실행본: `P:\AI\Codex\projects\drakoi-drama-cd\review-build\extra08-continuous-review`
- 저장소의 이미지는 해당 UI 작업을 위한 자료입니다. 공개 저장소로 전환하거나 별도 배포하지 않습니다.
- 실제 게임 실행 파일, `.pak`, 음원, 세이브, 개인 설정, 인증 정보, 이전 빌드/백업은 추적하지 않습니다.
- 로컬 미커밋 수정이 있는 상태에서는 덮어쓰기나 강제 초기화 없이 차이를 먼저 확인하세요.

참고: 두 사전 컴파일된 훅 오브젝트는 그대로 링크합니다. source/ko_hook_r2e.c를 따로 재빌드하려면 원래 한글 패치 생성 과정의 ko_map.h 등이 추가로 필요하며, 이번 UI 빌드에는 필요하지 않습니다.
