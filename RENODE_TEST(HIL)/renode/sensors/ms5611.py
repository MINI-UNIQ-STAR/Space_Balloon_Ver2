# MS5611 Barometric Pressure Sensor Model (Python)
from Antmicro.Renode.Peripherals.I2C import II2CPeripheral
from Antmicro.Renode.Logging import Logger, LogLevel
import System
import math

class MS5611(II2CPeripheral):
    def __init__(self):
        self.Reset()

    def Reset(self):
        self.state = "IDLE"
        self.cmd = 0
        # Calibration (PROM)
        self.prom = [
            0x0000, 40127, 36924, 23317, 23282, 32463, 28312, 0x0000
        ]
        self.adc_value = 0
        self.simulation_time = 0.0

    def Write(self, data):
        if len(data) == 0: return
        cmd = data[0]
        self.cmd = cmd
        
        if cmd == 0x1E: # Reset
            self.Reset()
        elif 0xA0 <= cmd <= 0xAE: # Read PROM
            self.state = "PROM_READ"
        elif 0x40 <= cmd <= 0x4E: # D1 (Pressure)
            self.UpdateSimulation()
            # Fake pressure ~1013 hPa
            self.adc_value = 6000000 + int(self.sim_pressure * 100)
            self.state = "ADC_WAIT"
        elif 0x50 <= cmd <= 0x5E: # D2 (Temp)
            self.adc_value = 8000000 # ~20C
            self.state = "ADC_WAIT"
        elif cmd == 0x00: # ADC Read
            self.state = "ADC_READ"

    def Read(self, count):
        resp = []
        if self.state == "PROM_READ":
            idx = (self.cmd - 0xA0) >> 1
            if idx < len(self.prom):
                val = self.prom[idx]
                resp = [(val >> 8) & 0xFF, val & 0xFF]
        elif self.state == "ADC_READ":
            val = self.adc_value
            resp = [(val >> 16) & 0xFF, (val >> 8) & 0xFF, val & 0xFF]
        
        if not resp: resp = [0] * count
        return System.Array[System.Byte](resp)

    def FinishTransmission(self):
        pass

    def UpdateSimulation(self):
        self.simulation_time += 0.1
        # P decreases with time
        self.sim_pressure = 1013.25 - (self.simulation_time * 0.1)
