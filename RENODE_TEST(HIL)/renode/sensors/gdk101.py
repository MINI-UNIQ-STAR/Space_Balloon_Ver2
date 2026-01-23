# GDK101 Radiation Sensor Model (Python)
from Antmicro.Renode.Peripherals.I2C import II2CPeripheral
import System

class GDK101(II2CPeripheral):
    def __init__(self): self.Reset()
    def Reset(self): self.reg=0; self.rad=0.08
    def Write(self, data):
        if len(data)>0: self.reg=data[0]
    def Read(self, count):
        if self.reg==0x00: return System.Array[System.Byte]([0x01]) # Status
        if self.reg==0x03: # 1min Value
            val = int(self.rad)
            dec = int((self.rad - val)*100)
            return System.Array[System.Byte]([val, dec])
        return System.Array[System.Byte]([0]*count)
    def FinishTransmission(self): pass
