# SHT31 Humidity Sensor Model (Python)
from Antmicro.Renode.Peripherals.I2C import II2CPeripheral
from Antmicro.Renode.Logging import Logger, LogLevel
import System

# Debug: Prove file loading
try:
    with open(r'C:\Users\hyuns\sht31_loaded.txt', 'w') as f:
        f.write("LOADED")
except: pass

class SHT31(II2CPeripheral):
    def __init__(self): 
        self.Reset()
        
    def log_debug(self, msg):
        try:
            # Log to user home directory
            with open(r'C:\Users\hyuns\debug_sht31.txt', 'a') as f:
                f.write(msg + "\n")
        except: pass

    def Reset(self): 
        self.log_debug("SHT31 Reset Called")
        self.state = "IDLE"
        self.t = -10.0
        self.rh = 50.0
        self.sim_time = 0.0
        self.heater_on = False
        
    def Write(self, data):
        self.log_debug("SHT31 Write: " + str(list(data)))
        if len(data) >= 2:
            if data[0] == 0x2C and data[1] == 0x06: 
                self.state = "READ"
                self.UpdateSimulation()
            elif data[0] == 0x30 and data[1] == 0x6D:
                self.heater_on = True
                self.log_debug(">>> SHT31 HEATER ENABLED <<<")
                try:
                    with open(r'C:\Users\hyuns\Desktop\project\SpaceBalloon_2.0\spaceballoon_stm32_lora32\RENODE_TEST(HIL)\heater_log.txt', 'w') as f:
                        f.write("SUCCESS")
                except: pass
            elif data[0] == 0x30 and data[1] == 0x66:
                self.heater_on = False
                self.log_debug(">>> SHT31 HEATER DISABLED <<<")
                
    def Read(self, count):
        self.log_debug("SHT31 Read count: " + str(count))
        if self.state == "READ":
            t_raw = int((self.t + 45) * 65535 / 175)
            h_raw = int(self.rh * 65535 / 100)
            return System.Array[System.Byte]([(t_raw>>8)&0xFF, t_raw&0xFF, 0x00, (h_raw>>8)&0xFF, h_raw&0xFF, 0x00])
        return System.Array[System.Byte]([0]*count)
        
    def FinishTransmission(self): pass
    
    def UpdateSimulation(self):
        self.sim_time += 0.2
        # Temperature drops 1 deg per call
        self.t = 25.0 - (self.sim_time * 1.0)
        if self.t < -40.0: self.t = -40.0
