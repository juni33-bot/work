# 궤적 예측(칼만) 합치기: 서영·준우 전달 사항

2026-11-04 · 준이 · 자세한 작업 기록은 `1104_kal.md`

## 0. 한눈에 보기

| 누가 | 할 일 | 급한 정도 |
| --- | --- | --- |
| 준이 | 보내기 전에 이름 충돌 정리 (1장) | 먼저 |
| 서영 | 후처리에 `nc_traj_update` 넣기, render 루프에 예측 호출, tracker 설정 1줄, FSM에 프레임 시각 갱신 (2장) | 높음 |
| 준우 | 입력 코드에 프레임 시각 갱신 줄 유지, 내 차선 영역 함수 (3장) | 중간 |

**예측 원리**: 후처리 스레드에서 측정값으로 칼만을 갱신하고, render 스레드에서 **지금 화면에 띄우는 프레임의 캡처 시각**까지 예측한 bbox를 그린다. 그래서 k프레임 값을 따로 넘길 필요가 없다. 마지막 측정 뒤 지난 시간만큼 알아서 예측한다(최대 120ms). LIGHT·SKIP 모드의 66ms 간격도 그대로 맞는다.

---

## 1. 준이가 보내기 전에 정리할 것

서영 쪽과 이름이 같은 파일·함수가 있어서, 그대로 합치면 링크 에러가 나거나 엉뚱한 헤더가 잡힌다.

| 준이 파일 | 서영 쪽 | 문제 | 정리 |
| --- | --- | --- | --- |
| `core/nc_meas.c/h` | `nc_meas.h` (`nc_meas_get_latency_ms(void)`) | 함수 이름은 같고 인자가 다름(준이 쪽 `(uint32_t ch)`) → 링크 에러 | **서영 것으로 통일.** 준이 파일 삭제, `nc_meas_on_pipeline_done` / `nc_meas_on_display` 호출 4곳 삭제 |
| `core/nc_model.c/h` (`nc_model_is_vehicle`) | `nc_model.h` (모드 FSM) | 파일 이름 충돌 | `nc_class.c/h`, `nc_class_is_vehicle()`로 이름 변경 |
| `core/nc_run_mode.c/h` (`-l` 옵션) | 서영 모델 전환 FSM | 같은 기능이 두 개 | **서영 FSM으로 통일.** 준이 파일 삭제 |
| `nc_cnn_yolov8_postprocess.c:216` `[TRK]` printf | — | 매 프레임, 객체마다 출력해서 느려짐 | 삭제 또는 `#ifdef NC_TRAJ_DEBUG` |

정리한 뒤 보낼 파일: `nc_traj.c/h`, `nc_class.c/h`, `nc_cnn_tracker.c` 수정분, 그리고 아래 코드 조각.

---

## 2. 서영

### 2-1. 파일·Makefile
`common/nc_app_modules/cnn/core/`에 `nc_traj.c/h`, `nc_class.c/h`를 넣는다. `nc_traj.c/h`는 전에 받은 것과 API가 달라졌으니 **새 파일로 덮어써 줘.**

```make
CSRCS += $(NC_CNN_CORE_DIR)/nc_traj.c
CSRCS += $(NC_CNN_CORE_DIR)/nc_class.c
```

`nc_class`는 network ini의 `class_name`(car, bus, tru…)으로 차량인지 판단한다. TRICHIMERA, PeleeDet, YOLOv8(COCO)처럼 **class 번호가 모델마다 달라도 변환표가 필요 없다.**

### 2-2. 후처리: 측정값 넣기
`nc_cnn_track()` 바로 다음 줄에 넣는다(`#ifdef USE_BYTETRACK` 안). 파일 3개:
- `networks/yolov8/nc_cnn_yolov8_postprocess.c` (TRICHIMERA 객체 결과도 여기를 지나감)
- `networks/pelee/nc_cnn_pelee_postprocess.c` (SKIP 모드)
- `networks/yolov5/nc_cnn_yolov5_postprocess.c`

```c
#include "nc_traj.h"
...
        nc_cnn_track(out_cnn_results, cnn_output->net_id, cnn_output->cam_ch);
        // trajectory: measurement update only (prediction is done in render loop)
        nc_traj_update((int)cnn_output->cam_ch, cnn_output->net_id,
                       out_cnn_results, cnn_output->time_stamp_us);
```

- 결과를 읽기만 한다. 예측값이 tracker로 다시 들어가지 않는다.
- **yolov5 버그 수정도 같이 넣어줘.** `postprocess_yolov5_detections()`에 `uint64_t time_stamp = 0;`이 남아 있어서 yolov5 결과에 캡처 시각이 안 들어간다.
  ```c
  uint64_t time_stamp = cnn_output->time_stamp_us;
  ```

### 2-3. render 루프 (`wayland_npu_app.c`)

**① include + 전역 변수** (`nc_draw_gl_npu()` 위)
```c
#include "nc_traj.h"

// capture timestamp (monotonic us) of the video frame currently displayed per channel.
// Written and read only in the render thread.
static uint64_t g_disp_frame_ts_us[VIDEO_MAX_CH];
```

**② DQBUF 직후, 매 프레임 갱신.** NPU로 보낼지 정하는 조건문의 **밖**에 둬야 한다.
```c
            }else   // DQBUF 성공
            {
                // capture(DQBUF) time of the frame shown this render (also NPU input ts)
                uint64_t frame_ts_us = nc_get_mono_us_time();
                g_disp_frame_ts_us[i] = frame_ts_us;      // ← 모든 프레임
#if (VIDEO_MAX_CH == 4)
                if (...기존 4채널 교대 조건...)
#endif
                if (/* 서영 FSM: 이번 프레임을 NPU로 보낼지 */)
                {
                    uint64_t time_stamp_us = frame_ts_us;  // NPU 입력 시각도 같은 값
                    ...
```
> LIGHT·SKIP에서 NPU로 안 보내는 프레임도 화면에는 나간다. 이 시각이 매 프레임 바뀌어야 사이 프레임을 예측으로 채울 수 있다.

**③ 검출 결과 그리기 직전**
```c
            if (det_buf) {
                // predict tracks to the capture time of the displayed video frame
                // (one short lock per frame), then draw. Runs before swap.
                nc_traj_predict_frame((int)ch, g_disp_frame_ts_us[ch]);
                nc_draw_gl_npu(g_viewport[ch], det_buf->net_task, det_buf, g_npu_prog);
                // (서영 측정 함수 호출은 여기 그대로)
            }
```

**④ `nc_draw_gl_npu()` DETECTION 분기, 사각형 그리기 직전**
```c
                stObjInfo obj_info = det_result->class_objs[i].objs[bidx];
                // predicted box if available (vehicle, enough history), else measured box.
                // Only the drawn copy changes; det_result / tracker input stay measured.
                if (net_result->cam_channel < VIDEO_MAX_CH) {
                    stBBox pred_box;
                    if (nc_traj_get_predicted_box((int)net_result->cam_channel, i, obj_info.track_id,
                                                  g_disp_frame_ts_us[net_result->cam_channel], &pred_box)) {
                        obj_info.bbox = pred_box;
                    }
                }
                nc_opengl_draw_rectangle(obj_info.bbox.x, obj_info.bbox.y, obj_info.bbox.w, obj_info.bbox.h, color[i], g_npu_prog);
```
- 차량이 아니거나, `track_id < 0`이거나, 측정이 3번 미만이면 0을 돌려준다. 이때는 측정 bbox를 그대로 그린다.
- 예측 → 그리기 → swap(`render()` 끝 `nc_wayland_display_draw`) 순서라서 swap 전에 예측이 끝난다.

### 2-4. ByteTrack 설정
`core/nc_cnn_tracker.c`, `nc_init_bytetrackers()`:
```c
#define BYTETRACK_TRACK_BUFFER  (15)  /* -> 0.5 s lost-track tolerance */
...
    /* ByteTrack: max_time_lost(frames) = fps/30 * track_buffer.
     * Fixed track_buffer keeps lost-track tolerance at 0.5 s for any fps */
    return nc_init_bytetracker(fps, BYTETRACK_TRACK_BUFFER, net_info->class_id, net_info->class_num, cam_ch);
```
기존 `fps/2`는 fps가 두 번 곱해지는 문제가 있다. 15fps로 초기화하면 0.2초만 지나도 ID가 끊긴다. 30fps에서는 이전과 같다.

### 2-5. 서영 FSM에 부탁할 것
- **모드가 바뀔 때 `nc_traj_reset()` 호출.** 이미 확인했다고 들었는데 새 파일에서도 그대로 동작한다.
- **tracker fps**: tracker는 시작할 때 fps로 초기화된다. 실행 중에 LIGHT·SKIP으로 바꾸면 결과 간격은 66ms인데 tracker는 30fps로 알고 있다. 그래서 ID 유지 시간이 0.5초에서 약 1초로 늘어난다. 동작에는 문제없다. 정확히 맞추려면 모드를 바꿀 때 tracker를 다시 초기화해야 한다.
- **지연값 공유 부탁.** 예측 상한(지금 120ms)을 정하려면 실기 지연값이 필요하다. 시뮬레이션에서는 LIGHT·SKIP일 때 지연이 약 54ms를 넘으면 120ms로는 사이 프레임 끊김이 남는다. 모드별 `nc_meas_get_latency_ms()` 값을 알려주면 `nc_traj_set_max_predict_ms()`로 맞출게.

---

## 3. 준우

### 3-1. 입력 코드 겹침
준이도 테스트하려고 `wayland_npu_app.c` 입력 부분을 고쳤다. 준우 담당(도로 영상 입력)과 같은 곳이다.

| 준이 변경 | 정리 |
| --- | --- |
| `VIS0_MAX_CH` 4 → 1 | 준우 계획과 같음. 한쪽만 반영 |
| `USE_FILE_INPUT` (ffmpeg `popen`으로 mp4 → RGB24, `-i <파일>`) | v4l2loopback이 되면 빼도 됨. Makefile에서 끄면 기존 카메라 경로 그대로 |

**어느 입력을 쓰든 프레임을 받은 직후 `g_disp_frame_ts_us[i] = frame_ts_us;` 줄(2-3 ②)은 꼭 남겨줘.** 파일 입력이면 `file_input_read_frame(i) == 0` 바로 안쪽이다. 이 줄이 빠지면 예측이 멈춘다.

### 3-2. 차선 쪽 시간·지연값
- 차선 결과에도 입력 timestamp가 있다(`nc_cnn_ufld_postprocess.c:715`, `pp_buf->time_stamp`). 옆으로 움직이는 속도를 구할 때 dt를 고정값 대신 **timestamp 차이**로 쓰면 LIGHT·SKIP에서도 맞는다.
- 지연값은 **서영 함수** `nc_meas_get_latency_ms()`(인자 없음)를 쓴다.

### 3-3. 끼어들기용 내 차선 영역 (요청)
끼어들기 판단에 "y 높이에서 내 차선의 왼쪽·오른쪽 x"가 필요하다. 이름이나 형태는 편한 대로 바꿔도 된다.
```c
/* nc_ldw.h
 * y(bbox와 같은 화면 좌표)에서 내 차선 좌우 경계 x.
 * 1: 성공, 0: 차선 없음(오래된 결과, 한쪽만 검출).
 * render 스레드에서 부르니까 안에서 뮤텍스나 스냅샷으로 보호해줘. */
int nc_ldw_get_ego_lane_x(int ch, float y, float *x_left, float *x_right, uint64_t *lane_ts_us);
```
준이는 예측 bbox 아래쪽 가운데 `(x + w/2, y + h)`를 넣어서 `x_left < cx < x_right`인지 본다.

### 3-4. 같은 함수를 고칠 때
`nc_draw_gl_npu()`의 **DETECTION 분기**는 준이가 고쳤다(2-3 ④). 차선 이탈 표시는 LANE 분기나 화면 위쪽 텍스트에 넣으면 안 겹친다.

### 3-5. (미니맵을 넘겨받으면) 쓸 함수
```c
// 예측 bbox (화면 좌표, x,y = 왼쪽 위). class_id가 꼭 필요함: track_id는 class마다 따로 매겨짐
int nc_traj_get_predicted_box(int ch, int class_id, int track_id, uint64_t target_time_us, stBBox *out);
// 지나온 중심점 기록, 최대 16개 (오래된 것부터). 미니맵 궤적선용
int nc_traj_get_history(int ch, int class_id, int track_id, stTrajPoint *out, int max_cnt);
```
`get_predicted_box`는 render 스레드에서 `nc_traj_predict_frame()` 다음에 불러야 한다. `get_history`는 어느 스레드에서 불러도 된다.

---

## 4. 같이 알아둘 것
- **track_id는 class마다 따로 매겨진다.** ByteTrack이 (채널, class)마다 하나씩 돌아서 car #1과 bus #1이 동시에 있을 수 있다. track_id로 뭔가를 찾을 때는 꼭 class_id와 같이 쓴다.
- **`-f`/`-o` 옵션이 원래부터 안 먹는다.** `main()`이 인자 파싱 뒤에 `memset(&window, 0, ...)`을 한다(`wayland_npu_app.c` 약 1195행). 전체 화면이 안 되면 이것 때문이다. 고칠지 정해야 한다.

## 5. 합친 뒤 확인

| 확인 | 방법 |
| --- | --- |
| 빌드 | `nc_meas_*`, `nc_model*` 중복 정의 에러가 없어야 함 |
| 차량 class | 시작 로그 `[nc_class] net_id:N vehicle classes: ...`가 모델마다 맞는지 (TRICHIMERA는 아직 확인 못 함) |
| 예측 동작 | 차가 옆으로 움직일 때 bbox가 영상보다 늦지 않은지. LIGHT·SKIP 사이 프레임에서도 bbox가 2프레임씩 멈추지 않고 부드러운지 |
| 모드 전환 | 전환 직후 측정 bbox로 돌아왔다가 3번 측정된 뒤 예측으로 넘어가는지 |
| 지연값 | 모드별 `nc_meas_get_latency_ms()` 값을 준이에게 공유 (예측 상한 조정용) |
