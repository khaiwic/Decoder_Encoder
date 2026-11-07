# Hướng dẫn sử dụng Zephyr (Đã cài trên máy)

## Cài các công cụ còn thiếu

- Mở và kích hoạt môi trường and SDK:

```powershell
    Set-Location C:\zephyrproject

    Set-ExecutionPolicy -Scope Process -ExecutionPolicy RemoteSigned

    .\.venv312\Scripts\Activate.ps1

    $env:ZEPHYR_BASE = 'C:\zephyrproject\zephyr'
    $env:ZEPHYR_SDK_INSTALL_DIR = 'C:\zephyr-sdk-1.0.1'
    $env:ZEPHYR_TOOLCHAIN_VARIANT = 'zephyr'
```

- Chạy và kiểm tra môi trường:

```powershell
    python --version
    west --version
    west topdir
    west sdk list
```

- Mở dự án trong VScode:

```powershell
    code -n C:\Users\ADMIN\Documents\Decoder_and_Encoder
```

## Hướng dẫn sử dụng các công cụ build và flash của Zephyr

### Build

- Mở Terminal và chạy lệnh sau:

```powershell
    west build -p always `
    -b esp32s3_devkitc/esp32s3/procpu `
    -s C:\Users\ADMIN\Documents\Decoder_and_Encoder\firmware\encoder `
    -d C:\Users\ADMIN\Documents\Decoder_and_Encoder\build\encoder
```

Sau khi build xong sẽ có folder build xuất hiện(Nếu thành công).

### Flash

- Mở Terminal và chạy lệnh sau:

```powershell
    west flash `
    -d C:\Users\ADMIN\Documents\Decoder_and_Encoder\build\encoder `
    --esp-device COMx
```

### Tìm cổng COM

- Mở máy tính và thao tác như sau:

Mở Device Manager -> Ports(COM & LPT) -> Ta sẽ nhìn thấy cổng kết nối của esp với máy tính sau đó điền nó vào x.

### Serial

- Xem cổng COM của board

```powershell
    python -m serial.tools.list_ports
```

- Mở Serial Monitor

```powershell
Set-Location C:\Users\ADMIN\Documents\Decoder_and_Encoder\build\encoder

west espressif monitor -p COM5 -b 115200
```
