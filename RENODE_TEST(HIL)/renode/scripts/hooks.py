# Renode Python Hooks for STM32 SpaceBalloon
import System

def install_ds18b20_hook(machine):
    sysbus = machine["sysbus"]
    cpu = machine["sysbus.cpu"]
    
    try:
        # Resolve symbol address
        # Note: Renode might return 0 if symbol not found or logic is different
        # We assume the ELF is loaded
        symbol = "DS18B20_ReadTemp_x100"
        address = sysbus.GetSymbolAddress(symbol)
        
        if address:
            print(f"Installing DS18B20 Hook at '{symbol}' (0x{address:X})")
            
            def read_temp_hook(cpu, _):
                # Function signature: int16_t DS18B20_ReadTemp_x100(uint8_t sensor_idx)
                # Argument is in R0, Return value goes to R0
                
                # Mock Temperature: 25.00 C -> 2500
                cpu.SetRegisterUnsafe(0, 2500)
                
                # Force return: PC = LR
                # LR (Link Register) is valid because the hook is at entry (or instruction execution)
                lr = cpu.GetRegisterUnsafe(14)
                
                # Ensure thumb bit is handled if needed (usually PC takes absolute value)
                cpu.PC = lr 
                
                # print("  [Hook] DS18B20 Read -> 25.00 C")

            # Add hook at the function entry
            cpu.AddHook(address, read_temp_hook)
        else:
            print(f"Warning: Symbol '{symbol}' not found. DS18B20 hook not installed.")
            
    except Exception as e:
        print(f"Error installing DS18B20 hook: {e}")
