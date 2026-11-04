# 1104 ByteTrack 작업 정리

작성: 준이 · 2026-11-04 · 기준 코드: `juni33-bot/work` `4377b00` (master)

## 요약

- timestamp 수정, ByteTrack 활성화, `nc_traj` 빈 함수는 코드 반영과 푸시까지 끝났다.
- 보드 화면에서 `track_id`가 표시되고, 같은 차량에서 ID가 유지되는 것을 확인했다.
- **보드에서 GPU 오류가 반복되고 커널 오류로 멈춘 적이 한 번 있다. 원인은 아직 모른다.**
- 이 문제가 풀릴 때까지 순서 0(푸시와 보드 확인)은 완료로 보지 않는다.

## 작업 상태

| 작업 | 상태 | 근거 |
| --- | --- | --- |
| timestamp 수정 | 완료, 내 보드 로그는 미확보 | 코드 확인. 서영이 패치를 넣어 지연값 약 35ms를 측정했고, PeleeDet에서도 값이 들어오는 것을 확인했다. |
| ByteTrack 활성화 | 동작 확인, 안정성 미확인 | 화면에 `[17]car:0.94` 형태로 표시되고 ID가 유지된다. 34ms/28fps. |
| `nc_traj` 빈 함수 | 완료 | 서영이 파일을 반영했고, 모델 전환 때 `nc_traj_reset()`이 호출되는 것도 확인했다. |
| `[TRK]` 로그로 `ts` 확인 | 미완료 | 13:46 실행에서 `[TRK]` 출력을 얻지 못했다. |
| GPU 오류 원인 파악 | 미완료 | 아래 비교 시험 결과가 없다. |

## 코드 변경

| 파일 | 위치 | 내용 |
| --- | --- | --- |
| `wayland_npu_app/wayland_npu_app.c` | 716 | `time_stamp_us = nc_get_mono_us_time()` |
| 〃 | 67 | `#include "nc_utils.h"` 추가 |
| 〃 | 92 | `VIS0_MAX_CH`를 `(4)`에서 `(1)`로 변경 (timestamp와 별개인 보드 설정) |
| 〃 | 951~953 | `SHOW_TRI_CHIMERA`일 때 `net_id = NETWORK_TRI_CHIMERA` |
| `common/nc_app_modules/cnn/core/nc_cnn_aiware_runtime.h` | 89 | `cnn_output_info`에 `time_stamp_us` 추가 |
| `common/nc_app_modules/cnn/core/nc_cnn_aiware_runtime.c` | 405 | `cnn_output.time_stamp_us = time_stamp;` |
| YOLOv8, UFLD, Segmentation, PeleeDet 후처리 | 각 파일 | `time_stamp = cnn_output->time_stamp_us` |
| `common/nc_app_modules/cnn/core/nc_traj.c/h` | 새 파일 | 빈 `nc_traj_reset()` |
| `wayland_npu_app/Makefile` | 54, 331 | `USE_BYTETRACK = yes`, `nc_traj.c` 추가 |

`nc_cnn_yolov5_postprocess.c:226`은 아직 `time_stamp = 0`이다. 현재 빌드에는 포함되지 않지만, YOLOv5로 바꾸면 고쳐야 한다.

## 오늘 발견한 문제

### `net_id` 초기화 누락 (해결)

`0550acb`에서는 `USE_BYTETRACK`만 켜져 있어서, `TRICHIMERA` 설정에서 `npu_init()`의 `net_id`가 초기화되지 않았다. 이 값이 `net_info[]`의 인덱스로 쓰이므로 위험했고, `4377b00`에서 고쳤다.

### GPU 오류와 커널 오류 (미해결)

| 시점 | 로그 | 의미 |
| --- | --- | --- |
| 첫 실행, 2768초 | `Unhandled Page fault`, `TRANSLATION_FAULT`, VA `0x5F2A2129C0` | GPU가 매핑되지 않은 주소를 읽으려 했다. |
| 첫 실행, 2929~2937초 | `DATA_INVALID_FAULT` 반복, VA `0x5F2A290E40` | GPU가 잘못된 작업 데이터를 받았다. |
| 첫 실행, 2937초 | `kernel BUG at mm/slub.c:4020`, `kfree`, `Comm: bc`, `kdb` 진입 | 커널이 멈췄다. `bc`는 오류 당시 실행 중이던 프로그램으로, 원인은 아닌 것으로 보인다. |
| 재부팅 후, 425~431초 | `DATA_INVALID_FAULT` 반복 | 재부팅한 뒤 스크립트로 실행해도 재현됐다. |

첫 화면 캡처의 왼쪽 위 청록색 도형도 이 오류와 관련 있을 수 있다. 아직 추정이다.

**후보 정리**

| 후보 | 판단 | 근거 |
| --- | --- | --- |
| 앱 중복 실행, 드라이버 꼬임 | 가능성 낮음 | 재부팅 후에도 재현됐다. 단, 그 실행의 `ps` 결과는 기록하지 못했다. |
| 예전 빌드 파일 혼입 | 제외 | `Makefile`이 매번 모든 소스를 새로 컴파일한다. |
| `(float**)color` 형 변환 | 제외 | 받는 함수가 다시 `GLfloat*`로 쓴다. |
| 트래커 결과 개수 초과 | 가능성 낮음 | 100개로 제한되어 있다. |
| ByteTrack, timestamp, `VIS0_MAX_CH` 변경 | 미확인 | 비교 시험 필요 |
| 원래 SDK나 드라이버 문제 | 미확인 | 비교 시험 필요 |

**비교 시험** (시험마다 재부팅하고, 같은 영상으로 2분 실행)

```bash
dmesg | grep -cE "Unhandled Page fault|DATA_INVALID_FAULT"
```

| 시험 | 바꿀 것 | 결과 |
| --- | --- | --- |
| A | `Makefile:54`의 `USE_BYTETRACK`만 주석 처리 | 미실시 |
| B | A에 이어 `VIS0_MAX_CH`를 `(4)`로 복원 | 미실시 |
| C | 원본 커밋 `f68c7e8` 빌드 | 미실시 |

서영과 준우 보드에서 같은 오류가 나는지도 물어봐야 한다.

## 보드 실행 방법

앱과 스크립트는 `/mnt/user_data/applications`에 있다. 로그인 직후 위치는 `/root`다.

```bash
cd /mnt/user_data/applications
ps | grep app_wayland_npu
sh run_app_wayland_npu.sh > /tmp/trk.log 2>&1
```

- 앱은 한 번만 실행한다. 이미 떠 있으면 스크립트를 또 실행하지 않는다.
- 스크립트의 출력을 파일로 보내면 실행 오류도 화면에 안 나오므로 `cat /tmp/trk.log`로 확인한다.
- `kdb>`로 떨어지면 `go`로 계속하지 말고 재부팅한다.
- 파이프나 파일로 출력할 때는 `printf` 뒤에 `fflush(stdout);`가 있어야 로그가 바로 남는다.

## `ts` 확인용 임시 로그

`nc_cnn_yolov8_postprocess.c`의 `nc_cnn_track(...)` 바로 다음에 넣는다. 확인 후 삭제한다. 이 로그는 저장소에 커밋하지 않았다.

```c
for (int c = 0; c < g_yolov8_net_info->class_num; c++)
    for (int n = 0; n < out_cnn_results->class_objs[c].obj_cnt; n++) {
        stObjInfo *o = &out_cnn_results->class_objs[c].objs[n];
        printf("[TRK] ts=%llu c=%d id=%d x=%.1f y=%.1f w=%.1f h=%.1f\n",
               (unsigned long long)cnn_output->time_stamp_us, c, o->track_id,
               o->bbox.x, o->bbox.y, o->bbox.w, o->bbox.h);
        fflush(stdout);
    }
```

`ts`가 0이 아니고 계속 커지며, 프레임 간 차이가 약 33,000(us)이면 정상이다.

## 다음 작업: 궤적 예측

| 순서 | 작업 | 메모 |
| --- | --- | --- |
| 0 | GPU 오류 비교 시험, `[TRK]` 로그 확인 | 이 문제가 남아 있으면 보드 시험 결과를 믿기 어렵다. |
| 1 | 칼만 필터, 위치 이력, `nc_traj_reset()`, 차량 필터 | 상태 테이블을 함께 쓰므로 같이 만든다. |
| 2 | `nc_cnn_track()` 직후 호출, 로그 확인 | 먼저 k를 고정값으로 두고 검증한다. |
| 3 | 측정값으로 k 계산 | 서영의 `nc_meas_get_latency_ms()` 사용 |
| 4 | LIGHT 모드 사이 프레임 채우기 | 읽는 쪽 구조가 정해진 뒤 넣는다. |

**구현할 때 지킬 것**

- **8번 위치:** 후처리는 결과가 올 때만 돌기 때문에, 사이 프레임 예측은 `render()`가 읽을 때 현재 시각까지 계산한다.
- **뮤텍스:** 후처리, `render()`, 서영의 reset 호출이 같은 상태를 건드릴 수 있다.
- **키:** 트래커가 클래스별로 따로 있어 ID가 겹칠 수 있으므로 `(class, track_id)`로 잡는다.
- **오래된 궤적:** 검출 0개인 클래스는 트래커 갱신을 건너뛰므로, 프레임 수가 아니라 시간으로 지운다.
- **`dt`:** timestamp 차이로 계산하고, 0이면 update를 건너뛴다. LIGHT와 SKIP에서는 약 66ms가 된다.
- **지연값:** 앱 시작 후 1초 동안은 0이므로 기본값을 쓴다.
- **k:** NORMAL에서는 35ms ÷ 33ms ≈ 1이지만, 66ms 간격에서는 약 0.5다. 정수로 반올림하면 틀어지므로 시간 기준 예측도 함께 검토한다.
- **SKIP 모드:** PeleeDet은 자체 후처리에서 `nc_cnn_track()`을 부르므로 그쪽에도 호출이 필요하다.
- **좌표:** `bbox`의 x, y는 왼쪽 위 좌표이므로 `cx = x + w/2`로 변환한다. 그리기 코드 기준으로 픽셀 단위로 보이며, 로그로 확정한다.
- **임시 함수:** 서영의 드라이브에 `nc_c_stub.c`, `nc_meas.h`, `nc_model.h`, `nc_rec.h`가 있다. 지연은 35ms 고정, 모드는 항상 NORMAL이다.

## 정리할 것

- `.orig` 백업 파일 7개를 저장소에서 삭제한다. (`4377b00` 기준으로 아직 있음)
- `*.orig`, `*.rej`, `*.o`를 `.gitignore`에 추가한다.
- `이건 뭐지?` 커밋(`7de48fe`)의 `practice/` 변경이 필요한 내용인지 확인한다. 프로젝트 코드에는 영향이 없다.
