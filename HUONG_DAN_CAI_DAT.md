# 📦 Hướng Dẫn Cài Đặt & Sử Dụng Bộ Cài Đặt (Unity Hub 3D Vulkan Engine)

Chào mừng bạn đến với tài liệu hướng dẫn triển khai và sử dụng bộ cài đặt **Unity Hub 3D Vulkan Engine**. Bộ cài đặt này được thiết kế theo dạng **Portable / Standalone**, cho phép bạn chạy trực tiếp mà không cần qua các bước cài đặt hệ thống phức tạp.

---

## 🖥️ 1. Yêu Cầu Cấu Hình Máy Tính (System Requirements)

| Tiêu chí | Cấu hình tối thiểu | Cấu hình khuyến nghị |
| :--- | :--- | :--- |
| **Hệ điều hành** | Windows 10 (64-bit) 1909 trở lên | Windows 10 / Windows 11 (64-bit) |
| **Bộ xử lý (CPU)** | Intel Core i3 gen 6th / AMD Ryzen 3 | Intel Core i5 gen 8th / AMD Ryzen 5 trở lên |
| **Bộ nhớ RAM** | 4 GB RAM | 8 GB RAM trở lên |
| **Card đồ họa (GPU)** | GPU hỗ trợ **Vulkan API 1.2+**<br>*(NVIDIA GeForce GTX 900 trở lên, AMD Radeon RX 400 trở lên, Intel UHD 620 / Iris Xe)* | Card đồ họa rời NVIDIA GeForce GTX 1060 / RTX 2060 / AMD RX 580 trở lên |
| **Driver GPU** | Đã cài driver card đồ họa mới nhất từ trang chủ | Phiên bản driver Game Ready / Studio mới nhất |
| **Gói hỗ trợ C++** | [Microsoft Visual C++ 2015-2022 Redistributable (x64)](https://aka.ms/vs/17/release/vc_redist.x64.exe) | Bắt buộc phải có |

---

## 📂 2. Cấu Trúc Thư Mục Bộ Cài Đặt (Standalone Package)

Khi bạn giải nén file `UnityHub3D_Vulkan_Portable.zip` hoặc truy cập thư mục `dist/UnityHub3D_Vulkan/`, các thành phần bắt buộc phải có đầy đủ:

```text
UnityHub3D_Vulkan/
├── 🚀 ShapeRenderer.exe       # File khởi động chính của ứng dụng
├── ⚙️ glfw3.dll               # Thư viện quản lý cửa sổ và input chuột/bàn phím
├── 📜 lua.dll                 # Thư viện động Lua Engine
├── 🎨 vert.spv                # Shader đồ họa Vertex đã biên dịch SPIR-V
├── 🎨 frag.spv                # Shader đồ họa Fragment PBR đã biên dịch SPIR-V
├── 🌑 shadow_vert.spv         # Shader tính toán bóng đổ thời gian thực (Real-time Shadows)
├── 🎛️ imgui.ini               # Lưu cấu hình vị trí các cửa sổ Editor (Docking layout)
├── 📄 HUONG_DAN_SU_DUNG.txt   # Bản hướng dẫn tóm tắt nhanh
└── 📁 assets/                 # THƯ MỤC TÀI NGUYÊN (BẮT BUỘC)
    ├── 📁 models/             # Chứa các file mô hình 3D (.glb, .gltf, .obj)
    ├── 📁 Texture/            # Chứa ảnh vân bề mặt PBR (gỗ, cỏ, đá, vỏ cây)
    └── 📁 *.lua               # Các file kịch bản điều khiển vật thể
```

> [!CAUTION]
> **LƯU Ý QUAN TRỌNG:**
> Không di chuyển riêng lẻ file `ShapeRenderer.exe` ra ngoài thư mục. File `.exe` phải luôn luôn nằm chung một thư mục với các file `.dll`, `.spv` và thư mục `assets/`. Nếu muốn đưa ra màn hình Desktop, hãy **Nhấp chuột phải vào `ShapeRenderer.exe` ➔ Chọn `Show more options` ➔ `Send to` ➔ `Desktop (create shortcut)`**.

---

## 🚀 3. Hướng Dẫn Cài Đặt Và Khởi Chạy Từng Bước

### Bước 1: Cài đặt Visual C++ Runtime (Chỉ cần làm 1 lần cho máy tính mới)
Nếu máy tính của bạn chưa có bộ thư viện C++ Redistributable:
1. Tải về gói cài đặt từ Microsoft: 👉 **[Tải vc_redist.x64.exe](https://aka.ms/vs/17/release/vc_redist.x64.exe)**
2. Chạy file vừa tải về và chọn **Install**.

### Bước 2: Tải và Giải Nén Bộ Cài Đặt
1. Tải file nén `UnityHub3D_Vulkan_Portable.zip`.
2. Nhấp chuột phải vào file `.zip` ➔ Chọn **Extract All...** (Giải nén tất cả).
3. Đặt đường dẫn giải nén tại nơi thuận tiện (ví dụ: `D:\UnityHub3D_Vulkan` hoặc `C:\Games\UnityHub3D_Vulkan`).  
   *(Khuyến nghị: Không đặt trong các thư mục có dấu tiếng Việt có dấu như `D:\Đồ Án Game\` để tránh lỗi đường dẫn hệ thống).*

### Bước 3: Mở Engine
1. Mở thư mục đã giải nén.
2. Nhấp đúp chuột vào file **`ShapeRenderer.exe`**.
3. Nếu Windows hiển thị thông báo **Windows protected your PC** (SmartScreen):  
   - Bấm vào dòng chữ **More info** (Thông tin thêm).  
   - Bấm nút **Run anyway** (Vẫn chạy).

### Bước 4: Thiết Lập Ưu Tiên Card Rời (Dành Cho Laptop Có 2 GPU)
Để ứng dụng đạt 60+ FPS mượt mà và tận dụng sức mạnh Vulkan:
1. Mở menu **Start** ➔ Gõ tìm kiếm **Graphics Settings** (Cài đặt đồ họa).
2. Tại mục *Custom options for apps*, bấm nút **Browse**.
3. Tìm và chọn tới file `ShapeRenderer.exe`.
4. Bấm **Options** ➔ Chọn **High performance (GPU rời NVIDIA / AMD)** ➔ Bấm **Save**.

---

## 📦 4. Hướng Dẫn Tự Đóng Gói Bộ Cài Đặt Mới (Dành Cho Lập Trình Viên)

Khi bạn chỉnh sửa thêm tính năng trong code C++, thêm model 3D mới hoặc cập nhật texture, bạn có thể tạo lại bộ cài hoàn chỉnh chỉ với 1 cú nhấp chuột:

1. Vào thư mục gốc của dự án.
2. Nhấp đúp chuột vào file **`package_release.bat`**.
3. Hệ thống sẽ tự động:
   - Biên dịch bản Release mới nhất (`cmake --build build --config Release`).
   - Tạo thư mục `dist/UnityHub3D_Vulkan/` chứa đầy đủ file exe, dll, spv và assets.
   - Nén tự động ra file `dist/UnityHub3D_Vulkan_Portable.zip` để gửi cho đối tác hoặc tải lên mạng.

---

## ❓ 5. Xử Lý Các Sự Cố Thường Gặp (FAQ & Troubleshooting)

### 1. Báo lỗi `MSVCP140.dll` hoặc `VCRUNTIME140.dll was not found`
- **Nguyên nhân:** Máy thiếu runtime Visual C++ 64-bit.
- **Giải pháp:** Cài đặt gói [vc_redist.x64.exe](https://aka.ms/vs/17/release/vc_redist.x64.exe) từ trang chủ Microsoft và khởi động lại.

### 2. Báo lỗi `Failed to create Vulkan Instance` hoặc `vkCreateInstance failed`
- **Nguyên nhân:** Card màn hình chưa cài driver Vulkan hoặc driver đã quá cũ.
- **Giải pháp:** 
  - NVIDIA: Cập nhật qua phần mềm GeForce Experience hoặc tải từ [nvidia.com/drivers](https://www.nvidia.com/Download/index.aspx).
  - AMD: Cập nhật qua AMD Software: Adrenalin Edition hoặc tải từ [amd.com/support](https://www.amd.com/en/support).
  - Intel: Tải driver Intel Graphics mới nhất từ [intel.com](https://www.intel.com/content/www/us/en/download-center/home.html).

### 3. Báo lỗi `Failed to open vert.spv` hoặc `frag.spv`
- **Nguyên nhân:** File thực thi không tìm thấy shader nằm cùng cấp.
- **Giải pháp:** Kiểm tra xem các file `vert.spv`, `frag.spv`, `shadow_vert.spv` có nằm ngay cạnh file `ShapeRenderer.exe` trong cùng thư mục hay không.

### 4. Vào ứng dụng nhưng model không có vân hoặc không tải được model
- **Nguyên nhân:** Thiếu thư mục `assets/` hoặc bạn chạy file exe từ một phần mềm nén mà chưa giải nén ra ổ đĩa.
- **Giải pháp:** Hãy chắc chắn bạn đã giải nén (Extract) toàn bộ file `.zip` ra ổ cứng trước khi chạy.
