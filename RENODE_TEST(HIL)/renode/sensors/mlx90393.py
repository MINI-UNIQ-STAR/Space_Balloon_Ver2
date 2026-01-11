# MLX90393 Magnetometer Sensor Model for Renode
# 3-axis magnetic field sensor

from Antmicro.Renode.Peripherals.I2C import II2CPeripheral
from Antmicro.Renode.Core import EmptyRegisterPeripheralBase
from Antmicro.Renode.Logging import Logger
import math

class MLX90393(EmptyRegisterPeripheralBase, II2CPeripheral):
    def __init__(self):
        EmptyRegisterPeripheralBase.__init__(self)
        self.currentCommand = 0
        self.readData = []
        self.time = 0.0

        # Simulated Earth's magnetic field components (uT)
        # At mid-latitudes: ~50 uT total
        self.magX = 20.0
        self.magY = 15.0
        self.magZ = 40.0

    def Write(self, data):
        if len(data) == 0:
            return

        self.currentCommand = data[0]
        self.DebugLog("Command: 0x{0:02X}".format(self.currentCommand))

        # MLX90393 commands (simplified)
        if self.currentCommand == 0x3E:  # Start burst mode (SB)
            self.readData = [0x00]  # Status byte
        elif self.currentCommand == 0x4E:  # Read measurement (RM)
            self.UpdateMagData()
            # Return status + 6 bytes (X, Y, Z as 16-bit values)
            self.readData = [0x00]  # Status
            self.readData += self.EncodeMagData()
        elif self.currentCommand == 0x80:  # Exit mode (EX)
            self.readData = [0x00]
        elif self.currentCommand == 0xF0:  # Reset (RT)
            self.Reset()
            self.readData = [0x00]
        else:
            self.readData = [0x00]

    def Read(self, count):
        result = self.readData[:count]
        # Pad if needed
        while len(result) < count:
            result.append(0)
        return result

    def UpdateMagData(self):
        """Simulate slowly rotating magnetic field"""
        self.time += 0.1

        # Simulate rotation around Z-axis
        angle = self.time * 0.05
        self.magX = 20.0 * math.cos(angle)
        self.magY = 20.0 * math.sin(angle)
        self.magZ = 40.0 + 5.0 * math.sin(self.time * 0.02)

    def EncodeMagData(self):
        """Convert magnetic field to 16-bit raw values"""
        # MLX90393 resolution: ~0.161 uT/LSB (approximate)
        # Range: ±50000 uT
        scale = 0.161

        x_raw = int(self.magX / scale) & 0xFFFF
        y_raw = int(self.magY / scale) & 0xFFFF
        z_raw = int(self.magZ / scale) & 0xFFFF

        result = []
        result.append((x_raw >> 8) & 0xFF)
        result.append(x_raw & 0xFF)
        result.append((y_raw >> 8) & 0xFF)
        result.append(y_raw & 0xFF)
        result.append((z_raw >> 8) & 0xFF)
        result.append(z_raw & 0xFF)

        self.DebugLog("Mag: X={0:.2f} Y={1:.2f} Z={2:.2f} uT".format(
            self.magX, self.magY, self.magZ))

        return result

    def FinishTransmission(self):
        pass

    def Reset(self):
        self.readData = []
        self.time = 0.0
        self.DebugLog("MLX90393 Reset")
