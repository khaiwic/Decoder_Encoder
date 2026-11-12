#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
collect_logs.py
===============================================================================
Thu thập dữ liệu Serial từ ESP32-S3 Máy Phát (TX Encoder) đọc cảm biến SHT30
thực tế và tự động xuất file .csv vào thư mục results.

Cách sử dụng:
    # 1. Tự động tìm cổng COM của ESP32-S3 và bắt đầu ghi log:
    python firmware/encoder/sourcepy/collect_logs.py

    # 2. Chỉ định cổng COM và baudrate cụ thể:
    python firmware/encoder/sourcepy/collect_logs.py --port COM5 --baud 115200
===============================================================================
"""

import os
import sys
import time
import argparse
from datetime import datetime

try:
    import serial
    import serial.tools.list_ports
    HAS_PYSERIAL = True
except ImportError:
    HAS_PYSERIAL = False

TAG_HEADER = "[CSV_HEADER]"
TAG_DATA   = "[CSV_DATA]"

DEFAULT_HEADER = (
    "seq_id,batch_id,mode,valid_samples,dropped_samples,avg_temp_c,avg_rh_pct,"
    "raw_bytes,raw_crc32,entropy,huff_symbols,codebook_bytes,huff_bits,"
    "huff_bytes,payload_ratio,net_ratio,ham_bits,ham_bytes,ham_rate,encode_us,status"
)

def find_esp32_port():
    """Tự động quét và tìm cổng COM kết nối với ESP32-S3."""
    if not HAS_PYSERIAL:
        return None
    ports = serial.tools.list_ports.comports()
    if not ports:
        return None
    for p in ports:
        desc = (p.description or "").lower()
        hwid = (p.hwid or "").lower()
        if any(keyword in desc or keyword in hwid for keyword in [
            "esp32", "cp210", "ch340", "ch341", "ftdi", "usb serial", "uart", "jtag"
        ]):
            return p.device
    return ports[0].device

def get_default_results_dir():
    """Xác định đường dẫn thư mục results."""
    script_dir = os.path.dirname(os.path.abspath(__file__))
    candidate1 = os.path.abspath(os.path.join(script_dir, "..", "results"))
    if os.path.exists(candidate1):
        return candidate1
    candidate2 = os.path.abspath(os.path.join(script_dir, "..", "..", "results"))
    return candidate2

def process_line(line, csv_file, raw_log_file, stats):
    """Xử lý từng dòng serial nhận được từ ESP32-S3."""
    clean_line = line.strip()
    if not clean_line:
        return

    if raw_log_file:
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f")[:-3]
        raw_log_file.write(f"[{timestamp}] {clean_line}\n")
        raw_log_file.flush()

    if clean_line.startswith(TAG_HEADER):
        header_content = clean_line[len(TAG_HEADER):].strip()
        print(f"\n[LOGGER] Nhan duoc tieu de CSV: {header_content}")
        if csv_file.tell() == 0:
            csv_file.write(header_content + "\n")
            csv_file.flush()
        return

    if clean_line.startswith(TAG_DATA):
        data_content = clean_line[len(TAG_DATA):].strip()
        csv_file.write(data_content + "\n")
        csv_file.flush()
        stats["records_captured"] += 1

        parts = [p.strip() for p in data_content.split(",")]
        if len(parts) >= 21:
            seq_id, batch_id, mode = parts[0], parts[1], parts[2]
            t_c, rh_pct = parts[5], parts[6]
            raw_b, huff_b, ham_b = parts[7], parts[13], parts[17]
            pay_r, net_r, c_rate = parts[14], parts[15], parts[18]
            time_us, status = parts[19], parts[20]

            print(f"  [CSV #{stats['records_captured']:03d}] "
                  f"Seq:{seq_id:>3s} (Batch #{batch_id}) | Mode:{mode:<15s} | "
                  f"T:{t_c:>5s}C RH:{rh_pct:>5s}% | "
                  f"Raw:{raw_b:>3s}B -> Nen:{huff_b:>3s}B -> Ham:{ham_b:>3s}B | "
                  f"PayR:{pay_r:>5s}x | NetR:{net_r:>4s}x | CodeR:{c_rate:>6s} | "
                  f"{time_us:>5s} us | [{status}]")
        else:
            print(f"  [CSV #{stats['records_captured']:03d}] {data_content}")
        return

    print(clean_line)

def main():
    parser = argparse.ArgumentParser(
        description="Thu thap Serial log va xuat file CSV tu ESP32-S3 + SHT30 TX Encoder."
    )
    parser.add_argument("--port", type=str, default=None,
                        help="Ten cong Serial COM (vi du COM3 hoac COM5). Tu dong quet neu de trong.")
    parser.add_argument("--baud", type=int, default=115200,
                        help="Toc do baudrate (mac dinh 115200).")
    parser.add_argument("--out-dir", type=str, default=None,
                        help="Thu muc luu ket qua CSV va logs (mac dinh: firmware/encoder/results).")
    parser.add_argument("--max-records", type=int, default=0,
                        help="Dung script sau khi thu thap du N ban ghi (0: chay vo han).")

    args = parser.parse_args()

    results_dir = args.out_dir if args.out_dir else get_default_results_dir()
    raw_logs_dir = os.path.join(results_dir, "raw_logs")
    os.makedirs(results_dir, exist_ok=True)
    os.makedirs(raw_logs_dir, exist_ok=True)

    timestamp_str = datetime.now().strftime("%Y%m%d_%H%M%S")
    csv_path = os.path.join(results_dir, "tx_metrics.csv")
    csv_timestamped_path = os.path.join(results_dir, f"tx_metrics_{timestamp_str}.csv")
    raw_log_path = os.path.join(raw_logs_dir, f"tx_serial_{timestamp_str}.log")

    print("========================================================================")
    print("    TRINH THU THAP NHAT KY SERIAL & XUAT CSV - SHT30 TX ENCODER         ")
    print("========================================================================")
    print(f" Thu muc luu ket qua : {results_dir}")
    print(f" File CSV tich luy   : {csv_path}")
    print(f" File CSV phien chay : {csv_timestamped_path}")
    print(f" File log Serial tho : {raw_log_path}")

    if not HAS_PYSERIAL:
        print("\n[ERROR] Chua cai dat thu vien 'pyserial'. Vui long chay: pip install pyserial")
        return

    port = args.port if args.port else find_esp32_port()
    if not port:
        print("\n[WARNING] Khong tim thay cong Serial ESP32-S3 nao dang cam!")
        print("Vui long cam ESP32-S3 vao may tinh hoac chi dinh cong: python collect_logs.py --port COMx")
        return

    stats = {"records_captured": 0}
    file_exists = os.path.exists(csv_path) and os.path.getsize(csv_path) > 0

    with open(csv_path, "a", encoding="utf-8") as f_csv, \
         open(csv_timestamped_path, "w", encoding="utf-8") as f_csv_ts, \
         open(raw_log_path, "w", encoding="utf-8") as f_raw:

        if not file_exists:
            f_csv.write(DEFAULT_HEADER + "\n")
            f_csv.flush()
        f_csv_ts.write(DEFAULT_HEADER + "\n")
        f_csv_ts.flush()

        class DualCsvWriter:
            def __init__(self, f1, f2):
                self.f1 = f1
                self.f2 = f2
            def write(self, s):
                self.f1.write(s)
                self.f2.write(s)
            def flush(self):
                self.f1.flush()
                self.f2.flush()
            def tell(self):
                return self.f1.tell()

        dual_csv = DualCsvWriter(f_csv, f_csv_ts)

        print(f"\n[SERIAL] Dang ket noi toi cong: {port} @ {args.baud} baud...")
        try:
            ser = serial.Serial(port=port, baudrate=args.baud, timeout=1.0)
            time.sleep(0.5)
            ser.reset_input_buffer()
            print(f"[SERIAL] Da ket noi thanh cong voi {port}! Dang lang nghe du lieu tu SHT30...\n")
            print("Nhan Ctrl+C de dung thu thap bat cu luc nao.\n")
            print("-" * 100)

            while True:
                line_bytes = ser.readline()
                if line_bytes:
                    try:
                        line_str = line_bytes.decode("utf-8", errors="replace")
                        process_line(line_str, dual_csv, f_raw, stats)
                    except Exception:
                        pass

                if args.max_records > 0 and stats["records_captured"] >= args.max_records:
                    print(f"\n[INFO] Da thu thap du {args.max_records} ban ghi. Ket thuc.")
                    break

        except serial.SerialException as e:
            print(f"\n[ERROR] Loi ket noi Serial tren {port}: {e}")
        except KeyboardInterrupt:
            print("\n\n[INFO] Nguoi dung nhan Ctrl+C dung thu thap.")
        finally:
            if 'ser' in locals() and ser.is_open:
                ser.close()
                print("[SERIAL] Da dong ket noi Serial.")

    print("\n========================================================================")
    print("                         TONG KET THU THAP                              ")
    print("========================================================================")
    print(f" So luong ban ghi hop le: {stats['records_captured']}")
    print(f" File CSV tong hop     : {csv_path}")
    print(f" File CSV phien hien tai: {csv_timestamped_path}")
    print(f" File log Serial raw   : {raw_log_path}")
    print("========================================================================\n")

if __name__ == "__main__":
    main()
