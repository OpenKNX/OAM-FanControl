# Funktionsbeschreibung — Dezentrale Lüftersteuerung

**Ist-Zustand, Stand 15.08.2026** · Applikation `0xAF` / `0x86`, Version 0.1

Dieses Dokument beschreibt, **was die Applikation tut**. Ohne Herleitung, ohne verworfene
Alternativen. Warum etwas so ist, steht in `Review_Anforderungen_KNX-Sicht.md`.

---

## 1. Überblick

Ein Gerät steuert **zwei Lüfterknoten**. Ein Knoten ist ein Lüfter mit seiner Ansteuerung und
in der ETS ein Kanal.

Mehrere Knoten bilden über gemeinsame Gruppenadressen eine **Gruppe**. Genau ein Knoten der
Gruppe ist **Master**: er erzeugt die Leistungsvorgabe, gibt den Takt vor und sendet ein
Lebenszeichen. Alle anderen sind **Slaves** und setzen die Vorgabe mit ihren eigenen
Parametern um. Master und Slave laufen mit derselben Firmware, unterschieden nur durch einen
Parameter.

Drei Größen sind strikt getrennt:

| Größe | Bedeutung |
|---|---|
| **Leistung** | der logische Befehl, 0–100 % |
| **Stellgröße** | das physikalische Signal am Lüfter, 0–100 % |
| **Drehzahl** | der gemessene Rückmeldewert |

Es gibt **keine Drehzahlregelung**. Die Steuerung arbeitet stellgrößengesteuert.

---

## 2. Ansteuerung

Es gibt zwei Ansteuerarten. Welche gilt, entscheidet allein der Kanaltyp.

### Nicht reversibel — gewöhnlich

| Stellgröße | Wirkung |
|---|---|
| 0 % | Stillstand |
| 100 % | volle Leistung |

Nichts Besonderes. Es gibt nur die Förderrichtung A, keine Mittelstellung, keinen Takt.

### Reversibel — bipolar über einen Ausgang

Reversierende Lüfter (Maico, Fawas) tragen Drehzahl **und** Richtung auf einem einzigen
Signal:

| Stellgröße | Wirkung |
|---|---|
| 0 % | volle Leistung Richtung A |
| Mittelstellung (Vorgabe 50 %) | Stillstand |
| 100 % | volle Leistung Richtung B |

**Hier ist 0 % nicht „aus"**, sondern volle Leistung in eine Richtung. Der sichere Zustand ist
die Mittelstellung; sie wird bei Stillstand, Störung, Reset und Neustart ausgegeben, und der
Lastschalter wird geöffnet.

Abbildung einer Leistungsvorgabe größer 0:

```
Richtung A:  Stellgröße = Mitte − Anteil × Mitte / 100
Richtung B:  Stellgröße = Mitte + Anteil × (100 − Mitte) / 100
```

begrenzt durch Minimum und Maximum der jeweiligen Richtung.

Der Parameter „Stellgröße Mittelstellung" existiert nur beim reversiblen Kanal. Beim nicht
reversiblen Kanal wird er nicht bloß in der ETS ausgeblendet, sondern in der Firmware
verworfen und durch 0 ersetzt — sonst würde ein Restwert aus einer früheren Konfiguration den
Lüfter auf halber Kraft festhalten. Eine Mittelstellung von 0 bei reversiblem Kanaltyp ist ein
Konfigurationsfehler (Fehlercode 3), weil damit keine zweite Richtung erreichbar wäre.

---

## 3. Kanalmodell

| Kanaltyp | Wirkung |
|---|---|
| Deaktiviert | Kanal nicht bestückt: kein Reiter, keine Kommunikationsobjekte |
| Nicht reversibel | eine Förderrichtung, kein Takt, keine Richtungsobjekte |
| Reversibel | zwei Förderrichtungen, Takt und Richtung vorhanden |

**Suspendiert** ist davon unabhängig: der Kanal bleibt mit allen Objekten und
Gruppenadress-Verknüpfungen bestehen, nur seine Funktion ruht. Er fährt auf Leistung 0,
nimmt nicht am Gruppenbetrieb teil, laufabhängige Überwachungen sind ausgesetzt und werden als
„nicht verfügbar" gemeldet. Setzbar als ETS-Parameter (Anfangswert) und zur Laufzeit über ein
Objekt; der Laufzeitwert überlebt den Spannungsausfall.

---

## 4. Parameter

### Geräteweit

| Parameter | Bereich | Vorgabe |
|---|---|---|
| PWM-Frequenz | 500–20000 Hz | 1000 |

Die Frequenz gilt für alle Kanäle, weil sie auf dem RP2040 keine Eigenschaft des einzelnen
Ausgangs ist.

### Je Kanal

| Parameter | Bereich | Vorgabe | Sichtbar |
|---|---|---|---|
| Kanaltyp | Deaktiviert / Nicht reversibel / Reversibel | Deaktiviert | Kanalauswahl |
| Beschreibung | Text, 40 Byte | leer | immer |
| Suspendiert | Nein / Ja | Nein | immer |
| Ist Master | Nein / Ja | Nein | immer |
| Drehzahlrückmeldung vorhanden | Nein / Ja | Nein | immer |
| Zuordnung | Phase / Gegenphase | Phase | reversibel, nur Slave |
| Anteilsfaktor | 0–100 % | 100 | immer |
| Stellgröße Mittelstellung | 0–100 % | 50 | nur reversibel |
| Stellgröße Minimum | 0–100 % | 20 | je Richtung |
| Stellgröße Maximum | 1–100 % | 100 | je Richtung |
| Anlaufpuls Stellgröße | 0–100 %, 0 = aus | 0 | immer |
| Anlaufpuls Dauer | 0–10000 ms | 0 | wenn Puls aktiv |
| Totzeit | 0–10000 ms | 0 | reversibel |
| Richtungsart | Automatisch reversieren / Nur A / Nur B / Über KO | Automatisch | Master, reversibel |
| Zykluszeit je Richtung | 0–3600 s | 40 | Master, wenn getaktet |
| Überwachungszeit Freigabe | 0–60 min, 0 = aus | 2 | immer |
| Überwachungszeit Master | 0–60 min, 0 = aus | 5 | Slave |
| Sendeabstand Lebenszeichen | 1–60 min | 1 | Master |
| Stoßlüftung Laufzeit | 0–3600 s | 600 | Master |
| Stoßlüftung Leistung | 0–100 % | 100 | Master |
| Sollwert kommt aus | Fester Wert / Externes KO / Interne Regelung | Externes KO | Master |
| Feste Leistung | 0–100 % | 30 | bei festem Wert |
| Regelgröße | CO₂ / rel. Feuchte / anderer Messwert | CO₂ | bei interner Regelung |
| Sollwert | 0–10000 | 800 | bei interner Regelung |
| Proportionalband | 1–10000 | 600 | bei interner Regelung |
| Grundlast | 0–100 % | 20 | bei interner Regelung |
| Maximalleistung | 0–100 % | 100 | bei interner Regelung |
| Volumenstrom invertieren | Nein / Ja | Nein | mit Rückmeldung |
| Kennlinie A, 3 Wertepaare | Drehzahl 0–20000 1/min, Volumenstrom 0–10000 m³/h | 0 | mit Rückmeldung |
| Kennlinie B, 3 Wertepaare | wie A; Endpunkt 0 = Kennlinie A gilt für beide Richtungen | 0 | reversibel, mit Rückmeldung |
| Drehzahl: Änderung um / mindestens aber | 0–100 % / 0–10000 1/min | 5 / 20 | mit Rückmeldung |
| Volumenstrom: Änderung um / mindestens aber | 0–100 % / 0–10000 m³/h | 5 / 5 | mit Rückmeldung |
| Mindest-Sendeabstand | 0–255 s | 10 | immer |

---

## 5. Kommunikationsobjekte

**32 Nummern je Kanal**, 23 belegt. Kanal 1 = KO 20–51, Kanal 2 = KO 52–83.
Frei sind die relativen Nummern **9, 20–26, 28, 29**.

### Eingänge

| Nr. | Name | DPT | Vorhanden |
|---|---|---|---|
| 0 | Freigabe | 1.003 Enable | immer |
| 1 | Leistung (Slave) / Eingang Sollwertvorgabe (Master) | 5.001 Scaling | Slave immer; Master bei externem KO |
| 2 | Richtungsart | 5.010 | reversibel; beim Master nur bei „Über KO" |
| 3 | Taktzustand | 1.002 Bool | reversibel |
| 4 | Master lebt | 1.011 State | Slave |
| 5 | Stoßlüftung | 1.010 Start/Stop | Master |
| 6 | Quittierung | 1.016 Ack | immer |
| 7 | Suspendieren | 1.003 Enable | immer |
| 8 | Istwert (CO₂ / Feuchte) | 9.008 / 9.007 | Master bei interner Regelung |

### Ausgänge

| Nr. | Name | DPT | Vorhanden |
|---|---|---|---|
| 10 | Leistung Rückmeldung | 5.001 Scaling | immer |
| 11 | Drehzahl | 7.001 | mit Rückmeldung |
| 12 | Volumenstrom | 13.002 m³/h | mit Rückmeldung und Kennlinie |
| 13 | Richtung | 1.002 Bool | reversibel |
| 14 | Läuft | 1.011 State | immer |
| 15 | Ist suspendiert | 1.011 State | immer |
| 16 | Störung | 1.005 Alarm | immer |
| 17 | Fehlercode | 5.010 | immer |
| 18 | Betriebsstunden | 12.001 | immer |
| 19 | Master erreichbar | 1.011 State | Slave |
| 27 | Leistung Soll Gruppe | 5.001 Scaling | Master |
| 30 | Master lebt Gruppe | 1.011 State | Master |
| 31 | Zyklus Restzeit | 7.005 s | Master, reversibel |

Richtungsart und Taktzustand sind **je ein Objekt für beide Rollen** auf einer gemeinsamen
Gruppenadresse — der Master sendet darauf, die Slaves empfangen dort. Nur die Beschriftung
wechselt mit der Rolle („… Sollwert Gruppe" beim Master).

**Benennung:** Anzeigename `<Beschreibung oder „Lüfter N">: <Semantik>`, Objektfunktion
`Lüfter N: <Eingang|Ausgang>, <Attribute>`.

---

## 6. Verhalten

### 6.1 Sollwertbildung

Nur der Master erzeugt die Gruppenvorgabe. Die Quelle ist eine Auswahl aus drei sich
gegenseitig ausschließenden Möglichkeiten:

- **Fester Wert** — die eingestellte Leistung, ohne Kommunikationsobjekt.
- **Externes Kommunikationsobjekt** — die Vorgabe kommt von außen.
- **Interne Regelung** — P-Regler auf einen Messwert:

```
Leistung = Grundlast + (Istwert − Sollwert) / Proportionalband × (Maximalleistung − Grundlast)
```

begrenzt auf Grundlast … Maximalleistung. Unterhalb des Sollwerts und solange kein Istwert
empfangen wurde, gilt die Grundlast.

Jeder Knoten multipliziert die Gruppenvorgabe mit seinem **Anteilsfaktor** und begrenzt das
Ergebnis auf 0–100 %.

### 6.2 Richtung und Takt

Die Richtungsart legt der Master fest: automatisch reversieren, nur Richtung A, nur Richtung B,
oder zur Laufzeit über das Objekt. Bei fester Vorgabe sendet er sie zyklisch mit, sodass auch
ein später hinzukommender Knoten sie erhält.

Beim automatischen Reversieren schaltet der Master nach Ablauf der Zykluszeit den Taktzustand
um und sendet ihn. Jeder Knoten leitet seine Förderrichtung **selbst** aus seiner Zuordnung
und dem Taktzustand ab; eine direkte Richtungsvorgabe von außen gibt es nicht. Der Master
läuft immer in Phase.

Bei „Nur A" und „Nur B" ist der Taktzustand wirkungslos.

Ein Richtungswechsel erfolgt erst nach Ablauf der **Totzeit**, während der der Lüfter steht.

### 6.3 Anlaufpuls

Bei jedem Übergang von Stillstand auf Lauf — also auch nach einem Richtungswechsel — fährt der
Knoten für die eingestellte Dauer die Anlaufpuls-Stellgröße, danach den Zielwert. Feste Dauer,
ohne Auswertung der Drehzahl. Der Puls erscheint **nicht** in der Leistungsrückmeldung.
Stellgröße 0 schaltet die Funktion ab; das ist die Vorgabe.

### 6.4 Stoßlüftung

Funktion des Masters: ein Anstoß hebt die Gruppenvorgabe für die eingestellte Laufzeit auf die
Stoßlüftungsleistung, danach gilt wieder der anliegende Wert. Jeder Slave wendet dabei seinen
Anteilsfaktor an. Ein erneuter Anstoß startet die Zeit neu, eine 0 auf dem Objekt bricht ab.

### 6.5 Freigabe

`Freigabe = 0` hat Vorrang vor allem anderen: Leistung 0, sofort, ohne Rücksicht auf Totzeit,
Stoßlüftung oder anstehende Taktumschaltung.

Zwei unabhängige Sperrauslöser: ein explizites `Freigabe = 0`, oder das Ausbleiben der
zyklischen Aktualisierung länger als die Überwachungszeit. Ein fehlendes Signal bedeutet
niemals „freigegeben".

Die Sperre ist **selbsthaltend und persistent** — sie übersteht Neustart und Spannungsausfall
und wird ausschließlich durch ein empfangenes `Freigabe = 1` aufgehoben, nicht durch
Quittierung, Zeitablauf oder Aus- und Einschalten. Das Ansprechen wird gemeldet.

Die Überwachung beginnt erst nach dem ersten empfangenen Freigabe-Telegramm. Eine Anlage, die
das Objekt nicht verknüpft, läuft also normal.

> KNX ist kein Sicherheitsbus. Für den Verbund mit einer Feuerstätte ist zusätzlich ein fest
> verdrahteter, potentialfreier Kontakt gefordert.

### 6.6 Masterüberwachung

Der Master sendet zyklisch sein Lebenszeichen und im selben Zyklus Taktzustand, Leistung und
gegebenenfalls die Richtungsart.

Beim Slave läuft die Überwachung in zwei Stufen:

1. Bleibt das Lebenszeichen aus, gilt **bis zum Ablauf** der Überwachungszeit der letzte Zustand.
2. Danach: Leistung 0, `Störung = 1`, Fehlercode 2, `Master erreichbar = 0`.

Startwerte werden nicht abgefragt — sie kommen über das zyklische Senden.

### 6.7 Drehzahl und Volumenstrom

Die Drehzahl ist ein Messwert. Bleibt das Signal trotz Ansteuerung aus, wird 0 ausgegeben,
kein Schätzwert.

Der Volumenstrom folgt unmittelbar der Drehzahl, berechnet über die Kennlinie: impliziter
Nullpunkt, zwei frei platzierbare Zwischenpunkte, Endpunkt, dazwischen linear interpoliert.
**Keine Extrapolation** — oberhalb des Endpunkts gilt dessen Wert, ohne Meldung. Ohne
hinterlegte Kennlinie entfällt die Ausgabe.

**Kennlinie A ist die Pflichtkennlinie, B ist optional.** Bleibt der Endpunkt von B auf 0, gilt
Kennlinie A für beide Richtungen — mit dem Vorzeichen der jeweils gefahrenen Richtung.
Reversierlüfter fördern in beide Richtungen praktisch gleich viel (beim ebm-papst AxiRev 126
liegen die Kennlinien übereinander), deshalb ist die einzelne Kennlinie der Normalfall und
zwei getrennte Kennlinien die Ausnahme.

Beide Kennlinien werden beim Start auf Plausibilität geprüft: die Drehzahlen von
Zwischenpunkt 1, Zwischenpunkt 2 und Endpunkt müssen aufsteigen. Richtung B wird nur geprüft,
wenn der Kanal reversibel ist und die Kennlinie überhaupt gepflegt wurde. Ein Verstoß setzt
Fehlercode 3.

Vorzeichen: **positiv = Zuluft, negativ = Abluft**. Richtung A gilt als Zuluft, Richtung B
liefert denselben Betrag mit negativem Vorzeichen; „Volumenstrom invertieren" dreht die
Zuordnung für Knoten, die andersherum eingebaut sind. Damit sind die Werte mehrerer Knoten
vorzeichenrichtig summierbar. Die Steuerung regelt nicht danach.

Das Objekt ist 4 Byte groß (DPT 13.002, vorzeichenbehaftet), negative Werte sind also
regulär übertragbar.

### 6.8 Blockiererkennung

Zyklische Prüfung im **5-Sekunden-Fenster**: soll der Lüfter laufen und werden in **zwei
aufeinanderfolgenden** Fenstern keine Tacho-Pulse gezählt, wird Fehlercode 5 gemeldet. Ein
einzelnes leeres Fenster löst nicht aus.

Die Erkennung ruht bei Leistung 0, während Totzeit und Anlaufpuls, bei Suspendierung und bei
fehlender Freigabe. Ohne Drehzahlrückmeldung findet sie nicht statt.

### 6.9 Störung und Fehlercode

`Störung` ist die Sammelmeldung, `Fehlercode` nennt die Ursache. Bei mehreren Ursachen wird die
mit der höchsten Priorität gesendet. Eine Suspendierung setzt die Sammelmeldung **nicht**.

| Wert | Bedeutung |
|---|---|
| 0 | kein Fehler |
| 1 | Freigabe fehlt oder Überwachungszeit abgelaufen |
| 2 | Master-Timeout |
| 3 | Konfigurations- bzw. Kennlinienfehler |
| 4 | ungültiger Empfangswert |
| 5 | keine Drehzahl trotz Ansteuerung |
| 6 | Überwachung ausgesetzt (suspendiert) |

Ein Telegramm auf `Quittierung` setzt gespeicherte Fehler zurück — die Freigabe-Sperre jedoch
nicht.

### 6.10 Sendebedingungen

Drehzahl und Volumenstrom werden gesendet, wenn sie sich um den eingestellten Prozentsatz
**oder** um mindestens den absoluten Sockelwert geändert haben, zusätzlich begrenzt durch den
Mindest-Sendeabstand. Der absolute Wert ist nötig, weil eine rein relative Schwelle in der Nähe
von 0 wirkungslos wäre.

Leistung, Laufzustand, Richtung, Störung und Fehlercode werden bei Änderung gesendet. Störung
und Fehlercode gehen gemeinsam raus, sobald sich der Code ändert; eine Suspendierung
(Code 6) setzt dabei die Sammelmeldung nicht.

Vier Objekte werden **nur beschrieben, nie gesendet**. Damit beantwortet das Gerät eine
Leseanforderung mit dem aktuellen Stand, ohne dafür zyklisch den Bus zu belegen:

| Objekt | Nachgeführt |
|---|---|
| Betriebsstunden | alle 30 min |
| Zyklus Restzeit | alle 5 s |
| Master erreichbar | bei jeder Änderung (die Prüfung läuft ohnehin in jedem Durchlauf) |
| Ist suspendiert | beim Start und bei jedem Wechsel |

`Zyklus Restzeit` ist die einzige Ausnahme: beim Richtungswechsel wird sie zusätzlich aktiv
gesendet, damit der Wechsel auf dem Bus sichtbar ist.

### 6.11 Anlauf

Während der OpenKNX-Startverzögerung läuft kein Lüfter an; empfangene Werte werden aber schon
angenommen, sodass der Start sofort mit aktuellen Vorgaben erfolgt. Ausgang bis dahin:
Mittelstellung, Lastschalter offen.

Der Freigabe-Latch steht im Auslieferungszustand auf **freigegeben** — ohne diese Vorgabe würde
eine Anlage ohne verknüpftes Freigabe-Objekt nie anlaufen. Aus demselben Grund startet die
Überwachung erst mit dem ersten empfangenen Freigabe-Telegramm.

Betriebsstunden, Freigabe-Latch und Suspendierung liegen im Flash und werden bei jeder Änderung
sofort geschrieben.

---

## 7. ETS-Oberfläche

Reiterfolge: **Allgemein → Kanalauswahl → Lüfter 1 → Lüfter 2**

- **Allgemein** — Modulversion, PWM-Frequenz, Hinweise.
- **Kanalauswahl** — Tabelle mit den Spalten Kanal / Kanaltyp / Beschreibung. Nur hier wird ein
  Kanal aktiviert oder deaktiviert; die Beschreibung bleibt auch bei deaktiviertem Kanal
  editierbar.
- **Lüfter N** — nur bei aktivem Kanal, Titel folgt der Beschreibung. Abschnitte in dieser
  Reihenfolge: Kanaldefinition, Rolle und Zuordnung, Sollwertbildung (Master), Stellgröße,
  Richtungsart (Master, reversibel), Anlaufpuls und Richtungswechsel, Überwachung, Stoßlüftung
  (Master), Volumenstrom, Sendebedingungen.

Über jeder Überschrift steht eine Trennlinie. Der Kanaltyp erscheint nur in der Kanalauswahl,
nicht noch einmal auf dem Kanalreiter.

---

## 8. Hardware

Zwei Boards, unterschieden allein über ein `DEVICE_*`-Define zur Compile-Zeit. Die
ETS-Applikation ist für beide identisch.

| | OpenKNX Reg1 Fan-Addon-X2 | MrSpieb HW-FanControl |
|---|---|---|
| Ausgänge je Knoten | einer | zwei, **gespiegelt** (identisches Signal, ein Lüfter je Ausgang) |
| Tachoeingang | ja, optogekoppelt, 2 Pulse je Umdrehung | nein |
| PWM-Polarität | **invertiert** (Level-Shifter auf NMOS mit Pullup) | nicht invertiert (SN74LVC125A) |
| Lastschalter | ja | ja |
| Build-Environment | `develop_RP2040` | `develop_RP2040_MrSpieb` |

Ein Maico enthält zwei Lüfter, die immer gleich herum drehen; beim Reg1-Board hängen sie an
derselben Klemme, was der hochohmige PWM-Eingang erlaubt.

---

## 9. Bekannte Abweichungen und offene Punkte

- **Nichts ist auf Hardware getestet** — Polarität, Schaltrichtung des Lastschalters und
  Tacho-Auswertung sind unverifiziert.

**Bewusst so, kein Fehler:**

- Die Freigabe der Volumenstromausgabe hängt allein an Kennlinie A. Wer nur Kennlinie B pflegt
  und A leer lässt, bekommt keine Ausgabe — A ist per Definition die Pflichtkennlinie.

- Ein `Freigabe = 0` unterdrückt die Stoßlüftung, **stoppt ihre Zeit aber nicht** — die läuft
  im Hintergrund weiter ab. Kehrt die Freigabe vor Ablauf zurück, wird der Rest gefahren.
- `Zyklus Restzeit` wird beim Richtungswechsel mit der vollen Zykluszeit gesendet, nicht als
  laufender Countdown.

**Noch zu erledigen:**

- Die ApplicationNumber ist noch die alte und muss vor dem ersten externen Test gewechselt werden.

---

## 10. Mitgelieferte Module

| Modul | Version | KO-Bereich | Zweck |
|---|---|---|---|
| OGM-Common (BASE) | 1.9.1 | — | OpenKNX-Grundgerüst |
| OFM-ConfigTransfer (UCT) | 0.5.0 | — | Konfigurationsübertragung |
| OFM-FanControl (FAN) | 0.1 | 20–83 | diese Anwendung, 2 Kanäle |
| OFM-LogicModule (LOG) | 4.4.1 | 280–… | 30 Logikkanäle |

Das Logikmodul ist unverändert eingebunden. Sein `op:define` trägt `ModuleType="10"`, also
denselben Typ wie BASE — das Modul legt seine Versionsinformation unter dem BASE-Typ ab, und
der Producer weist jeden anderen Wert zurück.
