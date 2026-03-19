import socket
import time
import subprocess

print("Starting protosim...")
proc = subprocess.Popen(["C:\\Users\\bitSOCIAL\\GitHub\\protosim\\bin\\protosim.exe", "build\\21-process-mcurses.ino.elf", "-m", "atmega328p", "-f", "16000000", "--max-steps", "50000000"])

print("Connecting to TCP 4000...")
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
connected = False
for _ in range(50):
    try:
        s.connect(("127.0.0.1", 4000))
        connected = True
        break
    except Exception:
        time.sleep(0.05)

if not connected:
    print("Could not connect to protosim")
    proc.terminate()
    exit(1)

s.settimeout(3.0)
data = b""
while True:
    try:
       chunk = s.recv(4096)
       if not chunk: break
       data += chunk
    except Exception:
       break

output = data.decode("utf-8", "ignore")
print("--- UART OUTPUT ---")
print(output)
print("-------------------")

if "mcurses" in output or "\x1b" in output or "Stats" in output:
    print("VERIFICATION SUCCESSFUL: VT100 symbols and title found in the output.")
else:
    print("VERIFICATION FAILED: Did not find expected VT100 or text.")

proc.terminate()
