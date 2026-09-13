# BASIL 업로드용 폴더

이 폴더는 네 Windows PC에서 쓰는 거야. 서버에서 빌드한 게 아니라 여기서 DLL을 만들고 제출 ZIP을 묶어.

## 준비물 (네 PC에 설치)

1. StarCraft: Brood War 1.16.1 (무료)
2. BWAPI 4.4.0 (https://github.com/bwapi/bwapi/releases)
3. Visual Studio 2017 Community (무료, C++ 워크로드 + XP 지원 포함)
4. Chaoslauncher (BWAPI에 포함)

## 순서

1. 이 저장소를 네 PC에 클론해.
2. 환경변수 `BWAPI_DIR`을 BWAPI 4.4.0 폴더로 지정해.
   (그 안에 `include\BWAPI.h` 와 `lib\BWAPI.lib` 이 있어야 해)
   ```
   setx BWAPI_DIR "C:\BWAPI440"
   ```
3. `valuebot\vs\ValueBot.sln` 을 VS2017로 열고 **Release / Win32** 로 빌드해.
   산출물: `valuebot\vs\Release\ValueBot.dll`
4. Chaoslauncher로 로컬 테스트 한 판 돌려봐.
   - `bwapi.ini` 의 `ai` 경로를 `ValueBot.dll` 로 지정
   - 상대는 빌트인 AI로 OK
5. 이 폴더에서 PowerShell을 열고 패키징 실행:
   ```
   .\package.ps1 -DllPath "..\valuebot\vs\Release\ValueBot.dll" -BwapiDll "C:\BWAPI440\BWAPI.dll"
   ```
   `ValueBot_submit.zip` 이 만들어져. 구성: ValueBot.dll + BWAPI.dll
6. SSCAIT에 제출해:
   - https://sscaitournament.com/users/register.php 가입/로그인
   - Submit 페이지에 `ValueBot_submit.zip` + 소스코드(`valuebot` 폴더 통째로 zip) 업로드
7. 하루 안에 BASIL (https://www.basil-ladder.net/) 에서 자동으로 돌기 시작해.

## 주의

- DLL은 꼭 32비트 Release (v141_xp) 로 빌드해야 해. 64비트면 로드 안 돼.
- BWAPI.dll 버전과 DLL 빌드 버전이 같아야 해 (둘 다 4.4.0 권장).
- 밸런스 패치나 맵이 바뀌면 시즌마다 재활성화가 필요할 수 있어.
