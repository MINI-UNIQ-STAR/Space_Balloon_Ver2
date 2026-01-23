// CM1107N CO2 Sensor Model for Renode
// Cubic CM1107N - UART Protocol over I2C Bus
//
// IMPORTANT: This sensor uses UART command/response format 
// transmitted over I2C bus (verified from firmware driver)
//
// Protocol (from STM32 firmware cm1107n_driver.c):
// - I2C Address: 0x31 (7-bit)
// - Send Command: 0x11 0x01 0x01 0xED (UART Read CO2 format)
// - Response: 0x16 0x05 0x01 [DF1] [DF2] [DF3] [DF4] [CS] (8 bytes)
// - CO2 (ppm) = DF1*256 + DF2
// - DF3 = Status (0x01 = OK)
// - DF4 = Reserved (0x00)
// - CS = Checksum: (256 - (sum of bytes 0-6) % 256) % 256
//
// Reference:
// - Cubic CM1107N Datasheet (UART Protocol Section)
// - Core/Drivers/cm1107n/cm1107n_driver.c (firmware implementation)
//
using System;
using System.Collections.Generic;
using Antmicro.Renode.Core;
using Antmicro.Renode.Logging;
using Antmicro.Renode.Peripherals.I2C;

namespace Antmicro.Renode.Peripherals.Sensors
{
    public class CM1107N : II2CPeripheral
    {
        // UART Command Constants (used over I2C)
        private const byte CMD_HEADER = 0x11;       // UART command header
        private const byte CMD_LEN = 0x01;          // Length byte
        private const byte CMD_READ_CO2 = 0x01;     // Read CO2 command
        private const byte CMD_CHECKSUM = 0xED;     // Command checksum
        
        // UART Response Constants
        private const byte RESP_HEADER = 0x16;      // UART response header
        private const byte RESP_LEN = 0x05;         // Response length (5 data bytes)
        private const byte RESP_CMD_ECHO = 0x01;    // Command echo
        
        // Status Codes (DF3)
        private const byte STATUS_INIT = 0x00;      // Sensor initializing
        private const byte STATUS_OK = 0x01;        // Normal operation
        private const byte STATUS_ERROR = 0x02;     // Sensor error
        private const byte STATUS_OUT_OF_RANGE = 0x03;
        private const byte STATUS_NOT_CALIBRATED = 0x05;
        
        // Simulation State
        private ushort co2Value = 450;
        private byte status = STATUS_OK;
        private Queue<byte> commandBuffer = new Queue<byte>();
        private State state;
        private double simulationTime = 0;
        private Random random = new Random();

        public CM1107N()
        {
            Reset();
        }

        public void Reset()
        {
            commandBuffer.Clear();
            state = State.Idle;
            co2Value = 450;
            status = STATUS_OK;
            simulationTime = 0;
            this.Log(LogLevel.Info, "CM1107N Reset - Default: 450 ppm");
        }

        public void Write(byte[] data)
        {
            // Buffer incoming bytes
            foreach (var b in data)
            {
                commandBuffer.Enqueue(b);
            }

            // Check for Read CO2 command: 0x11 0x01 0x01 0xED
            // Firmware sends this via I2C write (first byte as "register")
            if (commandBuffer.Count >= 4)
            {
                var cmd = commandBuffer.ToArray();
                
                if (cmd[0] == CMD_HEADER && 
                    cmd[1] == CMD_LEN && 
                    cmd[2] == CMD_READ_CO2 && 
                    cmd[3] == CMD_CHECKSUM)
                {
                    state = State.DataReady;
                    UpdateSimulation();
                    this.Log(LogLevel.Debug, "CM1107N: Received Read CO2 Command (0x11 0x01 0x01 0xED)");
                }
                else
                {
                    this.Log(LogLevel.Warning, "CM1107N: Unknown command: 0x{0:X2} 0x{1:X2} 0x{2:X2} 0x{3:X2}",
                        cmd[0], cmd[1], cmd[2], cmd.Length > 3 ? cmd[3] : (byte)0);
                }
                commandBuffer.Clear();
            }
        }

        public byte[] Read(int count)
        {
            if (state == State.DataReady)
            {
                // UART-format response over I2C (8 bytes):
                // [0x16] [0x05] [0x01] [DF1] [DF2] [DF3] [DF4] [CS]
                // 0x16 = Response header
                // 0x05 = Length (5 data bytes)
                // 0x01 = Command echo
                // DF1 = CO2 high byte
                // DF2 = CO2 low byte
                // DF3 = Status (0x01 = OK)
                // DF4 = Reserved (0x00)
                // CS = Checksum
                
                byte df1 = (byte)(co2Value >> 8);    // CO2 high byte
                byte df2 = (byte)(co2Value & 0xFF);  // CO2 low byte
                byte df3 = status;                    // Status
                byte df4 = 0x00;                      // Reserved
                
                var resp = new byte[] { RESP_HEADER, RESP_LEN, RESP_CMD_ECHO, df1, df2, df3, df4, 0x00 };
                
                // Calculate Checksum: CS = (256 - (sum % 256)) % 256
                int sum = 0;
                for (int i = 0; i < 7; i++) sum += resp[i];
                resp[7] = (byte)((256 - (sum % 256)) % 256);
                
                state = State.Idle;
                this.Log(LogLevel.Info, "CM1107N: Sending CO2={0} ppm (0x{1:X2} 0x{2:X2}), Status=0x{3:X2}, CS=0x{4:X2}", 
                    co2Value, df1, df2, status, resp[7]);
                return resp;
            }
            
            this.Log(LogLevel.Debug, "CM1107N: Read in Idle state, returning zeros ({0} bytes)", count);
            return new byte[count];
        }

        public void FinishTransmission()
        {
        }
        
        private void UpdateSimulation()
        {
            // Simulate CO2 variations during balloon flight
            // Sea level: ~420 ppm, Stratosphere (~30km): ~380 ppm
            simulationTime += 1.0; // Increment per read
            
            // Linear decrease with simulated altitude
            double baseCO2 = 420.0 - (simulationTime * 0.3);
            if (baseCO2 < 380.0) baseCO2 = 380.0;
            
            // Add realistic sensor noise (±3 ppm)
            double noise = (random.NextDouble() - 0.5) * 6.0;
            co2Value = (ushort)Math.Max(0, Math.Min(5000, baseCO2 + noise));
        }

        // Properties for Renode scripts and tests
        
        /// <summary>
        /// CO2 concentration in ppm (0-5000 typical range)
        /// </summary>
        public ushort CO2Value
        {
            get => co2Value;
            set 
            {
                co2Value = value;
                this.Log(LogLevel.Info, "CM1107N: CO2 manually set to {0} ppm", value);
            }
        }
        
        /// <summary>
        /// Sensor status: 0x00=init, 0x01=OK, 0x02=error, 0x03=out of range, 0x05=not calibrated
        /// </summary>
        public byte Status
        {
            get => status;
            set
            {
                status = value;
                this.Log(LogLevel.Info, "CM1107N: Status set to 0x{0:X2}", value);
            }
        }
        
        /// <summary>
        /// Reset simulation time (for repeatable tests)
        /// </summary>
        public void ResetSimulation()
        {
            simulationTime = 0;
            co2Value = 420;
            this.Log(LogLevel.Info, "CM1107N: Simulation reset to sea level (420 ppm)");
        }

        private enum State
        {
            Idle,
            DataReady
        }
    }
}
