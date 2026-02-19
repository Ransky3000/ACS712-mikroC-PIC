```

```

# Smart Outlet Firmware — Testing Guide

**Device:** PIC16F88 | **Firmware:** v5.1.0+
**Supported Devices:** `0xFE` (PIC 1) · `0xFD` (PIC 2) · `0xFC` (PIC 3)

---

## 1. Simulation Testing (Proteus)

### Prerequisites

- Uncomment the simulation mode block in `__main__.c` (lines 140-143):

```c
if (byte >= '1' && byte <= '6') {
    Process_Debug_Shortcut(byte);
    continue;
}
```

- Recompile and load the `.hex` into Proteus

### Keyboard Shortcuts

| Key   | Action                | Simulated Packet            |
| :---- | :-------------------- | :-------------------------- |
| `1` | Relay A**ON**   | `AA FE 00 02 00 01 FD BB` |
| `2` | Relay A**OFF**  | `AA FE 00 03 00 01 FC BB` |
| `3` | Relay B**ON**   | `AA FE 00 02 00 02 FE BB` |
| `4` | Relay B**OFF**  | `AA FE 00 03 00 02 FF BB` |
| `5` | Read Sensors          | `AA FE 00 04 00 00 FA BB` |
| `6` | Set Threshold 10000mA | `AA FE 00 07 27 10 C4 BB` |

### SoftUART Debug Output (Virtual Terminal)

| Output                | Meaning                                       |
| :-------------------- | :-------------------------------------------- |
| `FW:v3.1`           | Firmware booted                               |
| `Calib...`          | Sensor calibration in progress                |
| `Rdy`               | System ready                                  |
| `R1+` / `R1-`     | Relay A turned ON / OFF                       |
| `R2+` / `R2-`     | Relay B turned ON / OFF                       |
| `A:Rty` / `B:Rty` | Socket A/B auto-retry after overload cooldown |
| `CRC!`              | Bad checksum on received packet               |
| `Cfg:10000`         | Threshold set to 10000mA                      |

### Relay Pin Debug (NC Wiring)

| Pin State            | Meaning                                               |
| :------------------- | :---------------------------------------------------- |
| `0`                | Relay energized → NC opens → Socket**OFF**    |
| `1`                | Relay de-energized → NC closed → Socket**ON** |
| Boot default:`0/0` | Both sockets OFF                                      |

---

## 2. Hardware Testing (ESP32 + HC-12)

### Prerequisites

- Flash `Central_control_command_test.ino` to ESP32
- HC-12 modules configured to same channel/baud
- PIC16F88 flashed with simulation mode **commented out**

### Serial Commands (ESP32 Serial Monitor @ 115200)

Send these hex packets from the ESP32 to control **PIC 1** (`DEVICE_ID: 0xFE`):

| # | Action               | Hex Packet                  | Breakdown                    |
| :- | :------------------- | :-------------------------- | :--------------------------- |
| 1 | Relay A**ON**  | `AA FE 00 02 00 01 FD BB` | CMD=02, Data=0001 (Socket A) |
| 2 | Relay A**OFF** | `AA FE 00 03 00 01 FC BB` | CMD=03, Data=0001 (Socket A) |
| 3 | Relay B**ON**  | `AA FE 00 02 00 02 FE BB` | CMD=02, Data=0002 (Socket B) |
| 4 | Relay B**OFF** | `AA FE 00 03 00 02 FF BB` | CMD=03, Data=0002 (Socket B) |
| 5 | Read Sensors         | `AA FE 00 04 00 00 FA BB` | CMD=04, No data              |
| 6 | Set Threshold 3233mA | `AA FE 00 07 0C A1 54 BB` | CMD=07, Data=0CA1 (3233)     |

#### PIC 2 (`DEVICE_ID: 0xFD`)

| # | Action               | Hex Packet                  | Breakdown                    |
| :- | :------------------- | :-------------------------- | :--------------------------- |
| 1 | Relay A**ON**  | `AA FD 00 02 00 01 FE BB` | CMD=02, Data=0001 (Socket A) |
| 2 | Relay A**OFF** | `AA FD 00 03 00 01 FF BB` | CMD=03, Data=0001 (Socket A) |
| 3 | Relay B**ON**  | `AA FD 00 02 00 02 FD BB` | CMD=02, Data=0002 (Socket B) |
| 4 | Relay B**OFF** | `AA FD 00 03 00 02 FC BB` | CMD=03, Data=0002 (Socket B) |
| 5 | Read Sensors         | `AA FD 00 04 00 00 F9 BB` | CMD=04, No data              |
| 6 | Set Threshold 3233mA | `AA FD 00 07 0C A1 57 BB` | CMD=07, Data=0CA1 (3233)     |

#### PIC 3 (`DEVICE_ID: 0xFC`)

| # | Action               | Hex Packet                  | Breakdown                    |
| :- | :------------------- | :-------------------------- | :--------------------------- |
| 1 | Relay A**ON**  | `AA FC 00 02 00 01 FF BB` | CMD=02, Data=0001 (Socket A) |
| 2 | Relay A**OFF** | `AA FC 00 03 00 01 FE BB` | CMD=03, Data=0001 (Socket A) |
| 3 | Relay B**ON**  | `AA FC 00 02 00 02 FC BB` | CMD=02, Data=0002 (Socket B) |
| 4 | Relay B**OFF** | `AA FC 00 03 00 02 FD BB` | CMD=03, Data=0002 (Socket B) |
| 5 | Read Sensors         | `AA FC 00 04 00 00 F8 BB` | CMD=04, No data              |
| 6 | Set Threshold 3233mA | `AA FC 00 07 0C A1 56 BB` | CMD=07, Data=0CA1 (3233)     |

### Packet Format

```
[SOF] [TARGET] [SENDER] [CMD] [DATA_H] [DATA_L] [CRC] [EOF]
 AA     FE       00      xx     xx       xx      xx    BB
```

| Field      | Description                  |
| :--------- | :--------------------------- |
| `SOF`    | `0xAA` — Start of Frame   |
| `TARGET` | `0xFE` — PIC 1 Device ID  |
| `SENDER` | `0x00` — Master (ESP32)   |
| `CMD`    | Command code (see below)     |
| `DATA_H` | Data high byte               |
| `DATA_L` | Data low byte                |
| `CRC`    | XOR of bytes [1] through [5] |
| `EOF`    | `0xBB` — End of Frame     |

### Command Codes

| Code     | Name                  | Data Meaning               |
| :------- | :-------------------- | :------------------------- |
| `0x02` | `CMD_RELAY_ON`      | Socket ID (01=A, 02=B)     |
| `0x03` | `CMD_RELAY_OFF`     | Socket ID (01=A, 02=B)     |
| `0x04` | `CMD_READ_CURRENT`  | Unused (0x0000)            |
| `0x05` | `CMD_REPORT_DATA`   | Current in mA (response)   |
| `0x06` | `CMD_ACK`           | Socket ID + Command echoed |
| `0x07` | `CMD_SET_THRESHOLD` | Threshold in mA            |

### Expected ACK Response

```
AA [SENDER] FE 06 [SOCKET] [CMD_ECHOED] [CRC] BB
```

Example: Relay A ON → ACK: `AA 00 FE 06 01 02 FB BB`

### CRC Calculation

CRC = XOR of bytes 1 through 5 (TARGET ^ SENDER ^ CMD ^ DATA_H ^ DATA_L)

Example for Set Threshold 3233mA:

```
TARGET=FE, SENDER=00, CMD=07, DATA_H=0C, DATA_L=A1
CRC = FE ^ 00 ^ 07 ^ 0C ^ A1 = 54
```

---

## 3. Overload Protection Test

### Test Procedure

1. Set a low threshold: send `CMD_SET_THRESHOLD` with desired mA
2. Connect a load exceeding the threshold
3. **Expected:** Relay trips OFF within 200ms, SoftUART prints overload status
4. **After 5 seconds:** Auto-retry — relay turns back ON
5. If load still exceeds threshold → trips again immediately
6. During overload: `CMD_RELAY_ON` is **blocked** (overload guard)

### Overload Guard Verification

1. Trigger overload on Socket A
2. Send `CMD_RELAY_ON` for Socket A
3. **Expected:** Relay does NOT turn on, ACK still sent
4. Wait for 5s cooldown → retry succeeds → relay turns ON
