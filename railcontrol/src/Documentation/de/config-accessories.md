# Konfiguration der Zubehörartikel
Im Hauptbildschirm kann man über das Icon ![](../menu_accessory.png) zur Konfiguration des Zubehörartikels gelangen.

## Basisdaten
TODO FEHLENDES BILD (Auf railcontrol.org ist die Weichenkonfig, was falsch scheint)

### Name
Jeder Zubehörartikel benötigt einen eindeutigen Namen. Wird kein eindeutiger Name vergeben, so wird ein eindeutiger Name gewählt oder der Name wird mit einer Nummer ergänzt um ihn eindeutig zu machen.

### Zentrale
Sind mehrere Zentralen an RailControl konfiguriert, so muss die Zentrale ausgewählt werden, mit der die Weiche geschaltet werden soll. Ist nur eine Zentrale konfiguriert so wird das Auswahlfeld nicht angezeigt.

### Protokoll
Unterstützt eine Zentrale mehr als ein Digital-Protokoll, so muss das Protokoll ausgewählt werden, mit der die Weiche geschaltet werden soll. Unterstützt die Zentrale nur ein Protokoll, so wird das Auswahlfeld nicht angezeigt.

### Adresse
Die Digital-Adresse, mit der die  Weiche geschaltet werden soll.

### Schaltzeit (ms)
Alle Magnetartikel müssen nach dem eigentlichen Schaltvorgang wieder ausgeschaltet werden. Bei neueren Magnetartikeln reichen dazu 100ms, ältere und trägere Magnetartikel erfordern manchmal 250ms. Manche Zentralen schalten selbständig mit einem dort konfigurierten Wert wieder aus, dann kann hier 0ms angegeben werden. Je nach verwendetem Weichenantriebstyp bzw. Weichendecoder kann auf den Ausschaltvorgang verzichtet werden. Dies betrifft insbesondere viele Servo- und Motorweichenantriebe und den entsprechenden Decodern.

### Invertiert
Wenn die Weichenanschlüsse am Decoder vertauscht angeschlossen sind können die Anschlüsse in RailControl virtuell zurückgetauscht werden.

## Position
TODO FEHLENDES BILD (Auf railcontrol.org ist die Weichenpositionierung, was falsch scheint)

### Position X
Die Position des Elements in Quadraten ab dem linken Rand des Gleisbildes. Es wird bei null angefangen zu zählen. Ist ein Element grösser als ein Quadrat ist das Quadrat oben links relevant für die Zählung.

### Position Y
Die Position des Elements in Quadraten ab dem oberen Rand des Gleisbildes. Es wird bei null angefangen zu zählen. Ist ein Element grösser als ein Quadrat ist das Quadrat oben links relevant für die Zählung.

### Schicht
Die Schicht auf der das Element sichtbar sein soll.

### Drehung
Die Elemente können jeweils in 90 Grad Schritten gedreht werden.

