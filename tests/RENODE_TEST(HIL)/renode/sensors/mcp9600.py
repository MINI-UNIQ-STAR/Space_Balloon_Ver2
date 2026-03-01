# MCP9600 Thermocouple Sensor Model (Python)
from Antmicro.Renode.Peripherals.I2C import II2CPeripheral
import System

class MCP9600(II2CPeripheral):
    def __init__(self): self.Reset()
    def Reset(self): self.reg=0; self.hot=100.0
    def Write(self, data):
        if len(data)>0: self.reg=data[0]
    def Read(self, count):
        if self.reg==0x00: # Hot Junction
            val = int(self.hot / 0.0625)
            return System.Array[System.Byte]([(val>>8)&0xFF, val&0xFF])
        return System.Array[System.Byte]([0]*count)
    def FinishTransmission(self): pass
