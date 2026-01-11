// PMS3003 Dust Sensor Model for Renode
using System;
using System.Collections.Generic;
using Antmicro.Renode.Core;
using Antmicro.Renode.Logging;
using Antmicro.Renode.Peripherals.UART;
using Antmicro.Renode.Peripherals.Timers;
using Antmicro.Renode.Time;

namespace Antmicro.Renode.Peripherals.Sensors
{
    public class PMS3003 : IUART, IDisposable
    {
        public PMS3003(Machine machine)
        {
            this.machine = machine;
            outputBuffer = new Queue<byte>();
            
            // Start a timer to send data every 1 second
            timer = new LimitTimer(machine.ClockSource, 1000, this, "pms3003_timer", 1000, enabled: true, eventEnabled: true);
            timer.LimitReached += delegate
            {
                SendMeasurement();
            };
        }

        public void WriteChar(byte value)
        {
            // PMS3003 can receive commands (sleep/wake/passive-read)
            // For now, ignore
            this.Log(LogLevel.Debug, "PMS3003 Received: 0x{0:X}", value);
        }

        public void Reset()
        {
            outputBuffer.Clear();
            timer.Reset(); 
            timer.Enabled = true;
        }

        public event Action<byte> CharReceived;

        public uint BaudRate => 9600;
        public Parity ParityBit => Parity.None;
        public Bits StopBits => Bits.One;

        public void Dispose()
        {
            // timer.Dispose(); // LimitTimer does not implement IDisposable
            timer.Enabled = false;
        }

        private void SendMeasurement()
        {
            // Prepare 32-byte packet
            byte[] packet = new byte[32];
            
            packet[0] = 0x42;
            packet[1] = 0x4D;
            packet[2] = 0x00; // Length H
            packet[3] = 0x1C; // Length L (28 bytes)
            
            // Standard Particles (CF=1)
            SetBigEndian(packet, 4, 10); // PM1.0 = 10
            SetBigEndian(packet, 6, 25); // PM2.5 = 25
            SetBigEndian(packet, 8, 30); // PM10  = 30
            
            // Atmospheric Environment
            SetBigEndian(packet, 10, 10);
            SetBigEndian(packet, 12, 25);
            SetBigEndian(packet, 14, 30);
            
            // Reserved / Version / Error
            packet[28] = 0x00; 
            packet[29] = 0x00; 
            
            // Checksum (Sum of bytes 0-29)
            uint checksum = 0;
            for(int i=0; i<30; i++)
            {
                checksum += packet[i];
            }
            SetBigEndian(packet, 30, (ushort)checksum);

            // Send packet
            foreach(byte b in packet)
            {
                CharReceived?.Invoke(b);
            }
        }

        private void SetBigEndian(byte[] buf, int outputOffset, ushort value)
        {
            buf[outputOffset] = (byte)((value >> 8) & 0xFF);
            buf[outputOffset + 1] = (byte)(value & 0xFF);
        }

        private readonly Machine machine;
        private readonly LimitTimer timer;
        private readonly Queue<byte> outputBuffer;
    }
}
