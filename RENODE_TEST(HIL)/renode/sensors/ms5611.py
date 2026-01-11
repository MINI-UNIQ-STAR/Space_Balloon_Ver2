# MS5611 Barometric Pressure Sensor Model for Renode
# High-precision altimeter

from Antmicro.Renode.Peripherals.I2C import II2CPeripheral
from Antmicro.Renode.Core import EmptyRegisterPeripheralBase
from Antmicro.Renode.Logging import Logger
import math

class MS5611(EmptyRegisterPeripheralBase, II2CPeripheral):
    def __init__(self):
        EmptyRegisterPeripheralBase.__init__(self)
        self.currentCommand = 0
        self.adcResult = 0
        self.time = 0.0
        self.altitude = 0.0  # Current simulated altitude in meters

        # Factory calibration coefficients (PROM)
        # These are example values
        self.C1 = 40127  # Pressure sensitivity
        self.C2 = 36924  # Pressure offset
        self.C3 = 23317  # Temperature coefficient of pressure sensitivity
        self.C4 = 23282  # Temperature coefficient of pressure offset
        self.C5 = 33464  # Reference temperature
        self.C6 = 28312  # Temperature coefficient of the temperature

    def Write(self, data):
        if len(data) == 0:
            return

        self.currentCommand = data[0]
        self.DebugLog("Command: 0x{0:02X}".format(self.currentCommand))

        # Process commands
        if self.currentCommand == 0x1E:  # Reset
            self.Reset()
        elif self.currentCommand & 0xF0 == 0x40:  # Convert D1 (Pressure)
            self.adcResult = self.GetPressureADC()
        elif self.currentCommand & 0xF0 == 0x50:  # Convert D2 (Temperature)
            self.adcResult = self.GetTemperatureADC()

    def Read(self, count):
        result = []

        if self.currentCommand == 0x00:  # ADC Read
            # Return 24-bit ADC result (MSB first)
            result.append((self.adcResult >> 16) & 0xFF)
            result.append((self.adcResult >> 8) & 0xFF)
            result.append(self.adcResult & 0xFF)
            self.DebugLog("ADC Read: 0x{0:06X}".format(self.adcResult))

        elif self.currentCommand & 0xF0 == 0xA0:  # PROM Read
            promAddr = (self.currentCommand >> 1) & 0x07
            promValue = self.GetPROM(promAddr)
            result.append((promValue >> 8) & 0xFF)
            result.append(promValue & 0xFF)
            self.DebugLog("PROM[{0}] Read: 0x{1:04X}".format(promAddr, promValue))

        # Pad to requested count if needed
        while len(result) < count:
            result.append(0)

        return result[:count]

    def GetPROM(self, addr):
        """Return factory calibration data"""
        prom = [
            0x3132,  # Reserved for manufacturer
            self.C1,
            self.C2,
            self.C3,
            self.C4,
            self.C5,
            self.C6,
            0x0000   # CRC (simplified, always 0)
        ]
        if addr < len(prom):
            return prom[addr]
        return 0

    def GetPressureADC(self):
        """Simulate pressure ADC reading based on altitude"""
        self.time += 0.2

        # Simulate ascending balloon (oscillating altitude for demo)
        self.altitude = 1000.0 + 500.0 * math.sin(self.time * 0.1)

        # Barometric formula: P = P0 * (1 - L*h/T0)^(g*M/(R*L))
        # Simplified: P ≈ P0 * exp(-h/H) where H ≈ 8400m
        P0 = 101325.0  # Sea level pressure (Pa)
        H = 8400.0     # Scale height
        pressure_pa = P0 * math.exp(-self.altitude / H)

        # Convert to ADC value using calibration
        # MS5611 typical output: ~9085466 at sea level
        # Simplified linear mapping
        D1 = int(9085466 - (self.altitude * 400))

        return D1 & 0xFFFFFF

    def GetTemperatureADC(self):
        """Simulate temperature ADC reading"""
        # Temperature decreases with altitude (~6.5°C per km)
        temp_c = 15.0 - (self.altitude * 0.0065)

        # Convert to ADC value
        # MS5611 typical output: ~8077636 at 20°C
        # Simplified mapping: ~40 counts per °C
        D2 = int(8077636 + (temp_c - 20.0) * 40.0 * 256)

        return D2 & 0xFFFFFF

    def FinishTransmission(self):
        pass

    def Reset(self):
        self.adcResult = 0
        self.DebugLog("MS5611 Reset")
