# Web installer

CYD_Family_Clock installs from the browser with ESP Web Tools, using the shared
[cyd-web-installer](https://github.com/anthonyjclarke/cyd-web-installer)
tooling (1.1.0). The page is built by `.github/workflows/firmware.yml` and
published to GitHub Pages on a `v*` tag.

---

## Layout

The manifest has five parts. The fifth, the LittleFS image, is this project's
use of the tooling's optional filesystem part (`custom_installer_fs = yes`).

| Offset     | Part             | Notes                                   |
|:-----------|:-----------------|:----------------------------------------|
| `0x1000`   | `bootloader.bin` |                                         |
| `0x8000`   | `partitions.bin` | Standard dual-OTA `partitions_custom.csv` |
| `0xe000`   | `boot_app0.bin`  | Resets `otadata` to `app0`              |
| `0x10000`  | `firmware.bin`   | App, 1.18 MB of the 1.79 MB slot        |
| `0x390000` | `littlefs.bin`   | 384 KB; holds the four VLW fonts only   |

NVS (`0x9000`, namespace `worldclock`) is never written, so an **Update**
keeps WiFi and settings. Install and Update both rewrite the LittleFS image,
which is why it holds only shipped fonts; the web UI is in PROGMEM
(`src/web_pages.h`).

---

## Smoke test (RUNBOOK 5a)

**10-10-2026 – pass.** CI run 38006034798 (`dev`, `8441240`), preview served on
`localhost:8000`, macOS Chrome. Board: ESP32-2432S028R, ESP32-D0WD-V3 rev 3.1,
MAC `b0:cb:d8:da:ae:8c`.

| Step                         | Result                                        |
|:-----------------------------|:----------------------------------------------|
| CI                           | Green; `buildfs` ran; 5 parts in the manifest |
| FS image                     | Mounted offline: 4 fonts, byte-identical      |
| Erase (`pio run -t erase`)   | OK                                            |
| Fresh install, erase = yes   | OK; Improv name `FamilyClock-AE8C`            |
| Configure WiFi (Improv)      | Joined; IP `192.168.1.95`                     |
| Boot log                     | `Running from app0`; no crash or reboot       |
| Fonts                        | LittleFS mounted; all four `.vlw` listed      |
| NTP                          | Synced to `AEST-10AEDT`                       |
| Web UI (PROGMEM)             | `/`, `/app.js`, `/style.css` 200, full size   |
| Connect again                | "Connected to FamilyClock-AE8C", `CYD_Family_Clock 2.10.0-dev` |

---

## Tests owed

Smoke-tested only. Run these on the next real work on this project, or before
the next release, and tick them off with date and board MAC. This project
changed its partition table, platform and WiFi/Improv code in 2.10.0, so all of
them apply.

- [x] Case 1 – fresh install, erased (10-10-2026, `b0:cb:d8:da:ae:8c`; only
      one board env)
- [ ] Case 2 – Update on a provisioned board (settings kept, fonts rewritten)
- [ ] Case 3 – Update from `app1` (ArduinoOTA to `app1` first)
- [ ] Case 4 – wrong board image – not applicable, single env
- [ ] Install over a v2.9.0 board (`default.csv`, no Improv) without erase:
      settings kept, fonts present
