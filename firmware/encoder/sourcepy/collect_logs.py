#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Thu thap Serial log theo tung lo (lo1, lo2, ...) va giao tiep 2 chieu voi ESP32-S3."""

import argparse
import os
import re
import sys
import threading
import time

try:
    import serial
    import serial.tools.list_ports
    HAS_PYSERIAL = True
except ImportError:
    HAS_PYSERIAL = False

VALID_STAGES = ("collect", "encoder", "transmit_uart")
STAGE_START_RE = re.compile(r"^\[LOG_STAGE_START\]\s+batch=(\d+)\s+stage=([a-z_]+)$")
STAGE_END_RE = re.compile(r"^\[LOG_STAGE_END\]\s+batch=(\d+)\s+stage=([a-z_]+)$")
BATCH_DIR_RE = re.compile(r"^lo(\d+)$")


def find_esp32_port():
    if not HAS_PYSERIAL:
        return None
    ports = serial.tools.list_ports.comports()
    if not ports:
        return None
    keywords = ("esp32", "cp210", "ch340", "ch341", "ftdi", "usb serial", "uart", "jtag")
    for p in ports:
        desc = (p.description or "").lower()
        hwid = (p.hwid or "").lower()
        if any(k in desc or k in hwid for k in keywords):
            return p.device
    return ports[0].device


def get_default_results_dir():
    script_dir = os.path.dirname(os.path.abspath(__file__))
    return os.path.abspath(os.path.join(script_dir, "..", "results"))


class BatchLogManager:
    """Quan ly ghi log tach rieng tung lo vao results/lo<N>/{collect,encoder,transmit_uart}.log."""

    def __init__(self, results_dir, overwrite=False):
        self.results_dir = os.path.abspath(results_dir)
        os.makedirs(self.results_dir, exist_ok=True)
        self.batch_offset = 0 if overwrite else self._detect_existing_offset()
        self.active_file = None
        self.active_path = None
        self.active_stage = None
        self.active_batch = None
        self.saved_files = []

    def _detect_existing_offset(self):
        max_num = 0
        for name in os.listdir(self.results_dir):
            full_path = os.path.join(self.results_dir, name)
            if os.path.isdir(full_path):
                m = BATCH_DIR_RE.match(name)
                if m:
                    max_num = max(max_num, int(m.group(1)))
        return max_num

    def _close_active(self):
        if self.active_file is not None:
            self.active_file.flush()
            self.active_file.close()
            closed_path = self.active_path
            self.active_file = None
            self.active_path = None
            self.active_stage = None
            self.active_batch = None
            return closed_path
        return None

    def process_line(self, line, echo=True):
        raw_no_nl = line.rstrip("\r\n")
        stripped = raw_no_nl.strip()

        start_match = STAGE_START_RE.match(stripped)
        if start_match:
            batch_num = int(start_match.group(1))
            stage = start_match.group(2)
            if stage in VALID_STAGES:
                self._close_active()
                folder_num = self.batch_offset + batch_num
                batch_dir = os.path.join(self.results_dir, f"lo{folder_num}")
                os.makedirs(batch_dir, exist_ok=True)
                target_path = os.path.join(batch_dir, f"{stage}.log")
                self.active_file = open(target_path, "w", encoding="utf-8")
                self.active_path = target_path
                self.active_stage = stage
                self.active_batch = folder_num
                if echo:
                    print(f"[LOG] Bat dau ghi: {target_path}", flush=True)
                return

        end_match = STAGE_END_RE.match(stripped)
        if end_match:
            stage = end_match.group(2)
            if stage in VALID_STAGES and self.active_file is not None:
                saved_path = self._close_active()
                if saved_path:
                    self.saved_files.append(saved_path)
                    if echo:
                        print(f"[LOG] Da luu xong: {saved_path}", flush=True)
                return

        if self.active_file is not None:
            self.active_file.write(raw_no_nl + "\n")
            self.active_file.flush()

        if echo and raw_no_nl:
            print(raw_no_nl, flush=True)

    def close(self):
        saved_path = self._close_active()
        if saved_path and saved_path not in self.saved_files:
            self.saved_files.append(saved_path)


def serial_reader_loop(ser, manager, stop_event):
    while not stop_event.is_set():
        try:
            line_bytes = ser.readline()
            if line_bytes:
                line_str = line_bytes.decode("utf-8", errors="replace")
                manager.process_line(line_str, echo=True)
        except Exception:
            if stop_event.is_set():
                break


def main():
    parser = argparse.ArgumentParser(
        description="Thu thap Serial log theo lo (collect, encoder, transmit_uart) tu ESP32-S3."
    )
    parser.add_argument("--port", type=str, default=None, help="Cong Serial COM (vd COM5). Mac dinh tu dong quet.")
    parser.add_argument("--baud", type=int, default=115200, help="Toc do baudrate (mac dinh 115200).")
    parser.add_argument("--out-dir", type=str, default=None, help="Thu muc results (mac dinh: firmware/encoder/results).")
    parser.add_argument("--overwrite", action="store_true", help="Ghi de tu lo1 thay vi noi tiep so lo hien co.")
    args = parser.parse_args()

    results_dir = args.out_dir if args.out_dir else get_default_results_dir()

    if not HAS_PYSERIAL:
        print("[ERROR] Chua cai dat 'pyserial'. Hay chay: pip install pyserial")
        sys.exit(1)

    port = args.port if args.port else find_esp32_port()
    if not port:
        print("[WARNING] Khong tim thay cong Serial ESP32-S3! Dung --port COMx de chi dinh.")
        sys.exit(1)

    manager = BatchLogManager(results_dir, overwrite=args.overwrite)
    stop_event = threading.Event()
    ser = None

    print("========================================================================")
    print("        TRINH THU LOG THEO LO - ESP32-S3 TX ENCODER (2 CHIEU)           ")
    print("========================================================================")
    print(f" Thu muc luu log : {manager.results_dir}")
    print(f" Lo bat dau tu   : lo{manager.batch_offset + 1}")
    print(f" Cong Serial     : {port} @ {args.baud} baud")
    print(" Go lenh (COLLECT, ENCODER, TRANSMIT_UART, IDLE) roi nhan Enter.")
    print(" Nhan Ctrl+C de dung chuong trinh.")
    print("========================================================================")

    try:
        ser = serial.Serial(port=port, baudrate=args.baud, timeout=0.5)
        time.sleep(0.3)
        ser.reset_input_buffer()

        reader = threading.Thread(target=serial_reader_loop, args=(ser, manager, stop_event), daemon=True)
        reader.start()

        while not stop_event.is_set():
            try:
                cmd = input()
            except EOFError:
                time.sleep(0.5)
                continue
            if cmd is None:
                continue
            payload = (cmd.strip() + "\r\n").encode("utf-8")
            ser.write(payload)
            ser.flush()
    except serial.SerialException as e:
        print(f"\n[ERROR] Loi ket noi Serial tren {port}: {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Dung thu log theo yeu cau nguoi dung.")
    finally:
        stop_event.set()
        manager.close()
        if ser is not None and ser.is_open:
            ser.close()
            print("[SERIAL] Da dong ket noi Serial.")
        print(f"[TONG KET] Da luu {len(manager.saved_files)} file log trong {manager.results_dir}")


if __name__ == "__main__":
    main()
