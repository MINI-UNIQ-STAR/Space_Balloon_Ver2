// LSM6DSV16X IMU I2C Sensor for Renode
// 6-axis accelerometer + gyroscope
// Simple I2C implementation (no ByteRegisterCollection)

using System;
using Antmicro.Renode.Core;
using Antmicro.Renode.Logging;
using Antmicro.Renode.Peripherals.I2C;

namespace Antmicro.Renode.Peripherals.Sensors
{
    public class LSM6DSV16X : II2CPeripheral
    {
        public LSM6DSV16X()
        {
            Reset();
        }

        public void Reset()
        {
            currentRegister = 0;
            time = 0;
            
            // Initialize registers with default values
            registers = new byte[256];
            
            // WHO_AM_I (0x0F) = 0x70
            registers[0x0F] = 0x70;
            
            // CTRL1_XL (0x10) - Accelerometer control
            registers[0x10] = 0x00;
            
            // CTRL2_G (0x11) - Gyroscope control
            registers[0x11] = 0x00;
            
            // STATUS_REG (0x1E)
            registers[0x1E] = 0x07; // New data available
            
            UpdateSensorData();
            
            this.Log(LogLevel.Info, "LSM6DSV16X Reset - WHO_AM_I: 0x70");
        }

        public void Write(byte[] data)
        {
            if (data.Length == 0)
                return;

            // First byte is register address
            currentRegister = data[0];
            this.Log(LogLevel.Debug, "Register address set: 0x{0:X2}", currentRegister);

            // Remaining bytes are data to write
            for (int i = 1; i < data.Length; i++)
            {
                byte reg = (byte)(currentRegister + i - 1);
                registers[reg] = data[i];
                this.Log(LogLevel.Debug, "Write reg[0x{0:X2}] = 0x{1:X2}", reg, data[i]);
            }
        }

        public byte[] Read(int count)
        {
            byte[] result = new byte[count];

            // Update sensor data before reading
            UpdateSensorData();

            for (int i = 0; i < count; i++)
            {
                byte reg = (byte)((currentRegister + i) & 0xFF);
                result[i] = GetRegisterValue(reg);
            }

            this.Log(LogLevel.Debug, "Read {0} bytes from 0x{1:X2}", count, currentRegister);
            return result;
        }

        public void FinishTransmission()
        {
        }

        private byte GetRegisterValue(byte reg)
        {
            // Dynamic sensor output registers
            switch (reg)
            {
                // Temperature (0x20-0x21)
                case 0x20: return (byte)(temperature & 0xFF);
                case 0x21: return (byte)((temperature >> 8) & 0xFF);
                
                // Gyroscope X (0x22-0x23)
                case 0x22: return (byte)(gyroX & 0xFF);
                case 0x23: return (byte)((gyroX >> 8) & 0xFF);
                
                // Gyroscope Y (0x24-0x25)
                case 0x24: return (byte)(gyroY & 0xFF);
                case 0x25: return (byte)((gyroY >> 8) & 0xFF);
                
                // Gyroscope Z (0x26-0x27)
                case 0x26: return (byte)(gyroZ & 0xFF);
                case 0x27: return (byte)((gyroZ >> 8) & 0xFF);
                
                // Accelerometer X (0x28-0x29)
                case 0x28: return (byte)(accelX & 0xFF);
                case 0x29: return (byte)((accelX >> 8) & 0xFF);
                
                // Accelerometer Y (0x2A-0x2B)
                case 0x2A: return (byte)(accelY & 0xFF);
                case 0x2B: return (byte)((accelY >> 8) & 0xFF);
                
                // Accelerometer Z (0x2C-0x2D)
                case 0x2C: return (byte)(accelZ & 0xFF);
                case 0x2D: return (byte)((accelZ >> 8) & 0xFF);
                
                // Static registers
                default:
                    return registers[reg];
            }
        }

        private void UpdateSensorData()
        {
            time += 0.001;
            
            // Accelerometer: ±2g range, 16384 LSB/g
            // Simulate gravity on Z-axis with minor vibration
            double noise = (random.NextDouble() - 0.5) * 0.02;
            
            // X/Y: small oscillation (balloon sway)
            accelX = (short)(Math.Sin(time * 0.5) * 500);  // ~0.03g sway
            accelY = (short)(Math.Cos(time * 0.3) * 500);  // ~0.03g sway
            
            // Z: gravity (~1g = 16384 LSB) with noise
            accelZ = (short)(16384 * (1.0 + noise));
            
            // Gyroscope: ±250 dps range, 131 LSB/dps
            // Simulate slow rotation (balloon spin)
            gyroX = (short)(Math.Sin(time * 0.2) * 100);   // ~0.76 dps
            gyroY = (short)(Math.Cos(time * 0.2) * 100);   // ~0.76 dps
            gyroZ = (short)(Math.Sin(time * 0.1) * 200);   // ~1.5 dps (yaw)
            
            // Temperature: LSM6DSV16X outputs in 256 LSB/°C, offset 25°C
            // Simulate stratospheric temperature based on MS5611 altitude
            double temp_C = 25.0 + noise * 5.0;  // ~25°C with noise
            temperature = (short)(temp_C * 256);
            
            this.Log(LogLevel.Debug, 
                "Sensor: AccelX={0}, AccelY={1}, AccelZ={2}, GyroX={3}, GyroY={4}, GyroZ={5}",
                accelX, accelY, accelZ, gyroX, gyroY, gyroZ);
        }

        // State
        private byte currentRegister;
        private byte[] registers;
        private double time;
        private Random random = new Random();
        
        // Sensor data (16-bit signed)
        private short accelX, accelY, accelZ;
        private short gyroX, gyroY, gyroZ;
        private short temperature;
    }
}
