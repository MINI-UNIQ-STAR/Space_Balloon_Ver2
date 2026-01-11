// GDK101 Gamma Radiation Sensor for Renode
// FTLAB GDK101 - I2C Gamma Ray Detector Module
// Simple I2C implementation

using System;
using Antmicro.Renode.Core;
using Antmicro.Renode.Logging;
using Antmicro.Renode.Peripherals.I2C;

namespace Antmicro.Renode.Peripherals.Sensors
{
    public class GDK101 : II2CPeripheral
    {
        public GDK101()
        {
            Reset();
        }

        public void Reset()
        {
            currentRegister = 0;
            
            // Default values
            measuringTime1Min = 0;
            measuringTime10Min = 0;
            status = 0x01;  // Normal operation
            firmwareVersion = 0x11;  // v1.1
            
            // Radiation values (µSv/h)
            radiation1Min = 0.08;   // ~0.08 µSv/h (normal background)
            radiation10Min = 0.08;  // ~0.08 µSv/h average
            
            this.Log(LogLevel.Info, "GDK101 Reset - Background radiation: 0.08 µSv/h");
        }

        public void Write(byte[] data)
        {
            if (data.Length == 0)
                return;

            // First byte is register address
            currentRegister = data[0];
            this.Log(LogLevel.Debug, "Register: 0x{0:X2}", currentRegister);

            // Handle reset command
            if (currentRegister == REG_RESET && data.Length > 1 && data[1] == 0xA5)
            {
                Reset();
                this.Log(LogLevel.Debug, "Reset command received");
            }
        }

        public byte[] Read(int count)
        {
            byte[] result = new byte[count];

            // Update simulation
            UpdateSimulation();

            switch (currentRegister)
            {
                case REG_READ_STATUS:
                    // Status register
                    result[0] = status;
                    this.Log(LogLevel.Debug, "Status: 0x{0:X2}", status);
                    break;
                    
                case REG_READ_MEASURING_TIME_1MIN:
                    // Measuring time counter (1 minute)
                    if (count >= 1) result[0] = (byte)(measuringTime1Min >> 8);
                    if (count >= 2) result[1] = (byte)(measuringTime1Min & 0xFF);
                    break;
                    
                case REG_READ_MEASURING_TIME_10MIN:
                    // Measuring time counter (10 minute)
                    if (count >= 1) result[0] = (byte)(measuringTime10Min >> 8);
                    if (count >= 2) result[1] = (byte)(measuringTime10Min & 0xFF);
                    break;
                    
                case REG_READ_VALUE_1MIN:
                    // Radiation value (1 minute average) - unit: µSv/h
                    // Format: XX.YY (e.g., 0x00 0x08 = 0.08 µSv/h)
                    byte rad1Int = (byte)radiation1Min;
                    byte rad1Dec = (byte)((radiation1Min - rad1Int) * 100);
                    if (count >= 1) result[0] = rad1Int;
                    if (count >= 2) result[1] = rad1Dec;
                    this.Log(LogLevel.Debug, "1min radiation: {0:F2} µSv/h", radiation1Min);
                    break;
                    
                case REG_READ_VALUE_10MIN:
                    // Radiation value (10 minute average)
                    byte rad10Int = (byte)radiation10Min;
                    byte rad10Dec = (byte)((radiation10Min - rad10Int) * 100);
                    if (count >= 1) result[0] = rad10Int;
                    if (count >= 2) result[1] = rad10Dec;
                    this.Log(LogLevel.Debug, "10min radiation: {0:F2} µSv/h", radiation10Min);
                    break;
                    
                case REG_READ_FIRMWARE:
                    // Firmware version
                    result[0] = firmwareVersion;
                    break;
            }

            return result;
        }

        public void FinishTransmission()
        {
        }

        private void UpdateSimulation()
        {
            time += 0.1;
            measuringTime1Min++;
            if (measuringTime1Min >= 60) measuringTime1Min = 0;
            measuringTime10Min = (ushort)((measuringTime1Min / 10) % 10);
            
            // Simulate cosmic ray radiation increase with altitude
            // Ground level: ~0.08 µSv/h
            // 10km altitude: ~1-2 µSv/h
            // 20km altitude: ~5-10 µSv/h
            // 30km altitude: ~10-20 µSv/h
            
            double altitude_km = time * 0.5;  // Simulate ascent
            if (altitude_km > 40.0) altitude_km = 40.0;
            
            // Exponential increase with altitude
            double baseRadiation = 0.08 * Math.Exp(altitude_km / 8.0);
            
            // Add random fluctuation (cosmic rays are random)
            double noise = (random.NextDouble() - 0.5) * 0.2 * baseRadiation;
            
            radiation1Min = baseRadiation + noise;
            radiation10Min = baseRadiation;  // Smoothed average
            
            // Clamp to valid range
            radiation1Min = Math.Max(0.01, Math.Min(99.99, radiation1Min));
            radiation10Min = Math.Max(0.01, Math.Min(99.99, radiation10Min));
            
            // Update status
            if (radiation1Min > 1.0)
                status = 0x02;  // Elevated radiation warning
            else
                status = 0x01;  // Normal
        }

        // GDK101 Register Addresses
        private const byte REG_READ_STATUS = 0x00;
        private const byte REG_READ_MEASURING_TIME_1MIN = 0x01;
        private const byte REG_READ_MEASURING_TIME_10MIN = 0x02;
        private const byte REG_READ_VALUE_1MIN = 0x03;
        private const byte REG_READ_VALUE_10MIN = 0x04;
        private const byte REG_READ_FIRMWARE = 0xB4;
        private const byte REG_RESET = 0xA0;

        // State
        private byte currentRegister;
        private byte status;
        private byte firmwareVersion;
        private ushort measuringTime1Min;
        private ushort measuringTime10Min;
        private double time;
        private Random random = new Random();
        
        // Radiation data (µSv/h)
        private double radiation1Min;
        private double radiation10Min;
    }
}
