# FLASH Controller stub for Renode

registers = {
    0x00: 0x00000600,  # ACR - 0 wait states, prefetch disabled
    0x08: 0x00000000,  # KEYR
    0x0C: 0x00000000,  # OPTKEYR
    0x10: 0x00000000,  # SR
    0x14: 0xC0000000,  # CR - Locked
}

unlock_step = 0

def read_double_word(offset):
    return registers.get(offset, 0x00000000)

def write_double_word(offset, value):
    global unlock_step

    # Handle unlock sequence
    if offset == 0x08:  # KEYR
        if value == 0x45670123:
            unlock_step = 1
        elif value == 0xCDEF89AB and unlock_step == 1:
            registers[0x14] &= ~(1 << 31)  # Clear LOCK bit
            self.DebugLog("FLASH Unlocked")
            unlock_step = 0
        else:
            unlock_step = 0
        return

    registers[offset] = value

    if offset == 0x00:  # ACR
        latency = value & 0x7
        prften = (value >> 8) & 1
        icen = (value >> 9) & 1
        dcen = (value >> 10) & 1
        self.DebugLog("FLASH ACR: Latency={0}, PRFTEN={1}, ICEN={2}, DCEN={3}".format(
            latency, prften, icen, dcen))

def reset():
    registers[0x00] = 0x00000600
    registers[0x08] = 0x00000000
    registers[0x0C] = 0x00000000
    registers[0x10] = 0x00000000
    registers[0x14] = 0xC0000000
    self.DebugLog("FLASH Reset")
