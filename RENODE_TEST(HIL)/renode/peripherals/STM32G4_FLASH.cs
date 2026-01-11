// STM32G4 FLASH Controller Peripheral for Renode
// Implements flash memory interface control

using System;
using Antmicro.Renode.Core;
using Antmicro.Renode.Logging;
using Antmicro.Renode.Peripherals.Bus;
using Antmicro.Renode.Core.Structure.Registers;

namespace Antmicro.Renode.Peripherals.MTD
{
    public class STM32G4_FLASH : BasicDoubleWordPeripheral, IKnownSize
    {
        public STM32G4_FLASH(IMachine machine) : base(machine)
        {
            DefineRegisters();
            Reset();
        }

        public override void Reset()
        {
            base.Reset();

            // Default values
            acr.Value = 0x00000600;  // 0 wait states, prefetch disabled
            keyr.Value = 0x00000000;
            optkeyr.Value = 0x00000000;
            sr.Value = 0x00000000;
            cr.Value = 0xC0000000;   // Lock bit set

            this.Log(LogLevel.Info, "FLASH Controller Reset - Default latency, locked");
        }

        private void DefineRegisters()
        {
            // ACR - Access Control Register (0x00)
            Registers.ACR.Define(this, 0x00000600)
                .WithValueField(0, 3, out latency, name: "LATENCY")
                .WithReservedBits(3, 5)
                .WithFlag(8, out prften, name: "PRFTEN")
                .WithFlag(9, out icen, name: "ICEN")
                .WithFlag(10, out dcen, name: "DCEN")
                .WithFlag(11, FieldMode.Write, name: "ICRST", writeCallback: (_, val) =>
                {
                    if(val) this.Log(LogLevel.Debug, "Instruction cache reset");
                })
                .WithFlag(12, FieldMode.Write, name: "DCRST", writeCallback: (_, val) =>
                {
                    if(val) this.Log(LogLevel.Debug, "Data cache reset");
                })
                .WithFlag(13, name: "RUN_PD")
                .WithFlag(14, name: "SLEEP_PD")
                .WithReservedBits(15, 1)
                .WithFlag(16, name: "DBG_SWEN")
                .WithReservedBits(17, 15)
                .WithWriteCallback((_, __) =>
                {
                    this.Log(LogLevel.Debug, "ACR: Latency={0}, PRFTEN={1}, ICEN={2}, DCEN={3}",
                        latency.Value, prften.Value, icen.Value, dcen.Value);
                });

            // KEYR - Flash Key Register (0x08)
            Registers.KEYR.Define(this, 0x00000000)
                .WithValueField(0, 32, FieldMode.Write, name: "KEY", writeCallback: (_, val) =>
                {
                    // Unlock sequence: 0x45670123, then 0xCDEF89AB
                    if(val == 0x45670123)
                    {
                        unlockStep = 1;
                    }
                    else if(val == 0xCDEF89AB && unlockStep == 1)
                    {
                        locked.Value = false;
                        unlockStep = 0;
                        this.Log(LogLevel.Info, "FLASH Unlocked");
                    }
                    else
                    {
                        unlockStep = 0;
                    }
                });

            // OPTKEYR - Option Key Register (0x0C)
            Registers.OPTKEYR.Define(this, 0x00000000)
                .WithValueField(0, 32, FieldMode.Write, name: "OPTKEY");

            // SR - Status Register (0x10)
            Registers.SR.Define(this, 0x00000000)
                .WithFlag(0, FieldMode.Read | FieldMode.WriteOneToClear, name: "EOP")
                .WithFlag(1, FieldMode.Read | FieldMode.WriteOneToClear, name: "OPERR")
                .WithReservedBits(2, 1)
                .WithFlag(3, FieldMode.Read | FieldMode.WriteOneToClear, name: "PROGERR")
                .WithFlag(4, FieldMode.Read | FieldMode.WriteOneToClear, name: "WRPERR")
                .WithFlag(5, FieldMode.Read | FieldMode.WriteOneToClear, name: "PGAERR")
                .WithFlag(6, FieldMode.Read | FieldMode.WriteOneToClear, name: "SIZERR")
                .WithFlag(7, FieldMode.Read | FieldMode.WriteOneToClear, name: "PGSERR")
                .WithFlag(8, FieldMode.Read | FieldMode.WriteOneToClear, name: "MISERR")
                .WithFlag(9, FieldMode.Read | FieldMode.WriteOneToClear, name: "FASTERR")
                .WithReservedBits(10, 4)
                .WithFlag(14, FieldMode.Read | FieldMode.WriteOneToClear, name: "RDERR")
                .WithFlag(15, FieldMode.Read | FieldMode.WriteOneToClear, name: "OPTVERR")
                .WithFlag(16, FieldMode.Read, name: "BSY")
                .WithReservedBits(17, 15);

            // CR - Control Register (0x14)
            Registers.CR.Define(this, 0xC0000000)
                .WithFlag(0, name: "PG")
                .WithFlag(1, name: "PER")
                .WithFlag(2, name: "MER1")
                .WithValueField(3, 7, name: "PNB")
                .WithReservedBits(10, 5)
                .WithFlag(15, name: "MER2")
                .WithFlag(16, name: "START")
                .WithFlag(17, name: "OPTSTRT")
                .WithFlag(18, name: "FSTPG")
                .WithReservedBits(19, 5)
                .WithFlag(24, name: "EOPIE")
                .WithFlag(25, name: "ERRIE")
                .WithFlag(26, name: "RDERRIE")
                .WithFlag(27, name: "OBL_LAUNCH")
                .WithFlag(28, name: "SEC_PROT")
                .WithReservedBits(29, 1)
                .WithFlag(30, name: "OPTLOCK")
                .WithFlag(31, out locked, name: "LOCK");

            // ECCR - ECC Register (0x18)
            Registers.ECCR.Define(this, 0x00000000);
        }

        public long Size => 0x400;

        private enum Registers
        {
            ACR = 0x00,
            PDKEYR = 0x04,
            KEYR = 0x08,
            OPTKEYR = 0x0C,
            SR = 0x10,
            CR = 0x14,
            ECCR = 0x18,
        }

        // Register storage
        private IValueRegisterField acr;
        private IValueRegisterField keyr;
        private IValueRegisterField optkeyr;
        private IValueRegisterField sr;
        private IValueRegisterField cr;

        // ACR fields
        private IValueRegisterField latency;
        private IFlagRegisterField prften;
        private IFlagRegisterField icen;
        private IFlagRegisterField dcen;

        // CR fields
        private IFlagRegisterField locked;

        private int unlockStep = 0;
    }
}
