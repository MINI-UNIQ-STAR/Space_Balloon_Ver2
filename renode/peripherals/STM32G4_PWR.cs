// STM32G4 PWR (Power Control) Peripheral for Renode
// Implements power management functionality

using System;
using Antmicro.Renode.Core;
using Antmicro.Renode.Logging;
using Antmicro.Renode.Peripherals.Bus;
using Antmicro.Renode.Core.Structure.Registers;

namespace Antmicro.Renode.Peripherals.Miscellaneous
{
    public class STM32G4_PWR : BasicDoubleWordPeripheral, IKnownSize
    {
        public STM32G4_PWR(IMachine machine) : base(machine)
        {
            DefineRegisters();
            Reset();
        }

        public override void Reset()
        {
            base.Reset();

            // Default values from reference manual
            cr1.Value = 0x00000200;  // VOS = 01 (Range 1)
            cr2.Value = 0x00000000;
            cr3.Value = 0x00008000;  // APC set
            cr4.Value = 0x00000000;
            sr1.Value = 0x00000000;
            sr2.Value = 0x00000000;

            this.Log(LogLevel.Info, "PWR Reset - Range 1, low power disabled");
        }

        private void DefineRegisters()
        {
            // CR1 - Power Control Register 1 (0x00)
            Registers.CR1.Define(this, 0x00000200)
                .WithValueField(0, 3, name: "LPMS")
                .WithReservedBits(3, 1)
                .WithFlag(4, name: "FPD_STOP")
                .WithFlag(5, name: "FPD_LPRUN")
                .WithFlag(6, name: "FPD_LPSLP")
                .WithReservedBits(7, 1)
                .WithFlag(8, name: "DBP")
                .WithValueField(9, 2, out vos, name: "VOS")
                .WithReservedBits(11, 3)
                .WithFlag(14, name: "LPR")
                .WithReservedBits(15, 17)
                .WithWriteCallback((_, __) =>
                {
                    this.Log(LogLevel.Debug, "CR1: VOS={0}", vos.Value);
                });

            // CR2 - Power Control Register 2 (0x04)
            Registers.CR2.Define(this, 0x00000000)
                .WithFlag(0, name: "PVDE")
                .WithValueField(1, 3, name: "PVDFT")
                .WithValueField(4, 3, name: "PVDRT")
                .WithReservedBits(7, 25);

            // CR3 - Power Control Register 3 (0x08)
            Registers.CR3.Define(this, 0x00008000)
                .WithFlag(0, name: "EWUP1")
                .WithFlag(1, name: "EWUP2")
                .WithFlag(2, name: "EWUP3")
                .WithFlag(3, name: "EWUP4")
                .WithFlag(4, name: "EWUP5")
                .WithReservedBits(5, 3)
                .WithFlag(8, name: "RRS")
                .WithReservedBits(9, 1)
                .WithFlag(10, name: "APC")
                .WithReservedBits(11, 4)
                .WithFlag(15, name: "EIWUL")
                .WithReservedBits(16, 16);

            // CR4 - Power Control Register 4 (0x0C)
            Registers.CR4.Define(this, 0x00000000)
                .WithFlag(0, name: "WP1")
                .WithFlag(1, name: "WP2")
                .WithFlag(2, name: "WP3")
                .WithFlag(3, name: "WP4")
                .WithFlag(4, name: "WP5")
                .WithReservedBits(5, 3)
                .WithFlag(8, name: "VBE")
                .WithFlag(9, name: "VBRS")
                .WithReservedBits(10, 22);

            // SR1 - Power Status Register 1 (0x10)
            Registers.SR1.Define(this, 0x00000000)
                .WithFlag(0, FieldMode.Read, name: "WUF1")
                .WithFlag(1, FieldMode.Read, name: "WUF2")
                .WithFlag(2, FieldMode.Read, name: "WUF3")
                .WithFlag(3, FieldMode.Read, name: "WUF4")
                .WithFlag(4, FieldMode.Read, name: "WUF5")
                .WithReservedBits(5, 3)
                .WithFlag(8, FieldMode.Read, name: "SBF")
                .WithReservedBits(9, 6)
                .WithFlag(15, FieldMode.Read, name: "WUFI")
                .WithReservedBits(16, 16);

            // SR2 - Power Status Register 2 (0x14)
            Registers.SR2.Define(this, 0x00000000)
                .WithReservedBits(0, 8)
                .WithFlag(8, FieldMode.Read, name: "REGLPS")
                .WithFlag(9, FieldMode.Read, name: "REGLPF")
                .WithFlag(10, FieldMode.Read, valueProviderCallback: _ => true, name: "VOSF")
                .WithFlag(11, FieldMode.Read, name: "PVDO")
                .WithReservedBits(12, 2)
                .WithFlag(14, FieldMode.Read, valueProviderCallback: _ => true, name: "PVMO1")
                .WithFlag(15, FieldMode.Read, valueProviderCallback: _ => true, name: "PVMO2")
                .WithReservedBits(16, 16);

            // SCR - Power Status Clear Register (0x18)
            Registers.SCR.Define(this, 0x00000000)
                .WithFlag(0, FieldMode.Write, name: "CWUF1")
                .WithFlag(1, FieldMode.Write, name: "CWUF2")
                .WithFlag(2, FieldMode.Write, name: "CWUF3")
                .WithFlag(3, FieldMode.Write, name: "CWUF4")
                .WithFlag(4, FieldMode.Write, name: "CWUF5")
                .WithReservedBits(5, 3)
                .WithFlag(8, FieldMode.Write, name: "CSBF")
                .WithReservedBits(9, 23);

            // PUCRA - Port A pull-up control (0x20)
            Registers.PUCRA.Define(this, 0x00000000);
            // PDCRA - Port A pull-down control (0x24)
            Registers.PDCRA.Define(this, 0x00000000);
            // Similar for other ports...
        }

        public long Size => 0x400;

        private enum Registers
        {
            CR1 = 0x00,
            CR2 = 0x04,
            CR3 = 0x08,
            CR4 = 0x0C,
            SR1 = 0x10,
            SR2 = 0x14,
            SCR = 0x18,
            PUCRA = 0x20,
            PDCRA = 0x24,
        }

        // Register storage
        private IValueRegisterField cr1;
        private IValueRegisterField cr2;
        private IValueRegisterField cr3;
        private IValueRegisterField cr4;
        private IValueRegisterField sr1;
        private IValueRegisterField sr2;

        // CR1 fields
        private IValueRegisterField vos;
    }
}
