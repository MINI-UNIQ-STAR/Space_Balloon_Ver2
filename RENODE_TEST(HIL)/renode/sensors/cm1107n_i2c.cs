// CM1107N CO2 Sensor Model
using System;
using System.Collections.Generic;
using Antmicro.Renode.Peripherals.Bus;
using Antmicro.Renode.Logging;
using Antmicro.Renode.Core;
using Antmicro.Renode.Utilities;

namespace Antmicro.Renode.Peripherals.I2C
{
    public class CM1107N : II2CPeripheral
    {
        public CM1107N()
        {
            Reset();
        }

        public void Reset()
        {
            commandBuffer.Clear();
            state = State.Idle;
        }

        public void Write(byte[] data)
        {
            foreach (var b in data)
            {
                commandBuffer.Enqueue(b);
            }

            // Check if we received the read command: 11 01 01 ED
            if (commandBuffer.Count >= 4)
            {
                var cmd = commandBuffer.ToArray();
                if (cmd[0] == 0x11 && cmd[1] == 0x01 && cmd[2] == 0x01 && cmd[3] == 0xED)
                {
                    state = State.DataReady;
                    this.Log(LogLevel.Info, "Received Read CO2 Command");
                }
                commandBuffer.Clear();
            }
        }

        public byte[] Read(int count)
        {
            if (state == State.DataReady)
            {
                // Return fixed 450 ppm
                // Header: 16 05 01
                // CO2: 01 C2 (450)
                // Status: 00
                // Reserved: 00
                // Checksum: (should be calculated)
                
                var resp = new byte[] { 0x16, 0x05, 0x01, 0x01, 0xC2, 0x00, 0x00, 0x00 };
                
                // Calculate Checksum
                int sum = 0;
                for(int i=0; i<7; i++) sum += resp[i];
                resp[7] = (byte)((256 - (sum % 256)) % 256);
                
                state = State.Idle;
                this.Log(LogLevel.Info, "Sending CO2 Data: 450 ppm");
                return resp;
            }
            return new byte[count];
        }

        public void FinishTransmission()
        {
        }

        private Queue<byte> commandBuffer = new Queue<byte>();
        private State state;

        private enum State
        {
            Idle,
            DataReady
        }
    }
}
