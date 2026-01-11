# LSM6DSV16X IMU Sensor Model for Renode
# 6-axis accelerometer + gyroscope

from Antmicro.Renode.Peripherals.I2C import II2CPeripheral
from Antmicro.Renode.Core import EmptyRegisterPeripheralBase
from Antmicro.Renode.Logging import Logger
import math
import random

class LSM6DSV16X(EmptyRegisterPeripheralBase, II2CPeripheral):
    def __init__(self):
        EmptyRegisterPeripheralBase.__init__(self)
        self.registers = {}
        self.currentRegister = 0
        self.time = 0.0

        # Initialize registers
        self.registers[0x0F] = 0x70  # WHO_AM_I
        self.registers[0x10] = 0x00  # CTRL1_XL (accel control)
        self.registers[0x11] = 0x00  # CTRL2_G (gyro control)
        self.registers[0x12] = 0x00  # CTRL3_C

        # Initialize accel/gyro data registers
        for i in range(0x20, 0x2E):  # OUT_TEMP to OUTZ_G
            self.registers[i] = 0x00

    def Write(self, data):
        if len(data) == 0:
            return

        # First byte is register address
        self.currentRegister = data[0]

        # Remaining bytes are data to write
        for i in range(1, len(data)):
            reg = (self.currentRegister + i - 1) & 0xFF
            self.registers[reg] = data[i]
            self.DebugLog("Write to reg 0x{0:02X}: 0x{1:02X}".format(reg, data[i]))

    def Read(self, count):
        result = []
        for i in range(count):
            reg = (self.currentRegister + i) & 0xFF

            # Simulate sensor data
            if reg >= 0x28 and reg <= 0x2D:  # Accelerometer X, Y, Z (LSB, MSB)
                axis = (reg - 0x28) // 2
                value = self.GetAccelData(axis)
                if (reg - 0x28) % 2 == 0:  # LSB
                    result.append(value & 0xFF)
                else:  # MSB
                    result.append((value >> 8) & 0xFF)

            elif reg >= 0x22 and reg <= 0x27:  # Gyroscope X, Y, Z (LSB, MSB)
                axis = (reg - 0x22) // 2
                value = self.GetGyroData(axis)
                if (reg - 0x22) % 2 == 0:  # LSB
                    result.append(value & 0xFF)
                else:  # MSB
                    result.append((value >> 8) & 0xFF)

            else:
                value = self.registers.get(reg, 0x00)
                result.append(value)

        self.DebugLog("Read {0} bytes from reg 0x{1:02X}".format(count, self.currentRegister))
        return result

    def GetAccelData(self, axis):
        """Simulate accelerometer data (±2g range, 16-bit)
        Returns signed 16-bit value"""
        self.time += 0.001

        # Simulate gentle motion with gravity
        if axis == 0:  # X-axis: small oscillation
            accel_g = 0.1 * math.sin(self.time * 2.0)
        elif axis == 1:  # Y-axis: small oscillation
            accel_g = 0.1 * math.cos(self.time * 1.5)
        else:  # Z-axis: gravity (1g) with small noise
            accel_g = 1.0 + 0.05 * random.uniform(-1, 1)

        # Convert to raw 16-bit value (±2g range, LSB = 0.061 mg)
        raw = int(accel_g * 32768 / 2.0)
        return raw & 0xFFFF

    def GetGyroData(self, axis):
        """Simulate gyroscope data (±250 dps range, 16-bit)
        Returns signed 16-bit value"""

        # Simulate slow rotation
        gyro_dps = 5.0 * math.sin(self.time * 0.5 + axis)

        # Convert to raw 16-bit value (±250 dps range, LSB = 8.75 mdps)
        raw = int(gyro_dps * 32768 / 250.0)
        return raw & 0xFFFF

    def FinishTransmission(self):
        pass

    def Reset(self):
        self.__init__()
