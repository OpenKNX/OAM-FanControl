> # ⚠ VERALTET — ersetzt durch `Funktionsbeschreibung.md`
>
> Dieser Zwischenstand kennt weder die Sollwertbildung des Masters noch den
> Richtungsart-Parameter, die Zeiten in Minuten und Millisekunden oder die aktuelle
> KO-Belegung.
>
> **Nicht mehr verwenden.**

---

# Funktionsspezifikation — Dezentrale Lüftersteuerung

**Stand:** 15.08.2026 · **Applikation:** OpenKnxId `0xAF`, ApplicationNumber `0x86`, Version `0.1`

Dieses Dokument beschreibt **den festgelegten Funktionsstand**. Begründungen und verworfene
Alternativen stehen in `Review_Anforderungen_KNX-Sicht.md`.

---

## 1. Begriffe

| Begriff | Bedeutung |
|---|---|
| **Knoten** | Ein Lüfter mit seiner Ansteuerung. Kleinste adressierbare Einheit, in der ETS ein Kanal |
| **Gruppe** | Mehrere Knoten auf gemeinsamen Gruppenadressen |
| **Master** | Der Knoten einer Gruppe, der Vorgaben erzeugt und verteilt |
| **Slave** | Jeder andere Knoten der Gruppe |
| **Phase / Gegenphase** | Feste Zuordnung eines Knotens; Knoten in Gegenphase fördern gegenläufig |
| **Taktzustand** | Aktive Halbwelle, gilt für die ganze Gruppe |
| **Leistung** | Vorgabegröße 0–100 % |
| **Stellgröße** | Physikalisches Signal am Gerät (Sollwert an den ESC), 0–100 % |
| **Anteilsfaktor** | Fester Faktor je Knoten, Anteil an der Gruppenvorgabe |

Drei Größen sind strikt getrennt: **Leistung** (logischer Befehl), **Stellgröße** (physikalisches
Signal), **Drehzahl** (Messwert). Es gibt keine Drehzahlregelung; die Steuerung ist
stellgrößengesteuert.

---

## 2. Kanalmodell

- **2 Knoten je Gerät**, je Knoten ein ETS-Kanal.
- **32 KOs je Kanal** — 23 belegt, 9 als Reserve.
- **80 Byte Parameter je Kanal** — 52 belegt, 28 als Reserve.

Beide Blöcke sind bewusst großzügig bemessen, solange die Applikation nicht ausgeliefert ist:
nachträglich lassen sie sich nicht vergrößern, ohne die Nummern bzw. Offsets von Kanal 2 zu
verschieben und damit bestehende ETS-Projekte zu zerstören.
- Der **Kanaltyp** legt fest, ob und wie der Kanal existiert:

| Kanaltyp | Wirkung |
|---|---|
| `Deaktiviert` | Kanal nicht bestückt: kein Kanalreiter, keine KOs |
| `Nicht reversibel` | Eine Förderrichtung; keine Richtungs-KOs, kein Takt |
| `Reversibel` | Zwei Förderrichtungen, Takt- und Richtungs-KOs vorhanden |

- **Suspendiert** ist unabhängig vom Kanaltyp: der Kanal bleibt mit allen KOs und
  Gruppenadress-Verknüpfungen bestehen, seine Funktion ruht. Setzbar als ETS-Parameter
  (Anfangswert) und zur Laufzeit über KO; der Laufzeitwert ist persistent.
- Master und Slave nutzen **dieselbe Firmware**, unterschieden nur durch den Parameter
  „Ist Master".

---

## 3. Parameter je Kanal

| Parameter | Werte | Gilt für |
|---|---|---|
| Kanaltyp | Deaktiviert / Nicht reversibel / Reversibel | alle |
| Beschreibung | Text, 40 Byte | alle |
| Suspendiert | Nein / Ja | alle |
| Ist Master | Nein / Ja | alle |
| Hat Drehzahlrückmeldung | Nein / Ja | alle |
| Phase | Phase / Gegenphase | reversibel |
| Anteilsfaktor | 0–100 % | alle |
| Stellgröße Minimum | 0–100 % | je Richtung |
| Stellgröße Maximum | 0–100 % | je Richtung |
| Stellgröße Mittelstellung | 0–100 %, Vorgabe 50 (0 = gewöhnlicher Lüfter) | alle |
| Anlaufpuls Stellgröße | 0–100 %, 0 = aus (Vorgabe) | alle |
| Anlaufpuls Dauer | 0–2000 ms | alle |
| Totzeit | 0–20 s | reversibel |
| Zykluszeit | 10–3600 s | Master, reversibel |
| Freigabe-Überwachungszeit | 0–3600 s, 0 = aus | alle |
| Master-Überwachungszeit | 0–3600 s, 0 = aus | Slave |
| Stoßlüftung Dauer | 0–3600 s | Master |
| Stoßlüftung Leistung | 0–100 % | Master |
| Sendeabstand Lebenszeichen | 0–255 s | Master |
| Volumenstrom invertieren | Nein / Ja | mit Rückmeldung |
| Volumenstrom-Kennlinie | 3 Wertepaare (Drehzahl, m³/h) je Richtung | mit Rückmeldung |
| Totband Drehzahl | 0–100 % und absoluter Sockelwert | mit Rückmeldung |
| Totband Volumenstrom | 0–100 % und absoluter Sockelwert | mit Rückmeldung |
| Mindest-Sendeabstand | 0–255 s | alle |

---

## 4. Kommunikationsobjekte

Relative Nummer im Kanalblock. Absolut: **Kanal 1 = KO 20–51, Kanal 2 = KO 52–83.**

### Eingänge

| Nr. | Name | DPT | Sichtbar wenn |
|---|---|---|---|
| 0 | Freigabe | 1.003 Enable | immer |
| 1 | Leistung Soll | 5.001 Scaling | immer |
| 2 | Richtungsart | 5.010 | reversibel |
| 3 | Taktzustand | 1.002 Bool | reversibel und Slave |
| 4 | Master lebt | 1.011 State | Slave |
| 5 | Stoßlüftung | 1.010 Start/Stop | Master |
| 6 | Quittierung | 1.016 Ack | immer |
| 7 | Suspendieren | 1.003 Enable | immer |

### Ausgänge

| Nr. | Name | DPT | Sichtbar wenn |
|---|---|---|---|
| 10 | Leistung Ist | 5.001 Scaling | immer |
| 11 | Drehzahl Ist | 7.001 | mit Rückmeldung |
| 12 | Volumenstrom Ist | 13.002 FlowRate m³/h | mit Rückmeldung |
| 13 | Richtung Ist | 1.002 Bool | reversibel |
| 14 | Läuft | 1.011 State | immer |
| 15 | Ist suspendiert | 1.011 State | immer |
| 16 | Störung | 1.005 Alarm | immer |
| 17 | Fehlercode | 5.010 | immer |
| 18 | Betriebsstunden | 12.001 | immer |
| 19 | Master erreichbar | 1.011 State | Slave |

### Master-Ausgänge zur Gruppenverteilung

| Nr. | Name | DPT | Sichtbar wenn |
|---|---|---|---|
| 27 | Leistung Soll Gruppe | 5.001 Scaling | Master |
| 28 | Richtungsart Gruppe | 5.010 | Master und reversibel |
| 29 | Taktzustand Gruppe | 1.002 Bool | Master und reversibel |
| 30 | Master lebt Gruppe | 1.011 State | Master |
| 31 | Zyklus Rest | 7.005 s | Master und reversibel |

**Reserve:** die relativen Nummern **8, 9 und 20–26** sind frei (9 Plätze).

Die Blockgröße ergibt sich beim OpenKNXproducer allein aus der höchsten benutzten
`%Kn%`-Nummer; ein Attribut dafür gibt es nicht. Ein neues KO gehört deshalb **immer in eine
dieser Lücken**. Eine Nummer oberhalb von 31 vergrößert den Block und verschiebt damit
**alle** KO-Nummern von Kanal 2 — bestehende ETS-Projekte verlieren ihre Verknüpfungen.

**Benennung:** Anzeigename `<Beschreibung oder „Lüfter N">: <Semantik>`, Objektfunktion
`Lüfter N: <Eingang|Ausgang>, <Attribute>`.

---

## 5. Funktionsverhalten

### 5.1 Leistung und Stellgröße

- `Leistung Soll` ist die einzige Leistungsvorgabe. Herkunft (Hand, Zeitplan, Bedarfsregelung)
  ist für den Knoten nicht unterscheidbar.
- Slave rechnet: Stellwert = Gruppenvorgabe × eigener Anteilsfaktor, begrenzt auf 0–100 %.
- Die Stellgröße trägt **Drehzahl und Richtung gemeinsam auf einem Ansteuerpfad**: die
  Mittelstellung ist Stillstand, 0 % volle Leistung Richtung A, 100 % volle Leistung Richtung B.
  ⚠ 0 % ist damit **nicht** „aus".
- Abbildung, mit richtungsabhängigem Min/Max:
  `Richtung A: Stellgröße = Mitte − Anteil × Mitte/100`
  `Richtung B: Stellgröße = Mitte + Anteil × (100 − Mitte)/100`
- Mittelstellung 0 % konfiguriert einen gewöhnlichen Lüfter (0 = aus, 100 = volle Leistung).
- `Leistung Soll = 0` bedeutet Stillstand: Stellgröße auf Mittelstellung, Lastschalter offen.
  Ein Modus „Aus" existiert nicht.

### 5.2 Richtung und Takt

- `Richtungsart`: `Reversierend` (Richtung folgt Phase und Taktzustand), `Nur A`, `Nur B`.
  Bei `Nur A`/`Nur B` ist der Taktzustand wirkungslos.
- Der Knoten leitet seine Richtung selbst aus Phase und Taktzustand ab; eine direkte
  Richtungsvorgabe von außen gibt es nicht.
- Der Taktzustand wird als **Zustand** übertragen und vom Master **zyklisch** gesendet.
- Ein Richtungswechsel erfolgt erst nach Ablauf der `Totzeit`; während der Totzeit steht der
  Lüfter.
- Nur der Master kennt die `Zykluszeit` und gibt `Zyklus Rest` aus.

### 5.3 Anlaufpuls

- Bei jedem Übergang Stillstand → Lauf, auch nach einem Richtungswechsel, fährt der Knoten für
  `Anlaufpuls Dauer` die `Anlaufpuls Stellgröße`, danach den Zielwert.
- Feste Dauer, keine Auswertung der Drehzahl. `Anlaufpuls Stellgröße = 0` schaltet die Funktion
  ab (Vorgabe).
- Der Puls erscheint **nicht** in `Leistung Ist` und wird durch `Freigabe = 0` sofort abgebrochen.

### 5.4 Stoßlüftung

- Masterfunktion. Der Anstoß hebt die **Gruppenvorgabe** für `Stoßlüftung Dauer` auf
  `Stoßlüftung Leistung`, danach gilt wieder der anliegende Wert.
- Jeder Slave multipliziert wie immer mit seinem Anteilsfaktor.
- Ein erneuter Anstoß startet die Dauer neu. `0` auf dem KO bricht ab, `Freigabe = 0` ebenfalls.
- Die Richtungsart bleibt unverändert.

### 5.5 Freigabe

- `Freigabe = 0` hat Vorrang vor allem anderen: Leistung 0, sofort, ohne Rücksicht auf Totzeit,
  Stoßlüftung oder Taktumschaltung.
- Zwei unabhängige Sperrauslöser: explizites `Freigabe = 0`, oder Ausbleiben der zyklischen
  Aktualisierung länger als `Freigabe-Überwachungszeit`. Ein fehlendes Signal bedeutet niemals
  „freigegeben".
- Die Sperre ist **selbsthaltend und persistent**: sie übersteht Neustart und Spannungsausfall
  und wird ausschließlich durch ein empfangenes `Freigabe = 1` aufgehoben. Nicht durch
  Zeitablauf, Quittierung oder Aus- und Einschalten.
- Das Ansprechen der Sperre wird als Fehler gemeldet (`Fehlercode = 1`).

### 5.6 Suspendiert

- Leistung 0, keine Teilnahme am Gruppenbetrieb, KOs und Verknüpfungen bleiben erhalten.
- Laufabhängige Überwachungen ruhen und werden als „nicht verfügbar" gekennzeichnet
  (`Fehlercode = 6`); es wird **keine** Störung wegen Nichtlaufens gemeldet.
- Bleibt über Neustart erhalten und wird vom Master nicht überschrieben.

### 5.7 Master, Slave und Überwachung

- Genau ein Knoten je Gruppe ist per Parameter Master. Kein Rollenwechsel zur Laufzeit.
- Der Master verteilt `Leistung Soll`, `Richtungsart` und `Taktzustand` und sendet zyklisch
  `Master lebt`; Taktzustand geht im selben Zyklus mit.
- Lokale Parameter bleiben lokal: Kanaltyp, Rückmeldung, Phase, Anteilsfaktor und Kennlinien
  werden vom Master nicht überschrieben.
- Slave-Verhalten bei ausbleibendem `Master lebt`:
  1. bis zum Ablauf der `Master-Überwachungszeit` gilt der **letzte Zustand**,
  2. danach Leistung 0, `Master erreichbar = 0`, `Störung = 1`, `Fehlercode = 2`.
- Slaves bleiben ohne Master lokal bedienbar.

### 5.8 Drehzahl und Volumenstrom

- `Drehzahl Ist` ist ein Messwert. Bleibt das Signal trotz Ansteuerung aus, wird 0 ausgegeben,
  kein Schätzwert.
- `Volumenstrom Ist` folgt unmittelbar der Drehzahl, berechnet über die Kennlinie:
  impliziter Nullpunkt (0, 0), zwei frei platzierbare Zwischenpunkte, Endpunkt. Lineare
  Interpolation, **keine Extrapolation** — oberhalb des Endpunkts gilt dessen Wert.
- Vorzeichen: **positiv = Zuluft**, negativ = Abluft, anpassbar über „Volumenstrom invertieren".
  Damit sind die Werte mehrerer Knoten summierbar.
- Ohne hinterlegte Kennlinie entfällt `Volumenstrom Ist`.
- Der Volumenstrom ist rein anzeigend und wirkt nicht auf die Ansteuerung zurück.

### 5.9 Blockiererkennung

- Zyklische Prüfung im **5-s-Fenster**. Soll der Lüfter laufen und wird in **zwei
  aufeinanderfolgenden** Fenstern kein Tacho-Puls gezählt: `Fehlercode = 5`, `Störung = 1`.
- Die Erkennung ruht bei Leistung 0, während Totzeit, bei Suspendierung, bei `Freigabe = 0` und
  während des Anlaufpulses. Ohne Drehzahlrückmeldung findet keine Erkennung statt.

### 5.10 Fehlercode

Gesendet wird der oberste anliegende Wert. `Störung` ist die Sammelmeldung.

| Wert | Bedeutung |
|---|---|
| 0 | kein Fehler |
| 1 | Freigabe fehlt oder Überwachungszeit abgelaufen |
| 2 | Master-Timeout |
| 3 | Konfigurations- bzw. Kennlinienfehler |
| 4 | ungültiger Empfangswert |
| 5 | keine Drehzahl trotz Ansteuerung (umfasst blockierten Rotor) |
| 6 | Überwachung ausgesetzt (suspendiert) |

### 5.11 Sendebedingungen

- `Drehzahl Ist` und `Volumenstrom Ist` senden bei Änderung um **x %** oder um mindestens einen
  **absoluten Sockelwert**, zusätzlich begrenzt durch den `Mindest-Sendeabstand`.
- `Zyklus Rest` wird nur bei Richtungswechsel gesendet, nicht laufend.
- `Master lebt` und `Taktzustand` sendet der Master zyklisch.

### 5.12 Anlauf und sicherer Zustand

- Sicherer Zustand bei Reset, Neustart und Störung: Stellgröße auf **Mittelstellung**, Lastschalter offen.
- Während der OpenKNX-Startverzögerung läuft kein Lüfter an.
- Startwerte werden nicht abgefragt; sie kommen über das zyklische Senden von `Freigabe`,
  `Leistung Soll` und `Taktzustand`.
- `Betriebsstunden` sind persistent.

---

## 6. ETS-Oberfläche

Reiterfolge: **Allgemein → Kanalauswahl → Lüfter 1 → Lüfter 2**

- **Allgemein** (`cog-outline`): Modulversion, geräteweite Einstellungen.
- **Kanalauswahl** (`format-list-bulleted-type`): Tabelle mit den Spalten
  **Kanal | Kanaltyp | Beschreibung** (15 / 35 / 50 %). Nur hier wird ein Kanal aktiviert oder
  deaktiviert; die Beschreibung bleibt auch bei deaktiviertem Kanal editierbar.
- **Kanalreiter** (`fan`), Titel `Lüfter N: <Beschreibung>`, nur bei aktivem Kanal. Beginnt mit
  der Überschrift „Kanaldefinition" in der Reihenfolge
  **Beschreibung → Kanaltyp → Suspendiert**, danach die funktionalen Abschnitte.
