import pyvisa
import time

rm = pyvisa.ResourceManager()

# Define port for the pico-pulse
port = "/dev/ttyACM0"

# Connection over UART bridge, Baud rate must be set 115200
# dev = rm.open_resource(f"ASRL{port}::INSTR", baud_rate=115200)

# Collection over USB port
dev = rm.open_resource(f"ASRL{port}::INSTR")

def wrap_query(q):
    print(f"Query: {repr(q)}")
    resp = dev.query(q)
    print(f"Response: {repr(resp)}")
    return resp

wrap_query(f"PULSE 0 {1 << 32 - 1} 100000000,4,100000000,4")

# Fastest possible oscillation with external (DMA) looping
# wrap_query(f"PULSE 1 {1 << 32 - 1} 4,31,4,0")
