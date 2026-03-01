
import time
import threading

def disconnect_imu(machine, bus, name):
    time.sleep(5)
    print(f"[Renode] 🚨 FAULT INJECTION: Disconnecting {name} from {bus}...")
    try:
        peripheral = getattr(machine.SystemBus, bus).GetByName(name)
        machine.SystemBus.Unregister(peripheral)
        print(f"[Renode] 💀 FAULT INJECTED: {name} is gone!")
    except Exception as e:
        print(f"[Renode] ❌ FAULT INJECTION FAILED: {str(e)}")

# Start injection
t = threading.Thread(target=disconnect_imu, args=(self.Machine, "i2c1", "lsm6dsv16x"))
t.start()
