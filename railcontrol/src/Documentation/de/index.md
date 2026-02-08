# Herunterladen
RailControl kann auf der [Download](https://www.railcontrol.org/index.php/de/download-de "Download-Seite")-Seite heruntergeladen werden.

## Installieren
Das heruntergeladene Archiv kann an jedem Ort auf dem PC ausgepackt werden.

### Installieren unter Windows

Unter Windows muss beachtet werden, dass RailControl in einem Unterverzeichnis entpackt wird. z.B. ist D:\\ nicht zulässig, D:\\Modellbahn\\ hingegen ist in Ordnung.

### Installieren unter Debian GNU/Linux

Ab Debian GNU/Linux 13 "trixie" ist RailControl in Debian. Mit diesem Befehl wird RailControl installiert:

```
sudo apt install railcontrol
```

**Hinweis:** Debian-spezifische Dokumentation befindet sich im Verzeichnis
`/usr/share/doc/railcontrol`.

## Konfigurations-Datei
Im ausgepackten Archiv befindet sich eine Vorlage der Konfigurations-Datei (railcontrol.conf.dist). Diese wird beim ersten Start von RailControl nach railcontrol.conf kopiert. Üblicherweise ist es nicht nötig darin Einstellungen zu ändern.

## Starten von RailControl
RailControl kann gestartet werden mit einem Doppelklick auf die ausführbare Datei railcontrol.exe (Windows) bzw. railcontrol (andere Betriebssysteme). Um ausführliche Informationen zu erhalten wird empfohlen RailControl von der Kommandozeile aus zu starten. Bei einem Start von der Kommandozeile aus kann das Verhalten von RailControl noch angepasst werden mit verschiedenen [Argumenten beim Programmstart](#argumente-beim-programmstart).

## Beenden von RailControl
RailControl sollte beendet werden, indem man in der Server-Konsole q+Enter eingibt oder Ctrl+C drückt oder im Browser auf den entsprechenden Button klickt.

**Wichtig: Wenn einfach oben rechts das X gedrückt wird, werden die Einstellungen nicht vollständig gespeichert und beim nächsten Start treten Probleme auf.**

Mehrmaliges drücken von q+Enter, Ctrl+C oder Beenden-Button im Browser beendet RailControl vorzeitig. Dies sollte nur gemacht werden, wenn RailControl nicht sauber herunterfährt.

## Browser
Wenn RailControl gestartet wurde, kann mit einem aktuellen Browser zu RailControl verbunden werden. RailControl listet die möglichen Links im Fenster auf. Einer dieser Links kann in den Browser kopiert werden. Die Links mit localhost, 127.0.0.1 und [::1] funktionieren nur auf dem Gerät auf dem auch RailControl ausgeführt wird.

# Funktionen und Konfigurationen
Die wichtigen Funktionen und Konfigurationen sind in der Menüleiste zu erreichen:

Es sind dies von links nach rechts:

![](../menu_quit.png "Button Quit")  
Beenden von RailControl-Server (mit vorherigem Anhalten aller Züge)

![](../menu_booster.png "Button Booster")  
Gleisstrom Ein-/Ausschalten

![](../menu_stop.png "Button Stop")  
Alle Züge sofort anhalten (Fahrgeschwindigkeit Null)

![](../menu_signalred.png "Button Signal Red")  
Alle Züge im Automatikmodus am nächsten Signal/Fahrstraßenende anhalten

![](../menu_signalgreen.png "Button Signal Green")  
Alle Züge in den Automatikmodus versetzten

![](../menu_fullscreen.png "Button Full Screen")  
RailControl im Vollbild anzeigen

![](../menu_program.png "Button Program")  
CV-Programmierung. Wird nur angezeigt, wenn die Zentrale und deren API das unterstützt.

![](../menu_menu.png "Button Menu")  
Auf schmalen Bildschirmen kann der zweite Teil des Menus sichtbar gemacht werden

![](../menu_settings.png "Button Settings")  
[Allgemeine Einstellungen](#allgemeine-einstellungen)

![](../menu_control.png "Button Control")  
[Konfiguration der Zentralen](#konfiguration-der-zentralen)

![](../menu_loco.png "Button Loco")  
[Konfiguration der Lokomotiven](#konfiguration-der-lokomotiven)

![](../menu_multipleunit.png "Button Multiple Unit")  
[Konfiguration der Mehrfachtraktionen](#konfiguration-der-mehrfachtraktionen)

![](../menu_layer.png "Button Layer")  
[Konfiguration der Schichten](#konfiguration-der-schichten)

![](../menu_track.png "Button Track")  
[Konfiguration der Gleise](#konfiguration-der-gleise)

![](../menu_group.png "Button Track Group")  
[Konfiguration der Gleisgruppen](#konfiguration-der-gleisgruppen)

![](../menu_switch.png "Button Switch")  
[Konfiguration der Weichen](#konfiguration-der-weichen)

![](../menu_signal.png "Button Signal")  
[Konfiguration der Signale](#konfiguration-der-signale)

![](../menu_accessory.png "Button Accessory")  
[Konfiguration der Zubehörartikel](#konfiguration-der-zubehörartikel)

![](../menu_feedback.png "Button Feedback")  
[Konfiguration der Rückmelder](#konfiguration-der-rückmelder)

![](../menu_route.png "Button Route")  
[Konfiguration der Fahrstraßen](#konfiguration-der-fahrstraßen)

![](../menu_counter.png "Button Counter")  
[Konfiguration der Zähler](#konfiguration-der-zähler)

![](../menu_text.png "Button Text")  
[Konfiguration der Texte](#konfiguration-der-texte)

