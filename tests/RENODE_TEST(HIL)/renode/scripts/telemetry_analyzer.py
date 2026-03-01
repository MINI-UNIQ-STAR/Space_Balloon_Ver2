# Telemetry Frame Analyzer for Renode FDIR Testing
# Parses 148-byte frames from UART3 and extracts FDIR status_flags
#
# Frame Structure (telemetry.h):
#   [0-1]   Magic: 0xA5, 0x5A
#   [2]     Version: 1
#   [3]     MsgType: 0x01=Heartbeat, 0x02=SensorSnapshot
#   [4-5]   PayloadLen (LE)
#   [6-7]   Seq (LE)
#   [8-11]  Timestamp (LE)
#   [12-143] Payload (132 bytes)
#   [144-145] CRC16
#
# FDIR status_flags bits:
#   0: SYS_OK
#   1: GPS_WARN
#   2: BARO_WARN
#   3: IMU_WARN
#   4: TEMP_WARN
#   5: HEATER_ACTIVE
#   6: LOW_BATTERY
#   7: FDIR_RECOVERY
#   8: ALT_JUMP
#   9: RANGE_ERROR

from Antmicro.Renode.Peripherals.UART import UARTBackend
from Antmicro.Renode.Logging import Logger, LogLevel
from Antmicro.Renode.Core import IAnalyzable
import System
import struct

class TelemetryAnalyzer(UARTBackend):
    """UART backend that parses telemetry frames and logs FDIR status"""
    
    # FDIR status flag names
    FLAG_NAMES = {
        0: "SYS_OK",
        1: "GPS_WARN",
        2: "BARO_WARN",
        3: "IMU_WARN",
        4: "TEMP_WARN",
        5: "HEATER_ACTIVE",
        6: "LOW_BATTERY",
        7: "FDIR_RECOVERY",
        8: "ALT_JUMP",
        9: "RANGE_ERROR",
    }
    
    def __init__(self, machine=None):
        self.buffer = bytearray()
        self.frame_count = 0
        self.last_status_flags = 0
        self.last_uptime_ms = 0
        self.fdir_events = []  # List of (timestamp, event_type, sensor)
        self.Logger = Logger(self)
        
    def Reset(self):
        self.buffer = bytearray()
        self.frame_count = 0
        self.last_status_flags = 0
        self.last_uptime_ms = 0
        self.fdir_events = []
        
    def WriteChar(self, char):
        """Called by UART when a byte is transmitted"""
        byte = ord(char) if isinstance(char, str) else int(char) & 0xFF
        self.buffer.append(byte)
        self._try_parse_frame()
        return 0
        
    def _try_parse_frame(self):
        """Try to parse a complete 148-byte frame from buffer"""
        FRAME_SIZE = 148
        MAGIC = bytes([0xA5, 0x5A])
        
        while len(self.buffer) >= FRAME_SIZE:
            # Find frame start
            if self.buffer[0:2] != MAGIC:
                # Skip byte and resync
                self.buffer.pop(0)
                continue
                
            # Extract frame
            frame = bytes(self.buffer[:FRAME_SIZE])
            
            # Verify CRC
            if self._verify_crc(frame):
                self._process_frame(frame)
            else:
                self.Logger.Log(LogLevel.Warning, f"[Telemetry] CRC mismatch at frame #{self.frame_count}")
                
            # Remove processed frame
            self.buffer = self.buffer[FRAME_SIZE:]
            
    def _verify_crc(self, frame):
        """Verify CRC-16/CCITT-FALSE"""
        # CRC is calculated over first 146 bytes (excluding CRC field)
        crc_calc = self._crc16_ccitt(frame[:-2])
        crc_recv = struct.unpack('<H', frame[144:146])[0]
        return crc_calc == crc_recv
        
    def _crc16_ccitt(self, data):
        """CRC-16/CCITT-FALSE calculation"""
        crc = 0xFFFF
        for byte in data:
            crc ^= byte << 8
            for _ in range(8):
                if crc & 0x8000:
                    crc = ((crc << 1) ^ 0x1021) & 0xFFFF
                else:
                    crc = (crc << 1) & 0xFFFF
        return crc
        
    def _process_frame(self, frame):
        """Parse and log telemetry frame"""
        self.frame_count += 1
        
        # Parse header
        version = frame[2]
        msg_type = frame[3]
        payload_len = struct.unpack('<H', frame[4:6])[0]
        seq = struct.unpack('<H', frame[6:8])[0]
        timestamp_ms = struct.unpack('<I', frame[8:12])[0]
        
        # Parse payload (offset 12 in frame, offset 0 in payload struct)
        # uptime_ms: bytes 0-3
        # status_flags: bytes 4-5
        uptime_ms = struct.unpack('<I', frame[12:16])[0]
        status_flags = struct.unpack('<H', frame[16:18])[0]
        
        # Detect FDIR state changes
        if status_flags != self.last_status_flags:
            changes = status_flags ^ self.last_status_flags
            for bit in range(10):
                if changes & (1 << bit):
                    flag_name = self.FLAG_NAMES.get(bit, f"BIT{bit}")
                    state = "SET" if status_flags & (1 << bit) else "CLEARED"
                    
                    # Log FDIR event
                    event = (timestamp_ms, flag_name, state)
                    self.fdir_events.append(event)
                    
                    if state == "SET":
                        self.Logger.Log(LogLevel.Info, 
                            f"[FDIR] {flag_name} SET at {timestamp_ms}ms (uptime: {uptime_ms}ms)")
                    else:
                        self.Logger.Log(LogLevel.Info,
                            f"[FDIR] {flag_name} CLEARED at {timestamp_ms}ms")
                            
            self.last_status_flags = status_flags
            
        self.last_uptime_ms = uptime_ms
        
        # Log periodic summary (every 50 frames = ~1 second at 50Hz)
        if self.frame_count % 50 == 0:
            flags_str = self._format_flags(status_flags)
            self.Logger.Log(LogLevel.Debug, 
                f"[Telemetry] Frame #{self.frame_count}, uptime={uptime_ms}ms, flags=0x{status_flags:04X} ({flags_str})")
            
    def _format_flags(self, flags):
        """Format status flags as string"""
        active = []
        for bit, name in self.FLAG_NAMES.items():
            if flags & (1 << bit):
                active.append(name)
        return ", ".join(active) if active else "NONE"
        
    def GetFDIRStatus(self):
        """Return current FDIR status for test verification"""
        return {
            'status_flags': self.last_status_flags,
            'uptime_ms': self.last_uptime_ms,
            'frame_count': self.frame_count,
            'events': self.fdir_events.copy()
        }
        
    def HasFlag(self, flag_bit):
        """Check if a specific FDIR flag is set"""
        return bool(self.last_status_flags & (1 << flag_bit))
        
    def WaitForFlag(self, flag_bit, timeout_ms=30000):
        """Block until flag is set or timeout (for test synchronization)"""
        import time
        start = time.time()
        timeout_sec = timeout_ms / 1000.0
        
        while (time.time() - start) < timeout_sec:
            if self.HasFlag(flag_bit):
                return True
            time.sleep(0.1)
        return False
        
    def ClearEvents(self):
        """Clear recorded FDIR events"""
        self.fdir_events = []


# Factory function for Renode
def create_telemetry_analyzer():
    return TelemetryAnalyzer()
