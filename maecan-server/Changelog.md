# Changelog

## [2026-02-18.1]
- let UDP socket share the receive port to allow cooperation

## [2025-11-08.1]
- replace deprecated buffer initialisations

## [2025-08-03.1]
- change timing for protocol and version request
- cleanup several files without functional change

## [2024-02-09.1]
- Adapt to php version 8.

## Jun 2023
- Correction of tachomax handling.
- Missing fields icon, tachomax und sid added for CS2 export.
- Allow deleting of loco icons with blanks in the name.

## Aug 2022
- Correction of link to logo

## Feb 2021
- Take function icons from funcsymb.js

## Jan 2021
- Modified handling of websocket: use only one and start after page loading completed.
- Removal of functions which try to access not yet available objects.
- Correction of calling loadKeyboard. 


MäCAN-Sever by Maximilian Goldschmidt

## [2021-01-25.1]
### Hinzugefügt
- Config-Kanäle werden nun bei neu angemeldeten Geräten ausgelesen, um UI-Anzeigen konsistent zu halten.

### Geändert
- Kanalabfrage stabilisiert

## [2020-10-02.1]
### Hinzugefügt
- UI-Vorbereitungen für Firmwareupdates von CAN-Geräten.

### Geändert
- Das anfordern der Config-Kanäle wurde überarbeitet und funktioniert jetzt um ein vielfaches schneller und zuverlässiger.

## [2020-09-29.1]
### Geändert
- Auf den Selben Stand gebracht wie in Gerds Repo.

## [2018-08-12.1]
### Geändert
- Zum SRSEII-Repo hinzugefügt, sauberes Verzeichnis mit kleineren Änderungen des letzten Jahres