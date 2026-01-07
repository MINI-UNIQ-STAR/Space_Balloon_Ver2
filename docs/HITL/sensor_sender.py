import sys
import serial
import serial.tools.list_ports
import json
import time
import os
import random
import math
from collections import deque
from PySide6.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, 
                               QLabel, QComboBox, QPushButton, QGroupBox, QGridLayout, 
                               QMessageBox, QDialog, QFrame, QScrollArea)
from PySide6.QtCore import QTimer, Qt
from PySide6.QtGui import QAction, QColor, QPalette
import matplotlib
matplotlib.use('QtAgg')
from matplotlib.backends.backend_qtagg import FigureCanvasQTAgg
from matplotlib.figure import Figure
import matplotlib.pyplot as plt
import threading
import queue
import binascii
import struct
import csv
import datetime

# --- Telemetry Struct Definition ---
# Must match telemetry.h (Packed, Little Endian '<')
# Order:
# 1. System Status
#   uint32_t uptime_ms; (I)
#   uint16_t status_flags; (H)
#   uint16_t co2_ppm; (H)
# 2. IMU (x1000)
#   int32_t accel[3]; (3i)
#   int32_t gyro[3]; (3i)
# 3. Mag
#   float mag[3]; (3f)
# 4. Temp (x100)
#   int16_t board; (h)
#   int16_t ext; (h)
#   int16_t sht; (h)
# 11. Reserved/Ext
#   int16_t bat_temp; (h)
# 5. GPS
#   int32_t lat; (i)
#   int32_t lon; (i)
#   float alt; (f)
#   uint8_t fix; (B)
#   uint8_t sats_used; (B)
#   uint8_t sats_tot; (B)
#   uint8_t sats_gps; (B)
#   uint8_t sats_glo; (B)
#   uint8_t sats_gal; (B)
#   uint8_t sats_bei; (B)
#   uint8_t utc[6]; (6B)
#   uint16_t bat_mv; (H)
# 6. Air
#   uint16_t pm1; (H)
#   uint16_t pm25; (H)
#   uint16_t pm10; (H)
#   int16_t ozone; (h)
# 7. Press/Hum
#   uint16_t sht_rh; (H)
#   uint32_t ms5611_p; (I)
#   int16_t ms5611_t; (h)
# 8. Rad
#   uint16_t gdk; (H)
# 9. Heater
#   uint8_t h_bat; (B)
#   uint8_t h_brd; (B)
# 10. Fusion
#   float p_alt; (f)
#   float k_alt; (f)
#   float k_roll; (f)
#   float k_pitch; (f)

TELEM_FMT = "<IHH3i3i3fhhhhiifBBBBBBBBBBBHHHHhHIhHBBffff"
# Check size: 
# 4+2+2 = 8
# 12+12 = 24
# 12
# 2+2+2+2 = 8
# 4+4+4 = 12
# 1+1+1+1+1+1+1 = 7
# 6 = 6
# 2 = 2
# 2+2+2+2 = 8
# 2+4+2 = 8
# 2
# 1+1 = 2
# 4*4 = 16
# Total should be around 111 bytes. Calcs:
# 8+24+12+8+12+7+6+2+8+8+2+2+16 = 115 bytes? Struct calc needed.
TELEM_Header_FMT = "<2BBBHHIB" # Magic(2), Ver(1), Type(1), Len(2), Seq(2), TS(4), Payload... NO header is parsed separately in logic.

CSV_HEADERS = [
    "Timestamp", "Seq", "Uptime", "Status", "CO2", 
    "AccX", "AccY", "AccZ", "GyroX", "GyroY", "GyroZ", "MagX", "MagY", "MagZ",
    "TempBoard", "TempExt", "TempSHT", "TempBat",
    "Lat", "Lon", "Alt", "Fix", "Sats",
    "BatMV", "PM1", "PM2.5", "PM10", "Ozone",
    "Hum", "Press", "TempBaro", "Rad",
    "HeatBat", "HeatBrd",
    "FusAlt", "KalAlt", "Roll", "Pitch"
]
try:
    import sim_core
    if hasattr(sim_core, 'RustSimulator'):
        RUST_AVAILABLE = True
        print("[INFO] Rust Accelerator 'sim_core' loaded successfully.")
    else:
        # If import succeeds but class is missing, it's likely just the source directory
        RUST_AVAILABLE = False
        print("[WARN] Found 'sim_core' directory but not the compiled extension. Using Python mode.")
except ImportError:
    RUST_AVAILABLE = False
    print("[WARN] Rust Accelerator 'sim_core' not found. Running in Python mode.")
    print("       To enable Rust speedup: Install Rust, then run 'pip install maturin && cd sim_core && maturin develop'")


# --- Configuration ---
GNU_LAT = 35.1540
GNU_LON = 128.0981
GNU_ALT = 50.0

DEFAULT_JSON_PATH = r"C:\Users\hyuns\Desktop\project\SpaceBalloon_2.0\spaceballoon_stm32_lora32\simulation_reference_data\V4630075.json"

# Dark Theme Colors
BG_COLOR = "#1b1b1b"
FG_COLOR = "#ffffff"
ACCENT_COLOR = "#00a2ff"
GRAPH_BG = "#222222"
PANE_BG = "#2f2f2f"

class DarkPalette(QPalette):
    def __init__(self):
        super().__init__()
        self.setColor(QPalette.Window, QColor(BG_COLOR))
        self.setColor(QPalette.WindowText, QColor(FG_COLOR))
        self.setColor(QPalette.Base, QColor(PANE_BG))
        self.setColor(QPalette.AlternateBase, QColor(BG_COLOR))
        self.setColor(QPalette.ToolTipBase, QColor(FG_COLOR))
        self.setColor(QPalette.ToolTipText, QColor(FG_COLOR))
        self.setColor(QPalette.Text, QColor(FG_COLOR))
        self.setColor(QPalette.Button, QColor(PANE_BG))
        self.setColor(QPalette.ButtonText, QColor(FG_COLOR))
        self.setColor(QPalette.BrightText, QColor("red"))
        self.setColor(QPalette.Link, QColor(ACCENT_COLOR))
        self.setColor(QPalette.Highlight, QColor(ACCENT_COLOR))
        self.setColor(QPalette.HighlightedText, QColor("black"))


class TelemetryReceiver(threading.Thread):
    def __init__(self, ser, filename):
        super().__init__()
        self.ser = ser
        self.filename = filename
        self.running = True
        self.daemon = True
        
        # Open CSV and write headers
        with open(self.filename, 'w', newline='') as f:
            writer = csv.writer(f)
            writer.writerow(CSV_HEADERS)
            
    def run(self):
        print(f"[TELEM] Logging to {self.filename}")
        while self.running and self.ser:
            try:
                if self.ser == "MOCK":
                    time.sleep(1) # Nothing to receive in pure mock
                    continue
                
                if self.ser.in_waiting:
                    line = self.ser.readline().decode('utf-8', errors='replace').strip()
                    if line.startswith("TELEM_HEX:"):
                        payload_hex = line.split(":", 1)[1]
                        self._process_payload(payload_hex)
                    elif line.startswith("TELEM_RX") or line.startswith("[FEEDBACK]"):
                         print(line) # Debug print
            except Exception as e:
                print(f"[TELEM RX ERR] {e}")
                time.sleep(0.1)

    def _process_payload(self, hex_str):
        try:
            # Hex to Bytes
            data = binascii.unhexlify(hex_str)
            # Remove Header (First 12 bytes: Magic(2)+Ver(1)+Msg(1)+Len(2)+Seq(2)+TS(4)) ?
            ## WAIT: main_control.ino sends `t_buf` which includes Header!
            ## Struct TelemetryFrame: Magic(2), Ver(1), Msg(1), Len(2), Seq(2), TS(4), Payload...
            ## Header Header Size = 2+1+1+2+2+4 = 12 bytes.
            ## We need to unpack Header first to check length or just offset.
            
            if len(data) < 110: return # Too short
            
            # Unpack Header
            header_size = struct.calcsize(TELEM_Header_FMT) # 12 bytes
            header_data = data[:header_size]
            payload_data = data[header_size:-2] # Exclude Header and CRC(2) at end
            
            # Unpack Payload
            # Note: The TELEM_FMT defined earlier MUST match the payload structure EXACTLY.
            # Let's try unpacking
            
            fields = struct.unpack(TELEM_FMT, payload_data)
            
            # Extract important fields for CSV (Use list for easier writing)
            # Match CSV_HEADERS order
            
            # fields index mapping based on TELEM_FMT:
            # 0:uptime, 1:status, 2:co2 (Status)
            # 3-5:acc, 6-8:gyro (IMU)
            # 9-11:mag (Mag)
            # 12:brd, 13:ext, 14:sht (Temp)
            # 15:bat_temp (Res)
            # 16:lat, 17:lon, 18:alt (GPS)
            # 19:fix, 20:used, 21:tot, ... (GPS Sats)
            # ...
            
            u_ts = datetime.datetime.now().strftime("%H:%M:%S.%f")
            
            row = [
                u_ts, "?" , fields[0], fields[1], fields[2], # TS, Seq(skipped), Uptime, Status, CO2
                fields[3], fields[4], fields[5], # Acc X Y Z
                fields[6], fields[7], fields[8], # Gyro
                fields[9], fields[10], fields[11], # Mag
                fields[12], fields[13], fields[14], fields[15], # Temps (Brd, Ext, SHT, Bat)
                fields[16], fields[17], fields[18], # GPS Lat, Lon, Alt
                fields[19], fields[20], # Fix, Sats Used
                fields[32], # Bat MV (Index jump due to UTC bytes?)
                # Wait, unpacking "6B" (UTC) takes 6 items in tuple
                # Let's count indices carefully.
                
                # 0: I (upt)
                # 1: H (stat)
                # 2: H (co2)
                # 3,4,5: 3i (acc)
                # 6,7,8: 3i (gyro)
                # 9,10,11: 3f (mag)
                # 12: h (brd)
                # 13: h (ext)
                # 14: h (sht)
                # 15: h (bat_t)
                # 16,17,18: 2i f (lat, lon, alt)
                # 19: B (fix)
                # 20: B (used)
                # 21: B (tot)
                # 22-26: 5B (sats_view)
                # 27-32: 6B (UTC) -> items 27,28,29,30,31,32
                # 33: H (bat_mv)
                # 34: H (pm1)
                # 35: H (pm25)
                # 36: H (pm10)
                # 37: h (ozone)
                # 38: H (sht_rh)
                # 39: I (ms5611_p)
                # 40: h (ms_t)
                # 41: H (gdk)
                # 42: B (h_bat)
                # 43: B (h_brd)
                # 44: f (p_alt)
                # 45: f (k_alt)
                # 46: f (k_roll)
                # 47: f (k_pitch)
                
            ]
            
            # Correct Mapping
            # CSV: BatMV is after Sats
            bat_mv = fields[33]
            pm1 = fields[34]; pm25=fields[35]; pm10=fields[36]; ozone=fields[37]
            hum=fields[38]; press=fields[39]; t_baro=fields[40]
            rad=fields[41]
            heat_bat=fields[42]; heat_brd=fields[43]
            
            row_full = [
                u_ts, "SEQ?", fields[0], f"0x{fields[1]:04X}", fields[2],
                fields[3], fields[4], fields[5],
                fields[6], fields[7], fields[8],
                fields[9], fields[10], fields[11],
                fields[12], fields[13], fields[14], fields[15],
                fields[16], fields[17], f"{fields[18]:.2f}",
                fields[19], fields[20],
                bat_mv, pm1, pm25, pm10, ozone,
                hum, press, t_baro, rad,
                heat_bat, heat_brd,
                f"{fields[44]:.2f}", f"{fields[45]:.2f}", f"{fields[46]:.2f}", f"{fields[47]:.2f}"
            ]
            
            with open(self.filename, 'a', newline='') as f:
                writer = csv.writer(f)
                writer.writerow(row_full)
                
             # Optional: Update GUI state directly?
             # For now, just Log.
             
        except Exception as e:
            print(f"[DECODE ERR] {e}")

    def stop(self):
        self.running = False


class SensorSenderGUI(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("HITL Simulation Dashboard (PySide6)")
        self.resize(1400, 900)
        self.setPalette(DarkPalette())
        
        # Telemetry Receiver
        self.telem_rx = None
        
        # Matplotlib Dark Theme
        plt.style.use('dark_background')
        plt.rcParams['axes.facecolor'] = GRAPH_BG
        plt.rcParams['figure.facecolor'] = BG_COLOR
        plt.rcParams['savefig.facecolor'] = BG_COLOR

        # Data State
        self.ser = None
        self.is_running = False
        self.json_data = []
        self.data_index = 0
        self.start_time = 0

        # Buffers
        self.max_points = 100
        self.times = deque(maxlen=self.max_points)
        self.buf_alt = deque(maxlen=self.max_points)
        self.buf_press = deque(maxlen=self.max_points)
        self.buf_acc_z = deque(maxlen=self.max_points)
        self.buf_co2 = deque(maxlen=self.max_points)
        self.buf_rad = deque(maxlen=self.max_points)
        self.traj_lat = deque(maxlen=500)
        self.traj_lon = deque(maxlen=500)

        # Simulation Values
        self.s_lat = GNU_LAT
        self.s_lon = GNU_LON
        self.s_alt = GNU_ALT

        # UI Setup
        self._setup_ui()
        self._scan_ports()

        # Timer for Loop
        # Timer for Loop
        self.timer = QTimer()
        self.timer.timeout.connect(self._run_loop)
        self.timer.setInterval(20) # 20ms (50Hz) - High Speed

        # Rust Init
        if RUST_AVAILABLE:
            self.rust_sim = sim_core.RustSimulator(GNU_LAT, GNU_LON, GNU_ALT)

    def _setup_ui(self):
        central_widget = QWidget()
        self.setCentralWidget(central_widget)
        main_layout = QVBoxLayout(central_widget)

        # 1. Top Bar
        top_layout = QHBoxLayout()
        
        top_layout.addWidget(QLabel("COM Port:"))
        self.port_combo = QComboBox()
        self.port_combo.setMinimumWidth(150)
        top_layout.addWidget(self.port_combo)
        
        refresh_btn = QPushButton("R")
        refresh_btn.setFixedWidth(30)
        refresh_btn.clicked.connect(self._scan_ports)
        top_layout.addWidget(refresh_btn)

        self.connect_btn = QPushButton("CONNECT")
        self.connect_btn.clicked.connect(self._toggle_connection)
        self.connect_btn.setStyleSheet(f"background-color: {ACCENT_COLOR}; color: white; font-weight: bold;")
        top_layout.addWidget(self.connect_btn)

        top_layout.addSpacing(20)
        top_layout.addWidget(QLabel("Mode:"))
        self.mode_combo = QComboBox()
        self.mode_combo.addItems(["General (GNU)", "Scenario (JSON)", "FDIR Test"])
        top_layout.addWidget(self.mode_combo)

        self.start_btn = QPushButton("START")
        self.start_btn.setEnabled(False)
        self.start_btn.clicked.connect(self._toggle_simulation)
        self.start_btn.setStyleSheet("background-color: green; color: white; font-weight: bold;")
        top_layout.addWidget(self.start_btn)

        self.fault_btn = QPushButton("FAULT MENU")
        self.fault_btn.setEnabled(False)
        self.fault_btn.clicked.connect(self._open_fault_menu)
        self.fault_btn.setStyleSheet("background-color: orange; color: black; font-weight: bold;")
        top_layout.addWidget(self.fault_btn)

        self.auto_btn = QPushButton("RUN AUTO TEST")
        self.auto_btn.setEnabled(False)
        self.auto_btn.clicked.connect(self._toggle_auto_test)
        self.auto_btn.setStyleSheet("background-color: purple; color: white; font-weight: bold;")
        top_layout.addWidget(self.auto_btn)

        top_layout.addStretch()
        self.lbl_status = QLabel("DISCONNECTED")
        self.lbl_status.setStyleSheet("color: red; font-weight: bold; font-size: 14px;")
        top_layout.addWidget(self.lbl_status)

        main_layout.addLayout(top_layout)

        # 2. Graphs Grid
        grid_layout = QGridLayout()
        
        # Map
        self.fig_map = Figure(figsize=(5, 4), dpi=100)
        self.ax_map = self.fig_map.add_subplot(111)
        self.canvas_map = FigureCanvasQTAgg(self.fig_map)
        self._wrap_in_group(grid_layout, self.canvas_map, "Flight Trajectory", 0, 0)

        # Env (Alt/Press)
        self.fig_env = Figure(figsize=(5, 4), dpi=100)
        self.ax_alt = self.fig_env.add_subplot(211)
        self.ax_press = self.fig_env.add_subplot(212)
        self.canvas_env = FigureCanvasQTAgg(self.fig_env)
        self._wrap_in_group(grid_layout, self.canvas_env, "Environment", 0, 1)

        # IMU
        self.fig_imu = Figure(figsize=(5, 4), dpi=100)
        self.ax_imu = self.fig_imu.add_subplot(111)
        self.canvas_imu = FigureCanvasQTAgg(self.fig_imu)
        self._wrap_in_group(grid_layout, self.canvas_imu, "Dynamics (Acc Z)", 1, 0)

        # Payload
        self.fig_air = Figure(figsize=(5, 4), dpi=100)
        self.ax_co2 = self.fig_air.add_subplot(211)
        self.ax_rad = self.fig_air.add_subplot(212)
        self.canvas_air = FigureCanvasQTAgg(self.fig_air)
        self._wrap_in_group(grid_layout, self.canvas_air, "Payload (CO2 / Rad)", 1, 1)

        main_layout.addLayout(grid_layout)
        self._init_plots()

    def _wrap_in_group(self, layout, widget, title, row, col):
        group = QGroupBox(title)
        group.setStyleSheet(f"QGroupBox {{ color: {ACCENT_COLOR}; font-weight: bold; border: 1px solid {PANE_BG}; }}")
        vbox = QVBoxLayout()
        vbox.addWidget(widget)
        group.setLayout(vbox)
        layout.addWidget(group, row, col)

    def _init_plots(self):
        # Map
        self.ax_map.set_xlabel("Longitude")
        self.ax_map.set_ylabel("Latitude")
        self.ax_map.grid(True, linestyle="--", alpha=0.3)
        self.ax_map.set_title("GPS Track")
        
        # Init Lines (Empty)
        self.line_traj, = self.ax_map.plot([], [], 'y-', linewidth=2, label='Path')
        self.line_pos, = self.ax_map.plot([], [], 'ro', label='Curr')
        
        # Env
        self.ax_alt.set_ylabel("Alt (m)")
        self.ax_press.set_ylabel("Press (hPa)")
        self.line_alt, = self.ax_alt.plot([], [], 'g-')
        self.line_press, = self.ax_press.plot([], [], 'c-')
        
        # IMU
        self.ax_imu.set_ylabel("Acc Z (m/s²)")
        self.ax_imu.set_ylim(8, 12)
        self.line_acc, = self.ax_imu.plot([], [], 'm-')
        
        # Air
        self.ax_co2.set_ylabel("CO2 (ppm)")
        self.ax_rad.set_ylabel("Rad (uSv/h)")
        self.line_co2, = self.ax_co2.plot([], [], 'y-')
        self.line_rad, = self.ax_rad.plot([], [], 'r-')
        
        for fig in [self.fig_map, self.fig_env, self.fig_imu, self.fig_air]:
            fig.tight_layout()

    def _scan_ports(self):
        ports = [p.device for p in serial.tools.list_ports.comports()]
        ports.append("MOCK (Test Mode)")
        self.port_combo.clear()
        self.port_combo.addItems(ports)

    def _update_plots(self):
        if not self.times: return
        
        t = list(self.times)
        
        # 1. Map Update (Optimized)
        if len(self.traj_lat) > 0:
            self.line_traj.set_data(list(self.traj_lon), list(self.traj_lat))
            self.line_pos.set_data([self.traj_lon[-1]], [self.traj_lat[-1]])
            
            self.ax_map.relim()
            self.ax_map.autoscale_view()
            
            # Draw Map Background (Only once or if needed)
            if MAP_AVAILABLE and not self.map_drawn and len(self.traj_lat) > 1:
                try:
                    cx.add_basemap(self.ax_map, crs='EPSG:4326', source=cx.providers.OpenStreetMap.Mapnik, zoom=13)
                    self.map_drawn = True
                except: pass
        
        self.canvas_map.draw()

        # 2. Env
        self.line_alt.set_data(t, list(self.buf_alt))
        self.ax_alt.relim(); self.ax_alt.autoscale_view()
        
        self.line_press.set_data(t, list(self.buf_press))
        self.ax_press.relim(); self.ax_press.autoscale_view()
        self.canvas_env.draw()

        # 3. IMU
        self.line_acc.set_data(t, list(self.buf_acc_z))
        self.ax_imu.relim(); self.ax_imu.autoscale_view()
        self.canvas_imu.draw()

        # 4. Payload
        self.line_co2.set_data(t, list(self.buf_co2))
        self.ax_co2.relim(); self.ax_co2.autoscale_view()
        
        self.line_rad.set_data(t, list(self.buf_rad))
        self.ax_rad.relim(); self.ax_rad.autoscale_view()
        self.canvas_air.draw()
        ports = [p.device for p in serial.tools.list_ports.comports()]
        ports.append("MOCK (Test Mode)")
        self.port_combo.clear()
        self.port_combo.addItems(ports)

    def _toggle_connection(self):
        if self.ser and (self.ser == "MOCK" or self.ser.is_open):
            # Disconnect
            if self.telem_rx:
                self.telem_rx.stop()
                self.telem_rx.join(timeout=1.0)
                self.telem_rx = None
                
            if self.ser != "MOCK": self.ser.close()
            self.ser = None
            self.connect_btn.setText("CONNECT")
            self.connect_btn.setStyleSheet(f"background-color: {ACCENT_COLOR}; color: white; font-weight: bold;")
            self.start_btn.setEnabled(False)
            self.fault_btn.setEnabled(False)
            self.auto_btn.setEnabled(False)
            if hasattr(self, 'test_runner') and self.test_runner.is_alive():
                self.test_runner.stop()
            self.lbl_status.setText("DISCONNECTED")
            self.lbl_status.setStyleSheet("color: red; font-weight: bold; font-size: 14px;")
        else:
            # Connect
            port = self.port_combo.currentText()
            try:
                if "MOCK" in port:
                    self.ser = "MOCK"
                else:
                    self.ser = serial.Serial(port, 115200, timeout=1)
                
                # Start Telemetry Logger
                timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
                filename = f"telemetry_log_{timestamp}.csv"
                self.telem_rx = TelemetryReceiver(self.ser, filename)
                self.telem_rx.start()
                
                self.connect_btn.setText("DISCONNECT")
                self.connect_btn.setStyleSheet("background-color: red; color: white; font-weight: bold;")
                self.start_btn.setEnabled(True)
                self.fault_btn.setEnabled(True)
                self.auto_btn.setEnabled(True)
                self.lbl_status.setText(f"CONNECTED ({port})")
                self.lbl_status.setStyleSheet("color: #00ff00; font-weight: bold; font-size: 14px;")
            except Exception as e:
                QMessageBox.critical(self, "Connection Error", str(e))

    def _toggle_simulation(self):
        if self.is_running:
            self.is_running = False
            self.timer.stop()
            self.start_btn.setText("START")
            self.start_btn.setStyleSheet("background-color: green; color: white; font-weight: bold;")
        else:
            mode = self.mode_combo.currentText()
            if "Scenario" in mode:
                if not self._load_json(): return
            
            self.is_running = True
            self._reset_data()
            self.start_time = time.time()
            self.timer.start()
            self.start_btn.setText("STOP")
            self.start_btn.setStyleSheet("background-color: orange; color: black; font-weight: bold;")

    def _reset_data(self):
        self.times.clear()
        self.buf_alt.clear(); self.buf_press.clear()
        self.buf_acc_z.clear(); self.buf_co2.clear(); self.buf_rad.clear()
        self.traj_lat.clear(); self.traj_lon.clear()
        self.ax_map.clear(); self.ax_alt.clear(); self.ax_press.clear()
        self.ax_imu.clear(); self.ax_co2.clear(); self.ax_rad.clear()
        self._init_plots() # Re-apply labels

    def _load_json(self):
        if not os.path.exists(DEFAULT_JSON_PATH):
            QMessageBox.warning(self, "Error", f"File not found: {DEFAULT_JSON_PATH}")
            return False
        try:
            with open(DEFAULT_JSON_PATH, 'r') as f:
                self.json_data = json.load(f)
            self.data_index = 0
            return True
        except Exception as e:
            QMessageBox.warning(self, "Error", f"JSON Load Error: {e}")
            return False

    def _run_loop(self):
        mode = self.mode_combo.currentText()

        # 0. Rust Mode (High Performance) - Only for Non-Scenario
        if RUST_AVAILABLE and hasattr(self, 'rust_sim') and "Scenario" not in mode:
            t = time.time()
            self.rust_sim.update(t)
            
            # 1. Packet
            packet = self.rust_sim.get_packet(t)
            if packet and self.ser:
                if self.ser == "MOCK":
                    print(f"[MOCK TX] {packet.strip()}")
                else:
                    try: self.ser.write(packet.encode())
                    except: pass
            
            # 2. GUI Update
            state = self.rust_sim.get_gui_state()
            
            self.s_lat = state[0]; self.s_lon = state[1]; self.s_alt = state[2]
            
            self.times.append(t - self.start_time)
            self.buf_alt.append(state[2])
            self.buf_press.append(state[3])
            self.buf_acc_z.append(state[4])
            self.buf_co2.append(state[5])
            self.buf_rad.append(state[6])
            
            self.traj_lat.append(state[0])
            self.traj_lon.append(state[1])
            
            self._update_plots()
            return

        # --- Legacy Python Mode (Fallback & Scenario) ---
        
        # 1. Generate Base Data
        if "Scenario" in mode and self.json_data:
            # Throttle playback to 1Hz (JSON data is 1Hz)
            if not hasattr(self, 'last_json_time'): self.last_json_time = 0
            
            if time.time() - self.last_json_time >= 1.0:
                if self.data_index >= len(self.json_data): self.data_index = 0
                record = self.json_data[self.data_index]
                self.s_lat = record.get("lat", GNU_LAT)
                self.s_lon = record.get("lon", GNU_LON)
                self.s_alt = record.get("alt", GNU_ALT)
                self.data_index += 1
                self.last_json_time = time.time()
        else:
            self.s_lat = GNU_LAT
            self.s_lon = GNU_LON
            self.s_alt = GNU_ALT
            if not hasattr(self, 'last_json_time'): self.last_json_time = time.time()

        # 2. Simulate Sensors (Full Suite)
        # Time
        uptime = int((time.time() - self.start_time) * 1000)
        
        # Env (Floats for GUI, Integers for Packet)
        press = 1013.25 * math.pow((1 - 2.25577e-5 * self.s_alt), 5.25588) # hPa
        press_pa = int(press * 100) # Pascal
        
        temp = 25.0 - (0.0065 * self.s_alt) # Celsius
        temp_c = int(temp * 100) # x100
        hum = 50.0
        
        # IMU (Floats for GUI, Scaled Ints for Packet)
        acc_x_g = random.uniform(-0.1, 0.1)
        acc_y_g = random.uniform(-0.1, 0.1)
        acc_z_g = 9.8 + random.uniform(-0.5, 0.5) # m/s^2 approx
        
        acc_x = int(acc_x_g * 1000) # x1000 as per struct
        acc_y = int(acc_y_g * 1000)
        acc_z = int(acc_z_g * 1000) 
        
        gyro_x = 0; gyro_y = 0; gyro_z = 0
        mag_x = 0; mag_y = 0; mag_z = 0
        
        # Temps (x100)
        t_board = int(30.5 * 100)
        t_ext = temp_c
        t_sht = temp_c
        t_bat = int(28.0 * 100)
        
        # GPS (x10^7)
        lat_e7 = int(self.s_lat * 1e7)
        lon_e7 = int(self.s_lon * 1e7)
        
        # Payload
        co2 = 400 + int(self.s_alt / 10) + random.randint(-10, 10)
        pm1 = 5; pm25 = 10; pm10 = 15; ozone = 20
        
        rad = 0.1 + (self.s_alt / 5000) * 2.0 # uSv/h
        rad_x100 = int(rad * 100)
        
        # 3. Packet Construction (Strict Order matching Firmware)
        # Order: 
        # 0:uptime, 1:status, 2:co2, 
        # 3:ax, 4:ay, 5:az, 6:gx, 7:gy, 8:gz, 9:mx, 10:my, 11:mz, 
        # 12:t_board, 13:t_ext, 14:t_sht, 15:t_bat, 
        # 16:lat, 17:lon, 18:alt, 19:fix, 20:sat_used, 21:sat_tot, 22:sat_gps, 23:sat_glo, 24:sat_gal, 25:sat_bei, 
        # 26:h, 27:m, 28:s, 29:D, 30:M, 31:Y, 
        # 32:bat_mv, 
        # 33:pm1, 34:pm25, 35:pm10, 36:ozone, 
        # 37:rh, 38:p_pa, 39:p_temp, 
        # 40:rad, 41:heat_bat, 42:heat_brd, 
        # 43:p_alt, 44:k_alt, 45:k_roll, 46:k_pitch
        
        parts = []
        parts.append(str(uptime))           # 0
        parts.append("0")                   # 1: Status
        parts.append(str(co2))              # 2
        
        parts.extend([str(acc_x), str(acc_y), str(acc_z)]) # 3,4,5
        parts.extend([str(gyro_x), str(gyro_y), str(gyro_z)]) # 6,7,8
        parts.extend([str(mag_x), str(mag_y), str(mag_z)])    # 9,10,11
        
        parts.extend([str(t_board), str(t_ext), str(t_sht), str(t_bat)]) # 12-15
        
        parts.extend([str(lat_e7), str(lon_e7), f"{self.s_alt:.1f}"]) # 16-18
        parts.extend(["3", "12", "15", "8", "4", "2", "1"]) # 19-25 (Fix, Sats...)
        
        parts.extend(["12","00","00","1","1","2026"]) # 26-31 (Time)
        
        parts.append("4200") # 32: Bat mV
        
        parts.extend([str(pm1), str(pm25), str(pm10), str(ozone)]) # 33-36
        
        parts.extend([str(int(hum*100)), str(press_pa), str(temp_c)]) # 37-39
        
        parts.append(str(rad_x100)) # 40
        parts.extend(["0", "0"]) # 41-42 Heaters
        
        parts.extend([f"{self.s_alt:.1f}", f"{self.s_alt:.1f}", "0.0", "0.0"]) # 43-46 Fusion
        
        packet = "ALL:" + ",".join(parts) + "\n"

        if self.ser:
            if self.ser == "MOCK":
                print(f"[MOCK TX] {packet.strip()}")
            else:
                try:
                    self.ser.write(packet.encode())
                except: pass
        
        # 4. Update GUI Buffers
        t = time.time() - self.start_time
        self.times.append(t)
        self.buf_alt.append(self.s_alt)
        self.buf_press.append(press)
        self.buf_acc_z.append(acc_z_g) # Use physical float value
        self.buf_co2.append(co2)
        self.buf_rad.append(rad)
        self.traj_lat.append(self.s_lat)
        self.traj_lon.append(self.s_lon)

        self._update_plots()

    def _update_plots(self):
        if not self.times: return
        
        t = list(self.times)
        
        # 1. Map
        self.ax_map.clear()
        self.ax_map.set_xlabel("Longitude")
        self.ax_map.set_ylabel("Latitude")
        self.ax_map.grid(True, linestyle="--", alpha=0.3)
        self.ax_map.set_title("GPS Track")
        if len(self.traj_lat) > 0:
            self.ax_map.plot(list(self.traj_lon), list(self.traj_lat), 'y-', linewidth=2)
            self.ax_map.plot(self.traj_lon[-1], self.traj_lat[-1], 'ro') # Current Pos
        self.canvas_map.draw()

        # 2. Environment
        self.ax_alt.clear()
        self.ax_alt.plot(t, list(self.buf_alt), 'g-')
        self.ax_alt.set_ylabel("Alt (m)")
        self.ax_alt.grid(True, alpha=0.3)
        
        self.ax_press.clear()
        self.ax_press.plot(t, list(self.buf_press), 'c-')
        self.ax_press.set_ylabel("Press (hPa)")
        self.ax_press.grid(True, alpha=0.3)
        self.canvas_env.draw()

        # 3. IMU
        self.ax_imu.clear()
        self.ax_imu.plot(t, list(self.buf_acc_z), 'm-')
        self.ax_imu.set_ylabel("Acc Z (m/s²)")
        self.ax_imu.set_ylim(8, 12) # Zoom in around 9.8
        self.ax_imu.grid(True, alpha=0.3)
        self.canvas_imu.draw()

        # 4. Payload
        self.ax_co2.clear()
        self.ax_co2.plot(t, list(self.buf_co2), 'y-')
        self.ax_co2.set_ylabel("CO2 (ppm)")
        self.ax_co2.grid(True, alpha=0.3)
        
        self.ax_rad.clear()
        self.ax_rad.plot(t, list(self.buf_rad), 'r-')
        self.ax_rad.set_ylabel("Rad (uSv/h)")
        self.ax_rad.grid(True, alpha=0.3)
        self.canvas_air.draw()

        self.canvas_air.draw()

    def _toggle_auto_test(self):
        if not hasattr(self, 'test_runner') or not self.test_runner.is_alive():
            self.test_runner = AutomatedTestRunner(self.ser, self.lbl_status)
            self.test_runner.start()
            self.auto_btn.setText("STOP AUTO TEST")
            self.auto_btn.setStyleSheet("background-color: red; color: white; font-weight: bold;")
        else:
            self.test_runner.stop()
            self.auto_btn.setText("RUN AUTO TEST")
            self.auto_btn.setStyleSheet("background-color: purple; color: white; font-weight: bold;")

    def _open_fault_menu(self):
        dlg = QDialog(self)
        dlg.setWindowTitle("Fault Injection (All Sensors)")
        dlg.setMinimumWidth(450)
        dlg.setMinimumHeight(600)
        
        main_layout = QVBoxLayout()

        lbl = QLabel("Select Fault to Inject:")
        lbl.setStyleSheet("font-weight: bold; font-size: 16px; color: orange;")
        main_layout.addWidget(lbl)
        
        scroll = QScrollArea()
        scroll.setWidgetResizable(True)
        content_widget = QWidget()
        layout = QVBoxLayout(content_widget)

        def inject(s, t, d):
            cmd = f"CMD,FAULT,{s},{t},{d}\n"
            if self.ser == "MOCK":
                print(f"[MOCK FAULT] {cmd.strip()}")
            elif self.ser:
                try: self.ser.write(cmd.encode())
                except: pass
            QMessageBox.information(dlg, "Injected", f"Sent: {s} {t} ({d}s)")

        # 1. Navigation (GPS/Baro)
        grp_nav = QGroupBox("Navigation Sensors")
        l_nav = QGridLayout()
        l_nav.addWidget(QLabel("GPS:"), 0, 0)
        b1 = QPushButton("TIMEOUT (10s)"); b1.clicked.connect(lambda: inject("GPS","TIMEOUT",10))
        b2 = QPushButton("FIX LOSS (10s)"); b2.clicked.connect(lambda: inject("GPS","FIX_LOSS",10))
        l_nav.addWidget(b1, 0, 1); l_nav.addWidget(b2, 0, 2)
        
        l_nav.addWidget(QLabel("Baro:"), 1, 0)
        b3 = QPushButton("FREEZE (15s)"); b3.clicked.connect(lambda: inject("BARO","FREEZE",15))
        b4 = QPushButton("NOISE (20s)"); b4.clicked.connect(lambda: inject("BARO","NOISE",20))
        l_nav.addWidget(b3, 1, 1); l_nav.addWidget(b4, 1, 2)
        grp_nav.setLayout(l_nav)
        layout.addWidget(grp_nav)

        # 2. Dynamics (IMU)
        grp_imu = QGroupBox("Dynamics (IMU)")
        l_imu = QGridLayout()
        l_imu.addWidget(QLabel("Accel:"), 0, 0)
        b5 = QPushButton("HIGH-G (>16G)"); b5.clicked.connect(lambda: inject("IMU","HIGH_G",5))
        b6 = QPushButton("FREEZE (10s)"); b6.clicked.connect(lambda: inject("IMU","FREEZE",10))
        l_imu.addWidget(b5, 0, 1); l_imu.addWidget(b6, 0, 2)
        
        l_imu.addWidget(QLabel("Gyro:"), 1, 0)
        b7 = QPushButton("SPIN (Extreme)"); b7.clicked.connect(lambda: inject("IMU","SPIN",5))
        b8 = QPushButton("BIAS DRIFT"); b8.clicked.connect(lambda: inject("IMU","DRIFT",20))
        l_imu.addWidget(b7, 1, 1); l_imu.addWidget(b8, 1, 2)
        grp_imu.setLayout(l_imu)
        layout.addWidget(grp_imu)

        # 3. Environment
        grp_env = QGroupBox("Environment")
        l_env = QGridLayout()
        l_env.addWidget(QLabel("Temp:"), 0, 0)
        b9 = QPushButton("HEAT WAVE (+60C)"); b9.clicked.connect(lambda: inject("ENV","HEAT",10))
        b10 = QPushButton("SENS FAIL (-999)"); b10.clicked.connect(lambda: inject("ENV","FAIL",10))
        l_env.addWidget(b9, 0, 1); l_env.addWidget(b10, 0, 2)
        grp_env.setLayout(l_env)
        layout.addWidget(grp_env)

        # 4. Payload (Air/Rad)
        grp_pay = QGroupBox("Payload Sensors")
        l_pay = QGridLayout()
        l_pay.addWidget(QLabel("CO2:"), 0, 0)
        b11 = QPushButton("LEAK (>2000ppm)"); b11.clicked.connect(lambda: inject("CO2","HIGH",10))
        l_pay.addWidget(b11, 0, 1)
        
        l_pay.addWidget(QLabel("Rad:"), 1, 0)
        b12 = QPushButton("SPIKE (>10uSv)"); b12.clicked.connect(lambda: inject("RAD","HIGH",10))
        l_pay.addWidget(b12, 1, 1)
        
        l_pay.addWidget(QLabel("PM2.5:"), 2, 0)
        b13 = QPushButton("DUST STORM"); b13.clicked.connect(lambda: inject("PM","HIGH",10))
        l_pay.addWidget(b13, 2, 1)
        grp_pay.setLayout(l_pay)
        layout.addWidget(grp_pay)

        scroll.setWidget(content_widget)
        main_layout.addWidget(scroll)
        
        close_btn = QPushButton("Close")
        close_btn.clicked.connect(dlg.accept)
        main_layout.addWidget(close_btn)

        dlg.setLayout(main_layout)
        dlg.exec()

import threading

class AutomatedTestRunner(threading.Thread):
    def __init__(self, serial_port, status_label):
        super().__init__()
        self.ser = serial_port
        self.status_label = status_label
        self.running = True
        # Comprehensive FDIR Test Sequence (from FDIR.md/FMEA.md)
        self.sequence = [
            ("WARM_UP", "System Stabilization", 5, None, None, 0),
            # GPS Checks
            ("GPS_TIMEOUT", "S-04: GPS Timeout Test", 5, "GPS", "TIMEOUT", 5),
            ("RECOVERY", "Recovery Wait", 5, None, None, 0),
            ("GPS_FIX", "S-04: GPS Fix Loss Test", 5, "GPS", "FIX_LOSS", 5),
            ("RECOVERY", "Recovery Wait", 5, None, None, 0),
            # Baro Checks
            ("BARO_FREEZE", "S-08: Baro Freeze Test", 5, "BARO", "FREEZE", 5),
            ("RECOVERY", "Recovery Wait", 5, None, None, 0),
            ("BARO_NOISE", "Baro Noise Test", 5, "BARO", "NOISE", 5),
            ("RECOVERY", "Recovery Wait", 5, None, None, 0),
            # IMU Checks
            ("IMU_HIGH_G", "S-03: IMU High-G Test", 5, "IMU", "HIGH_G", 5),
            ("RECOVERY", "Recovery Wait", 5, None, None, 0),
            ("IMU_FREEZE", "S-01: IMU Freeze Test", 5, "IMU", "FREEZE", 5),
            ("RECOVERY", "Recovery Wait", 5, None, None, 0),
            ("IMU_SPIN", "IMU Spin Test", 5, "IMU", "SPIN", 5),
            ("RECOVERY", "Recovery Wait", 5, None, None, 0),
            # Env Checks
            ("ENV_HEAT", "S-11: Temp High Test", 5, "ENV", "HEAT", 5),
            ("RECOVERY", "Recovery Wait", 5, None, None, 0),
            ("ENV_FAIL", "Temp Sensor Fail Test", 5, "ENV", "FAIL", 5),
            ("RECOVERY", "Recovery Wait", 5, None, None, 0),
            # Payload Checks
            ("CO2_LEAK", "CO2 Leak Alert Test", 5, "CO2", "HIGH", 5),
            ("RECOVERY", "Recovery Wait", 5, None, None, 0),
            ("RAD_SPIKE", "S-12: Radiation Spike Test", 5, "RAD", "HIGH", 5),
            ("RECOVERY", "Recovery Wait", 5, None, None, 0),
            ("PM_DUST", "Dust Storm Alert Test", 5, "PM", "HIGH", 5),
            ("FINISH", "All Tests Completed", 0, None, None, 0)
        ]

    def run(self):
        for step_name, desc, duration, comp, fault, fault_dur in self.sequence:
            if not self.running: break
            
            # Update GUI Status (Thread-safe update needed in prod, simplified here)
            self.status_label.setText(f"AUTO TEST: {desc} ({duration}s)")
            self.status_label.setStyleSheet("color: magenta; font-weight: bold; font-size: 16px;")
            
            # Inject Fault if defined
            if comp and fault:
                cmd = f"CMD,FAULT,{comp},{fault},{fault_dur}\n"
                if self.ser == "MOCK":
                    print(f"[AUTO TEST] Injecting: {cmd.strip()}")
                elif self.ser:
                    try: self.ser.write(cmd.encode())
                    except: pass
            
            # Wait for duration
            for _ in range(duration):
                if not self.running: break
                time.sleep(1)
        
        if self.running:
            self.status_label.setText("AUTO TEST COMPLETED")
            self.status_label.setStyleSheet("color: green; font-weight: bold; font-size: 16px;")

    def stop(self):
        self.running = False

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = SensorSenderGUI()
    window.show()
    sys.exit(app.exec())
