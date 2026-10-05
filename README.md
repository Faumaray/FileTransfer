# FTU

File transfer over Unix-domain sockets for Linux. One executable runs either as a long-lived
receiving server or as a client that sends one file. Transfers are verified end to end with
CRC-64, resume after a crash or disconnect on either side.

## Build

Requires Linux, CMake 3.13+ and a C++20 compiler: GCC 10+(tested).

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

`-DFTU_WARNINGS_AS_ERRORS=ON` turns warnings into errors; `-DFTU_SANITIZERS=ON` builds with
AddressSanitizer and UndefinedBehaviorSanitizer.

## Usage

```sh
build/bin/FTU -s            # server: receive files until Ctrl+C or SIGTERM
build/bin/FTU -c FILE       # client: send FILE, then exit
```

Exit codes: `0` success, `1` failure, `2` invalid arguments.

The server stores each received file next to its own executable as
`YYYYMMDD_HHMMSS_NNNNNNNNN.hex` (UTC timestamp, mode 0600).

## Configuration

| Variable | Default | Meaning |
|---|---|---|
| `FTU_SOCKET` | `/tmp/FTU-<uid>/transfer.sock` | Absolute socket path |
| `FTU_WORKERS` | CPU count clamped to 2–8 | Server worker threads, 1–64 |
| `FTU_MAX_CLIENTS` | 128 | Concurrent connections, 1–4096 |
| `FTU_MAX_FILE_SIZE` | 0(noLimit) | Largest accepted file, in bytes|
| `FTU_TIMEOUT_MS` | 120000 | Inactivity and connect timeout, 100–86400000 |
| `FTU_RETRIES` | 8 | Client reconnects after the first attempt, 0–100 |

The socket's directory must belong to the current user and have mode 0700; the default one is
created that way. Client and server must run as the same user.

## How it works

```text
client                               server
HELLO  uuid, size, crc, name   →
                               ←     READY  offset, crc of the saved partial
                                     (or DONE at once if the file was already received)
RESET                          →     only if the partial does not match the source
                               ←     READY  0, 0
DATA   offset, up to 64 KiB    →
                               ←     ACK    new offset, running crc       (repeated)
FINISH                         →
                               ←     DONE   saved file name
```

Every frame is a 32-byte header (`FTU1`, version, type, payload size, payload CRC-64, header
CRC-64) followed by the payload; integers are big-endian. Failures are answered with an ERROR
frame carrying a code and a message.

- **Sessions.** A transfer is identified by a UUIDv8 built from the file's CRC-64 and a CRC-64 of
  its name and size, so restarting the client with the same file resumes the same session.
  For the content `123456789` named `test.bin`, the CRC-64 is `6c40df5f0b497347` and the UUID is
  `6c40df5f-0b49-8734-9ffe-53ef6bfbd152`.
- **Resume.** The server re-reads its partial file and reports its length and CRC; the client
  compares them with its own prefix and sends RESET when they differ.
- **Integrity.** Headers, payloads, every ACK and the whole file are CRC-checked, and the server
  re-reads the stored file before publishing it.
- **Durability.** The file and its directories are flushed with fsync before DONE. Publishing is
  a hard link, which is atomic and fails rather than replacing an existing name.
- **Retries.** The client reconnects after transient errors with backoff of 250 ms doubling up
  to 2 s.
- **Concurrency.** The server runs an epoll loop with a worker pool. A session UUID is held by
  one connection at a time; a concurrent duplicate is told the session is busy and retries.

The server keeps its state in a private directory beside the executable:

```text
FTU
20261005_143025_123456789.hex
.FTU-state/                  mode 0700
├── server.lock              one server per directory
└── <session uuid>/
    ├── manifest             file metadata and reserved output name
    └── data.part            bytes received so far
```

Unfinished sessions are kept until they complete; there is no expiry or quota. Stop the server
before cleaning up `.FTU-state`.
