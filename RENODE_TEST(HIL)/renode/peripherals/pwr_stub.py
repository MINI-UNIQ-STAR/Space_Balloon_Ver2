# PWR Peripheral stub for Renode

registers = {
    0x00: 0x00000200,  # CR1 - VOS=01 (Range 1)
    0x04: 0x00000000,  # CR2
    0x08: 0x00008000,  # CR3 - APC set
    0x0C: 0x00000000,  # CR4
    0x10: 0x00000000,  # SR1
    0x14: 0x00000400,  # SR2 - VOSF ready
}

def read_double_word(offset):
    # SR2 should have certain bits always set
    if offset == 0x14:
        return registers.get(0x14, 0x00000400) | (1 << 10) | (1 << 14) | (1 << 15)
    return registers.get(offset, 0x00000000)

def write_double_word(offset, value):
    registers[offset] = value

    if offset == 0x00:  # CR1
        vos = (value >> 9) & 0x3
        self.DebugLog("PWR VOS = {0} (Voltage Range)".format(vos))
    elif offset == 0x04:  # CR2
        pvde = value & 1
        if pvde:
            self.DebugLog("PWR PVD enabled")

def reset():
    registers[0x00] = 0x00000200
    registers[0x04] = 0x00000000
    registers[0x08] = 0x00008000
    registers[0x0C] = 0x00000000
    registers[0x10] = 0x00000000
    registers[0x14] = 0x00000400
    self.DebugLog("PWR Reset")
