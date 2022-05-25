//einbinden der Bibliotheken für das
//ansteuern des MFRC522 Moduls
#include <SPI.h>
#include <MFRC522.h>

//definieren der Pins  RST & SDA für den ESP32
#define RST_PIN     22
#define SS_PIN      21

#define ledRot 32
#define ledGruen 33

#define buzzer 25
const int channel = 0;
const int frequenz = 2000;
const int resolutionBits = 12;
const int toneFrequenzFail = 600;
const int toneFrequenzNoTone = 0;

#define relay 12

//erzeugen einer Objektinstanz
MFRC522 mfrc522(SS_PIN, RST_PIN);

//Variable zum speichern der bereits gelesenen RFID-ID
String lastRfid = "";

//Anzahl der zulässigen RFID-IDs im Array
const int NUM_RFIDS = 1;
//Array mit Vorname, Name, RFID-IDs welche zulässig sind
String rfids[NUM_RFIDS] = {" 39 42 FF 97"};

void setup() {
  //beginn der seriellen Kommunikation mit 115200 Baud
  Serial.begin(115200);
  //eine kleine Pause von 50ms.
  delay(50);

  pinMode(ledRot, OUTPUT);
  pinMode(ledGruen, OUTPUT);

  pinMode(buzzer, OUTPUT);
  pinMode(relay, OUTPUT);

  //begin der SPI Kommunikation
  SPI.begin();
  //initialisieren der Kommunikation mit dem RFID Modul
  mfrc522.PCD_Init();

  ledcSetup(channel, frequenz, resolutionBits);
  ledcAttachPin(buzzer, channel);

}

void loop() {
  //Wenn keine neue Karte vorgehalten wurde oder die serielle Kommunikation
  //nicht gegeben ist, dann...
  if ( !mfrc522.PICC_IsNewCardPresent()) {
    //Serial.println("!PICC_IsNewCardPresent");
    return;
  }

  if (!mfrc522.PICC_ReadCardSerial()) {
    //Serial.println("!PICC_ReadCardSerial");
    return;
  }


  String newRfidId = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    newRfidId.concat(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    newRfidId.concat(String(mfrc522.uid.uidByte[i], HEX));
  }

  //alle Buchstaben in Großbuchstaben umwandeln
  newRfidId.toUpperCase();

  //Wenn die neue gelesene RFID-ID ungleich der bereits zuvor gelesenen ist,
  //dann soll diese auf der seriellen Schnittstelle ausgegeben werden.
  if (!newRfidId.equals(lastRfid)) {
    //überschreiben der alten ID mit der neuen
    lastRfid = newRfidId;
  }


  bool zugangOk = false;
  //prüfen ob die gelesene RFID-ID im Array mit bereits gespeicherten IDs vorhanden ist
  for (int i = 0; i < NUM_RFIDS; i++) {
    //wenn die ID an der Stelle "i" im Array "rfids" mit dem gelesenen übereinstimmt, dann
    if (rfids[i].equals(newRfidId)) {
      zugangOk = true;
      //Schleife verlassen
      break;
    }
  }

  //Wenn die Variable "zugangOk" auf True ist, dann...
  if (zugangOk) {
    activateRelay();
    blinkLed(ledGruen);
    deactivateRelay();
    Serial.println("RFID-ID [" + newRfidId + "] OK!, Zugang wird gewährt");
  } else {
    //Wenn nicht dann...
    digitalWrite(ledRot, HIGH);
    playSound();
    Serial.println("RFID-ID [" + newRfidId + "] nicht OK!, Zugang wird nicht gewährt");
    digitalWrite(ledRot, LOW);
  }

  Serial.println();
}

void activateRelay() {
  digitalWrite(relay, HIGH);
}

void deactivateRelay() {
  digitalWrite(relay, LOW);
}

void playSound() {
  ledcWriteTone(channel, toneFrequenzFail);
  delay(250);
  ledcWriteTone(channel, toneFrequenzNoTone);
}

//Blinken einer LED am Pin "pin"
void blinkLed(int pin) {
  //Schleife von 0 bis 5
  for (int a = 0; a < 5; a++) {
    //LED aktivieren
    digitalWrite(pin, HIGH);
    //eine Pause von 125 ms.
    delay(125);
    //LED deaktivieren
    digitalWrite(pin, LOW);
    //eine Pause von 125 ms.
    delay(125);
  }
}