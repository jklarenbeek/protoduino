import socket
import time
import subprocess
import sys

ELF = r"build\22-process-serial\22-process-serial.ino.elf"
PROTOSIM = r"protosim"

print("Starting protosim for ATmega2560...")
proc = subprocess.Popen([
    PROTOSIM, ELF,
    "-m", "atmega2560",
    "-f", "16000000",
    "--max-steps", "10000000"
])

print("Connecting to TCP 4000...")
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
connected = False
for _ in range(80):
    try:
        s.connect(("127.0.0.1", 4000))
        connected = True
        break
    except Exception:
        time.sleep(0.1)

if not connected:
    print("FAILED: Could not connect to protosim TCP bridge")
    proc.terminate()
    sys.exit(1)

s.settimeout(3.0)

# Give firmware time to boot and send the banner
time.sleep(0.5)

# Read banner first
banner = b""
try:
    while True:
        chunk = s.recv(1024)
        if not chunk:
            break
        banner += chunk
except Exception:
    pass

print("--- BANNER ---")
print(banner.decode("utf-8", "ignore"))
print("--------------")

# Send lowercase input
s.sendall(b"hello\r\n")
time.sleep(0.5)

# Read the echo back
reply = b""
try:
    while True:
        chunk = s.recv(1024)
        if not chunk:
            break
        reply += chunk
except Exception:
    pass

print("--- REPLY ---")
print(repr(reply.decode("utf-8", "ignore")))
print("-------------")

ok = True
if b"protoduino" not in banner and b"UART" not in banner and b"> " not in banner:
    # Banner might be partially received; check combined
    combined = banner + reply
    if b"protoduino" not in combined and b"> " not in combined:
        print("WARNING: banner not detected (may still be OK if simulation is slow)")

if b"HELLO" in reply or b"HELLO" in banner + reply:
    print("PASS: Echo-upper working – received HELLO from hello input")
else:
    print("FAIL: Did not receive expected uppercase echo 'HELLO'")
    ok = False

proc.terminate()
sys.exit(0 if ok else 1)
