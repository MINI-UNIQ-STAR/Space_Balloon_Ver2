# LSM6DSV16X IMU Sensor Model (Python)
from Antmicro.Renode.Peripherals.I2C import II2CPeripheral
from Antmicro.Renode.Logging import Logger
import System
import math

class LSM6DSV16X(II2CPeripheral):
    def __init__(self):
        self.Reset()

    def Reset(self):
        self.reg_ptr = 0
        self.who_am_i = 0x70
        self.sim_time = 0.0
        self.regs = {0x0F: 0x70}

    def Write(self, data):
        if len(data) > 0:
            self.reg_ptr = data[0]
            if len(data) > 1:
                # Write data to register
                for i in range(1, len(data)):
                    self.regs[self.reg_ptr + i - 1] = data[i]

    def Read(self, count):
        resp = []
        for i in range(count):
            reg = self.reg_ptr + i
            val = self.ReadRegister(reg)
            resp.append(val)
        return System.Array[System.Byte](resp)

    def ReadRegister(self, reg):
        if reg == 0x0F: return 0x70
        
        # Simulate sensor data registers
        if 0x20 <= reg <= 0x2D:
            self.UpdateSimulation()
            if reg == 0x28: return self.acc_x & 0xFF
            if reg == 0x29: return (self.acc_x >> 8) & 0xFF
            if reg == 0x2A: return self.acc_y & 0xFF
            if reg == 0x2B: return (self.acc_y >> 8) & 0xFF
            if reg == 0x2C: return self.acc_z & 0xFF
            if reg == 0x2D: return (self.acc_z >> 8) & 0xFF
            
            # Gyro
            if reg == 0x22: return self.gyro_x & 0xFF
            if reg == 0x23: return (self.gyro_x >> 8) & 0xFF
            
        return self.regs.get(reg, 0)

    def UpdateSimulation(self):
        self.sim_time += 0.01
        # Gravity on Z (1g = 16384 LSB)
        self.acc_x = int(500 * math.sin(self.sim_time))
        self.acc_y = int(500 * math.cos(self.sim_time))
        self.acc_z = 16384
        
        self.gyro_x = int(200 * math.sin(self.sim_time))

    def FinishTransmission(self):
        pass
