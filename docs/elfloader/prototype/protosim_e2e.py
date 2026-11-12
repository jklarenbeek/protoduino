#!/usr/bin/env python3
"""protosim_e2e.py - the bootloader inside protosim, end to end.

protosim runs the bootloader on an empty ATmega328P with its serial port on a PTY.
This script is the other end of the line: it sends files with XMODEM-CRC, as a
terminal program would, and it pulls the reset pin (SIGUSR1) or cycles the power
(SIGUSR2) of the simulated part through protosim.

  P1  empty part, power-on             the bootloader probes; a file is sent and starts
  P2  power cycle                      the image starts at once, no probe
  P3  reset pin, a second file sent    the bootloader probes, takes the file, starts it
  P4  reset pin, nobody sends          three probes, then the image starts again
  P5  power cycle                      the new image starts at once

usage: protosim_e2e.py <protosim> <bootloader.elf> <first.elf> <second.elf>
"""
import os, select, signal, subprocess, sys, termios, time, tty

SOH, EOT, ACK, NAK, CAN = 0x01, 0x04, 0x06, 0x15, 0x18


def crc16(data):
    crc = 0
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


class Line:
    """The host end of the simulated serial line."""

    def __init__(self, path):
        self.fd = os.open(path, os.O_RDWR | os.O_NOCTTY)
        tty.setraw(self.fd)
        attrs = termios.tcgetattr(self.fd)
        attrs[3] &= ~termios.ECHO
        termios.tcsetattr(self.fd, termios.TCSANOW, attrs)
        self.seen = bytearray()   # everything the part has sent since the last mark()

    def mark(self):
        self.seen = bytearray()

    def read(self, timeout):
        r, _, _ = select.select([self.fd], [], [], timeout)
        if not r:
            return b""
        data = os.read(self.fd, 256)
        self.seen += data
        return data

    def wait_for(self, what, timeout):
        """Read until `what` (bytes) has arrived; returns what was seen, or None."""
        end = time.time() + timeout
        while time.time() < end:
            if what in self.seen:
                return bytes(self.seen)
            self.read(0.05)
        return None

    def byte(self, timeout):
        end = time.time() + timeout
        while time.time() < end:
            data = os.read(self.fd, 1) if select.select([self.fd], [], [], 0.05)[0] else b""
            if data:
                self.seen += data
                return data[0]
        return None

    def send_file(self, path, timeout=20):
        """XMODEM-CRC, after the receiver's 'C' has been seen. Returns blocks sent."""
        data = open(path, "rb").read()
        blocks = [data[i:i + 128].ljust(128, b"\x1a") for i in range(0, len(data), 128)]
        for n, block in enumerate(blocks, start=1):
            frame = bytes([SOH, n & 0xFF, ~n & 0xFF]) + block + crc16(block).to_bytes(2, "big")
            for _ in range(10):
                os.write(self.fd, frame)
                reply = self.byte(timeout)
                while reply == ord("C"):          # a probe that was already on its way
                    reply = self.byte(timeout)
                if reply == ACK:
                    break
                if reply == CAN:
                    raise RuntimeError("the part refused the file at block %d" % n)
            else:
                raise RuntimeError("block %d was never acknowledged" % n)
        os.write(self.fd, bytes([EOT]))
        if self.byte(timeout) != ACK:
            raise RuntimeError("the end of the file was not acknowledged")
        return len(blocks)


def main():
    sim, boot, first, second = sys.argv[1:5]
    log = open("out/protosim_e2e.log", "w")
    proc = subprocess.Popen([sim, boot, "-m", "atmega328p", "--reset-vector", "0x7800"],
                            stdout=log, stderr=subprocess.STDOUT)
    failures = 0

    def check(label, ok, detail=""):
        nonlocal failures
        print("  [%s] %s%s" % ("PASS" if ok else "FAIL", label, ("  " + detail) if detail else ""))
        failures += 0 if ok else 1

    def text(seen):
        return "".join(chr(b) if 32 <= b < 127 else "." for b in (seen or b""))

    try:
        for _ in range(100):                       # the PTY appears when protosim has started
            if os.path.exists("/tmp/simavr-uart0"):
                break
            time.sleep(0.05)
        time.sleep(0.2)
        line = Line(os.path.realpath("/tmp/simavr-uart0"))

        print("P1 empty part, power-on: the bootloader probes; a file is sent")
        seen = line.wait_for(b"C", 60)
        check("the bootloader probes", seen is not None)
        line.mark()
        blocks = line.send_file(first)
        seen = line.wait_for(b"]", 30)
        check("%d blocks acknowledged, the image starts and reads cause 01 (power-on)" % blocks,
              seen is not None and b"[S 01 00]" in seen, text(seen))

        print("P2 power cycle (SIGUSR2): the image starts at once")
        line.mark(); proc.send_signal(signal.SIGUSR2)
        seen = line.wait_for(b"]", 30)
        check("no probe before the image, cause 01", seen is not None and b"C" not in seen and b"[S 01 00]" in seen, text(seen))

        print("P3 reset pin (SIGUSR1): the bootloader waits; a second file is sent")
        line.mark(); proc.send_signal(signal.SIGUSR1)
        seen = line.wait_for(b"C", 60)
        check("the bootloader probes", seen is not None and b"[" not in seen, text(seen))
        line.mark()
        blocks = line.send_file(second)
        seen = line.wait_for(b"]", 30)
        check("%d blocks acknowledged, the new image starts and reads cause 02 (reset pin)" % blocks,
              seen is not None and b"[T 02 00]" in seen, text(seen))

        print("P4 reset pin, nobody sends: three probes, then the image")
        line.mark(); proc.send_signal(signal.SIGUSR1)
        seen = line.wait_for(b"]", 120)
        check("CCC and the image, cause 02", seen is not None and seen.count(b"C") == 3 and b"[T 02 00]" in seen, text(seen))

        print("P5 power cycle: the new image starts at once")
        line.mark(); proc.send_signal(signal.SIGUSR2)
        seen = line.wait_for(b"]", 30)
        check("no probe before the image, cause 01", seen is not None and b"C" not in seen and b"[T 01 00]" in seen, text(seen))
    except Exception as e:                           # a refusal or a timeout
        check("ran to the end", False, str(e))
    finally:
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()
        log.close()

    print("protosim log:")
    for l in open("out/protosim_e2e.log"):
        if l.startswith("[RST]"):
            print("  " + l.rstrip())
    print("%s" % ("all passed" if not failures else "%d failed" % failures))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
