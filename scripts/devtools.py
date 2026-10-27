#!/usr/bin/env python3
"""Drive and screenshot a Kiddo board running the esp32dev-devtools firmware.

Opening the serial port resets the board on some hosts, so keep one server
running with the port open and send it steps:

    ~/.platformio/penv/bin/python scripts/devtools.py serve /dev/cu.usbserial-10 &
    ~/.platformio/penv/bin/python scripts/devtools.py shot home.png  tap 120 200  wait 1.5  shot library.png

Steps also run directly against a port (resetting the board first):

    ~/.platformio/penv/bin/python scripts/devtools.py /dev/cu.usbserial-10 wait 6 shot home.png

Steps:
    shot FILE.png          save a screenshot
    tap X Y                tap a point (screen is 240x320, origin top left)
    drag X1 Y1 X2 Y2       press, move and release (to scroll)
    wait SECONDS           let the UI settle
    heap                   print free heap and the largest free block
    clearimg               delete cached images (stories and progress stay)
    catalog URL            set the story catalog URL (as Settings does)
    rm PATH                delete a file on the board, e.g. /story_x.json

Board log lines seen while running are printed with a "| " prefix.
Needs pyserial (bundled with PlatformIO's Python).
"""

import base64
import contextlib
import io
import os
import socket
import struct
import sys
import time
import zlib

import serial

SHOT_TIMEOUT_S = 60
SERVER_ADDR = ("127.0.0.1", 7790)


def open_port(path):
    port = serial.Serial()
    port.port, port.baudrate, port.timeout = path, 115200, 0.2
    port.dtr = False
    port.rts = False
    port.open()
    port.reset_input_buffer()
    return port


def read_line(port, deadline):
    buf = bytearray()
    while time.time() < deadline:
        ch = port.read(1)
        if not ch:
            continue
        if ch == b"\n":
            return buf.decode("utf-8", "replace").rstrip("\r")
        buf += ch
    raise TimeoutError("board stopped answering")


def write_png(path, width, height, rgb):
    def chunk(kind, data):
        body = kind + data
        return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF)

    rows = b"".join(b"\x00" + rgb[y * width * 3:(y + 1) * width * 3] for y in range(height))
    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(rows, 9))
    png += chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(png)


def screenshot(port, path):
    port.write(b"shot\n")
    deadline = time.time() + SHOT_TIMEOUT_S
    while True:
        line = read_line(port, deadline)
        if line.startswith("@S BEGIN"):
            width, height = map(int, line.split()[2:4])
            break
        print("| " + line)

    rgb = bytearray(width * height * 3)
    area, data = None, bytearray()

    def blit():
        x1, y1, x2, y2 = area
        w = x2 - x1 + 1
        for i in range(len(data) // 2):
            (px,) = struct.unpack_from("<H", data, i * 2)
            x, y = x1 + i % w, y1 + i // w
            if 0 <= x < width and 0 <= y < height:
                o = (y * width + x) * 3
                r, g, b = (px >> 11) & 31, (px >> 5) & 63, px & 31
                rgb[o:o + 3] = bytes(((r * 255) // 31, (g * 255) // 63, (b * 255) // 31))

    while True:
        line = read_line(port, deadline)
        if line.startswith("@S A "):
            if area:
                blit()
            area, data = tuple(map(int, line.split()[2:6])), bytearray()
        elif line.startswith("@S D "):
            data += base64.b64decode(line[5:])
        elif line == "@S END":
            if area:
                blit()
            break
        else:
            print("| " + line)

    write_png(path, width, height, bytes(rgb))
    print(f"saved {path}")


def drain(port, seconds):
    end = time.time() + seconds
    buf = b""
    while time.time() < end:
        buf += port.read(4096)
    for line in buf.decode("utf-8", "replace").splitlines():
        if line.strip() and not line.startswith("@OK"):
            print("| " + line)


def run_steps(port, steps):
    i = 0
    while i < len(steps):
        cmd = steps[i]
        if cmd == "shot":
            screenshot(port, steps[i + 1])
            i += 2
        elif cmd == "tap":
            port.write(f"tap {steps[i + 1]} {steps[i + 2]}\n".encode())
            drain(port, 0.4)
            i += 3
        elif cmd == "drag":
            port.write(("drag " + " ".join(steps[i + 1:i + 5]) + "\n").encode())
            drain(port, 0.8)
            i += 5
        elif cmd in ("catalog", "rm"):
            port.write(f"{cmd} {steps[i + 1]}\n".encode())
            drain(port, 0.6)
            i += 2
        elif cmd in ("heap", "clearimg"):
            port.write(f"{cmd}\n".encode())
            drain(port, 0.4)
            i += 1
        elif cmd == "wait":
            drain(port, float(steps[i + 1]))
            i += 2
        else:
            print(f"unknown step: {cmd}")
            return 1
    return 0


def serve(path):
    port = open_port(path)
    server = socket.create_server(SERVER_ADDR)
    print(f"serving {path} on {SERVER_ADDR[0]}:{SERVER_ADDR[1]}", flush=True)
    while True:
        conn, _ = server.accept()
        with conn:
            request = b""
            while not request.endswith(b"\n"):
                part = conn.recv(4096)
                if not part:
                    break
                request += part
            cwd, *steps = request.decode().rstrip("\n").split("\0")
            out = io.StringIO()
            with contextlib.redirect_stdout(out):
                try:
                    os.chdir(cwd)
                    code = run_steps(port, steps)
                except Exception as e:  # report and keep serving
                    print(f"error: {e}")
                    code = 1
            conn.sendall(out.getvalue().encode() + f"\0{code}".encode())


def send(steps):
    with socket.create_connection(SERVER_ADDR) as conn:
        conn.sendall(("\0".join([os.getcwd(), *steps]) + "\n").encode())
        reply = b""
        while part := conn.recv(65536):
            reply += part
    text, _, code = reply.decode("utf-8", "replace").rpartition("\0")
    sys.stdout.write(text)
    return int(code or 1)


def main(argv):
    if not argv:
        print(__doc__)
        return 1
    if argv[0] == "serve":
        serve(argv[1])
        return 0
    if argv[0].startswith("/dev/"):
        return run_steps(open_port(argv[0]), argv[1:])
    return send(argv)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
