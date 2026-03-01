# Dummy peripherals for STM32G4 simulation
# These provide realistic register responses for RCC, FLASH, and PWR

class DummyRCC:
    """STM32G4 RCC peripheral stub"""

    def __init__(self):
        # Default register values from STM32G4 reference manual
        self.registers = {
            0x00: 0x00000063,  # CR - HSI ON and ready
            0x04: 0x00000000,  # CFGR
            0x08: 0x00000000,  # PLLCFGR (bits 12-18)
            0x0C: 0x00001000,  # PLLCFGR
            0x18: 0x00000000,  # CIER
            0x48: 0x00000100,  # AHB1ENR - Flash interface enabled
            0x4C: 0x00000000,  # AHB2ENR
            0x58: 0x00000000,  # APB1ENR1
            0x5C: 0x00000000,  # APB1ENR2
            0x60: 0x00000000,  # APB2ENR
        }
        self.hsi_on = True
        self.hse_on = False
        self.pll_on = False

    def read(self, offset):
        """Read from RCC register"""
        # Handle CR register specially for status bits
        if offset == 0x00:
            val = self.registers[0x00]
            # Update ready bits based on ON bits
            if self.hsi_on:
                val |= (1 << 2)  # HSIRDY
            if self.hse_on:
                val |= (1 << 9)  # HSERDY
            if self.pll_on:
                val |= (1 << 17) # PLLRDY
            return val

        return self.registers.get(offset, 0x00000000)

    def write(self, offset, value):
        """Write to RCC register"""
        self.registers[offset] = value

        # Handle CR register
        if offset == 0x00:
            self.hsi_on = bool(value & (1 << 0))
            self.hse_on = bool(value & (1 << 8))
            self.pll_on = bool(value & (1 << 16))

        # Log important writes
        if offset == 0x4C:  # AHB2ENR
            print(f"[RCC] AHB2ENR = 0x{value:08X} (GPIO clocks)")
        elif offset == 0x58:  # APB1ENR1
            print(f"[RCC] APB1ENR1 = 0x{value:08X} (TIM/I2C clocks)")
        elif offset == 0x60:  # APB2ENR
            print(f"[RCC] APB2ENR = 0x{value:08X} (TIM/USART clocks)")


class DummyFLASH:
    """STM32G4 FLASH controller stub"""

    def __init__(self):
        self.registers = {
            0x00: 0x00000600,  # ACR - 0 wait states
            0x08: 0x00000000,  # KEYR
            0x0C: 0x00000000,  # OPTKEYR
            0x10: 0x00000000,  # SR
            0x14: 0xC0000000,  # CR - Locked
        }
        self.locked = True
        self.unlock_step = 0

    def read(self, offset):
        return self.registers.get(offset, 0x00000000)

    def write(self, offset, value):
        # Handle unlock sequence
        if offset == 0x08:  # KEYR
            if value == 0x45670123:
                self.unlock_step = 1
            elif value == 0xCDEF89AB and self.unlock_step == 1:
                self.locked = False
                self.registers[0x14] &= ~(1 << 31)  # Clear LOCK bit
                print("[FLASH] Unlocked")
                self.unlock_step = 0
            else:
                self.unlock_step = 0
            return

        self.registers[offset] = value

        if offset == 0x00:  # ACR
            latency = value & 0x7
            prften = (value >> 8) & 1
            icen = (value >> 9) & 1
            dcen = (value >> 10) & 1
            print(f"[FLASH] ACR: Latency={latency}, Prefetch={prften}, I-Cache={icen}, D-Cache={dcen}")


class DummyPWR:
    """STM32G4 PWR peripheral stub"""

    def __init__(self):
        self.registers = {
            0x00: 0x00000200,  # CR1 - VOS=01 (Range 1)
            0x04: 0x00000000,  # CR2
            0x08: 0x00008000,  # CR3
            0x0C: 0x00000000,  # CR4
            0x10: 0x00000000,  # SR1
            0x14: 0x00000400,  # SR2 - VOSF ready
        }

    def read(self, offset):
        # SR2 has some bits that should be high
        if offset == 0x14:
            return self.registers[0x14] | (1 << 10) | (1 << 14) | (1 << 15)  # VOSF, PVMO bits
        return self.registers.get(offset, 0x00000000)

    def write(self, offset, value):
        self.registers[offset] = value

        if offset == 0x00:  # CR1
            vos = (value >> 9) & 0x3
            print(f"[PWR] VOS = {vos} (Voltage Range)")
