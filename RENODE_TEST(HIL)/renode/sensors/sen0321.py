# SEN0321 Ozone Sensor Model (Python)
from Antmicro.Renode.Peripherals.I2C import II2CPeripheral
import System

class SEN0321(II2CPeripheral):
    def __init__(self): self.Reset()
    def Reset(self): self.reg=0; self.ppb=20
    def Write(self, data):
        if len(data)>0: self.reg=data[0]
    def Read(self, count):
        # Auto Data High/Low (0x09, 0x0A)
        # Note: In real sensor, you might read multiple bytes.
        # This mock simplifies to return 1 byte per read.
        if self.reg==0x09: return System.Array[System.Byte]([(self.ppb>>8)&0xFF])
        if self.reg==0x0A: return System.Array[System.Byte]([self.ppb&0xFF])
        return System.Array[System.Byte]([0]*count)
    def FinishTransmission(self): pass
