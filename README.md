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

### 4. Hệ Thống Sinh Cảnh 3D Tự Động (Procedural 3D Scene Generator)
Dự án được trang bị công cụ **Sinh Cảnh 3D (3D Scene Generator)** chuyên nghiệp cho phép sinh tự động địa hình 3D, phân bố vật thể, cây cối, đá, tòa nhà và hệ thống chiếu sáng chỉ với **1-Click**:
- **Tạo Địa Hình 3D Procedural (`createTerrainMesh`):** Thuật toán tính toán độ cao đa tần số (Multi-octave noise) sinh địa hình núi đồi, thung lũng, sa mạc, mê cung 3D mượt mà với Normal Vectors và dải màu đa dạng.
- **6 Mẫu Cảnh (Presets) Đa Dạng:**
  - 🌲 **Rừng & Núi Đồi (Forest & Mountain Landscape):** Địa hình đồi núi, rừng cây thông 3D (Cylinder trunk + Cone foliage), đá tảng rải rác trên mỏm núi.
  - 🏜️ **Sa Mạc & Cổ Tích (Desert Dunes & Ruins):** Sa mạc cát vàng rolling dunes, Kim Tự Tháp 3D trung tâm, cột đá cổ obelisk rải rác.
  - 🏙️ **Thành Phố Tương Lai (Sci-Fi Cyberpunk City Grid):** Mặt đường nhựa, mạng lưới tòa nhà chọc trời độ cao ngẫu nhiên, hệ thống đèn neon rực rỡ.
  - 🏰 **Ngôi Làng & Lâu Đài Citadel (Medieval Village):** Lâu đài trung tâm, 4 tháp canh, nhà thờ nhỏ và nhà dân xung quanh.
  - 🌌 **Quần Đảo Phao (Floating Islands Archipelago):** Các hòn đảo đá lơ lửng trên không trung kèm tinh thể năng lượng phát sáng.
  - 🌀 **Mê Cung 3D (3D Dungeon Maze):** Thuật toán DFS tạo mê cung tường đá 3D, ngọn đuốc thắp sáng hành lang.
- **Tương Thích & Sử Dụng Model 3D Thực Tế (.GLB / .OBJ / .GLTF):** Đã tích hợp sẵn các mẫu Model 3D thực tế trong thư mục `assets/models/` (`Fox.glb`, `Duck.glb`, `CesiumMan.glb`, `DamagedHelmet.glb`, `Lantern.glb`, `Avocado.glb`). Engine cho phép tải thêm **bất kỳ file 3D `.glb`, `.gltf` hoặc `.obj`** tải trên mạng từ các trang web như **Sketchfab, Poly Pizza, Kenney.nl, CGTrader, itch.io** và kéo thả trực tiếp vào không gian 3D hoặc dùng trong Sinh Cảnh!
- **Bảng Điều Khiển "🏞️ Sinh Cảnh 3D":** Tự do bật/tắt **"🌐 Sử Dụng Objects 3D Thực Tế (.GLB / .OBJ)"**, tùy chỉnh **Seed (Hạt giống)**, **Grid Size (Kích thước)**, **Height Scale (Độ cao)**, **Density (Mật độ)**, **Sunlight (Góc & màu nắng)** và xuất scene trực tiếp.

### 5. Plugin Tải Model 3D Trực Tuyến Từ API / URL (`📥 Online Model Downloader`)
Đã tích hợp Plugin tải tự động model 3D trực tiếp ngay trong ứng dụng:
- **Tải từ Đường Dẫn Direct URL:** Dán bất kỳ URL trực tiếp tới file 3D (`.glb`, `.gltf`, `.obj`), bấm nút **`📥 TẢI MODEL VỀ ASSETS & THẢ VÀO 3D SCENE`**, hệ thống sẽ tải ngầm (background thread), lưu thẳng vào `assets/models/` và spawn vật thể lập tức lên màn hình 3D!
- **Kho Catalog Model 3D Miễn Phí (1-Click Download):** Danh mục mẫu có sẵn các model chất lượng cao (Cây thông 3D, Cáo, Vịt, Đèn lồng cổ, Mũ bảo hiểm PBR, Quả bơ, Nhân vật Cesium Man). Bấm **`📥 1-Click Download & Import`** để tải về và tự động tạo vật thể trong scene!

---

## 📦 Hướng Dẫn Sử Dụng Bộ Cài Đặt (Installation & Portable Package)

Bộ cài đặt của **Unity Hub 3D Vulkan Engine** được thiết kế dưới dạng **Portable (Chạy ngay không cần cài đặt rườm rà)**. Bạn chỉ cần giải nén và mở file thực thi là có thể trải nghiệm ngay lập tức.

### 1. 🖥️ Yêu Cầu Hệ Thống (System Requirements)
| Thành phần | Yêu cầu tối thiểu | Yêu cầu khuyến nghị |
| :--- | :--- | :--- |
| **Hệ Điều Hành** | Windows 10 (64-bit) Version 1909+ | Windows 10/11 (64-bit) bản mới nhất |
| **Bộ Xử Lý (CPU)** | Intel Core i3 / AMD Ryzen 3 trở lên | Intel Core i5 / AMD Ryzen 5 trở lên |
| **Bộ Nhớ (RAM)** | 4 GB RAM | 8 GB RAM trở lên |
| **Card Đồ Họa (GPU)** | GPU hỗ trợ **Vulkan 1.2+** (NVIDIA GTX 900+, AMD RX 400+, Intel UHD 620+) | NVIDIA GTX 1060 / AMD RX 580 / RTX 2060+ |
| **Driver GPU** | Đã cài driver card màn hình bản mới nhất | Driver Game Ready / Studio mới nhất từ NVIDIA / AMD / Intel |
| **Thư Viện Phụ Thuộc** | [Microsoft Visual C++ 2015-2022 Redistributable (x64)](https://aka.ms/vs/17/release/vc_redist.x64.exe) | Bắt buộc phải có trên máy |

---

### 2. 📂 Cấu Trúc Thư Mục Của Bộ Cài Đặt
Khi giải nén file bộ cài (`UnityHub3D_Vulkan_Portable.zip`) hoặc thư mục phân phối `dist/UnityHub3D_Vulkan/`, bạn sẽ thấy các thành phần quan trọng sau:

```text
UnityHub3D_Vulkan/
├── 🚀 ShapeRenderer.exe       # File chạy chính của Game Engine (Nhấp đúp để mở)
├── ⚙️ glfw3.dll               # Thư viện quản lý cửa sổ hiển thị và tương tác chuột/phím
├── 📜 lua.dll                 # Bộ máy thực thi kịch bản gameplay Lua Engine
├── 🎨 vert.spv                # Shader đỉnh (Vertex Shader) đã biên dịch SPIR-V
├── 🎨 frag.spv                # Shader điểm ảnh (Fragment PBR Shader) đã biên dịch
├── 🌑 shadow_vert.spv         # Shader tính toán bóng đổ thời gian thực (Shadow Map)
├── 🎛️ imgui.ini               # File lưu bố cục vị trí các cửa sổ giao diện Editor
├── 📄 HUONG_DAN_SU_DUNG.txt   # File hướng dẫn tóm tắt nhanh đính kèm
└── 📁 assets/                 # THƯ MỤC TÀI NGUYÊN BẮT BUỘC
    ├── 📁 models/             # Chứa các mô hình 3D (.glb, .gltf, .obj như Fox, Duck, rock,...)
    ├── 📁 Texture/            # Chứa vân bề mặt vật liệu PBR, địa hình cỏ, đá, vỏ cây
    ├── 📁 textures/           # Thư mục phụ trợ texture
    └── 📁 scripts/ (hoặc .lua) # Các file mã nguồn kịch bản Lua (Player movement, Patrol,...)
```

> [!IMPORTANT]
> **Quy tắc vàng:** File thực thi `ShapeRenderer.exe`, các file `.dll`, các file `.spv` và thư mục `assets/` **PHẢI LUÔN NẰM CHUNG MỘT THƯ MỤC**. Không được kéo riêng file `ShapeRenderer.exe` ra màn hình Desktop (nếu muốn, hãy nhấp chuột phải vào file exe và chọn *Send to -> Desktop (create shortcut)*).

---

### 3. 🚀 Các Bước Cài Đặt Và Khởi Chạy Từng Bước

#### Bước 1: Chuẩn bị môi trường (Chỉ cần làm 1 lần)
1. Tải và cài đặt **Microsoft Visual C++ 2015-2022 Redistributable (x64)** từ trang chủ Microsoft:
   👉 **[Tải vc_redist.x64.exe tại đây](https://aka.ms/vs/17/release/vc_redist.x64.exe)**
2. Đảm bảo driver card màn hình (NVIDIA / AMD / Intel) đã được cập nhật phiên bản mới nhất để hỗ trợ đầy đủ tập lệnh đồ họa **Vulkan 1.2+**.

#### Bước 2: Tải và Giải Nén Bộ Cài
1. Tải gói cài đặt **`UnityHub3D_Vulkan_Portable.zip`**.
2. Nhấp chuột phải vào file `.zip` -> chọn **Extract All...** (Giải nén tất cả).
3. Chọn thư mục lưu trữ (Khuyến nghị lưu vào đường dẫn không có dấu tiếng Việt, ví dụ: `D:\UnityHub3D_Vulkan` hoặc `C:\Games\UnityHub3D_Vulkan`).

#### Bước 3: Khởi Chạy Ứng Dụng
1. Mở thư mục vừa giải nén.
2. Nhấp đúp chuột vào file **`ShapeRenderer.exe`**.
3. *Lưu ý khi gặp cảnh báo Windows Defender SmartScreen:* Nếu Windows hiện bảng xanh "Windows protected your PC", hãy bấm vào chữ **More info** (Thông tin thêm) -> chọn **Run anyway** (Vẫn chạy).

#### Bước 4: Tối Ưu Hiệu Năng Cho Laptop Có 2 Card Đồ Họa (NVIDIA Optimus / AMD Dual GPU)
Nếu máy bạn có cả card đồ họa tích hợp (Intel Onboard) và card rời (NVIDIA GeForce / AMD Radeon):
1. Mở **Windows Settings** -> **System** -> **Display** -> **Graphics** (hoặc gõ tìm kiếm *Graphics Settings*).
2. Bấm nút **Browse** và chọn tới file `ShapeRenderer.exe`.
3. Bấm **Options** -> Chọn **High performance (GPU rời)** -> Bấm **Save**.
4. Điều này đảm bảo engine luôn chạy với FPS cao nhất và tận dụng tối đa phần cứng đồ họa Vulkan.

---

### 4. 📦 Tự Đóng Gói Bộ Cài Mới (Dành Cho Nhà Phát Triển - 1 Click Package)
Nếu bạn thay đổi mã nguồn hoặc cập nhật model/texture mới và muốn xuất ra bộ cài hoàn chỉnh cho người khác dùng:
1. Chạy file script tự động: **`package_release.bat`** (Nhấp đúp chuột trực tiếp trong thư mục gốc dự án).
2. Hệ thống sẽ tự động:
   - Biên dịch bản Release mới nhất bằng CMake/MSBuild.
   - Tự động gom các file `.exe`, `.dll`, Shaders `.spv`, và toàn bộ thư mục `assets/` vào thư mục `dist/UnityHub3D_Vulkan/`.
   - Tự động nén thành file **`dist/UnityHub3D_Vulkan_Portable.zip`** sẵn sàng để gửi cho người dùng hoặc upload lên Drive/GitHub Releases!

---

### 5. ❓ Xử Lý Sự Cố Thường Gặp (Troubleshooting & FAQ)

#### ❌ Lỗi 1: "The code execution cannot proceed because MSVCP140.dll / VCRUNTIME140.dll was not found"
- **Nguyên nhân:** Máy tính của bạn thiếu gói thư viện C++ Runtime của Microsoft.
- **Cách khắc phục:** Tải và cài đặt [Visual C++ 2015-2022 Redistributable (x64)](https://aka.ms/vs/17/release/vc_redist.x64.exe), sau đó khởi động lại máy hoặc mở lại file exe.

#### ❌ Lỗi 2: "Failed to find a compatible Vulkan Physical Device" hoặc crash khi mở
- **Nguyên nhân:** Card đồ họa của bạn chưa cài driver Vulkan hoặc đang dùng driver mặc định của Windows.
- **Cách khắc phục:** 
  1. Tải driver mới nhất từ trang chủ: [NVIDIA Drivers](https://www.nvidia.com/Download/index.aspx) | [AMD Drivers](https://www.amd.com/en/support) | [Intel Drivers](https://www.intel.com/content/www/us/en/download-center/home.html).
  2. Nếu dùng Laptop có 2 card, hãy ép ứng dụng chạy bằng card rời theo hướng dẫn ở **Bước 4** phía trên.

#### ❌ Lỗi 3: "Failed to open vert.spv / frag.spv / shadow_vert.spv"
- **Nguyên nhân:** File thực thi không tìm thấy các file Shader đồ họa.
- **Cách khắc phục:** Đảm bảo các file đuôi `.spv` nằm cùng cấp thư mục với `ShapeRenderer.exe`. Tuyệt đối không copy mỗi file `.exe` ra chỗ khác mà không mang theo các file này.

#### ❌ Lỗi 4: Mở ứng dụng thấy khung cảnh trống trơn hoặc mô hình 3D màu trắng/tím
- **Nguyên nhân:** Thiếu thư mục `assets/` hoặc đường dẫn thư mục `assets/` bị đổi tên.
- **Cách khắc phục:** Đảm bảo trong thư mục cài đặt có thư mục con tên là `assets/` chứa đầy đủ các file model `.glb`, `.obj` và các ảnh texture `.jpg`, `.png`.

---

## 🛠 Hướng Dẫn Điều Khiển Trong Editor (Dành cho Người Dùng)

### 🕹️ Phím Tắt & Thao Tác (Controls & Hotkeys)
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
- **Đồ họa:** `Vulkan SDK` (Vulkan 1.2 / 1.3)
- **Cửa sổ & Input:** `GLFW`
- **Toán học 3D:** `GLM`
- **Giao diện Editor:** `Dear ImGui` (Tích hợp docking nhánh `docking`)
- **Vật lý thời gian thực:** `Jolt Physics`
- **Kịch bản Scripting:** `Lua 5.4` & `sol2`
- **Đọc mô hình 3D:** `tiny_gltf`, `tiny_obj_loader`
- **Trình mở file hệ thống:** `tinyfiledialogs`

### 2. Biên Dịch Dự Án (Build)
Dự án sử dụng CMake để quản lý cấu hình.
1. Khởi tạo thư mục build: `cmake -B build`
2. Biên dịch dự án (Chế độ Release): `cmake --build build --config Release`
3. File chạy thực thi sẽ được tạo ra tại: `build/Release/ShapeRenderer.exe`
4. Đóng gói bộ cài tự động: chạy file `package_release.bat`

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
