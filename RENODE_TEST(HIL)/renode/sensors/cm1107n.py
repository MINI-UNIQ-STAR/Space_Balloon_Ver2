# CM1107N CO2 Sensor Model for Renode (Python Version)
# Cubic CM1107N - UART Protocol over I2C Bus
# Protocol:
#   Send: 0x11 0x01 0x01 0xED
#   Recv: 0x16 0x05 0x01 [DF1] [DF2] [DF3] [DF4] [CS]

from Antmicro.Renode.Peripherals.I2C import II2CPeripheral
from Antmicro.Renode.Logging import Logger, LogLevel
import System

class CM1107N(II2CPeripheral):
    def __init__(self):
        self.Reset()

    def Reset(self):
        self.commandBuffer = []
        self.state = "Idle"
        self.co2Value = 450
        self.status = 0x01 # OK
        self.simulationTime = 0.0

    def Write(self, data):
        # data is a byte array (System.Byte[])
        for b in data:
            self.commandBuffer.append(b)
        
        # Check command: 11 01 01 ED
        if len(self.commandBuffer) >= 4:
            cmd = self.commandBuffer
            if cmd[0] == 0x11 and cmd[1] == 0x01 and cmd[2] == 0x01 and cmd[3] == 0xED:
                self.state = "DataReady"
                self.UpdateSimulation()
                # self.DebugLog("Received Read CO2 Command")
            else:
                pass
                # self.DebugLog("Unknown command")
            self.commandBuffer = []

    def Read(self, count):
        if self.state == "DataReady":
            # Response: 16 05 01 [DF1] [DF2] [DF3] [DF4] [CS]
            df1 = (self.co2Value >> 8) & 0xFF
            df2 = self.co2Value & 0xFF
            df3 = self.status
            df4 = 0x00
            
            resp = [0x16, 0x05, 0x01, df1, df2, df3, df4, 0x00]
            
            # Checksum: (256 - sum) % 256
            s = sum(resp[:7])
            checksum = (256 - (s % 256)) % 256
            resp[7] = checksum
            
            self.state = "Idle"
            # self.DebugLog("Sending CO2: " + str(self.co2Value))
            
            # Convert list to .NET array
            return System.Array[System.Byte](resp)
            
        return System.Array[System.Byte]([0] * count)

    def FinishTransmission(self):
        pass

    def UpdateSimulation(self):
        self.simulationTime += 1.0
        # Simple simulation: decrease CO2 with time
        base = 450 - int(self.simulationTime * 0.1)
        if base < 380: base = 380
        self.co2Value = base
