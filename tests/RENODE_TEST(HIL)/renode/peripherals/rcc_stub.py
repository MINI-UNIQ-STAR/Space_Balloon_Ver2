# RCC Peripheral stub for Renode
# Provides realistic register responses

# Default register values
registers = {
    0x00: 0x00000063,  # CR - HSI ON and ready
    0x04: 0x00000000,  # CFGR
    0x08: 0x00000000,  # PLLCFGR low
    0x0C: 0x00001000,  # PLLCFGR
    0x18: 0x00000000,  # CIER
    0x48: 0x00000100,  # AHB1ENR - Flash enabled
    0x4C: 0x00000000,  # AHB2ENR
    0x50: 0x00000000,  # AHB3ENR
    0x58: 0x00000000,  # APB1ENR1
    0x5C: 0x00000000,  # APB1ENR2
    0x60: 0x00000000,  # APB2ENR
}

def read_double_word(offset):
    # Make sure CR always has ready bits set when ON bits are set
    if offset == 0x00:
        val = registers.get(0x00, 0x00000063)
        # HSI ready if HSI on
        if val & (1 << 0):
            val |= (1 << 2)
        # HSE ready if HSE on
        if val & (1 << 8):
            val |= (1 << 9)
        # PLL ready if PLL on
        if val & (1 << 16):
            val |= (1 << 17)
        return val

    return registers.get(offset, 0x00000000)

def write_double_word(offset, value):
    registers[offset] = value

    # Log important writes
    if offset == 0x00:
        self.DebugLog("RCC CR = 0x{0:08X}".format(value))
    elif offset == 0x04:
        sw = value & 0x3
        self.DebugLog("RCC CFGR = 0x{0:08X}, SW={1}".format(value, sw))
    elif offset == 0x4C:
        self.DebugLog("RCC AHB2ENR = 0x{0:08X} (GPIO clocks)".format(value))
    elif offset == 0x58:
        self.DebugLog("RCC APB1ENR1 = 0x{0:08X} (TIM/I2C)".format(value))
    elif offset == 0x60:
        self.DebugLog("RCC APB2ENR = 0x{0:08X} (TIM/USART)".format(value))

def reset():
    registers[0x00] = 0x00000063
    registers[0x04] = 0x00000000
    registers[0x0C] = 0x00001000
    registers[0x48] = 0x00000100
    registers[0x4C] = 0x00000000
    registers[0x58] = 0x00000000
    registers[0x5C] = 0x00000000
    registers[0x60] = 0x00000000
    self.DebugLog("RCC Reset")
