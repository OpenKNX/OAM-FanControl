> # ⚠ Nicht mehr der aktuelle Stand
>
> Dieses Dokument war die **Anforderung für die Umsetzung** und beschreibt, *warum* die
> Funktionen so aussehen, wie sie aussehen. Die Beschlüsse gelten weiter — die Details
> haben sich während der Implementierung an mehreren Stellen verschoben, unter anderem bei
> Zeitbereichen und Einheiten, bei der Sollwertquelle des Masters und bei der KO-Belegung.
>
> **Was die Applikation heute tatsächlich tut, steht in `Funktionsbeschreibung.md`.**
> Bei Abweichungen gilt jene Datei.
>
> Dieses Dokument bleibt als Begründungsquelle bestehen und wird nicht mehr nachgeführt.

---

# Anforderungen Dezentrale Lüftersteuerung — Ergänzung aus KNX-Sicht

**Bezug:** `Anforderungen_Dezentrale_Lueftersteuerung.md`, Stand 14.08.2026
**Stand dieses Dokuments:** 15.08.2026
**Status:** Alle blockierenden Punkte sind entschieden. Offen sind nur noch sieben
Detailfragen (Abschnitt 10), die während der Implementierung fallen können.

Die Anforderungen wurden **absichtlich ohne KNX-Bezug** formuliert, um die Funktion nicht
vorzeitig auf Bus-Eigenheiten zu verengen. Dieses Dokument ist der bewusst nachgelagerte
zweite Durchgang aus Bussicht — keine Korrektur eines Versäumnisses, sondern die ergänzende
Schicht: DPTs, Gruppenadress-Topologie, Sendebedingungen, Überwachung, ETS-Modell.

---

## 1. Ergebnis in drei Sätzen

Die Anforderungen sind funktional tragfähig und weitgehend unverändert umsetzbar. Aus
KNX-Sicht ergaben sich drei Klassen von Befunden: zwei echte Widersprüche (beide aufgelöst),
zwei Konstrukte, die KNX bereits mitbringt und die deshalb entfallen, und eine Reihe von
Anforderungen, die der Bus erzwingt und die im Dokument fehlten — vor allem Sendebedingungen
und Überwachungszeiten. Der Kern der Regelungslogik bleibt, wie er ist.

---

## 2. Entscheidungen

| # | Thema | Entscheidung |
|---|---|---|
| 1 | Freigabe-Persistenz | **Persistent über Spannungsausfall speichern.** E-1e gilt, **M-6 ist zu korrigieren** („bleibt nach Neustart: ja") |
| 2 | E-2 gegen E-3 | Erledigt durch die DPT-Wahl: DPT 5.001 kann keine ungültigen Werte übertragen. Kein Implementierungsaufwand |
| 3 | Sendebedingungen | `DREHZAHL_IST` und `VOLUMENSTROM_IST` senden bei **Änderung um x %** (konfigurierbar) |
| 4 | Masterausfall | Ausfallzustand = **letzter Zustand**; zusätzlich **„Master lebt"-Ping** mit konfigurierbarer Zeit, bleibt er aus → **abschalten + Fehlerzustand** |
| 5 | Takt-Refresh nach verlorenem Telegramm | **Nicht behandeln.** Im Zweifel wartet ein Knoten bis zur nächsten Aktualisierung und läuft dann wieder mit |
| 6 | Architektur | **Master-Slave wie in MA-2 spezifiziert.** Verteilung per Gruppenadresse ohne Master und Taktbildung aus Buszeit wurden geprüft und **verworfen** |
| 7 | DPT-Abbildung | **Angenommen**, Festlegungen in Abschnitt 7 |
| 8 | ApplicationNumber | **Bleibt vorerst `0xAF` / `0x86`.** Eine Dev-Nummer wird ausgehandelt, Umstellung vor dem Release |
| 9 | Blockgrößen | **32 KOs je Kanal** (23 belegt, 9 Reserve in den Lücken 8, 9 und 20–26) und **80 Byte Parameter je Kanal** (52 belegt, 28 Reserve), **2 Knoten je Gerät** |
| 10 | `DEAKTIVIERT` | **Aufteilen** in ETS-Kanaltyp „Deaktiviert" und Laufzeit-„Suspendiert" (Abschnitt 5) |
| 11 | Stoßlüftung | **Masterfunktion.** Der Master hebt die Gruppenleistung an, die Slaves folgen |
| 12 | `KNOTEN_ID` / `GRUPPEN_ID` | **Gestrichen** — in KNX implizit über physikalische Adresse und Gruppenadresse vorhanden |
| 13 | Stellgrößen-Kennlinie | **Entfällt**, ersetzt durch Tot-Stellgröße + Anlaufpuls (Abschnitt 4.2) |
| 14 | Blockiererkennung | **Zyklisch**, Fenster 5 s, zwei leere Fenster in Folge → Fehler (S-7) |
| 15 | `FEHLERCODE` | **Enum** (DPT 5.010) mit Priorität, Wertevorrat in Abschnitt 6 |
| 16 | Versionierung | **Alles auf 0.1**, `ReplacesVersions` listet nur die eigene Version |
| 17 | ETS-XML-Aufbau | Nach `TAS-UP-4x-TouchRGB/doc/OpenKNX-ETS-XML-Styleguide.md` |

### 2.1 Präzisierung zu Entscheidung 4 — zwei Phasen, kein Widerspruch

„Letzter Zustand" und „abschalten" schließen sich nicht aus, sie beschreiben zwei Phasen:

1. Ping bleibt aus → der Knoten behält **bis zum Ablauf** der Überwachungszeit den letzten Zustand.
2. Überwachungszeit abgelaufen → **Leistung 0** (bei reversibel: Mittelstellung),
   `MASTER_ERREICHBAR = 0`, `STOERUNG = 1`, `FEHLERCODE = 2`.

Daraus folgt eine Nebenbedingung: die Überwachungszeit sollte **≤ eine Zykluszeit** betragen.
Sonst fördert ein reversierender Knoten ohne Takt so lange in eine Richtung, dass die
Gebäudebilanz merklich kippt, bevor die Abschaltung greift.

### 2.2 Präzisierung zu Entscheidung 3 — Totband braucht einen Sockelwert

Eine rein *relative* Schwelle versagt bei `VOLUMENSTROM_IST`: der Wert ist
vorzeichenbehaftet und geht durch 0, dort wird x % beliebig klein und der Knoten sendet
dauerhaft. Die Bedingung lautet deshalb „ändert sich um **x %** *oder* um mindestens
**y m³/h**". Für `DREHZAHL_IST` gilt dasselbe um den Stillstand herum.

### 2.3 Präzisierung zu Entscheidung 5 — Read-on-Init existiert nicht

Die **Read-on-Init-Flags sind nicht funktional** und in OpenKNX deshalb ausgeblendet; der
Producer entfernt sie generell. Ein Leseweg beim Anlauf ist damit **kein verfügbarer
Mechanismus** und wird nicht eingeplant.

Er ist auch nicht nötig: **zyklisches Senden ersetzt das Lesen.** `FREIGABE` wird nach dem
Ruhestromprinzip ohnehin zyklisch aktualisiert (E-1b), spätestens nach
`FREIGABE_UEBERWACHUNGSZEIT` liegt also ein frischer Wert an — genau das, was E-1f braucht.
Für `LEISTUNG_SOLL` und `TAKT_ZUSTAND` leistet das der Master-Ping-Zyklus. Ein
zurückkehrender Knoten wartet schlimmstenfalls ein Ping-Intervall, bei ungünstigem Zeitpunkt
eine Zyklusperiode. Das ist bewusst akzeptiert.

### 2.4 Präzisierung zu Entscheidung 16 — `ReplacesVersions` lässt sich nicht leeren

Geprüft und verworfen: der OpenKNXproducer behandelt das Attribut als **Pflichtfeld**.
`value=""` → *„ETS Attribute ReplacesVersions contains , this is not a valid version number"*,
Attribut entfernt → *„Required ETS-Attribute ReplacesVersions is missing"*.

Umgesetzt ist deshalb der Weg, der die Absicht erfüllt und akzeptiert wird: **nur die eigene
Version listen.** Damit existiert kein Migrationspfad aus einer älteren Applikation, ETS
erzwingt eine Neuparametrierung statt einer Übernahme auf ein inkompatibles Layout.

Verifizierter Stand: `ApplicationVersion = 0.1`, `ReplacesVersions = 0.1` →
`M-00FA_A-AF86-01-0000`, Producer Exit 0.

⚠ Ein Gerät mit der alten 0.9-Applikation lässt sich unter derselben ApplicationNumber nicht
auf 0.1 „zurückdrehen". Für die Entwicklung unerheblich, aber ein Argument, die Umstellung auf
die neue ApplicationNumber vor dem ersten externen Test zu machen.

---

## 3. Konsolidierte Änderungsliste für das Anforderungsdokument

Das ist die Arbeitsliste. Alles darin ist entschieden. Die Spalte „Betrifft" verweist auf
Stellen im **Anforderungsdokument**, nicht in diesem Dokument.

| Nr. | Betrifft (Anforderungen) | Änderung |
|---|---|---|
| Ä-1 | M-6, Tabelle | Zeile `FREIGABE = 0`, Spalte „Bleibt nach Neustart" von **nein** auf **ja (Latch)** |
| Ä-2 | Abschn. 2 | `KNOTEN_ID` und `GRUPPEN_ID` streichen |
| Ä-3 | Abschn. 2 | `DEAKTIVIERT` ersetzen durch **Kanaltyp** (ETS) und **`SUSPENDIERT`** (Laufzeit-KO, persistent) |
| Ä-4 | Abschn. 4 | Ausgang `IST_DEAKTIVIERT` → **`IST_SUSPENDIERT`** umbenennen (Begründung 5.4) |
| Ä-5 | Abschn. 2, K-4 | `KENNLINIE_STELLGROESSE` streichen; `STELLGROESSE_MIN`/`_MAX` je Richtung sowie `ANLAUFPULS_STELLGROESSE`/`_DAUER` aufnehmen. In K-4 „`KENNLINIE_STELLGROESSE` rechnet …" → „die Stellgrößen-Abbildung rechnet …" |
| Ä-6 | K-8, K-9 | gelten **nur noch** für `KENNLINIE_VOLUMENSTROM` |
| Ä-7 | Abschn. 2 und 5.3 | `STOSSLUEFTUNG_DAUER` und `_LEISTUNG` nur beim **Master**; Eingang `STOSSLUEFTUNG` nur beim Master |
| Ä-8 | Abschn. 4 | **`TAKT_ZUSTAND` als Master-Ausgang** aufnehmen — MA-2 verlangt ihn, die Ausgangstabelle listet ihn nicht |
| Ä-9 | Abschn. 2 und 4 | `MASTER_UEBERWACHUNGSZEIT` (Parameter, Slave) und `MASTER_LEBT` (Ausgang Master / Eingang Slave) aufnehmen |
| Ä-10 | Abschn. 2 | Sendebedingungen aufnehmen: Totband in % **und** Sockelwert je Analogausgang, Mindest-Sendeabstand |
| Ä-11 | Abschn. 4 | `FEHLERCODE`-Wertevorrat festschreiben (siehe Abschnitt 6 hier) |
| Ä-12 | Abschn. 5 | Neuer Regelblock **S-1 bis S-9** (Anlaufpuls, Blockiererkennung) |

### 3.1 Warum `KNOTEN_ID` und `GRUPPEN_ID` entfallen

In KNX ist Adressierung gelöst: die **physikalische Adresse** identifiziert das Gerät, die
**Gruppenadresse** bildet die Gruppe. Eine Gruppe entsteht dadurch, dass die Takt- und
Leistungs-KOs mehrerer Knoten auf **derselben Gruppenadresse** liegen — nicht durch eine
Parameternummer.

Eigene IDs wären ein **zweites, paralleles Adressschema**, das gegenüber der tatsächlichen
Verknüpfung auseinanderdriften kann: ein Knoten mit `GRUPPEN_ID = 2`, aber auf der
Gruppenadresse der Gruppe 1 verknüpft — und die Firmware kann den Widerspruch nicht einmal
erkennen. Eine Anzeige „wer gehört zu wem" gehört in die Visualisierung, nicht ins Gerät.

---

## 4. Die zwei Kennlinien

Das Anforderungsdokument kennt **zwei** Kennlinien. Sie werden leicht verwechselt, weil bei
beiden das Argument „das ist doch nichtlinear" zutrifft — aber sie tun Verschiedenes, und nur
eine von beiden wird ersetzt.

| | `KENNLINIE_VOLUMENSTROM` | `KENNLINIE_STELLGROESSE` |
|---|---|---|
| Rechnet | **Drehzahl → Volumenstrom** (min⁻¹ → m³/h) | **Leistung → Stellgröße** (% → %) |
| Wirkung | rein **anzeigend** (K-6) | **steuernd** |
| Nichtlinear wegen | Aerodynamik und Anlagenkennlinie | Anlaufschwelle und Motorkennung |
| Im Dokument | K-7 bis K-10, vollständig spezifiziert | nur „Stützstellen", unterspezifiziert |
| **Status** | **bleibt unverändert** | **entfällt** → Tot-Stellgröße + Anlaufpuls |

Merksatz: **die eine misst, die andere stellt.** Ersetzt wird nur die stellende.

### 4.1 Volumenstrom-Kennlinie (anzeigend) — bleibt unverändert

Hier ist nichts zu entscheiden, das Anforderungsdokument hat es bereits gelöst:

- **K-7** — vier Punkte: impliziter Nullpunkt (0, 0), zwei frei platzierbare Zwischenpunkte,
  Endpunkt (maximale Drehzahl, zugehöriger Volumenstrom). Also **drei Wertepaare je Richtung**,
  bei `REVERSIBEL = ja` eigene Punkte je Richtung, **nicht** gespiegelt.
- **K-8** — lineare Interpolation. Genau hier steckt die Antwort auf „Lüfter haben keinen
  linearen Zusammenhang zwischen Drehzahl und Volumenstrom": die freien Zwischenpunkte legt man
  dorthin, wo die Kurve am stärksten krümmt.
- **K-9** — keine Extrapolation, oberhalb des Endpunkts wird begrenzt, keine Meldung.
- **K-10** — ohne hinterlegte Kennlinie entfällt `VOLUMENSTROM_IST` ersatzlos.

Umsetzung: numerische Parameter, drei Wertepaare je Richtung. Damit ist diese Kennlinie der
größte Parameterposten des Kanals — bei reversiblen Knoten 12 Werte.

### 4.2 Stellgrößen-Kennlinie (steuernd) — ersetzt durch Tot-Stellgröße + Anlaufpuls

`KENNLINIE_STELLGROESSE` wird gestrichen. An ihre Stelle treten eine Tot-Stellgröße
(Mindestwert) und optional ein Anlaufpuls. Das ist die bessere Lösung, weil sie **zwei
physikalisch verschiedene Schwellen trennt**, die eine Polylinie in einen einzigen Verlauf presst:

| Physikalischer Effekt | Was ihn löst |
|---|---|
| **Losbrechen** aus dem Stillstand — Haftreibung braucht kurzzeitig hohes Drehmoment | **Anlaufpuls**: hohe Stellgröße für kurze Zeit |
| **Dauerlauf** — unter einer Mindest-Stellgröße läuft der Lüfter nicht stabil | **Tot-Stellgröße**: Mindestwert, unter dem nie gefahren wird |

Darin liegt der Gewinn: eine Kennlinie hätte das Anlaufproblem nur *maskiert*, indem sie den
unteren Bereich generell anhebt — und damit im Dauerlauf unnötig schnell gedreht. Getrennt
betrachtet darf die Tot-Stellgröße der echte **Halte**wert sein, das Losbrechen übernimmt der Puls.

**Abbildung:**

```
Stellgröße = 0                                        für Leistung = 0
Stellgröße = MIN + (MAX − MIN) × Leistung/100         für Leistung > 0
```

Bei `REVERSIBEL = ja` **je Richtung eigene MIN/MAX**, weil Zu- und Abluft unterschiedliche
aerodynamische Last sehen. K-3 (bipolar, Mittelstellung = Stillstand) bleibt gültig, nur anders
ausgedrückt.

**Parameter.** Bewusst **ohne** „PWM" im Namen: das Anforderungsdokument hält die Stellgröße
absichtlich hardware-neutral („physikalische Stellgröße, z. B. Tastverhältnis").

| Parameter | Gilt für | Wert |
|---|---|---|
| `STELLGROESSE_MIN` | je Richtung | 0–100 % — Mindestwert für stabilen Dauerlauf |
| `STELLGROESSE_MAX` | je Richtung | 0–100 % — Wert bei Leistung 100 % (Geräusch, Derating) |
| `ANLAUFPULS_STELLGROESSE` | Knoten | 0–100 %, **0 = kein Anlaufpuls** |
| `ANLAUFPULS_DAUER` | Knoten | Zeit, typisch 100–500 ms |

Die Anlaufpuls-Parameter gelten **knotenweit**, nicht je Richtung — und zwar aus genau dem
Grund, aus dem `MIN`/`MAX` umgekehrt je Richtung *nötig* sind: der Unterschied zwischen den
Richtungen ist die **aerodynamische Last**. Beim Losbrechen liegt aber noch keine Strömung an,
die Last ist also praktisch nicht vorhanden. Übrig bleibt die Haftreibung, und die ist eine
Eigenschaft des Motors, nicht der Förderrichtung. Eine Aufteilung je Richtung hätte deshalb
keinen Nutzen — sie würde nur zwei Parameter kosten und zwei Werte zum Auseinanderlaufen bringen.

**Speicherbilanz:** reversibler Knoten 2 × 2 + 2 = **6 Byte** statt rund 20–24 Byte für zwei
Polylinien. Die ETS zeigt vier sprechende Zahlenfelder statt einer abstrakten Kurve.

**Risiko:** gering und rückholbar. Für PWM- und EC-Lüfter (Maico PPB30, Fawas) ist die Drehzahl
über der Stellgröße oberhalb der Anlaufschwelle näherungsweise linear. Verstärkt wird das durch
die Leistungsklasse und die Bauart: es geht um Lüfter **bis etwa 10 W**, typisch 2,5 W, **mit
integriertem ESC**, der intern selbst regelt. Die Stellgröße ist damit ein *Sollwert an den
ESC*, nicht die direkte Motoransteuerung — und ein ESC bildet seinen Eingang deutlich linearer
auf die Drehzahl ab als rohes PWM auf einen Motor. Sollte später doch eine Kurve nötig werden,
dürfen Parameter **angehängt** werden; nur Umnummerieren und Entfernen ist verboten. Unumkehrbar
sind KO-Nummern und ETS-IDs, und die berührt diese Änderung gar nicht.

### 4.3 Regeln S-1 bis S-9

**Anlaufpuls**

- **S-1** Der Anlaufpuls wird bei jedem Übergang **Stillstand → Lauf** ausgelöst, also auch
  nach dem Richtungswechsel: nach Ablauf der `TOTZEIT` steht der Lüfter, die Haftreibung ist
  wieder da.
- **S-2** Der Anlaufpuls erscheint **nicht** in `LEISTUNG_IST`. `LEISTUNG_IST` ist nach K-4
  eine *Leistung*, der Puls eine *Stellgröße* — würde er durchschlagen, sendete jeder Start ein
  unsinniges 100-%-Telegramm auf den Bus. Der Puls bleibt geräteintern.
- **S-3** Der Puls läuft mit **fester Dauer** (`ANLAUFPULS_DAUER`), danach wird auf den
  Zielwert zurückgefahren. **Keine** Auswertung der Drehzahlrückmeldung — für Knoten mit und
  ohne Rückmeldung identisch.
- **S-4** *Folge aus S-3:* Der Puls liefert keine Diagnose. Ein blockierter Rotor ist ohnehin
  grundsätzlich **nicht** von einem defekten Drehzahlgeber unterscheidbar — das Symptom ist in
  beiden Fällen „angesteuert, aber keine Drehzahl". Genau das sagt A-1 bereits. Konsequenz:
  „Blockade" und „keine Rückmeldung" fallen zu **einem** Fehlerwert zusammen.
- **S-5** `FREIGABE = 0` bricht einen laufenden Anlaufpuls sofort ab (folgt aus E-1c).
- **S-6** Für Zeitparameter im Millisekundenbereich die BASE-ParameterTypes prüfen, bevor ein
  eigener angelegt wird (Styleguide Abschnitt 13: nicht neu erfinden).

**Wird der Anlaufpuls überhaupt gebraucht?** Der integrierte ESC übernimmt Kommutierung und
Sanftanlauf selbst, das Losbrechmoment ist also möglicherweise schon geräteseitig gelöst — und
ein plötzlicher Sollwertsprung auf 100 % kann einen regelnden ESC eher stören als helfen. Der
Puls bleibt deshalb als Parameter vorgesehen, wird aber mit **Vorgabewert 0 (aus)** ausgeliefert
und erst auf der Hardware verifiziert. `ANLAUFPULS_STELLGROESSE = 0` deckt diesen Fall bereits
ab, es braucht keine Sonderbehandlung im Code.

**Blockiererkennung** — bewusst getrennt vom Anlaufpuls: der Puls ist eine
Stellgrößen-Maßnahme mit fester Dauer, die Erkennung eine laufende Überwachung.

- **S-7** Zyklische Prüfung mit festem Fenster (**5 s**): soll der Lüfter laufen und wird in
  **zwei aufeinanderfolgenden** Fenstern **kein** Tacho-Puls gezählt, dann `FEHLERCODE = 5`
  und `STOERUNG = 1`. Erkennungszeit rund 10 s; ein einzelnes leeres Fenster löst bewusst
  **nicht** aus.
- **S-8** Die Erkennung **ruht**, solange der Lüfter nicht laufen soll (Leistung 0, `TOTZEIT`,
  suspendiert, `FREIGABE = 0`) und während des Anlaufpulses — das erste Fenster beginnt erst
  nach Ende des Pulses. Ohne `HAT_RUECKMELDUNG` existiert keine Erkennung, sie ist dann als
  „nicht verfügbar" zu kennzeichnen (M-4, `FEHLERCODE = 6`).
- **S-9** Offen: fällt `STOERUNG` bei zurückkehrenden Pulsen selbsttätig, oder braucht es eine
  `QUITTIERUNG`? Siehe Abschnitt 10.

Das 5-s-Fenster ist reichlich bemessen: der Tacho-Eingang liefert 2 Pulse je Umdrehung, also
selbst bei 30 min⁻¹ noch 5 Pulse im Fenster. Ein leeres Fenster ist ein belastbares Signal und
kein Auflösungsproblem.

### 4.4 Konfiguration per Begleit-Tool statt per Freitext-Parameter

Erwogen wurde, Kennlinien als **Freitext** zu parametrieren und die Strings mit einem
GUI-Werkzeug zu erzeugen. Mit 4.2 ist das für die Stellgröße erledigt, und für die
Volumenstrom-Kennlinie gibt K-7 numerische Wertepaare vor. Zwei Erkenntnisse bleiben nützlich:

**Freitext hätte hier keinen Platz gespart, sondern gekostet.** Weil Leistung und Stellgröße
beide 0–100 % sind, passt jeder Wert in ein Byte: fünf Punkte numerisch sind 10 Byte, der
String `"0,0;20,15;50,45;80,85;100,100"` sind 29. Dazu kommt der Verlust der ETS-Validierung
über `minInclusive`/`maxInclusive`, die bei Freitext in JavaScript nachgebaut werden müsste.

**Der Wunsch „Konfiguration im Werkzeug erzeugen statt in der ETS klicken" bleibt berechtigt —
und braucht kein neues Format.** Das Projekt liefert **OFM-ConfigTransfer** schon mit: es
serialisiert einen kompletten Modul-Kanal als Schlüssel-Wert-Zeichenkette mit Applikations-,
Modul- und Versions-Referenz samt Kompatibilitätsprüfung und tauscht sie über die
Zwischenablage mit der ETS. Ein Begleit-Tool, das Datenblattpunkte aufnimmt und einen
**ConfigTransfer-Import-String** ausgibt, liefert denselben Komfort für **alle** Parameter des
Kanals — ohne zweiten Parser in der Firmware und ohne unvalidiertes Freitextfeld.

⚠ Vorbehalt: das ConfigTransfer-README bezeichnet die Formatspezifikation ausdrücklich als
**Entwurf**. Bevor ein Generator darauf aufsetzt, sollten Grammatik und Stabilität mit dem
Modul-Maintainer geklärt werden.

---

## 5. „Deaktiviert" und „Suspendiert" nach OpenKNX-Konvention

Der Parameter `DEAKTIVIERT` deckt zwei Fälle ab, die OpenKNX seit dem Kanalauswahl-Beschluss
(09.07.2026) ausdrücklich trennt. Die Abbildung ist eindeutig:

| Fall im Anforderungsdokument | OpenKNX-Mechanismus | Wirkung in der ETS |
|---|---|---|
| „nicht bestückter Kanal" | **Kanaltyp = Deaktiviert** (Wert 0) in der Kanalauswahl-Tabelle | kein Kanalreiter, **keine KOs** — spart 32 KO-Nummern je ungenutztem Kanal |
| „Service, Fehlersuche" (M-3 bis M-5) | **Suspendiert** (`PT-Suspended`) auf dem Kanalreiter | KOs **und** Gruppenadress-Verknüpfungen bleiben erhalten, die Funktion ruht |

Dass der zweite Fall gemeint ist, steht schon im Anforderungsdokument: M-4 verlangt, der
stillgelegte Knoten bleibe „über die Schnittstelle lesbar und parametrierbar". Das trifft
ausschließlich auf *Suspendiert* zu — ein in der ETS deaktivierter Kanal hat keine KOs mehr.

### 5.1 Kanaltyp

**Haupttyp** (`UP-`, memory-backed) mit `0 = Deaktiviert`, `1 = Nicht reversibel`,
`2 = Reversibel`. Er steht **nur** in der Kanalauswahl-Tabelle; dort allein wird ein Kanal
aktiviert und deaktiviert. Damit erledigen sich K-1 und K-2 über die ETS-Dynamik: bei `1`
fehlen die Richtungs-KOs, ohne Rückmeldung der Drehzahl-Ausgang.

**TypeSelect** (`P-`, ohne `<Memory>`) mit denselben Werten **ohne** „Deaktiviert" auf dem
Kanalreiter, gekoppelt über `BASE_SyncChannelType` mit `AliasName="TypeValue"` — die vom
Beschluss vorgeschriebene Zwei-Parameter-Konstruktion.

### 5.2 Suspendiert

**Niemals** ein Kanaltyp-Wert, sondern ein eigener Parameter `PT-Suspended` mit
`HelpContext="BASE-ChannelSuspended"`. Reihenfolge auf dem Kanalreiter:
**Beschreibung → Kanaltyp → Startverzögerung → Suspendiert**.

### 5.3 Erweiterung gegenüber der reinen Konvention

M-5 fordert, dass die Stilllegung **zur Laufzeit** gesetzt werden kann und **über
Spannungsausfall erhalten** bleibt. `PT-Suspended` ist aber ein ETS-Parameter und damit nur
beim Download veränderbar. Auflösung, ohne ein neues Konzept zu erfinden:

- der **ETS-Parameter** `Suspendiert` liefert den **Anfangswert** nach dem Download,
- das **KO** `SUSPENDIERT` (DPT 1.003) ist die **Laufzeit-Übersteuerung**, im Flash gehalten,
- nach einem ETS-Download wird der Modul-Flashbereich neu initialisiert, es gilt wieder der
  Parameterwert.

### 5.4 Folge für den Ausgang `IST_DEAKTIVIERT`

Der Ausgang ist in dieser Form **nicht umsetzbar**: ein deaktivierter Kanal besitzt keine KOs
und kann grundsätzlich nichts melden. Gemeint sein kann nur die Suspendierung. Deshalb Ä-4:
Umbenennung in **`IST_SUSPENDIERT`** (DPT 1.011). Die Information „Kanal nicht bestückt" ist
keine Laufzeitmeldung, sondern am Fehlen der KOs erkennbar.

---

## 6. `FEHLERCODE`

Entschieden ist ein **Enum** (DPT 5.010) als Einzelwert mit dokumentierter Priorität. Tragend
dafür ist eine Beobachtung: die meisten Fehlerursachen schließen sich **konstruktiv gegenseitig
aus**, weil jeder Fehler, der den Lüfter stoppt, die laufabhängigen Fehler unterdrückt — genau
das Prinzip aus M-4. Nach Freigabe- oder Master-Timeout ist die Leistung 0, „keine Drehzahl
trotz Ansteuerung" kann dann nicht mehr auslösen. Echte Gleichzeitigkeit bleibt fast nur für
statische Konfigurationsfehler übrig; der Informationsverlust eines Einzelwerts ist gering.

Verworfene Alternativen: **DPT 27.001** ist `DPT_CombinedInfoOnOff` (16 Ein/Aus-Werte + 16
Gültigkeitsbits) für Kanalzustände und als Fehlerregister zweckfremd; **ein KO je Fehler**
würde von 32 KOs nach sechs Fehlermeldungen nur zwei Reserve lassen und `STOERUNG` duplizieren;
**DPT 20.012 `ErrorClass_HVAC`** hat einen fixen, zu groben Wertevorrat — die Unterscheidung
„Master-Timeout" gegen „Lüfter dreht nicht" ginge verloren, also genau das, was der Techniker braucht.

Zusammenspiel: `STOERUNG` (DPT 1.005 Alarm) ist die **Sammelmeldung** — Visualisierungen und
Alarmlisten brauchen nur dieses Bit. `FEHLERCODE` liefert die **Ursache**. Für Klartext kommt
der ohnehin vorhandene **BASE-Diagnose-String** (DPT 16.001) dazu, ohne zusätzliches KO.

| Wert | Bedeutung | Priorität |
|---|---|---|
| 0 | kein Fehler | — |
| 1 | Freigabe fehlt oder Überwachungszeit abgelaufen | 1 (höchste) |
| 2 | Master-Timeout | 2 |
| 3 | Konfigurations- bzw. Kennlinienfehler | 3 |
| 4 | ungültiger Empfangswert | 4 |
| 5 | keine Drehzahl trotz Ansteuerung (umfasst blockierten Rotor, S-4/S-7) | 5 |
| 6 | Überwachung ausgesetzt (suspendiert) | 6 (niedrigste) |

Gesendet wird der oberste anliegende Wert. Wert 6 ist zugleich die konkrete Darstellung des von
A-2 geforderten „nicht verfügbar", ohne mit `STOERUNG = 0` die Falschaussage „in Ordnung" zu machen.

**Rückfalloption**, falls später Gleichzeitigkeit gebraucht wird: ein zusätzliches KO als
16-Bit-Rohbitmaske (DPT 7.001). Jetzt **nicht** vorsehen — KO-Nummern sind im 32er-Block
reichlich, ETS-IDs aber unumkehrbar.

---

## 7. KNX-Abbildung

Alle DPTs sind im knx-Stack v2.4.0 vorhanden (in `src/knx/dpt.h` verifiziert).

### 7.1 Eingänge

| Signal | DPT | Hinweis |
|---|---|---|
| `FREIGABE` | **1.003 Enable** | zyklisch überwacht (E-1b), Sperre als persistenter Latch (Entscheidung 1) |
| `LEISTUNG_SOLL` | **5.001 Scaling** | 0–100 %, Auflösung 0,4 % |
| `RICHTUNGSART` | **5.010** als Enum | kein passender Norm-DPT; Werte dokumentieren |
| `TAKT_ZUSTAND` | **1.002 Bool** | nur Slave; reiner Zustand, keine Auf/Ab-Semantik |
| `MASTER_LEBT` | **1.011 State** | nur Slave; Watchdog-Eingang (Entscheidung 4) |
| `STOSSLUEFTUNG` | **1.010 Start/Stop** | nur Master; **nicht** 1.017 Trigger, damit `0` abbricht |
| `QUITTIERUNG` | **1.016 Ack** | exakte Semantik „Quittieren" |
| `SUSPENDIERT` | **1.003 Enable** | Laufzeit-Stilllegung, persistent (5.3) |

### 7.2 Ausgänge

| Signal | DPT | Hinweis |
|---|---|---|
| `LEISTUNG_IST` | **5.001 Scaling** | Anlaufpuls schlägt nicht durch (S-2) |
| `DREHZAHL_IST` | **7.001 Value_2_Ucount** | kein rpm-DPT in der Norm; Einheit dokumentieren |
| `VOLUMENSTROM_IST` | **13.002 FlowRate_m3_per_h** | 4 Byte **vorzeichenbehaftet** → A-4 erfüllt. Bewusst nicht 9.025: das hätte l/h geliefert und einen Faktor 1000 in jede Visualisierung getragen. Der Busvorteil von 2 Byte entfällt durch das Totband |
| `RICHTUNG_IST` | **1.002 Bool** | Zuordnung A/B dokumentieren |
| `ZYKLUS_REST` | **7.005 TimePeriodSec** | nur Master; nicht sekündlich senden |
| `LAEUFT` | **1.011 State** | |
| `IST_SUSPENDIERT` | **1.011 State** | umbenannt, siehe 5.4 |
| `STOERUNG` | **1.005 Alarm** | Sammelmeldung |
| `FEHLERCODE` | **5.010** Enum | Wertevorrat Abschnitt 6 |
| `BETRIEBSSTUNDEN` | **12.001 Value_4_Ucount** | ⚠ **nicht** 7.007: 2 Byte laufen bei 65535 h ≈ 7,5 Jahren über, bei Dauerlüftung real erreichbar |
| `MASTER_ERREICHBAR` | **1.011 State** | nur Slave |
| `TAKT_ZUSTAND` | **1.002 Bool** | nur Master, Ä-8 |
| `LEISTUNG_SOLL` (Gruppe) | **5.001 Scaling** | nur Master, Verteilung nach MA-2 |
| `RICHTUNGSART` (Gruppe) | **5.010** | nur Master, Verteilung nach MA-2 |
| `MASTER_LEBT` | **1.011 State** | nur Master, zyklisch |

### 7.3 KO-Blockgröße

**23 KOs** je Knoten sind belegt (Master und Slave teilen sich Firmware und Block, die Rolle
entscheidet über die Sichtbarkeit). Der Block ist **29 KOs je Kanal** groß, bei **2 Knoten je
Gerät**: Kanal 1 = KO 20–48, Kanal 2 = KO 49–77.

Die **6 freien Plätze liegen bewusst innerhalb des Blocks** (relative Nummern 8, 9 und 20–23),
nicht am Ende. Grund: der OpenKNXproducer leitet die Blockgröße ausschließlich aus der höchsten
benutzten `%Kn%`-Nummer ab — dokumentiert in `doc/Anleitung-OpenKNXproducer.md`, Abschnitt
KoOffset. Ein Attribut zum Setzen der Blockgröße existiert nicht; ein am `op:define`
angegebenes `KoBlockSize` wird **ohne Fehlermeldung ignoriert**. Reserve am oberen Ende ließe
sich also nur über ein Dummy-Objekt erzwingen, was kein OpenKNX-Modul tut (VirtualButton nutzt
`%K0%`–`%K11%`, THPSensor `%K0%`–`%K31%`, beide lückenlos).

Das ist eine Einbahnstraße: KO-Nummern und ETS-IDs dürfen sich innerhalb eines Update-Pfades
nie ändern. Ein neues KO gehört daher in eine der Lücken, niemals oberhalb von 28.

### 7.4 ETS-Aufbau

Ein Gerät hostet zwei Knoten → die Applikation ist **mehrkanalig**, also greift das
Kanalauswahl-Pattern: eigener Reiter „Kanalauswahl" unter „Allgemein", Spalten
Kanal / Kanaltyp / Beschreibung in 15/35/50 %, **eine Grid-Tabelle pro Kanal**. Details im
Styleguide `TAS-UP-4x-TouchRGB/doc/OpenKNX-ETS-XML-Styleguide.md`; Referenzimplementierung ist
OFM-VirtualButton (upstream `v1`).

---

## 8. Master-Slave über KNX

### 8.1 Rollenverteilung

Genau ein Knoten je Gruppe ist per Parameter Master (MA-1), keine Aushandlung. Der Master
verteilt `LEISTUNG_SOLL`, `RICHTUNGSART` und `TAKT_ZUSTAND` an die Gruppe (MA-2) und ist
Zeitreferenz (MA-4). Die Stoßlüftung ist Masterfunktion (Entscheidung 11): der Master hebt die
Gruppenvorgabe an, jeder Slave multipliziert wie immer mit seinem eigenen `ANTEILSFAKTOR` —
damit ist auch geklärt, dass die Stoßlüftungsleistung **nach** dem Anteilsfaktor wirkt, ohne
Sonderregel.

### 8.2 Überwachung

KNX kennt keinen Verbindungszustand. „Master erreichbar" ist ausschließlich über zyklisches
Senden plus Timeout darstellbar:

- Der Master sendet **`MASTER_LEBT` zyklisch**, und im **selben Zyklus** `TAKT_ZUSTAND`. Damit
  ist der Takt-Refresh gratis erledigt, ohne zusätzliche Buslast — und der ohnehin nicht
  verfügbare Leseweg (2.3) wird nicht gebraucht.
- Der Slave überwacht mit `MASTER_UEBERWACHUNGSZEIT`; Verhalten in zwei Phasen nach 2.1.
- `TAKT_ZUSTAND` wird als **Zustand** übertragen, nicht als Impuls (E-6). Ein verspätet
  zugeschalteter Knoten übernimmt mit dem nächsten Zyklus die richtige Halbwelle.

### 8.3 Buslast und Sendebedingungen

TP1 läuft mit 9600 bit/s, dauerhaft sinnvoll sind ≲ 10–20 Telegramme/s. Kritisch sind die
Analogausgänge, weil eine gemessene Drehzahl permanent jittert. Je Analogausgang
konfigurierbar:

- Senden bei Änderung um **x %** **oder** um mindestens einen **absoluten Sockelwert** (2.2)
- **Mindest-Sendeabstand** als harte Sperre gegen Dauerfeuer
- optional zyklische Wiederholung (Standard-OpenKNX-Sendebedingung)
- `ZYKLUS_REST` nur bei Richtungswechsel senden, nicht sekündlich

### 8.4 Anlauf nach Neustart und ETS-Download

- OpenKNX hat einen **Startup-Delay** (`BASE_StartupDelayBase`): während dieser Zeit läuft kein
  Lüfter an, Start erst in `processAfterStartupDelay()`.
- **Read-on-Init steht nicht zur Verfügung** (2.3). Startwerte kommen über das zyklische Senden.
- Sicherer Zustand bis dahin: Leistung 0 (M-15), Freigabe = gesperrt.
- ⚠ Nach einem ETS-Download wird der Modul-Flashbereich neu initialisiert. Der Freigabe-Latch
  (Entscheidung 1) fällt damit zurück — ein bewusster, autorisierter Eingriff, der zu
  dokumentieren, aber nicht zu verhindern ist.

### 8.5 Sicherheitshinweis

KNX ist **kein Sicherheitsbus**. E-1h benennt das richtig: für den Verbund mit einer Feuerstätte
ist üblicherweise ein fest verdrahteter, potentialfreier Kontakt gefordert; der Busweg nach
E-1b ist Ergänzung, nicht Ersatz. Das gehört in die Produktdokumentation.

---

## 9. Versionierung und ApplicationNumber

Aktueller, verifizierter Stand: `OpenKnxId 0xAF`, `ApplicationNumber 0x86`,
`ApplicationVersion 0.1`, `ReplacesVersions 0.1`, beide `library.json` auf `0.1`.

Die Neuentwicklung bricht das KO-Layout. Sauber wäre eine **neue ApplicationNumber**; sie wird
noch ausgehandelt (Entscheidung 8) und ist vor dem ersten externen Test umzustellen. Bis dahin
verhindert die Selbstreferenz in `ReplacesVersions` (2.4), dass die ETS Bestandsanlagen eine
Migration auf ein inkompatibles Layout anbietet.

---

## 10. Noch offene Detailfragen

Nicht blockierend, entscheidbar während der Implementierung.

| Nr. | Frage | Bezug |
|---|---|---|
| O-1 | Ist `TOTZEIT` Teil der `ZYKLUSZEIT` oder kommt sie obendrauf? Realistisch liegt sie bei **unter 2 s** (Lüfter mit integriertem ESC), bei 70 s Zyklus also unter 3 % Unterschied in der Förderleistung — eher eine Frage der Sauberkeit als eine mit Wirkung. Wertebereich des Parameters entsprechend klein auslegen: Sekunden, nicht Zehnersekunden | Abschnitt 2 der Anforderungen |
| O-2 | Fällt `STOERUNG` bei Ping-Rückkehr bzw. zurückkehrenden Tacho-Pulsen selbsttätig, oder braucht es eine `QUITTIERUNG`? | S-9, 2.1 |
| O-3 | Sockelwert des Totbands: fester Wert oder Parameter? | 2.2 |
| O-4 | Ungültige `RICHTUNGSART`-Codes (3…255): abweisen und melden, oder klemmen? | Entscheidung 2, `FEHLERCODE = 4` |
| O-5 | Vorgabewert und Grenzen der `MASTER_UEBERWACHUNGSZEIT` (Empfehlung ≤ eine Zykluszeit) | 2.1 |
| O-6 | Sollen Master und Slave im **selben Gerät** geräteintern gekoppelt werden, statt Telegramme über den Bus zu sich selbst zu schicken? | 8.1 |
| O-7 | Grammatik und Stabilität des ConfigTransfer-Formats klären, falls das Begleit-Tool gebaut wird | 4.4 |
