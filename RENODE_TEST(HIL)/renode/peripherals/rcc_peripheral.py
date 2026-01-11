# RCC Peripheral for Renode using Python API

from Antmicro.Renode.Peripherals.Bus import IDoubleWordPeripheral
from Antmicro.Renode.Core import EmptyRegisterPeripheralBase
from Antmicro.Renode.Logging import Logger, LogLevel

class RCC_Peripheral(EmptyRegisterPeripheralBase, IDoubleWordPeripheral):
    def __init__(self):
        EmptyRegisterPeripheralBase.__init__(self)
        # Default register values
        self.CR = 0x00000063      # HSI ON and ready
        self.CFGR = 0x00000000
        self.PLLCFGR = 0x00001000
        self.AHB1ENR = 0x00000100 # Flash enabled
        self.AHB2ENR = 0x00000000
        self.APB1ENR1 = 0x00000000
        self.APB1ENR2 = 0x00000000
        self.APB2ENR = 0x00000000

    def ReadDoubleWord(self, offset):
        if offset == 0x00:
            return self.CR
        elif offset == 0x04:
            return self.CFGR
        elif offset == 0x0C:
            return self.PLLCFGR
        elif offset == 0x48:
            return self.AHB1ENR
        elif offset == 0x4C:
            return self.AHB2ENR
        elif offset == 0x58:
            return self.APB1ENR1
        elif offset == 0x5C:
            return self.APB1ENR2
        elif offset == 0x60:
            return self.APB2ENR
        else:
            return 0x00000000

    def WriteDoubleWord(self, offset, value):
        if offset == 0x00:
            self.CR = value
            self.NoisyLog("RCC CR = 0x{0:08X}".format(value))
        elif offset == 0x04:
            self.CFGR = value
            self.NoisyLog("RCC CFGR = 0x{0:08X}".format(value))
        elif offset == 0x0C:
            self.PLLCFGR = value
        elif offset == 0x48:
            self.AHB1ENR = value
            self.NoisyLog("RCC AHB1ENR = 0x{0:08X}".format(value))
        elif offset == 0x4C:
            self.AHB2ENR = value
            self.NoisyLog("RCC AHB2ENR = 0x{0:08X} (GPIO clocks)".format(value))
        elif offset == 0x58:
            self.APB1ENR1 = value
            self.NoisyLog("RCC APB1ENR1 = 0x{0:08X} (TIM/I2C)".format(value))
        elif offset == 0x5C:
            self.APB1ENR2 = value
        elif offset == 0x60:
            self.APB2ENR = value
            self.NoisyLog("RCC APB2ENR = 0x{0:08X} (TIM/USART)".format(value))

    def Reset(self):
        self.CR = 0x00000063
        self.CFGR = 0x00000000
        self.PLLCFGR = 0x00001000
        self.AHB1ENR = 0x00000100
        self.AHB2ENR = 0x00000000
        self.APB1ENR1 = 0x00000000
        self.APB1ENR2 = 0x00000000
        self.APB2ENR = 0x00000000
