# ByteTrack 공유 사항

작성: 준이 · 2026-11-04 · 기준 코드: `juni33-bot/work` `4377b00` (master)

## 핵심

- ByteTrack을 켜서 화면에 `track_id`가 나오고, 같은 차량에서 ID가 유지되는 것을 확인했다.
- **ByteTrack을 켠 내 보드에서 GPU 오류가 반복되고, 커널이 멈춘 적도 한 번 있다.** 원인은 아직 모르고, ByteTrack 때문인지도 확인되지 않았다.
- 그래서 먼저 **각자 보드에서 같은 오류가 나는지 확인**이 필요하다.

## 각자 확인해 줄 것

### 서영, 준우 공통: GPU 오류 확인

평소처럼 앱을 2분 정도 돌린 뒤 실행한다.

```bash
dmesg | grep -cE "Unhandled Page fault|DATA_INVALID_FAULT"
```

아래 세 가지를 알려 주면 된다.

1. 나온 숫자
2. 그 빌드에서 `USE_BYTETRACK`을 켰는지
3. `VIS0_MAX_CH` 값

내 보드에서 나온 로그는 이런 형태다.

```text
mali 78e0000.gpu: error detected from slot 1, job status 0x00000058 (DATA_INVALID_FAULT)
mali 78e0000.gpu: Unhandled Page fault in AS1 at VA 0x0000005F2A290E40
kernel BUG at mm/slub.c:4020!
```

`kdb>`로 떨어지면 `go`로 계속하지 말고 재부팅해 줘.

### 서영: 트래커 재초기화 확인

모델을 바꿀 때 트래커를 다시 초기화한다고 했는데, 기존 함수를 그대로 쓰면 문제가 될 수 있다. 코드만 읽고 찾은 것이고, 서영 코드는 아직 못 봤다.

| 위치 | 동작 | 위험 |
| --- | --- | --- |
| `nc_bytetrack_wrapper.cpp` `nc_init_bytetracker()` | `bytetrackers_g.push_back()`으로 뒤에 추가 | 다시 초기화해도 `[cam_ch]`는 이전 트래커를 그대로 가리킨다. ID도 새로 시작하지 않을 수 있다. |
| 〃 `nc_destroy_byettracker()` | `delete`만 하고 항목을 지우지 않음 | 해제한 트래커를 계속 쓰게 될 수 있다. |
| `nc_cnn_tracker.c` `nc_deInit_bytetrackers()` | `tracked_objs_info_arr`를 `free`하고 NULL로 두지 않음 | 후처리 스레드가 해제된 메모리에 쓸 수 있다. |

확인 부탁:

1. 모델 전환 때 어떤 함수로 트래커를 초기화하는지
2. 전환 후 `track_id`가 실제로 새로 시작하는지
3. 후처리 스레드가 트래커를 쓰는 중에 초기화가 일어날 수 있는지

참고로 `nc_traj_reset()`은 아직 빈 함수다. 궤적 구현을 넣으면 뮤텍스를 쓸 예정이니, 호출 위치(어느 스레드인지)도 알려 주면 좋다.

### 준우: `VIS0_MAX_CH` 확인

내 코드에서 `VIS0_MAX_CH`를 `(1)`로 바꿨다. 보드의 카메라 1채널 설정에 맞춘 값이고, timestamp나 ByteTrack과는 별개다. 도로 영상 입력 작업과 겹치면 알려 줘.

## 수정할 코드

서영이 이미 손으로 반영한 내용과 겹칠 수 있으니, 각자 코드에 없는 것만 넣으면 된다. 줄 번호는 내 코드 기준이다.

### 1. ByteTrack 활성화

**`wayland_npu_app/Makefile:54`** 주석 해제

```makefile
USE_BYTETRACK       = yes
```

**`wayland_npu_app/wayland_npu_app.c:951`** `npu_init()`의 `#ifndef SHOW_PELEE_SEG` 바로 위에 추가

```c
#ifdef SHOW_TRI_CHIMERA
        net_id = NETWORK_TRI_CHIMERA;
#endif
```

`Makefile`만 켜면 안 된다. `TRICHIMERA` 설정에서 `net_id`가 초기화되지 않은 채 배열 인덱스로 쓰여서, 초기화 실패나 크래시가 날 수 있다.

### 2. `nc_traj` 빈 함수

**`common/nc_app_modules/cnn/core/nc_traj.h`** 새 파일

```c
#ifndef NC_TRAJ_H
#define NC_TRAJ_H

#ifdef __cplusplus
extern "C" {
#endif

void nc_traj_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* NC_TRAJ_H */
```

**`common/nc_app_modules/cnn/core/nc_traj.c`** 새 파일

```c
#include "nc_traj.h"

void nc_traj_reset(void)
{
    /* TODO: 궤적 상태 초기화 구현 */
}
```

**`wayland_npu_app/Makefile:331`** `ifeq ($(USE_BYTETRACK), yes)` 바깥에 추가

```makefile
CSRCS               += $(NC_CNN_CORE_DIR)/nc_traj.c
```

### 3. timestamp

**`wayland_npu_app/wayland_npu_app.c`**

```c
#include "nc_utils.h"                                   /* 67줄 */
uint64_t time_stamp_us = nc_get_mono_us_time();         /* 716줄, 기존 = 0 */
```

**`common/nc_app_modules/cnn/core/nc_cnn_aiware_runtime.h:89`** `cnn_output_info` 구조체 끝에 추가

```c
uint64_t time_stamp_us; // input frame time (monotonic, us)
```

**`common/nc_app_modules/cnn/core/nc_cnn_aiware_runtime.c:405`** 출력 텐서 반복문 바로 위에 추가

```c
cnn_output.time_stamp_us = time_stamp;
```

**후처리 4개 파일:** `uint64_t time_stamp = 0;`을 아래로 변경

```c
uint64_t time_stamp = cnn_output->time_stamp_us;
```

| 파일 (`common/nc_app_modules/cnn/networks/`) | 줄 |
| --- | --- |
| `yolov8/nc_cnn_yolov8_postprocess.c` | 230 |
| `ufld/nc_cnn_ufld_postprocess.c` | 715 |
| `nc_cnn_segmentation_postprocess.c` | 77 |
| `pelee/nc_cnn_pelee_postprocess.c` | 262 |

## 코드를 쓸 때 알아 둘 점

- **`track_id` 중복:** 트래커가 클래스별로 따로 있어서 클래스끼리 ID가 겹칠 수 있다. 객체를 구분할 때는 `(class, track_id)`로 잡아야 한다.
- **클래스가 바뀔 때:** 같은 차도 클래스 판정이 바뀌면 새 ID를 받을 수 있다.
- **`bbox` 좌표:** x, y는 중심이 아니라 왼쪽 위 좌표다.
- **`time_stamp`:** 단위는 us이고, 측정 시작점은 앱이 프레임을 꺼낸 시각이다.
- **YOLOv5:** `nc_cnn_yolov5_postprocess.c:226`은 아직 `time_stamp = 0`이다. 쓰게 되면 같은 방식으로 고쳐야 한다.

## 받을 때 주의할 점

`4377b00`에는 패치 백업 파일 7개(`*.orig`)와 `practice/` 변경이 함께 들어 있다. 프로젝트 빌드에는 영향이 없고, 곧 정리할 예정이다. 저장소에서 코드를 받기보다 위의 변경만 옮기는 것을 권한다.

## 다음 일정 (준이)

1. GPU 오류 원인 확인
2. 궤적 예측 본체 구현 (`nc_traj_reset()` 포함)
3. 구현 후 `nc_traj.c/h`를 다시 공유
