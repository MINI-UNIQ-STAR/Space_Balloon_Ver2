// SEN0321 Ozone Sensor Model for Renode
using System;
using Antmicro.Renode.Core;
using Antmicro.Renode.Core.Structure.Registers;
using Antmicro.Renode.Peripherals.I2C;
using Antmicro.Renode.Logging;

namespace Antmicro.Renode.Peripherals.Sensors
{
    public class SEN0321 : II2CPeripheral, IProvidesRegisterCollection<ByteRegisterCollection>
    {
        public SEN0321()
        {
            RegistersCollection = new ByteRegisterCollection(this);
            DefineRegisters();
        }

        public void Reset()
        {
            RegistersCollection.Reset();
        }

        public void Write(byte[] data)
        {
            if(data.Length == 0) return;
            // Set current register address
            currentRegister = data[0];
            
            // If there is data to write
            if(data.Length > 1) 
            {
                // We could iterate and write, but SEN0321 is mostly read-only for data
                // For Write commands like MODE (0x03), simple handling:
                RegistersCollection.Write(currentRegister, data[1]);
            }
        }

        public byte[] Read(int count)
        {
            var result = new byte[count];
            for(int i=0; i<count; i++)
            {
                // Read from RegistersCollection using current register offset
                // We increment the register address for sequential reads
                result[i] = RegistersCollection.Read(currentRegister);
                currentRegister++;
            }
            return result;
        }

        private byte currentRegister;

        public void FinishTransmission()
        {
        }

        private void DefineRegisters()
        {
            // Auto Data High (0x09)
            Registers.AutoDataH.Define(this)
                .WithValueField(0, 8, FieldMode.Read, name: "AutoDataH", valueProviderCallback: _ => (byte)((OzonePPB >> 8) & 0xFF));

            // Auto Data Low (0x0A)
            Registers.AutoDataL.Define(this)
                .WithValueField(0, 8, FieldMode.Read, name: "AutoDataL", valueProviderCallback: _ => (byte)(OzonePPB & 0xFF));
            
            // Mode Register (0x03)
            Registers.Mode.Define(this)
                .WithValueField(0, 8, name: "Mode");
        }

        public ByteRegisterCollection RegistersCollection { get; }

        private const int OzonePPB = 20; // Mock Value: 20 ppb

        private enum Registers : byte
        {
            Mode = 0x03,
            AutoDataH = 0x09,
            AutoDataL = 0x0A
        }
    }
}
