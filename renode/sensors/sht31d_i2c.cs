// SHT31D Temperature & Humidity Sensor for Renode
// Sensirion SHT31-D - I2C Digital Humidity and Temperature Sensor
// Simple I2C implementation (no ByteRegisterCollection)

using System;
using Antmicro.Renode.Core;
using Antmicro.Renode.Logging;
using Antmicro.Renode.Peripherals.I2C;

namespace Antmicro.Renode.Peripherals.Sensors
{
    public class SHT31D : II2CPeripheral
    {
        public SHT31D()
        {
            Reset();
        }

        public void Reset()
        {
            currentCommand = 0;
            measurementReady = false;
            
            // Default values (room temperature, moderate humidity)
            temperature = 25.0;  // °C
            humidity = 50.0;     // %RH
            
            this.Log(LogLevel.Info, "SHT31D Reset - Default: 25°C, 50%RH");
        }

        public void Write(byte[] data)
        {
            if (data.Length == 0)
                return;

            // SHT31D uses 16-bit commands (MSB first)
            if (data.Length >= 2)
            {
                currentCommand = (ushort)((data[0] << 8) | data[1]);
            }
            else
            {
                currentCommand = data[0];
            }

            this.Log(LogLevel.Debug, "Command: 0x{0:X4}", currentCommand);

            // Process commands
            switch (currentCommand)
            {
                case CMD_SOFT_RESET:
                    Reset();
                    this.Log(LogLevel.Debug, "Soft Reset");
                    break;
                    
                case CMD_MEASURE_HIGHREP:
                case CMD_MEASURE_MEDREP:
                case CMD_MEASURE_LOWREP:
                case CMD_MEASURE_HIGHREP_STRETCH:
                case CMD_MEASURE_MEDREP_STRETCH:
                case CMD_MEASURE_LOWREP_STRETCH:
                    // Trigger measurement
                    UpdateSimulation();
                    measurementReady = true;
                    this.Log(LogLevel.Debug, "Measurement triggered: T={0:F1}°C, RH={1:F1}%", 
                        temperature, humidity);
                    break;
                    
                case CMD_HEATER_ENABLE:
                    heaterEnabled = true;
                    this.Log(LogLevel.Debug, "Heater enabled");
                    break;
                    
                case CMD_HEATER_DISABLE:
                    heaterEnabled = false;
                    this.Log(LogLevel.Debug, "Heater disabled");
                    break;
                    
                case CMD_READ_STATUS:
                    // Status register read
                    this.Log(LogLevel.Debug, "Status read request");
                    break;
                    
                case CMD_CLEAR_STATUS:
                    this.Log(LogLevel.Debug, "Status cleared");
                    break;
            }
        }

        public byte[] Read(int count)
        {
            byte[] result = new byte[count];

            if (measurementReady && count >= 6)
            {
                // Return 6 bytes: Temp MSB, Temp LSB, Temp CRC, Hum MSB, Hum LSB, Hum CRC
                
                // Convert temperature to raw value
                // T[°C] = -45 + 175 * S_T / (2^16 - 1)
                // S_T = (T + 45) * 65535 / 175
                ushort rawTemp = (ushort)((temperature + 45.0) * 65535.0 / 175.0);
                
                // Convert humidity to raw value
                // RH = 100 * S_RH / (2^16 - 1)
                // S_RH = RH * 65535 / 100
                ushort rawHum = (ushort)(humidity * 65535.0 / 100.0);
                
                result[0] = (byte)(rawTemp >> 8);      // Temp MSB
                result[1] = (byte)(rawTemp & 0xFF);    // Temp LSB
                result[2] = CalculateCRC(result, 0, 2); // Temp CRC
                result[3] = (byte)(rawHum >> 8);       // Hum MSB
                result[4] = (byte)(rawHum & 0xFF);     // Hum LSB
                result[5] = CalculateCRC(result, 3, 2); // Hum CRC
                
                this.Log(LogLevel.Debug, 
                    "Read: RawTemp=0x{0:X4}, RawHum=0x{1:X4}", rawTemp, rawHum);
            }
            else if (currentCommand == CMD_READ_STATUS && count >= 3)
            {
                // Status register (16-bit + CRC)
                ushort status = 0x0000;
                if (heaterEnabled) status |= 0x2000; // Bit 13 = heater
                
                result[0] = (byte)(status >> 8);
                result[1] = (byte)(status & 0xFF);
                result[2] = CalculateCRC(result, 0, 2);
            }

            return result;
        }

        public void FinishTransmission()
        {
        }

        private void UpdateSimulation()
        {
            time += 0.1;
            
            // Simulate stratospheric conditions
            // Temperature: starts at 25°C, decreases with altitude
            // Humidity: starts at 50%, decreases with altitude
            
            double noise = (random.NextDouble() - 0.5) * 0.2;
            
            // Simulate gradual temperature decrease (balloon ascent)
            double baseTemp = 25.0 - time * 0.5;  // -0.5°C per update cycle
            if (baseTemp < -40.0) baseTemp = -40.0;  // Min -40°C
            temperature = baseTemp + noise;
            
            // Humidity decreases as altitude increases
            double baseHum = 50.0 - time * 2.0;  // -2% per update cycle
            if (baseHum < 5.0) baseHum = 5.0;  // Min 5%
            humidity = baseHum + noise * 5.0;
            
            // Clamp values to valid range
            temperature = Math.Max(-40.0, Math.Min(125.0, temperature));
            humidity = Math.Max(0.0, Math.Min(100.0, humidity));
        }

        private byte CalculateCRC(byte[] data, int start, int length)
        {
            // CRC-8 polynomial: x^8 + x^5 + x^4 + 1 = 0x31
            byte crc = 0xFF;
            
            for (int i = start; i < start + length; i++)
            {
                crc ^= data[i];
                for (int bit = 0; bit < 8; bit++)
                {
                    if ((crc & 0x80) != 0)
                        crc = (byte)((crc << 1) ^ 0x31);
                    else
                        crc = (byte)(crc << 1);
                }
            }
            
            return crc;
        }

        // SHT31D Commands (16-bit)
        private const ushort CMD_MEASURE_HIGHREP = 0x2400;         // High repeatability, no clock stretch
        private const ushort CMD_MEASURE_MEDREP = 0x240B;          // Medium repeatability
        private const ushort CMD_MEASURE_LOWREP = 0x2416;          // Low repeatability
        private const ushort CMD_MEASURE_HIGHREP_STRETCH = 0x2C06; // High rep with clock stretch
        private const ushort CMD_MEASURE_MEDREP_STRETCH = 0x2C0D;  // Medium rep with clock stretch
        private const ushort CMD_MEASURE_LOWREP_STRETCH = 0x2C10;  // Low rep with clock stretch
        private const ushort CMD_HEATER_ENABLE = 0x306D;
        private const ushort CMD_HEATER_DISABLE = 0x3066;
        private const ushort CMD_SOFT_RESET = 0x30A2;
        private const ushort CMD_READ_STATUS = 0xF32D;
        private const ushort CMD_CLEAR_STATUS = 0x3041;

        // State
        private ushort currentCommand;
        private bool measurementReady;
        private bool heaterEnabled;
        private double time;
        private Random random = new Random();
        
        // Sensor data
        private double temperature;  // °C
        private double humidity;     // %RH
    }
}
