#!/usr/bin/env python3
"""Send commands to the synth over USB serial and print the replies.

Usage:
  python3 firmware/tools/synth_cmd.py /dev/cu.usbmodemXXXX "status" "note 1 69" "wave 2 square"
  python3 firmware/tools/synth_cmd.py /dev/cu.usbmodemXXXX            # interactive: type commands, Ctrl-D to quit
"""
import os
import select
import sys
import termios
import time
import tty


def open_port(path):
    fd = os.open(path, os.O_RDWR | os.O_NOCTTY | os.O_NONBLOCK)
    tty.setraw(fd)
    attrs = termios.tcgetattr(fd)
    attrs[4] = attrs[5] = termios.B115200
    termios.tcsetattr(fd, termios.TCSANOW, attrs)
    return fd


def read_for(fd, seconds):
    out = b""
    end = time.time() + seconds
    while time.time() < end:
        r, _, _ = select.select([fd], [], [], 0.05)
        if r:
            try:
                out += os.read(fd, 4096)
            except BlockingIOError:
                pass
    return out.decode("utf-8", "replace")


def send(fd, line, wait=0.3):
    os.write(fd, (line + "\n").encode())
    print(f"> {line}")
    print(read_for(fd, wait).rstrip())


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    fd = open_port(sys.argv[1])
    try:
        read_for(fd, 0.2)  # discard pending output
        if len(sys.argv) > 2:
            for line in sys.argv[2:]:
                send(fd, line)
        else:
            while True:
                try:
                    line = input("> ")
                except EOFError:
                    break
                send(fd, line)
    finally:
        os.close(fd)


if __name__ == "__main__":
    main()
