# AI Robot — Manager State Ownership & Robot Orchestrator

> **Loại tài liệu:** Architecture Decision Note (định hướng thiết kế)
> **Ngày ghi nhận:** 2026-11-08
> **Trạng thái:** Đã thống nhất về nguyên tắc qua thảo luận; interface, danh sách state và cách triển khai cụ thể sẽ được review ở feature tương ứng.
> **Vị trí đề xuất trong repository:** `docs/project/robot-manager-orchestrator-architecture.md`

## 1. Mục tiêu

Xây dựng **Voice Command Robot MVP** với ba cụm chức năng độc lập, mỗi cụm có một Manager; phía trên là **Robot Orchestrator** điều phối hoạt động tổng thể. Kiến trúc phải có khả năng mở rộng về sau thành **Conversational AI Robot**, nhưng không thiết kế quá mức cho MVP.

```text
                     +-----------------------+
                     |   Robot Orchestrator  |
                     |    Global Workflow    |
                     +-----------+-----------+
                                 |
                +----------------+----------------+
                |                |                |
        +-------v--------+ +-----v-----------+ +--v--------------+
        | AudioManager   | | CommandManager  | | ActionManager    |
        +----------------+ +-----------------+ +-----------------+
        | Capture (F002) | | Recognition /   | | Action dispatch |
        | Playback (F001)| | command parsing | | Device control  |
        | Audio I/O      | | Command validate| | Execution result|
        +----------------+ +-----------------+ +-----------------+
```

**Lưu ý:** Vị trí cụ thể của ASR/STT (bên trong CommandManager hay qua một interface/adapter riêng) **chưa được chốt**. Sơ đồ thể hiện trách nhiệm xử lý command ở mức logic, không áp đặt implementation.

## 2. Quyết định kiến trúc chính: state ownership

**Mỗi Manager tự quản lý trạng thái nội bộ; Robot Orchestrator chỉ quản lý trạng thái và workflow tổng thể.**

- **Orchestrator** quyết định *khi nào* bắt đầu nghe, xử lý command, thực thi action, phản hồi và phục hồi khi xảy ra lỗi.
- **Manager** quyết định *cách* thực hiện operation, kiểm tra tính hợp lệ của thao tác theo trạng thái cục bộ và quản lý tài nguyên thuộc subsystem.
- Orchestrator **không quản lý chi tiết** ALSA PCM handle, audio buffer, WAV finalization, codec registers, v.v.
- Manager **không tự ý điều phối Manager khác**; các bước liên subsystem được khởi tạo từ Orchestrator.

### State ở từng tầng (ví dụ, chưa chốt enum)

| Component | State đề xuất | Ranh giới quản lý |
|---|---|---|
| Robot Orchestrator | `IDLE`, `LISTENING`, `PROCESSING`, `EXECUTING`, `RESPONDING`, `ERROR` | Trạng thái nghiệp vụ và thứ tự hoạt động của robot |
| AudioManager | `IDLE`, `CAPTURING`, `PLAYING`, `ERROR` | Capture/playback session, PCM, buffer, tài nguyên audio |
| CommandManager | `IDLE`, `RECOGNIZING`, `PARSING`, `ERROR` | Nhận dạng/diễn giải/kiểm tra command; các state sẽ điều chỉnh theo thiết kế ASR |
| ActionManager | `IDLE`, `RUNNING`, `COMPLETED`, `ERROR` | Thực thi action, trạng thái thiết bị và kết quả thực thi |

**Nguyên tắc giao tiếp:** Orchestrator gọi API ở mức operation và nhận **result/status/event**; không điều khiển các bước nội bộ của Manager bằng cách trực tiếp sửa local state.

## 3. Luồng Voice Command MVP — ví dụ “Robot, bật đèn”

| Bước | Global state | Manager và operation | Kết quả chuyển bước |
|---|---|---|---|
| 1 | `LISTENING` | AudioManager capture âm thanh | `AudioReady` hoặc lỗi |
| 2 | `PROCESSING` | CommandManager nhận và phân tích command | `TURN_ON_LIGHT` hoặc không hiểu/lỗi |
| 3 | `EXECUTING` | ActionManager thực hiện lệnh bật đèn | `ActionSucceeded` hoặc lỗi |
| 4 | `RESPONDING` | AudioManager phát phản hồi | `PlaybackFinished` hoặc lỗi |
| 5 | `IDLE` | Các Manager trở về trạng thái sẵn sàng phù hợp | Chờ lệnh tiếp theo |

Ví dụ ở mức **pseudocode** (chưa phải C++ interface đã phê duyệt):

```cpp
auto audio   = audio_manager.capture();
auto command = command_manager.process(audio);
auto result  = action_manager.execute(command);
audio_manager.play_response(result);
```

Luồng thực tế phải **kiểm tra kết quả sau từng bước**: capture lỗi thì không xử lý command; command không hợp lệ thì không thực thi action; action lỗi thì Orchestrator quyết định cách phản hồi hoặc recovery.

## 4. Phân chia trách nhiệm khi có xung đột hoặc lỗi

| Tình huống / câu hỏi | Thành phần chịu trách nhiệm |
|---|---|
| Khi nào robot bắt đầu nghe hoặc dừng nghe? | Orchestrator |
| Có cho phép capture trong lúc playback không? | Orchestrator quyết định **policy**; AudioManager kiểm tra khả năng phần cứng và trạng thái hiện tại |
| Mở/đóng PCM, quản lý WAV, buffer, ALSA error | AudioManager |
| Nhận dạng và diễn giải lệnh | CommandManager (ranh giới tích hợp ASR sẽ quyết định sau) |
| Command có hợp lệ và được hỗ trợ không? | CommandManager xác thực; Orchestrator quyết định có chuyển sang thực thi hay không |
| Làm thế nào để điều khiển thiết bị và thu kết quả? | ActionManager |
| Một operation thất bại thì retry, phản hồi hay về IDLE? | Orchestrator quyết định recovery ở mức robot; Manager báo lỗi và tự dọn tài nguyên của mình |

## 5. Nguyên tắc triển khai MVP

1. **Subsystem trước, Orchestrator sau:** Hoàn thiện capability độc lập, rồi bọc vào Manager API; Orchestrator dùng các API đó để kết nối workflow.
2. **Synchronous trước khi cần asynchronous:** MVP có thể dùng operation blocking và gọi tuần tự; chưa cần event bus, hệ thống đa luồng hoặc state machine phức tạp nếu chưa có yêu cầu thực tế.
3. **Giữ encapsulation:** Không expose chi tiết ALSA/libsndfile hoặc state nội bộ lên Orchestrator; chỉ expose hành động, kết quả và trạng thái cần thiết.
4. **Không overengineering:** Các state bên trên là minh họa, không bắt buộc tạo đầy đủ nếu flow hiện tại đơn giản hơn.
5. **Thiết kế có đường mở rộng:** Phần Audio có thể chuyển từ WAV sang PCM streaming; phần Command có thể tích hợp ASR và xử lý hội thoại; Action có thể thêm thiết bị thật; Orchestrator chỉ nên thay đổi khi workflow cấp robot thay đổi.

## 6. Tình trạng dự án và việc cần quyết định sau

- **F001 Audio Playback:** capability đã triển khai và kiểm chứng.
- **F002 Audio Capture:** capability đã triển khai và kiểm chứng.
- **AudioManager:** chưa được xem là hoàn thành chỉ vì có F001/F002; cần interface để quản lý hoạt động và trạng thái ở mức subsystem.
- **CommandManager, ActionManager và Robot Orchestrator:** chưa có thiết kế/implementation MVP được phê duyệt trong ghi chú này.

**Các quyết định dành cho Phase 1 của những feature sau:**

- Hình thức dữ liệu giao giữa AudioManager và CommandManager (WAV path, PCM buffer hay stream).
- Ranh giới ASR/STT và CommandManager.
- Operation interface, error/result types và state tối thiểu cho từng Manager.
- Action phần cứng tối thiểu để chứng minh end-to-end; ví dụ LED/GPIO trước khi triển khai motion phức tạp.
- Policy cho capture/playback overlap, timeout, retry và recovery trong Orchestrator.

## 7. Tiêu chí thành công của kiến trúc MVP

Robot có thể nhận một câu lệnh bằng giọng nói, xử lý thành command, thực thi một hành động quan sát được và phát phản hồi. Việc này phải diễn ra qua **AudioManager → CommandManager → ActionManager**, dưới sự điều phối của **Robot Orchestrator**, với trách nhiệm và trạng thái của từng tầng rõ ràng.

> **Tóm tắt quyết định:** *Orchestrator owns the robot workflow. Managers own their internal state and resources.*
