# Minimal sensors for FDIR test - single line compatible
from Antmicro.Renode.Peripherals.I2C import II2CPeripheral
import System

class LSM6DSV16X(II2CPeripheral):
    def __init__(self): self.reg = 0
    def Reset(self): pass
    def Write(self, data):
        if len(data) > 0: self.reg = data[0]
    def Read(self, count):
        r = [0x70 if (self.reg + i) == 0x0F else 0 for i in range(count)]
        return System.Array[System.Byte](r)
    def FinishTransmission(self): pass

class MS5611(II2CPeripheral):
    def __init__(self): self.prom = [0, 40127, 36924, 23317, 23282, 32463, 28312, 0]; self.idx = 0; self.cmd = 0
    def Reset(self): pass
    def Write(self, data):
        if len(data) == 0: return
        self.cmd = data[0]
        if 0xA0 <= self.cmd <= 0xAE: self.idx = (self.cmd - 0xA0) >> 1
    def Read(self, count):
        if self.idx < len(self.prom):
            v = self.prom[self.idx]
            return System.Array[System.Byte]([(v>>8)&0xFF, v&0xFF])
        return System.Array[System.Byte]([0]*count)
    def FinishTransmission(self): pass

class SHT31(II2CPeripheral):
    def __init__(self): pass
    def Reset(self): pass
    def Write(self, data): pass
    def Read(self, count): return System.Array[System.Byte]([0x30, 0x65, 0x41, 0x88, 0x0F, 0x4D][:count])
    def FinishTransmission(self): pass
