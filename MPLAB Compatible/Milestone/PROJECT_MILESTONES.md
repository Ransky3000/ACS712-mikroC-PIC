# 🔌 Smart Outlet System — Project Milestones

**Last Updated:** February 23, 2026

---

## System Architecture

![Smart Outlet System Architecture](Smart%20outlet%20system%20Architecture.png)

---

## Component Status

| Component                | Version | Status         | Documentation                                                                 |
|:-------------------------|:--------|:---------------|:------------------------------------------------------------------------------|
| SmartOutlet Firmware     | v5.3.1  | ✅ Stable      | [FIRMWARE_DOCS.md](Smart%20Outlet%20Device%20dev/FIRMWARE_DOCS.md)            |
| CCU Firmware (ESP32)     | v3.0.0  | ✅ Stable      | [FIRMWARE_DOCS.md](Central%20Control%20Unit%20dev/FIRMWARE_DOCS.md)           |
| Smart-Outlet-WebApp      | —       | 🔧 In Progress | —                                                                             |
| Outlet Breaker (SCT013)  | —       | 📋 Planned     | —                                                                             |

---

## Milestone Log

### CCU Firmware (ESP32)

| Date       | Version | Milestone                                                                 |
|:-----------|:--------|:--------------------------------------------------------------------------|
| 2026-02-23 | v3.0.0  | Added developer documentation and user testing guide                      |
| 2026-02-23 | v2.x    | Bug fixes — current routing by socket ID, Device ID ACK detection         |
| 2026-02-10 | v2.0.0  | Dashboard UI overhaul — device list, toggle switches, auto-poll, REST API |

### SmartOutlet Firmware (PIC16F88)

| Date       | Version | Milestone                                                                 |
|:-----------|:--------|:--------------------------------------------------------------------------|
| 2026-02-10 | v5.3.1  | Multi-device support — configurable Device IDs (0xFE, 0xFD)              |
|            | v5.x    | Overload protection with auto-retry, config mode via RB3 hold             |
|            | v5.x    | NC relay wiring, dual ACS712 sensors, factory reset via 3× press          |

### Smart-Outlet-WebApp

| Date       | Version | Milestone                                                                 |
|:-----------|:--------|:--------------------------------------------------------------------------|
| 2026-02-12 | —       | Initial setup — Django project with dev server running                    |

---

## Roadmap

- [ ] Outlet Breaker — SCT013-100A main load monitoring via ESP32
- [ ] WebApp full cloud integration — ESP32 ↔ Django server data sync
- [ ] Online Dashboard — CRUD for outlets, threshold config, AI chat panel
- [ ] Persistent device storage on ESP32 (SPIFFS/NVS instead of RAM)
- [ ] OTA firmware updates for ESP32

---

## Documentation Index

| Document                      | Location                                                                                          |
|:------------------------------|:--------------------------------------------------------------------------------------------------|
| System Architecture           | [Smart outlet system Architecture.png](Smart%20outlet%20system%20Architecture.png)                |
| PIC Firmware Docs             | [FIRMWARE_DOCS.md](Smart%20Outlet%20Device%20dev/FIRMWARE_DOCS.md)                                |
| PIC Circuit Diagram           | [Smart outlet_Actual_Circuit_v5.2.0.PDF](Smart%20Outlet%20Device%20dev/Smart%20outlet_Actual_Circuit_v5.2.0.PDF) |
| CCU Firmware Docs             | [FIRMWARE_DOCS.md](Central%20Control%20Unit%20dev/FIRMWARE_DOCS.md)                               |
| User Testing Guide            | [User_Testing_Guide.md](../Central_Control_Unit_Firmware/Documentation/User_Testing_Guide.md)     |

---

## Team Notes / Discussion

> Use this section for ongoing decisions, open questions, or notes for the team.

<!-- Add notes below this line -->


