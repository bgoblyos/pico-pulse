import pyvisa
import time

rm = pyvisa.ResourceManager()

# Define port for the pico-pulse
port = "/dev/ttyACM0"

# Open resource. The baud rate may not be necessary when using the USB
# connection directly instead of bridging UART with a second Pico
dev = rm.open_resource(f"ASRL{port}::INSTR", baud_rate=115200)

def wrap_query(q):
    print(f"Query: {repr(q)}")
    resp = dev.query(q)
    print(f"Response: {repr(resp)}")
    return resp

wrap_query("IDN?")
wrap_query("CLK?")
wrap_query("BUFFER?")
wrap_query("MAXT?")

for i in range(65):
    wrap_query(f"CPULSE 0 0 {(1 << i) - 2},31,1,0")
