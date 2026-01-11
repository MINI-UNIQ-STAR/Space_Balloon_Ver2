# SHT31-D Temperature & Humidity Sensor Model for Renode

from Antmicro.Renode.Peripherals.I2C import II2CPeripheral
from Antmicro.Renode.Core import EmptyRegisterPeripheralBase
from Antmicro.Renode.Logging import Logger
import math
import random

class SHT31(EmptyRegisterPeripheralBase, II2CPeripheral):
    def __init__(self):
        EmptyRegisterPeripheralBase.__init__(self)
        self.currentCommand = []
        self.measurementReady = False
        self.temperature = 25.0  # °C
        self.humidity = 50.0     # %RH
        self.time = 0.0

    def Write(self, data):
        if len(data) == 0:
            return

        self.currentCommand = list(data)
        self.DebugLog("Command: {0}".format(' '.join('0x{0:02X}'.format(x) for x in data)))

        # Check for measurement commands
        # Single Shot High Repeatability: 0x2C 0x06
        if len(data) == 2 and data[0] == 0x2C and data[1] == 0x06:
            self.measurementReady = True
            self.UpdateSensorData()
            self.DebugLog("Measurement started")

        # Soft Reset: 0x30 0xA2
        elif len(data) == 2 and data[0] == 0x30 and data[1] == 0xA2:
            self.Reset()

    def Read(self, count):
        result = []

        if self.measurementReady and count >= 6:
            # Return temperature and humidity data
            # Format: Temp MSB, Temp LSB, Temp CRC, RH MSB, RH LSB, RH CRC

            # Temperature: T = -45 + 175 * (raw / 65535)
            # raw = (T + 45) * 65535 / 175
            temp_raw = int((self.temperature + 45.0) * 65535.0 / 175.0)
            temp_raw = max(0, min(65535, temp_raw))

            # Humidity: RH = 100 * (raw / 65535)
            # raw = RH * 65535 / 100
            rh_raw = int(self.humidity * 65535.0 / 100.0)
            rh_raw = max(0, min(65535, rh_raw))

            # Temperature data
            result.append((temp_raw >> 8) & 0xFF)
            result.append(temp_raw & 0xFF)
            result.append(self.CalculateCRC([result[0], result[1]]))

            # Humidity data
            result.append((rh_raw >> 8) & 0xFF)
            result.append(rh_raw & 0xFF)
            result.append(self.CalculateCRC([result[3], result[4]]))

            self.DebugLog("Read T={0:.2f}°C ({1}), RH={2:.2f}% ({3})".format(
                self.temperature, temp_raw, self.humidity, rh_raw))

            self.measurementReady = False

        # Pad if needed
        while len(result) < count:
            result.append(0)

        return result[:count]

    def UpdateSensorData(self):
        """Simulate changing environmental conditions"""
        self.time += 1.0

        # Temperature oscillates between 20-30°C
        self.temperature = 25.0 + 5.0 * math.sin(self.time * 0.1) + random.uniform(-0.5, 0.5)

        # Humidity oscillates between 30-70%
        self.humidity = 50.0 + 20.0 * math.cos(self.time * 0.08) + random.uniform(-2.0, 2.0)
        self.humidity = max(0.0, min(100.0, self.humidity))

    def CalculateCRC(self, data):
        """Calculate CRC-8 checksum (polynomial 0x31)"""
        crc = 0xFF
        for byte in data:
            crc ^= byte
            for _ in range(8):
                if crc & 0x80:
                    crc = (crc << 1) ^ 0x31
                else:
                    crc = crc << 1
            crc &= 0xFF
        return crc

    def FinishTransmission(self):
        pass

    def Reset(self):
        self.measurementReady = False
        self.DebugLog("SHT31 Reset")
