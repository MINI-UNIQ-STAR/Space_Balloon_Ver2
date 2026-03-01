# MLX90393 Magnetometer Sensor Model (Python)
from Antmicro.Renode.Peripherals.I2C import II2CPeripheral
import System

class MLX90393(II2CPeripheral):
    def __init__(self): self.Reset()
    def Reset(self): self.state="IDLE"
    def Write(self, data):
        if len(data)>0:
            if data[0]==0x3E: pass # Start Burst
            elif data[0]==0x4E: self.state="READ_MEAS"
            elif data[0]==0x50: pass # Exit Burst
            elif data[0]==0xF0: self.Reset()
    def Read(self, count):
        if self.state=="READ_MEAS":
            # Status(1) + X(2) + Y(2) + Z(2) + T(2)
            # Returning mock data
            return System.Array[System.Byte]([0x00, 0x00, 0x10, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00])
        return System.Array[System.Byte]([0]*count)
    def FinishTransmission(self): pass
