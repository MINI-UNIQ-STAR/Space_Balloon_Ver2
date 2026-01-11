// MLX90393 3-Axis Magnetometer Sensor for Renode
// Melexis MLX90393 - I2C Triaxial Magnetic Field Sensor
// Simple I2C implementation

using System;
using Antmicro.Renode.Core;
using Antmicro.Renode.Logging;
using Antmicro.Renode.Peripherals.I2C;

namespace Antmicro.Renode.Peripherals.Sensors
{
    public class MLX90393 : II2CPeripheral
    {
        public MLX90393()
        {
            Reset();
        }

        public void Reset()
        {
            currentCommand = 0;
            measurementReady = false;
            burstMode = false;
            
            // Default magnetic field values (Earth's field ~25-65 µT)
            magX = 0;
            magY = 0;
            magZ = 400;  // ~40 µT vertical component
            temperature = 25;
            
            // Status register
            status = 0x00;
            
            this.Log(LogLevel.Info, "MLX90393 Reset - Default: X=0, Y=0, Z=400");
        }

        public void Write(byte[] data)
        {
            if (data.Length == 0)
                return;

            currentCommand = data[0];
            this.Log(LogLevel.Debug, "Command: 0x{0:X2}", currentCommand);

            switch (currentCommand & 0xF0)
            {
                case CMD_START_BURST:
                    // Start burst mode (continuous measurement)
                    burstMode = true;
                    UpdateSimulation();
                    measurementReady = true;
                    this.Log(LogLevel.Debug, "Burst mode started");
                    break;
                    
                case CMD_START_WAKEUP:
                    // Wake up on change mode
                    this.Log(LogLevel.Debug, "Wake-up mode");
                    break;
                    
                case CMD_START_SINGLE:
                    // Single measurement
                    UpdateSimulation();
                    measurementReady = true;
                    this.Log(LogLevel.Debug, "Single measurement: X={0}, Y={1}, Z={2}", 
                        magX, magY, magZ);
                    break;
                    
                case CMD_READ_MEASUREMENT:
                    // Read measurement command
                    this.Log(LogLevel.Debug, "Read measurement request");
                    break;
                    
                case CMD_EXIT:
                    // Exit mode
                    burstMode = false;
                    this.Log(LogLevel.Debug, "Exit mode");
                    break;
                    
                case CMD_RESET:
                    Reset();
                    this.Log(LogLevel.Debug, "Reset");
                    break;
                    
                case CMD_MEMORY_RECALL:
                    // Memory recall
                    this.Log(LogLevel.Debug, "Memory recall");
                    break;
                    
                case CMD_MEMORY_STORE:
                    // Memory store
                    this.Log(LogLevel.Debug, "Memory store");
                    break;
            }
        }

        public byte[] Read(int count)
        {
            byte[] result = new byte[count];

            if (measurementReady)
            {
                // Response format: Status, T_MSB, T_LSB, X_MSB, X_LSB, Y_MSB, Y_LSB, Z_MSB, Z_LSB
                int idx = 0;
                
                // Status byte
                status = 0x00;
                if (burstMode) status |= 0x80;  // Burst mode active
                status |= 0x01;  // Data ready
                
                if (idx < count) result[idx++] = status;
                
                // Optional: Temperature (if TXYZE = 1)
                if (idx < count) result[idx++] = (byte)(temperature >> 8);
                if (idx < count) result[idx++] = (byte)(temperature & 0xFF);
                
                // Magnetic field X
                if (idx < count) result[idx++] = (byte)(magX >> 8);
                if (idx < count) result[idx++] = (byte)(magX & 0xFF);
                
                // Magnetic field Y
                if (idx < count) result[idx++] = (byte)(magY >> 8);
                if (idx < count) result[idx++] = (byte)(magY & 0xFF);
                
                // Magnetic field Z
                if (idx < count) result[idx++] = (byte)(magZ >> 8);
                if (idx < count) result[idx++] = (byte)(magZ & 0xFF);
                
                this.Log(LogLevel.Debug, "Read: Status=0x{0:X2}, X={1}, Y={2}, Z={3}", 
                    status, magX, magY, magZ);
            }
            else
            {
                // Just return status
                result[0] = status;
            }

            return result;
        }

        public void FinishTransmission()
        {
        }

        private void UpdateSimulation()
        {
            time += 0.01;
            
            // Simulate slow rotation (balloon spinning)
            // Earth's magnetic field: ~25-65 µT
            // MLX90393 resolution: 0.161 µT/LSB at default gain
            
            double noise = (random.NextDouble() - 0.5) * 20;
            
            // Simulate balloon rotation around Z axis
            double angle = time * 0.1;  // Slow rotation
            
            // Horizontal field component (~20 µT)
            double horizontalField = 200;  // ~32 µT horizontal
            magX = (short)(horizontalField * Math.Cos(angle) + noise);
            magY = (short)(horizontalField * Math.Sin(angle) + noise);
            
            // Vertical field component (~40 µT, constant)
            magZ = (short)(400 + noise);
            
            // Temperature (internal sensor)
            temperature = (short)(2500 + random.NextDouble() * 100);  // ~25°C in 0.01°C units
        }

        // MLX90393 Commands
        private const byte CMD_START_BURST = 0x10;     // Start burst mode SB
        private const byte CMD_START_WAKEUP = 0x20;    // Start wake-up on change WOC
        private const byte CMD_START_SINGLE = 0x30;    // Start single measurement SM
        private const byte CMD_READ_MEASUREMENT = 0x40; // Read measurement RM
        private const byte CMD_READ_REGISTER = 0x50;   // Read register RR
        private const byte CMD_WRITE_REGISTER = 0x60;  // Write register WR
        private const byte CMD_EXIT = 0x80;            // Exit mode EX
        private const byte CMD_MEMORY_RECALL = 0xD0;   // Memory recall HR
        private const byte CMD_MEMORY_STORE = 0xE0;    // Memory store HS
        private const byte CMD_RESET = 0xF0;           // Reset RT

        // State
        private byte currentCommand;
        private byte status;
        private bool measurementReady;
        private bool burstMode;
        private double time;
        private Random random = new Random();
        
        // Sensor data (16-bit signed)
        private short magX, magY, magZ;
        private short temperature;
    }
}
