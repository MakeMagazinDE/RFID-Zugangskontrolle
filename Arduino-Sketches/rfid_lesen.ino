//einbinden der Bibliotheken für das
//ansteuern des MFRC522 Moduls
#include <SPI.h>
#include <MFRC522.h>

//definieren der Pins  RST & SDA für den ESP32
#define RST_PIN     22
#define SS_PIN      21

//erzeugen einer Objektinstanz
MFRC522 mfrc522(SS_PIN, RST_PIN);

//Variable zum speichern der bereits gelesenen RFID-ID
String lastRfid = "";

void setup() {
  //beginn der seriellen Kommunikation mit 115200 Baud
  Serial.begin(115200);
  //eine kleine Pause von 50ms.
  delay(50);
  //begin der SPI Kommunikation
  SPI.begin();
  //initialisieren der Kommunikation mit dem RFID Modul
  mfrc522.PCD_Init();
}

void loop() {
  //Wenn keine neue Karte vorgehalten wurde oder die serielle Kommunikation
  //nicht gegeben ist, dann...
  if ( !mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  if (!mfrc522.PICC_ReadCardSerial()) {
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
    Serial.print(" gelesene RFID-ID :");
    Serial.println(newRfidId);
    Serial.println();
  }
}
