// STM32G4 RCC (Reset and Clock Control) Peripheral for Renode
// Implements essential clock management functionality

using System;
using System.Collections.Generic;
using Antmicro.Renode.Core;
using Antmicro.Renode.Logging;
using Antmicro.Renode.Peripherals.Bus;
using Antmicro.Renode.Core.Structure.Registers;

namespace Antmicro.Renode.Peripherals.Miscellaneous
{
    public class STM32G4_RCC : BasicDoubleWordPeripheral, IKnownSize
    {
        public STM32G4_RCC(IMachine machine) : base(machine)
        {
            DefineRegisters();
            Reset();
        }

        public override void Reset()
        {
            base.Reset();

            // Default reset values from STM32G4 reference manual
            cr.Value = 0x00000063;      // HSI ON and ready
            cfgr.Value = 0x00000000;
            pllcfgr.Value = 0x00001000;
            cier.Value = 0x00000000;
            ahb1enr.Value = 0x00000100; // Flash interface enabled
            ahb2enr.Value = 0x00000000;
            apb1enr1.Value = 0x00000000;
            apb1enr2.Value = 0x00000000;
            apb2enr.Value = 0x00000000;

            this.Log(LogLevel.Info, "RCC Reset - HSI enabled, default clocks");
        }

        private void DefineRegisters()
        {
            // CR - Clock Control Register (0x00)
            Registers.CR.Define(this, 0x00000063)
                .WithFlag(0, out hsiOn, name: "HSION")
                .WithFlag(1, FieldMode.Read, name: "HSIKERON")
                .WithFlag(2, FieldMode.Read, valueProviderCallback: _ => hsiOn.Value, name: "HSIRDY")
                .WithReservedBits(3, 5)
                .WithFlag(8, out hseOn, name: "HSEON")
                .WithFlag(9, FieldMode.Read, valueProviderCallback: _ => hseOn.Value, name: "HSERDY")
                .WithFlag(10, name: "HSEBYP")
                .WithFlag(11, name: "CSSON")
                .WithReservedBits(12, 4)
                .WithFlag(16, out pllOn, name: "PLLON")
                .WithFlag(17, FieldMode.Read, valueProviderCallback: _ => pllOn.Value, name: "PLLRDY")
                .WithReservedBits(18, 6)
                .WithFlag(24, out pllsai1On, name: "PLLSAI1ON")
                .WithFlag(25, FieldMode.Read, valueProviderCallback: _ => pllsai1On.Value, name: "PLLSAI1RDY")
                .WithReservedBits(26, 6);

            // CFGR - Clock Configuration Register (0x04)
            Registers.CFGR.Define(this, 0x00000000)
                .WithValueField(0, 2, out sw, name: "SW")
                .WithValueField(2, 2, FieldMode.Read, valueProviderCallback: _ => sw.Value, name: "SWS")
                .WithValueField(4, 4, out hpre, name: "HPRE")
                .WithValueField(8, 3, out ppre1, name: "PPRE1")
                .WithValueField(11, 3, out ppre2, name: "PPRE2")
                .WithReservedBits(14, 1)
                .WithFlag(15, name: "STOPWUCK")
                .WithReservedBits(16, 8)
                .WithValueField(24, 3, name: "MCOSEL")
                .WithValueField(27, 1, name: "MCOPRE")
                .WithValueField(28, 4, name: "MCO2PRE");

            // PLLCFGR - PLL Configuration Register (0x0C)
            Registers.PLLCFGR.Define(this, 0x00001000)
                .WithValueField(0, 2, name: "PLLSRC")
                .WithReservedBits(2, 2)
                .WithValueField(4, 7, name: "PLLM")
                .WithReservedBits(11, 1)
                .WithValueField(12, 7, name: "PLLN")
                .WithReservedBits(19, 1)
                .WithFlag(20, name: "PLLPEN")
                .WithValueField(21, 1, name: "PLLP")
                .WithReservedBits(22, 2)
                .WithFlag(24, name: "PLLQEN")
                .WithValueField(25, 2, name: "PLLQ")
                .WithReservedBits(27, 1)
                .WithFlag(28, name: "PLLREN")
                .WithValueField(29, 2, name: "PLLR")
                .WithFlag(31, name: "PLLPDIV");

            // CIER - Clock Interrupt Enable Register (0x18)
            Registers.CIER.Define(this, 0x00000000);

            // AHB1ENR - AHB1 Peripheral Clock Enable (0x48)
            Registers.AHB1ENR.Define(this, 0x00000100)
                .WithFlag(0, out dma1en, name: "DMA1EN")
                .WithFlag(1, out dma2en, name: "DMA2EN")
                .WithFlag(2, out dmamuxen, name: "DMAMUXEN")
                .WithReservedBits(3, 5)
                .WithFlag(8, FieldMode.Read | FieldMode.Write, name: "FLASHEN")
                .WithReservedBits(9, 3)
                .WithFlag(12, name: "CRCEN")
                .WithReservedBits(13, 19)
                .WithWriteCallback((_, __) => this.Log(LogLevel.Debug, "AHB1ENR written: DMA1={0}, DMA2={1}", dma1en.Value, dma2en.Value));

            // AHB2ENR - AHB2 Peripheral Clock Enable (0x4C)
            Registers.AHB2ENR.Define(this, 0x00000000)
                .WithFlag(0, out gpioaen, name: "GPIOAEN")
                .WithFlag(1, out gpioben, name: "GPIOBEN")
                .WithFlag(2, out gpiocen, name: "GPIOCEN")
                .WithFlag(3, out gpioden, name: "GPIODEN")
                .WithFlag(4, out gpioeen, name: "GPIOEEN")
                .WithFlag(5, out gpiofen, name: "GPIOFEN")
                .WithReservedBits(6, 7)
                .WithFlag(13, out adc12en, name: "ADC12EN")
                .WithReservedBits(14, 18)
                .WithWriteCallback((_, __) => this.Log(LogLevel.Debug, "AHB2ENR written: GPIOA={0}, ADC={1}", gpioaen.Value, adc12en.Value));

            // APB1ENR1 - APB1 Peripheral Clock Enable 1 (0x58)
            Registers.APB1ENR1.Define(this, 0x00000000)
                .WithFlag(0, out tim2en, name: "TIM2EN")
                .WithFlag(1, out tim3en, name: "TIM3EN")
                .WithFlag(2, out tim4en, name: "TIM4EN")
                .WithFlag(3, out tim5en, name: "TIM5EN")
                .WithFlag(4, out tim6en, name: "TIM6EN")
                .WithFlag(5, out tim7en, name: "TIM7EN")
                .WithReservedBits(6, 5)
                .WithFlag(11, name: "CRSREN")
                .WithReservedBits(12, 2)
                .WithFlag(14, name: "RTCAPBEN")
                .WithFlag(15, name: "WWDGEN")
                .WithReservedBits(16, 5)
                .WithFlag(21, out i2c1en, name: "I2C1EN")
                .WithFlag(22, out i2c2en, name: "I2C2EN")
                .WithFlag(23, out i2c3en, name: "I2C3EN")
                .WithReservedBits(24, 1)
                .WithFlag(25, name: "I2C4EN")
                .WithReservedBits(26, 6)
                .WithWriteCallback((_, __) => this.Log(LogLevel.Debug, "APB1ENR1 written: TIM2={0}, I2C1={1}, I2C3={2}", tim2en.Value, i2c1en.Value, i2c3en.Value));

            // APB1ENR2 - APB1 Peripheral Clock Enable 2 (0x5C)
            Registers.APB1ENR2.Define(this, 0x00000000)
                .WithFlag(0, name: "LPUART1EN")
                .WithFlag(1, name: "I2C4EN")
                .WithReservedBits(2, 30);

            // APB2ENR - APB2 Peripheral Clock Enable (0x60)
            Registers.APB2ENR.Define(this, 0x00000000)
                .WithFlag(0, name: "SYSCFGEN")
                .WithReservedBits(1, 10)
                .WithFlag(11, out tim1en, name: "TIM1EN")
                .WithFlag(12, name: "SPI1EN")
                .WithFlag(13, out tim8en, name: "TIM8EN")
                .WithFlag(14, out usart1en, name: "USART1EN")
                .WithFlag(15, name: "SPI4EN")
                .WithFlag(16, out tim15en, name: "TIM15EN")
                .WithFlag(17, out tim16en, name: "TIM16EN")
                .WithFlag(18, out tim17en, name: "TIM17EN")
                .WithReservedBits(19, 3)
                .WithFlag(22, name: "TIM20EN")
                .WithReservedBits(23, 3)
                .WithFlag(26, name: "SAI1EN")
                .WithReservedBits(27, 5)
                .WithWriteCallback((_, __) => this.Log(LogLevel.Debug, "APB2ENR written: TIM1={0}, USART1={1}", tim1en.Value, usart1en.Value));
        }

        public long Size => 0x400;

        private enum Registers
        {
            CR = 0x00,
            CFGR = 0x04,
            PLLCFGR = 0x0C,
            CIER = 0x18,
            AHB1ENR = 0x48,
            AHB2ENR = 0x4C,
            APB1ENR1 = 0x58,
            APB1ENR2 = 0x5C,
            APB2ENR = 0x60,
        }

        // Register storage
        private IValueRegisterField cr;
        private IValueRegisterField cfgr;
        private IValueRegisterField pllcfgr;
        private IValueRegisterField cier;
        private IValueRegisterField ahb1enr;
        private IValueRegisterField ahb2enr;
        private IValueRegisterField apb1enr1;
        private IValueRegisterField apb1enr2;
        private IValueRegisterField apb2enr;

        // Flags
        private IFlagRegisterField hsiOn;
        private IFlagRegisterField hseOn;
        private IFlagRegisterField pllOn;
        private IFlagRegisterField pllsai1On;

        private IValueRegisterField sw;
        private IValueRegisterField hpre;
        private IValueRegisterField ppre1;
        private IValueRegisterField ppre2;

        // Peripheral enables
        private IFlagRegisterField dma1en, dma2en, dmamuxen;
        private IFlagRegisterField gpioaen, gpioben, gpiocen, gpioden, gpioeen, gpiofen;
        private IFlagRegisterField adc12en;
        private IFlagRegisterField tim1en, tim2en, tim3en, tim4en, tim5en, tim6en, tim7en, tim8en;
        private IFlagRegisterField tim15en, tim16en, tim17en;
        private IFlagRegisterField i2c1en, i2c2en, i2c3en;
        private IFlagRegisterField usart1en;
    }
}
