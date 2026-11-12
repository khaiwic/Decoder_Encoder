#!/usr/bin/env python3

import os
import sys

script_dir = os.path.dirname(os.path.abspath(__file__))
target_script = os.path.join(script_dir, "..", "firmware", "encoder", "sourcepy", "collect_logs.py")

if __name__ == "__main__":
    if os.path.exists(target_script):
        with open(target_script, "r", encoding="utf-8") as f:
            code = f.read()
        exec(code, {"__name__": "__main__", "__file__": target_script})
    else:
        print(f"Error: {target_script} not found.")

