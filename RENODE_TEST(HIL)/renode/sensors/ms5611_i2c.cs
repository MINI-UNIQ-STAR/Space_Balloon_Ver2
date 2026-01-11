// MS5611-01BA03 Barometric Pressure Sensor for Renode
// High-precision altitude measurement with stratospheric simulation (0-40km)
// TE Connectivity datasheet-compliant implementation

using System;
using Antmicro.Renode.Core;
using Antmicro.Renode.Logging;
using Antmicro.Renode.Peripherals.I2C;

namespace Antmicro.Renode.Peripherals.Sensors
{
    public class MS5611 : II2CPeripheral
    {
        public MS5611()
        {
            InitializePROM();
            Reset();
        }

        public void Reset()
        {
            currentCommand = 0;
            conversionReady = false;
            readIndex = 0;
            simulationTime = 0;
            altitude = 0;
            ascentRate = 5.0; // 5 m/s typical balloon ascent

            D1 = 0;
            D2 = 0;

            UpdateSimulation();

            this.Log(LogLevel.Info, "MS5611 Reset - Altitude: 0m");
        }

        public void Write(byte[] data)
        {
            if (data.Length == 0)
                return;

            currentCommand = data[0];

            // Reset command
            if (currentCommand == CMD_RESET)
            {
                Reset();
                this.Log(LogLevel.Debug, "Reset command received");
                return;
            }

            // PROM read commands (0xA0-0xAE)
            if ((currentCommand & 0xF0) == 0xA0)
            {
                readIndex = (currentCommand >> 1) & 0x07;
                this.Log(LogLevel.Debug, "PROM Read C{0}: 0x{1:X4}", readIndex, prom[readIndex]);
                return;
            }

            // Convert D1 (pressure) commands (0x40-0x48)
            if ((currentCommand & 0xF0) == 0x40)
            {
                UpdateSimulation();
                conversionReady = true;
                this.Log(LogLevel.Debug, "Convert D1 (pressure) OSR={(0}}", (currentCommand & 0x0F));
                return;
            }

            // Convert D2 (temperature) commands (0x50-0x58)
            if ((currentCommand & 0xF0) == 0x50)
            {
                UpdateSimulation();
                conversionReady = true;
                this.Log(LogLevel.Debug, "Convert D2 (temperature) OSR={(0}}", (currentCommand & 0x0F));
                return;
            }

            // ADC read command (0x00)
            if (currentCommand == CMD_ADC_READ)
            {
                readIndex = 0;
                this.Log(LogLevel.Debug, "ADC Read command");
                return;
            }
        }

        public byte[] Read(int count)
        {
            byte[] result = new byte[count];

            // PROM read (16-bit, big-endian)
            if ((currentCommand & 0xF0) == 0xA0)
            {
                int promIndex = (currentCommand >> 1) & 0x07;
                ushort value = prom[promIndex];

                result[0] = (byte)(value >> 8);    // High byte
                if (count > 1)
                    result[1] = (byte)(value & 0xFF); // Low byte

                this.Log(LogLevel.Debug, "PROM C{0} = 0x{1:X4}", promIndex, value);
                return result;
            }

            // ADC read (24-bit, big-endian)
            if (currentCommand == CMD_ADC_READ && conversionReady)
            {
                uint adcValue = wasLastConversionD1 ? D1 : D2;

                result[0] = (byte)((adcValue >> 16) & 0xFF); // MSB
                if (count > 1)
                    result[1] = (byte)((adcValue >> 8) & 0xFF);
                if (count > 2)
                    result[2] = (byte)(adcValue & 0xFF);      // LSB

                this.Log(LogLevel.Debug, "ADC = 0x{0:X6} ({0})", adcValue);
                return result;
            }

            return result;
        }

        public void FinishTransmission()
        {
            // Track which conversion was last
            if ((currentCommand & 0xF0) == 0x40)
                wasLastConversionD1 = true;
            else if ((currentCommand & 0xF0) == 0x50)
                wasLastConversionD1 = false;
        }

        private void InitializePROM()
        {
            // Factory calibration coefficients (typical MS5611 values)
            prom[0] = 0x3132;  // Factory reserved
            prom[1] = 0xA2E0;  // C1 = 41696 (SENS_T1)
            prom[2] = 0x9188;  // C2 = 37256 (OFF_T1)
            prom[3] = 0x5B93;  // C3 = 23443 (TCS)
            prom[4] = 0x5D1D;  // C4 = 23837 (TCO)
            prom[5] = 0x7D8F;  // C5 = 32143 (T_REF)
            prom[6] = 0x6D0F;  // C6 = 27919 (TEMPSENS)
            prom[7] = (ushort)(CalculateCRC() << 12); // CRC in upper 4 bits
        }

        private ushort CalculateCRC()
        {
            uint n_rem = 0;

            // CRC-4 calculation as per MS5611 datasheet
            for (int i = 0; i < 16; i++)
            {
                if (i % 2 == 1)
                    n_rem ^= (uint)(prom[i >> 1] & 0x00FF);
                else
                    n_rem ^= (uint)(prom[i >> 1] >> 8);

                for (int j = 0; j < 8; j++)
                {
                    if ((n_rem & 0x8000) != 0)
                        n_rem = (n_rem << 1) ^ 0x3000;
                    else
                        n_rem = n_rem << 1;
                }
            }

            return (ushort)((n_rem >> 12) & 0x000F);
        }

        private void UpdateSimulation()
        {
            // Update time (10 Hz simulation)
            simulationTime += 0.1;

            // Calculate current altitude (balloon ascent)
            altitude = ascentRate * simulationTime;
            if (altitude > 40000)
                altitude = 40000; // Cap at 40km

            // Calculate atmospheric conditions at current altitude
            var (pressure_mbar, temperature_C) = CalculateAtmosphere(altitude);

            // Add realistic sensor noise (±0.5%)
            Random random = new Random();
            double noise = (random.NextDouble() - 0.5) * 0.01;
            pressure_mbar *= (1.0 + noise);
            temperature_C += noise * 2.0;

            // Convert physical values to ADC readings
            (D1, D2) = PressureTempToADC(pressure_mbar, temperature_C);

            this.Log(LogLevel.Debug,
                "Simulation: Alt={0:F0}m, P={1:F2}mbar, T={2:F1}°C, D1={3}, D2={4}",
                altitude, pressure_mbar, temperature_C, D1, D2);
        }

        private (double pressure_mbar, double temperature_C) CalculateAtmosphere(double h)
        {
            // International Standard Atmosphere model
            double temperature_K;
            double pressure_Pa;

            if (h < 11000)
            {
                // Troposphere: 0-11 km
                temperature_K = 288.15 - 0.0065 * h;
                pressure_Pa = 101325.0 * Math.Pow(temperature_K / 288.15, 5.2561);
            }
            else if (h < 20000)
            {
                // Lower Stratosphere: 11-20 km (isothermal)
                temperature_K = 216.65;
                pressure_Pa = 22632.0 * Math.Exp(-0.00015769 * (h - 11000));
            }
            else if (h < 32000)
            {
                // Mid Stratosphere: 20-32 km
                temperature_K = 216.65 + 0.001 * (h - 20000);
                double factor = Math.Pow(216.65 / temperature_K, 34.1632 / 0.001);
                pressure_Pa = 5474.9 * Math.Exp(-0.00015769 * (h - 20000)) * factor;
            }
            else
            {
                // Upper Stratosphere: 32-47 km
                temperature_K = 228.65 + 0.0028 * (h - 32000);
                pressure_Pa = 868.02 * Math.Pow(temperature_K / 228.65, -34.1632 / 0.0028);
            }

            double temperature_C = temperature_K - 273.15;
            double pressure_mbar = pressure_Pa / 100.0;

            return (pressure_mbar, temperature_C);
        }

        private (uint D1, uint D2) PressureTempToADC(double pressure_mbar, double temperature_C)
        {
            // Reverse MS5611 algorithm to generate realistic ADC values

            // Target values (MS5611 uses 0.01 resolution)
            long TEMP_target = (long)(temperature_C * 100);
            long P_target = (long)(pressure_mbar * 100);

            // Get calibration coefficients
            long C1 = prom[1];
            long C2 = prom[2];
            long C3 = prom[3];
            long C4 = prom[4];
            long C5 = prom[5];
            long C6 = prom[6];

            // Calculate dT from target temperature
            // TEMP = 2000 + dT * C6 / 2^23
            long dT = ((TEMP_target - 2000) * 8388608) / C6;

            // Calculate D2 from dT
            // dT = D2 - C5 * 2^8
            uint D2_calc = (uint)(dT + C5 * 256);

            // Calculate OFF and SENS
            long OFF = C2 * 65536 + (C4 * dT) / 128;
            long SENS = C1 * 32768 + (C3 * dT) / 256;

            // Calculate D1 from target pressure
            // P = (D1 * SENS / 2^21 - OFF) / 2^15
            long D1_calc = ((P_target * 32768 + OFF) * 2097152) / SENS;

            // Ensure 24-bit range
            D1_calc = Math.Max(0, Math.Min(0xFFFFFF, D1_calc));
            D2_calc = Math.Max(0, Math.Min(0xFFFFFF, D2_calc));

            return ((uint)D1_calc, D2_calc);
        }

        // Commands
        private const byte CMD_RESET = 0x1E;
        private const byte CMD_ADC_READ = 0x00;

        // State
        private byte currentCommand;
        private bool conversionReady;
        private bool wasLastConversionD1;
        private int readIndex;

        // PROM calibration data (8x 16-bit)
        private ushort[] prom = new ushort[8];

        // ADC values (24-bit)
        private uint D1; // Pressure ADC
        private uint D2; // Temperature ADC

        // Simulation
        private double simulationTime;  // seconds
        private double altitude;        // meters
        private double ascentRate;      // m/s
    }
}
