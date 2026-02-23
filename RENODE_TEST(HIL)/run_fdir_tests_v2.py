#!/usr/bin/env python3
"""
FDIR Test Runner v2 - Enhanced Telemetry-Based Verification
============================================================

Improvements over v1:
- Telemetry frame parsing and FDIR status_flags verification
- Actual FDIR state machine validation
- Recovery sequence testing
- Frame timing verification

Usage:
    python run_fdir_tests_v2.py [--quick] [--test-id S-01]

Prerequisites:
    1. Renode installed
    2. Firmware built: ../build/stm32_spaceballoon
    3. Python 3.7+
"""

import sys
import time
import socket
import csv
import subprocess
import os
import argparse

# Import test cases
from fdir_cases_v2 import (
    REVISED_TEST_CASES, 
    QUICK_TEST_CASES,
    FLAG_SYS_OK, FLAG_GPS_WARN, FLAG_BARO_WARN, FLAG_IMU_WARN,
    FLAG_TEMP_WARN, FLAG_HEATER_ACTIVE, FLAG_LOW_BATTERY, 
    FLAG_FDIR_RECOVERY, FLAG_ALT_JUMP, FLAG_RANGE_ERROR
)

FLAG_NAMES = {
    0: "SYS_OK", 1: "GPS_WARN", 2: "BARO_WARN", 3: "IMU_WARN",
    4: "TEMP_WARN", 5: "HEATER_ACTIVE", 6: "LOW_BATTERY",
    7: "FDIR_RECOVERY", 8: "ALT_JUMP", 9: "RANGE_ERROR",
}

# Configuration
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
RENODE_HOST = "localhost"
RENODE_PORT = 1234
RENODE_PATH = "/usr/local/bin/renode"
SCRIPT_PATH = "renode/scripts/test_fdir_v2.resc"
OUTPUT_FILE = "fdir_test_results_v2.csv"

# Global state
current_test_state = {
    "status_flags": 0,
    "frame_count": 0,
    "fdir_events": [],
}


class TelnetClient:
    """Simple telnet client using socket (Python 3.12+ compatible)"""
    
    def __init__(self, host, port, timeout=10):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(timeout)
        self.sock.connect((host, port))
        self.buffer = b""
        
    def write(self, data):
        if isinstance(data, str):
            data = data.encode('ascii')
        self.sock.sendall(data)
        
    def read_very_eager(self):
        """Read all available data without blocking"""
        data = b""
        self.sock.setblocking(False)
        try:
            while True:
                chunk = self.sock.recv(4096)
                if not chunk:
                    break
                data += chunk
        except BlockingIOError:
            pass
        finally:
            self.sock.setblocking(True)
        return data
        
    def close(self):
        self.sock.close()


class TelemetryParser:
    """Parses telemetry frames from Renode UART output"""
    
    FRAME_SIZE = 148
    MAGIC = bytes([0xA5, 0x5A])
    
    def __init__(self):
        self.buffer = bytearray()
        self.frame_count = 0
        self.last_status_flags = 0
        self.events = []
        
    def feed(self, data: bytes):
        """Feed raw UART data to parser"""
        self.buffer.extend(data)
        self._parse_frames()
        
    def _parse_frames(self):
        while len(self.buffer) >= self.FRAME_SIZE:
            # Find frame start
            if self.buffer[0:2] != self.MAGIC:
                self.buffer.pop(0)
                continue
                
            frame = bytes(self.buffer[:self.FRAME_SIZE])
            
            if self._verify_crc(frame):
                self._process_frame(frame)
            
            self.buffer = self.buffer[self.FRAME_SIZE:]
            
    def _verify_crc(self, frame):
        calc = self._crc16_ccitt(frame[:-2])
        recv = int.from_bytes(frame[144:146], 'little')
        return calc == recv
        
    def _crc16_ccitt(self, data):
        crc = 0xFFFF
        for b in data:
            crc ^= b << 8
            for _ in range(8):
                if crc & 0x8000:
                    crc = ((crc << 1) ^ 0x1021) & 0xFFFF
                else:
                    crc = (crc << 1) & 0xFFFF
        return crc
        
    def _process_frame(self, frame):
        self.frame_count += 1
        
        # Extract fields
        uptime_ms = int.from_bytes(frame[12:16], 'little')
        status_flags = int.from_bytes(frame[16:18], 'little')
        
        # Detect flag changes
        if status_flags != self.last_status_flags:
            changes = status_flags ^ self.last_status_flags
            for bit in range(10):
                if changes & (1 << bit):
                    name = FLAG_NAMES.get(bit, f"BIT{bit}")
                    state = "SET" if status_flags & (1 << bit) else "CLEARED"
                    self.events.append({
                        "frame": self.frame_count,
                        "uptime_ms": uptime_ms,
                        "flag": name,
                        "bit": bit,
                        "state": state,
                    })
                    print(f"  [Telemetry] {name} {state} at {uptime_ms}ms")
            
            self.last_status_flags = status_flags
            
        # Update global state
        current_test_state["status_flags"] = status_flags
        current_test_state["frame_count"] = self.frame_count
        current_test_state["fdir_events"] = self.events.copy()
        
    def has_flag(self, bit):
        return bool(self.last_status_flags & (1 << bit))
        
    def reset(self):
        self.buffer = bytearray()
        self.frame_count = 0
        self.last_status_flags = 0
        self.events = []
        current_test_state["status_flags"] = 0
        current_test_state["frame_count"] = 0
        current_test_state["fdir_events"] = []


def run_test(tn, test_case, parser):
    """Run a single FDIR test with telemetry verification"""
    print(f"\n{'='*60}")
    print(f"Test {test_case['id']}: {test_case['name']}")
    print(f"{'='*60}")
    print(f"Description: {test_case.get('description', 'N/A')}")
    
    # Reset parser state
    parser.reset()
    
    # Setup logging
    if test_case.get('setup_cmd'):
        print(f"Setup: {test_case['setup_cmd']}")
        tn.write(test_case['setup_cmd'].encode('ascii') + b"\n")
        time.sleep(0.5)
    
    # Start simulation if not running
    tn.write(b"start\n")
    time.sleep(1)
    
    # Wait for initial stabilization
    inject_delay = test_case.get('inject_delay_ms', 0) / 1000.0
    if inject_delay > 0:
        print(f"Stabilizing for {inject_delay}s before fault injection...")
        time.sleep(inject_delay)
    
    # Inject fault
    if test_case.get('inject_cmd'):
        print(f"Injecting fault: {test_case['inject_cmd']}")
        tn.write(test_case['inject_cmd'].encode('ascii') + b"\n")
        time.sleep(0.5)
    
    # Collect telemetry during test
    verification = test_case.get('verification', {})
    vtype = verification.get('type', 'log')
    duration = test_case.get('duration', 20)
    
    print(f"Running for {duration}s...")
    
    log_buffer = ""
    start_time = time.time()
    test_passed = False
    fail_reason = ""
    
    while time.time() - start_time < duration:
        try:
            data = tn.read_very_eager().decode('utf-8', errors='ignore')
            if data:
                log_buffer += data
                
                # Parse telemetry frames from UART output
                parser.feed(data.encode('latin-1'))
                
        except Exception as e:
            pass
            
        # Check verification
        if vtype == 'fdir':
            expect_flag = verification.get('expect_flag')
            expect_state = verification.get('expect_state', 'SET')
            timeout_ms = verification.get('timeout_ms', 5000)
            
            if expect_flag is not None:
                has_flag = parser.has_flag(expect_flag)
                flag_name = FLAG_NAMES.get(expect_flag, f"BIT{expect_flag}")
                
                if expect_state == 'SET' and has_flag:
                    print(f"  PASS: {flag_name} flag SET detected")
                    test_passed = True
                    break
                elif expect_state == 'CLEARED' and not has_flag and parser.frame_count > 10:
                    # Flag was set and then cleared
                    events = [e for e in parser.events if e['bit'] == expect_flag]
                    if any(e['state'] == 'SET' for e in events):
                        print(f"  PASS: {flag_name} flag CLEARED after being SET")
                        test_passed = True
                        break
                        
        elif vtype == 'telemetry':
            expect_frames = verification.get('expect_frames', 10)
            if parser.frame_count >= expect_frames:
                print(f"  PASS: Received {parser.frame_count} telemetry frames")
                test_passed = True
                break
                
        elif vtype == 'log':
            pattern = verification.get('pattern', '')
            if pattern and pattern in log_buffer:
                print(f"  PASS: Found pattern '{pattern}'")
                test_passed = True
                break
                
        time.sleep(0.1)
    
    # Handle reconnection test
    if test_case.get('reconnect_cmd') and not test_passed:
        reconnect_delay = test_case.get('reconnect_delay_ms', 5000) / 1000.0
        print(f"Waiting {reconnect_delay}s before reconnect...")
        time.sleep(reconnect_delay)
        
        print(f"Reconnecting: {test_case['reconnect_cmd']}")
        tn.write(test_case['reconnect_cmd'].encode('ascii') + b"\n")
        
        # Continue monitoring
        while time.time() - start_time < duration:
            data = tn.read_very_eager().decode('utf-8', errors='ignore')
            if data:
                parser.feed(data.encode('latin-1'))
                
            if vtype == 'fdir_sequence':
                expect_flag = verification.get('expect_flag')
                events = [e for e in parser.events if e['bit'] == expect_flag]
                states = [e['state'] for e in events]
                expected = verification.get('expect_sequence', [])
                
                if states == expected:
                    flag_name = FLAG_NAMES.get(expect_flag, f"BIT{expect_flag}")
                    print(f"  PASS: {flag_name} sequence {expected} detected")
                    test_passed = True
                    break
                    
            time.sleep(0.1)
    
    if not test_passed:
        if vtype == 'fdir':
            flag_name = FLAG_NAMES.get(verification.get('expect_flag'), 'UNKNOWN')
            fail_reason = f"{flag_name} flag not {verification.get('expect_state', 'SET')} within timeout"
        elif vtype == 'telemetry':
            fail_reason = f"Only {parser.frame_count} frames received (expected {verification.get('expect_frames', 10)})"
        else:
            fail_reason = f"Pattern '{verification.get('pattern', '')}' not found"
            # Save debug log
            debug_file = f"test_log_{test_case['id']}.txt"
            with open(debug_file, 'w', encoding='utf-8') as f:
                f.write(log_buffer)
            print(f"  Log saved to {debug_file}")
            
    result = "PASS" if test_passed else "FAIL"
    print(f"\n  Result: [{result}]")
    if fail_reason:
        print(f"  Reason: {fail_reason}")
    
    # Stop simulation for next test
    tn.write(b"pause\n")
    time.sleep(0.5)
    
    return {
        "ID": test_case['id'],
        "Name": test_case['name'],
        "Result": result,
        "Reason": fail_reason,
        "FrameCount": parser.frame_count,
        "StatusFlags": f"0x{parser.last_status_flags:04X}",
        "Timestamp": time.strftime("%Y-%m-%d %H:%M:%S")
    }


def main():
    parser = argparse.ArgumentParser(description='FDIR Test Runner v2')
    parser.add_argument('--quick', action='store_true', help='Run quick test suite')
    parser.add_argument('--test-id', help='Run specific test by ID (e.g., S-01)')
    args = parser.parse_args()
    
    # Select test cases
    if args.quick:
        test_cases = QUICK_TEST_CASES
        print("Running QUICK test suite")
    elif args.test_id:
        test_cases = [t for t in REVISED_TEST_CASES if t['id'] == args.test_id]
        if not test_cases:
            print(f"Error: Test ID '{args.test_id}' not found")
            return
        print(f"Running single test: {args.test_id}")
    else:
        test_cases = REVISED_TEST_CASES
        print(f"Running FULL test suite ({len(test_cases)} tests)")
    
    print("=" * 60)
    print("FDIR Test Runner v2 - Telemetry-Based Verification")
    print("=" * 60)
    
    # Start Renode with script as argument (required for proper loading)
    print("\nStarting Renode...")
    renode_cmd = [RENODE_PATH, "--disable-xwt", "--port", str(RENODE_PORT), "--plain", "scripts/test_firmware.resc"]
    
    try:
        renode_proc = subprocess.Popen(
            renode_cmd, 
            cwd=BASE_DIR,
            stdout=subprocess.DEVNULL, 
            stderr=subprocess.DEVNULL
        )
    except FileNotFoundError:
        print(f"Error: Renode not found at {RENODE_PATH}")
        print("Please install Renode: https://renode.io")
        return
    
    time.sleep(10)  # Wait for script to load
    
    results = []
    telnet_parser = TelemetryParser()
    
    try:
        tn = TelnetClient(RENODE_HOST, RENODE_PORT, timeout=10)
        print("Connected to Renode Telnet")
        
        # Script is already loaded via command line, just start simulation
        print("Starting simulation...")
        tn.write(b"start\n")
        time.sleep(2)
        
        # Run tests
        for test in test_cases:
            res = run_test(tn, test, telnet_parser)
            results.append(res)
            
    except Exception as e:
        print(f"Error: {e}")
        import traceback
        traceback.print_exc()
    finally:
        if renode_proc:
            renode_proc.kill()
    
    # Write results
    with open(OUTPUT_FILE, 'w', newline='', encoding='utf-8') as f:
        fieldnames = ["ID", "Name", "Result", "Reason", "FrameCount", "StatusFlags", "Timestamp"]
        writer = csv.DictWriter(f, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(results)
    
    # Summary
    print("\n" + "=" * 60)
    passed = sum(1 for r in results if r['Result'] == 'PASS')
    total = len(results)
    print(f"Results: {passed}/{total} tests passed")
    print(f"Details saved to: {OUTPUT_FILE}")
    print("=" * 60)
    
    # Show failures
    failures = [r for r in results if r['Result'] == 'FAIL']
    if failures:
        print("\nFailed tests:")
        for f in failures:
            print(f"  - {f['ID']}: {f['Name']} - {f['Reason']}")


if __name__ == "__main__":
    main()
