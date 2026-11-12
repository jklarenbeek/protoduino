#!/usr/bin/env python3
"""shell_session.py - talk to a protoduino shell that runs in protosim.

Starts protosim with the firmware, waits for the prompt on its PTY, sends one
command per prompt and prints the conversation. A command of the form
"wait:<seconds>" lets simulated time pass and collects what arrives.
With "raw" as the first command there is no shell at the other end: the
commands are sent as they are and nothing is waited for but "wait:".

usage: shell_session.py <protosim> <firmware.elf> <mcu> <command> [<command> ...]
"""
import os, select, subprocess, sys, time, tty


def main():
    sim, elf, mcu = sys.argv[1:4]
    commands = sys.argv[4:]
    log = open(os.devnull, "w")
    proc = subprocess.Popen([sim, elf, "-m", mcu], stdout=log, stderr=subprocess.STDOUT)
    try:
        for _ in range(100):
            if os.path.exists("/tmp/simavr-uart0"):
                break
            time.sleep(0.05)
        time.sleep(0.3)
        fd = os.open(os.path.realpath("/tmp/simavr-uart0"), os.O_RDWR | os.O_NOCTTY)
        tty.setraw(fd)

        def read_until(marker, timeout):
            seen = b""
            end = time.time() + timeout
            while time.time() < end:
                if select.select([fd], [], [], 0.05)[0]:
                    seen += os.read(fd, 256)
                    if marker and seen.endswith(marker):
                        break
            return seen

        raw = bool(commands) and commands[0] == "raw"   # no shell at the other end: just a line
        if raw:
            commands = commands[1:]
        else:
            os.write(fd, b"\r")                   # an empty line: the shell answers with a prompt
            sys.stdout.write(read_until(b"> ", 20).decode(errors="replace"))
        for command in commands:
            if command.startswith("wait:"):
                sys.stdout.write(read_until(None, float(command[5:])).decode(errors="replace"))
                if not raw:
                    os.write(fd, b"\r")
                    sys.stdout.write(read_until(b"> ", 20).decode(errors="replace"))
                continue
            if raw:
                os.write(fd, command.encode())
                continue
            os.write(fd, command.encode() + b"\r")
            sys.stdout.write(read_until(b"> ", 20).decode(errors="replace"))
        print()
    finally:
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()


if __name__ == "__main__":
    main()
