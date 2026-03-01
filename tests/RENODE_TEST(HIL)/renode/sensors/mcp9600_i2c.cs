// MCP9600 Thermocouple EMF to Temperature Converter for Renode
// Microchip MCP9600 - I2C Thermocouple Interface
// Simple I2C implementation

using System;
using Antmicro.Renode.Core;
using Antmicro.Renode.Logging;
using Antmicro.Renode.Peripherals.I2C;

namespace Antmicro.Renode.Peripherals.Sensors
{
    public class MCP9600 : II2CPeripheral
    {
        public MCP9600()
        {
            Reset();
        }

        public void Reset()
        {
            currentRegister = 0;
            
            // Default temperatures
            hotJunctionTemp = 25.0;   // °C
            coldJunctionTemp = 25.0;  // °C (ambient)
            deltaTemp = 0.0;
            
            // Device configuration
            deviceId = 0x40;  // MCP9600 device ID
            deviceRevision = 0x00;
            
            this.Log(LogLevel.Info, "MCP9600 Reset - Hot: 25°C, Cold: 25°C");
        }

        public void Write(byte[] data)
        {
            if (data.Length == 0)
                return;

            // First byte is register address
            currentRegister = data[0];
            this.Log(LogLevel.Debug, "Register: 0x{0:X2}", currentRegister);

            // Handle register writes
            if (data.Length > 1)
            {
                switch (currentRegister)
                {
                    case REG_THERMOCOUPLE_CONFIG:
                        thermocoupleConfig = data[1];
                        this.Log(LogLevel.Debug, "Thermocouple config: 0x{0:X2}", thermocoupleConfig);
                        break;
                        
                    case REG_DEVICE_CONFIG:
                        deviceConfig = data[1];
                        this.Log(LogLevel.Debug, "Device config: 0x{0:X2}", deviceConfig);
                        break;
                        
                    case REG_ALERT1_CONFIG:
                    case REG_ALERT2_CONFIG:
                    case REG_ALERT3_CONFIG:
                    case REG_ALERT4_CONFIG:
                        // Alert configuration
                        this.Log(LogLevel.Debug, "Alert config written");
                        break;
                }
            }
        }

        public byte[] Read(int count)
        {
            byte[] result = new byte[count];

            // Update simulation before reading
            UpdateSimulation();

            switch (currentRegister)
            {
                case REG_HOT_JUNCTION:
                    // Hot junction temperature (16-bit, 0.0625°C resolution)
                    short rawHot = (short)(hotJunctionTemp / 0.0625);
                    if (count >= 1) result[0] = (byte)(rawHot >> 8);
                    if (count >= 2) result[1] = (byte)(rawHot & 0xFF);
                    this.Log(LogLevel.Debug, "Hot junction: {0:F2}°C (raw: 0x{1:X4})", 
                        hotJunctionTemp, rawHot);
                    break;
                    
                case REG_JUNCTION_DELTA:
                    // Temperature delta (16-bit, 0.0625°C resolution)
                    short rawDelta = (short)(deltaTemp / 0.0625);
                    if (count >= 1) result[0] = (byte)(rawDelta >> 8);
                    if (count >= 2) result[1] = (byte)(rawDelta & 0xFF);
                    break;
                    
                case REG_COLD_JUNCTION:
                    // Cold junction (ambient) temperature (16-bit, 0.0625°C resolution)
                    short rawCold = (short)(coldJunctionTemp / 0.0625);
                    if (count >= 1) result[0] = (byte)(rawCold >> 8);
                    if (count >= 2) result[1] = (byte)(rawCold & 0xFF);
                    this.Log(LogLevel.Debug, "Cold junction: {0:F2}°C", coldJunctionTemp);
                    break;
                    
                case REG_RAW_ADC:
                    // Raw ADC value (24-bit)
                    int rawAdc = (int)(deltaTemp * 1000);  // Approximate µV value
                    if (count >= 1) result[0] = (byte)((rawAdc >> 16) & 0xFF);
                    if (count >= 2) result[1] = (byte)((rawAdc >> 8) & 0xFF);
                    if (count >= 3) result[2] = (byte)(rawAdc & 0xFF);
                    break;
                    
                case REG_STATUS:
                    // Status register
                    result[0] = 0x40;  // Data ready, no alerts
                    break;
                    
                case REG_THERMOCOUPLE_CONFIG:
                    result[0] = thermocoupleConfig;
                    break;
                    
                case REG_DEVICE_CONFIG:
                    result[0] = deviceConfig;
                    break;
                    
                case REG_DEVICE_ID:
                    // Device ID/Revision
                    if (count >= 1) result[0] = deviceId;
                    if (count >= 2) result[1] = deviceRevision;
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
            
            double noise = (random.NextDouble() - 0.5) * 0.5;
            
            // Simulate cold junction (ambient) temperature decrease with altitude
            // Starts at 25°C, decreases ~6.5°C per km
            coldJunctionTemp = 25.0 - time * 0.3;  // Slow decrease
            if (coldJunctionTemp < -40.0) coldJunctionTemp = -40.0;
            coldJunctionTemp += noise;
            
            // Simulate hot junction (external probe) temperature
            // Could be measuring exhaust, electronics, or external environment
            // In balloon: typically similar to ambient but may have heat sources
            hotJunctionTemp = coldJunctionTemp + random.NextDouble() * 5.0;  // Slightly warmer
            
            // Delta temperature
            deltaTemp = hotJunctionTemp - coldJunctionTemp;
            
            // Clamp to valid range (-40°C to +125°C typical)
            hotJunctionTemp = Math.Max(-40.0, Math.Min(125.0, hotJunctionTemp));
            coldJunctionTemp = Math.Max(-40.0, Math.Min(125.0, coldJunctionTemp));
        }

        // MCP9600 Register Addresses
        private const byte REG_HOT_JUNCTION = 0x00;
        private const byte REG_JUNCTION_DELTA = 0x01;
        private const byte REG_COLD_JUNCTION = 0x02;
        private const byte REG_RAW_ADC = 0x03;
        private const byte REG_STATUS = 0x04;
        private const byte REG_THERMOCOUPLE_CONFIG = 0x05;
        private const byte REG_DEVICE_CONFIG = 0x06;
        private const byte REG_ALERT1_CONFIG = 0x08;
        private const byte REG_ALERT2_CONFIG = 0x09;
        private const byte REG_ALERT3_CONFIG = 0x0A;
        private const byte REG_ALERT4_CONFIG = 0x0B;
        private const byte REG_ALERT1_HYSTERESIS = 0x0C;
        private const byte REG_ALERT2_HYSTERESIS = 0x0D;
        private const byte REG_ALERT3_HYSTERESIS = 0x0E;
        private const byte REG_ALERT4_HYSTERESIS = 0x0F;
        private const byte REG_ALERT1_LIMIT = 0x10;
        private const byte REG_ALERT2_LIMIT = 0x11;
        private const byte REG_ALERT3_LIMIT = 0x12;
        private const byte REG_ALERT4_LIMIT = 0x13;
        private const byte REG_DEVICE_ID = 0x20;

        // State
        private byte currentRegister;
        private byte thermocoupleConfig = 0x00;  // Type K by default
        private byte deviceConfig = 0x00;
        private byte deviceId;
        private byte deviceRevision;
        private double time;
        private Random random = new Random();
        
        // Temperature data
        private double hotJunctionTemp;
        private double coldJunctionTemp;
        private double deltaTemp;
    }
}
