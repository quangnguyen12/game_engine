# 🚀 Unity Hub 3D Vulkan Engine

Chào mừng bạn đến với tài liệu hướng dẫn của **Unity Hub 3D Vulkan Engine**! Đây là một 3D Engine được phát triển hoàn toàn bằng **C++** và đồ họa **Vulkan API**, kết hợp với giao diện UI hiện đại mượt mà từ **Dear ImGui**. 

Dự án mang lại trải nghiệm phát triển mô phỏng lại giao diện quen thuộc của Unity Editor, cung cấp các tính năng mạnh mẽ từ Render bóng đổ thời gian thực (Real-time Shadows) cho đến khả năng tương thích định dạng mô hình 3D đa dạng.

---

## ✨ Tính Năng Nổi Bật

### 1. Giao Diện Người Dùng (Editor UI) Chuyên Nghiệp
- **Hierarchy Panel:** Quản lý danh sách các vật thể (Scene Objects) đang có trong cảnh. Hỗ trợ thêm/xóa/đổi tên vật thể dễ dàng.
- **Scene View & Game View:** 
  - **Scene View:** Không gian để thao tác, xoay góc nhìn camera (Kéo thả chuột, cuộn chuột) và chọn vật thể.
  - **Game View:** Góc nhìn thực tế từ Main Camera của trò chơi.
- **Inspector Panel:** Quản lý và điều chỉnh các thông số của Entity (GameObject). Tích hợp kiến trúc **Hybrid Component-Based System (ECS)** với các nút **`➕ Add Component`** và **`🗑️ Remove Component`** động!
- **Console / Project Logs:** Hiển thị trạng thái của hệ thống và cảnh báo.
- **Visual Profiler Panel:** Bảng giám sát hiệu năng trực quan thời gian thực bao gồm **FrameTime (ms)**, **FPS**, **CPU Usage (%)**, **RAM (Working Set MB)** và **VRAM (Vulkan Memory MB)** đi kèm với biểu đồ lịch sử sinh động (Realtime History Graphs).
- **Asset Browser & Drag-and-Drop System:** Cửa sổ quản lý tài nguyên dự án chuẩn phong cách Unity Editor/Unity Hub. Cho phép duyệt thư mục `assets/` (Model 3D `.obj`, `.glb`, `.gltf`, Texture `.png`, `.jpg`, Lua Script `.lua`), danh sách hình khối cơ bản (Primitives) và **bấm giữ kéo thả (Drag & Drop)** trực tiếp tài nguyên thả vào Scene View 3D! Khối 3D sẽ được định vị tự động theo tia 3D Raycasting tại vị trí thả chuột!
- **Component-Based Architecture (Entity-Component System - ECS / Hybrid):** Tách rời tính năng thành các mảnh ghép Component độc lập (`📌 Transform`, `🧊 Mesh Renderer`, `⚖️ RigidBody Physics`, `📖 Lua Script`, `💡 Light`). Cho phép thêm/xóa Component ở thời gian thực mượt mà!

### 2. Hệ Thống Đồ Họa Vulkan & Ánh Sáng
- **Shadow Mapping (Bóng đổ):** Ánh sáng định hướng (Directional Light) hỗ trợ đổ bóng thời gian thực lên các vật thể khác.
- **Directional Light Gizmo:** Hệ thống Gizmo trực quan hóa góc chiếu và độ phủ bóng đổ của nguồn sáng dưới dạng khối chóp (Frustum), giúp bạn định vị ánh sáng chuẩn xác như đang thao tác với Camera.
- **Ultra-Smooth 3D Raycasted Gizmo:** Công cụ điều khiển 3D trực quan mượt mà không khựng hay giật với thuật toán 3D Raycasting (Ray-Plane / Ray-Line Closest Point). Hỗ trợ đầy đủ Translate (Di chuyển), Rotate (Xoay), Scale (Co giãn), Rect Tool và Combined Transform.

### 3. Tương Thích Mô Hình 3D Đa Dạng
Engine sở hữu bộ giải mã mạnh mẽ cho phép bạn mang bất kỳ mô hình nào vào không gian 3D:
- **Nguyên mẫu cơ bản (Primitives):** Cube, Sphere, Capsule, Cylinder, Quad,...
- **Mô hình phức tạp (.OBJ):** Tích hợp qua `tinyobjloader`.
- **Mô hình hiện đại (.GLB / .GLTF):** Tích hợp qua `tinygltf`, hỗ trợ xử lý linh hoạt cấu trúc Node Hierachy, ma trận biến đổi vật thể (Transform Matrix), giải mã dữ liệu gộp (Interleaved Data) chính xác ở cấp độ Byte Stride.

---

## 🛠 Hướng Dẫn Sử Dụng (Dành cho Người Dùng)

### 🕹️ Điều Khiển (Controls) & Phím Tắt (Hotkeys)
- **Chuyển đổi Gizmo Tool:**
  - `Q`: **Hand Tool** (Chế độ điều hướng & xoay quan sát Scene View)
  - `W`: **Translate** (Di chuyển vật thể theo 3 trục X, Y, Z)
  - `E`: **Rotate** (Xoay vật thể theo các vòng tròn 3D 1:1 theo chuột)
  - `R`: **Scale** (Co giãn kích thước vật thể theo trục hoặc đồng dạng)
  - `T`: **Rect Tool** (Điều chỉnh kích thước mặt phẳng XZ)
  - `Y`: **Combined Tool** (Kết hợp di chuyển & xoay đồng thời)
- **Thao tác Chuột trong Scene View:**
  - **Chuột trái:** Click vào vật thể trong `Scene View` hoặc `Hierarchy` để chọn. Rê chuột qua các trục Gizmo sẽ có hiệu ứng sáng nổi bật (Highlight) và thay đổi con trỏ chuột. Bấm giữ chuột trái để kéo điều chỉnh mượt mà theo tia 3D Ray.
  - **Giữ Phím Ctrl khi kéo:** Kích hoạt chế độ Snap tự động (Snap vị trí `0.5m`, Snap góc xoay `15°`, Snap Scale `0.1`).
  - **Chuột phải (Giữ + Di chuyển):** Xoay góc nhìn Camera trong Scene View.
  - **Con lăn chuột:** Phóng to / Thu nhỏ (Zoom) Scene View.

---

## 💻 Hướng Dẫn Dành Cho Lập Trình Viên (Developers)

### 1. Kiến Trúc & Thư Viện Cốt Lõi
- **Đồ họa:** `Vulkan SDK`
- **Cửa sổ & Input:** `GLFW`
- **Toán học 3D:** `GLM`
- **Giao diện Editor:** `Dear ImGui` (Tích hợp docking nhánh `docking`)
- **Đọc mô hình 3D:** `tiny_gltf`, `tiny_obj_loader`
- **Trình mở file:** `tinyfiledialogs`

### 2. Biên Dịch Dự Án (Build)
Dự án sử dụng CMake để quản lý cấu hình.
1. Khởi tạo thư mục build: `cmake -B build`
2. Biên dịch dự án (Chế độ Release): `cmake --build build --config Release`
3. File chạy thực thi sẽ được tạo ra tại: `build/Release/ShapeRenderer.exe`

---

# 📖 Sổ Tay Hướng Dẫn Sử Dụng Unity Hub 3D Vulkan Editor

Bản hướng dẫn chi tiết "từ A-Z" này sẽ giúp bạn làm quen và làm chủ hoàn toàn các chức năng bên trong **Vulkan 3D Editor**. Giao diện phần mềm được thiết kế theo tư duy của Unity Engine để mang lại cảm giác thân thuộc và chuyên nghiệp.

---

## 1. Tổng Quan Giao Diện (UI Layout)
Khi mở phần mềm lên, màn hình của bạn sẽ được chia thành 5 khu vực chính:
1. **Hierarchy (Cây thư mục):** Bên trái. Hiển thị tất cả mọi vật thể (Object), ánh sáng, camera đang tồn tại trong thế giới 3D.
2. **Scene View (Màn hình thiết kế):** Ở giữa. Nơi bạn nhìn bao quát thế giới, tự do điều hướng và trực tiếp dùng chuột để sắp xếp vật thể.
3. **Game View (Màn hình Game):** Ở giữa (bên cạnh Scene). Góc nhìn thực tế từ con mắt của người chơi (Main Camera).
4. **Inspector (Bảng thuộc tính):** Bên phải. Hiển thị chi tiết và cho phép chỉnh sửa mọi thông số (tọa độ, kích thước, màu sắc,...) của vật thể bạn đang chọn.
5. **Console (Bảng nhật ký):** Dưới cùng. Nơi hệ thống in ra các thông báo, trạng thái tải file hoặc lỗi.

---

## 2. Di Chuyển Góc Nhìn (Navigation trong Scene View)
Để thao tác thoải mái trong không gian 3D, hãy ghi nhớ các phím tắt Camera:
- **Xoay góc nhìn:** Đưa chuột vào vùng `Scene View`, **Giữ Chuột Phải** và di chuyển chuột xung quanh.
- **Phóng to / Thu nhỏ (Zoom):** Sử dụng **Con Lăn Chuột** (Scroll Wheel) lướt lên hoặc lướt xuống.

---

## 3. Tạo Mới, Chọn và Xóa Vật Thể
- **Tạo vật thể cơ bản (Primitives):** 
  - Trong bảng `Hierarchy`, bấm vào nút **`+ Create`**. 
  - Một Menu sổ xuống sẽ hiện ra cho phép bạn thêm nhanh các hình khối cơ bản (Cube - Khối lập phương, Sphere - Hình cầu, Capsule - Viên nang,...).
- **Chọn vật thể:** 
  - Bấm **Chuột Trái** trực tiếp vào khối 3D trong `Scene View`. Khối được chọn sẽ được bọc bởi khung viền lưới (wireframe) và hệ thống trục Gizmo sẽ hiện ra.
  - Hoặc bấm chọn tên vật thể tương ứng ở bảng `Hierarchy`.
- **Xóa vật thể:** 
  - Chọn vật thể muốn xóa.
  - Nhìn sang bảng `Inspector` bên phải, cuộn xuống dưới cùng và bấm nút màu đỏ **`Delete Object`**.

---

## 4. Chỉnh Sửa Vật Thể (Sử dụng Gizmo & Inspector)
Khi một vật thể được chọn, bạn có 2 cách để điều chỉnh chúng:

### Cách 1: Sử dụng công cụ kéo thả trực quan (Gizmo) trong Scene View
Nhấn các phím tắt `W`, `E`, `R`, `T`, `Y` để chuyển đổi chế độ Gizmo mong muốn:
- **Di chuyển (Translate - Phím W):** Rê chuột lên các mũi tên (X: Đỏ, Y: Xanh lá, Z: Xanh dương), trục sẽ sáng rực lên. Bấm giữ **chuột trái** và kéo để di chuyển vật thể bám sát 1:1 theo con trỏ chuột mượt mà không hề bị giật hay lệch hướng.
- **Xoay (Rotate - Phím E):** Bấm giữ **chuột trái** vào các vòng tròn 3D (X: Đỏ, Y: Xanh lá, Z: Xanh dương). Di chuyển con trỏ chuột xoay quanh vòng tròn để xoay vật thể mượt mà theo đúng góc chuột.
- **Co giãn (Scale - Phím R):** Bấm giữ **chuột trái** vào khối vuông ở đầu các trục để co giãn chiều dài tương ứng, hoặc kéo khối tròn ở tâm để co giãn đồng dạng (Uniform Scale).
- **Kích hoạt Snap Grid (`Ctrl` + Kéo chuột):** Giữ phím `Ctrl` khi kéo Gizmo để di chuyển/xoay/co giãn theo các bước nhảy cố định chuẩn xác (`0.5m` cho Translate, `15°` cho Rotate, `0.1` cho Scale).

### Cách 2: Nhập số chính xác trong bảng Inspector
Nhìn sang bảng `Inspector` bên phải:
- **Tên:** Có thể gõ để đổi tên vật thể (Ví dụ: `Player`, `Quái vật`,...).
- **Khối Transform:** Chỉnh sửa tọa độ `Position`, góc xoay `Rotation` và tỷ lệ `Scale`. Nhấn đúp chuột vào ô số để gõ hoặc bấm giữ chuột trái vào ô số và kéo sang trái/phải để tăng giảm từ từ.
- **Khối Mesh Settings (Màu sắc):** Bấm vào dải màu bên cạnh chữ "Mesh Color", một bảng chọn màu (Color Picker) sẽ hiện lên để bạn pha màu tự do cho vật thể.

---

## 5. Ánh Sáng và Đổ Bóng (Lighting & Shadows)
Engine có hệ thống đổ bóng thời gian thực siêu mượt. Mặc định luôn có một nguồn sáng tên là **Directional Light** trong Hierarchy.
- **Tìm tia sáng:** Chọn `Directional Light`, bạn sẽ thấy trong Scene View xuất hiện một chiếc đèn bọc trong một "hình chóp" màu xanh/đỏ (Gizmo) mô phỏng góc chiếu, đi kèm là tia sáng lao thẳng xuống mặt đất (`y = 0`).
- **Thay đổi hướng nắng:** Hãy dùng vòng xoay Gizmo hoặc chỉnh phần `Rotation` trong Inspector để nghiêng cái đèn. Hướng của tia nắng sẽ thay đổi và **bóng đổ của toàn bộ các vật thể khác cũng sẽ nghiêng theo** một cách chân thực nhất.

---

## 6. Nhập Mô Hình 3D Ngoại Vi (.OBJ, .GLB, .GLTF)
Nếu khối vuông, khối tròn là chưa đủ, bạn hoàn toàn có thể mang các mô hình hoành tráng tải từ mạng vào:
1. Chọn một vật thể đang có (Hoặc tạo mới 1 Cube).
2. Ở bảng `Inspector`, kéo xuống phần **Asset Loading**.
3. Bấm nút **`Load 3D Mesh (.obj, .glb, .gltf)`**.
4. Chọn file tải về từ máy. Hệ thống sẽ ngay lập tức vẽ mô hình đó (bao gồm cả góc cạnh cực kỳ chi tiết) thế chỗ cho hình dáng cũ của vật thể!
5. Bấm nút **`Load Material Texture`** nếu mô hình của bạn có đi kèm file ảnh `.png` dán bề mặt (Ví dụ vân gỗ, kim loại,...).

---

## 7. Chế Độ Chơi (Play Mode)
Nhìn lên trên cùng của phần mềm, bạn sẽ thấy thanh Toolbar.
- Mặc định bạn đang ở **`[ EDIT MODE ]`**.
- Bấm vào nút **`[ PLAY ]`**, phần mềm sẽ chuyển sang chế độ chơi thực tế. Hệ thống vật lý Gravity (nếu được kích hoạt) sẽ thả rơi các đồ vật. Khung hình bên Game View sẽ kích hoạt hoạt ảnh của người chơi. Để quay lại chỉnh sửa, hãy bấm `[ STOP ]`.
