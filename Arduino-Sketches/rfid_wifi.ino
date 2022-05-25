//einbinden der Bibliotheken für das
//ansteuern des MFRC522 Moduls
#include <SPI.h>
#include <MFRC522.h>

//einbinden der Bibliothek für
//den Zugang zum WiFi Netzwerk
#include <WiFi.h>
//Bibliothek für das absenden von E-Mails
#include <ESP_Mail_Client.h>

//Zugangsdaten für das WiFi Netzwerk
#define WIFI_SSID "<ssid>"
#define WIFI_PASSWORD "<passwort>"

//Verbindungsdaten zum E-Mail Provider
#define SMTP_HOST "smtp.web.de"
#define SMTP_PORT 587
#define SENDER_EMAIL "<absender_email>"
#define SENDER_PASSWORD "<absender_passwort>"

void smtpCallback(SMTP_Status status);

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

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(200);
  }
  //Wenn die Verbindung erfolgreich aufgebaut wurde,
  //dann soll die IP-Adresse auf der seriellen Schnittstelle
  //ausgegeben werden.
  Serial.println("");
  Serial.print("Zum Wi-Fi Netzwerk ");
  Serial.print(WIFI_SSID);
  Serial.println(" verbunden.");
  Serial.print("IP-Adresse: ");
  Serial.println(WiFi.localIP());
  Serial.println();


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
    sendMail(newRfidId);
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

void sendMail(String rfidId) {
  SMTPSession smtp;
  //Debug Meldungen anzeigen
  smtp.debug(1);
  //Ein Callback definieren welcher nach dem Senden der Mail ausgeführt werden soll.
  smtp.callback(smtpCallback);
  //Mail Session
  ESP_Mail_Session session;
  //Serverdaten setzen
  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = SENDER_EMAIL;
  session.login.password = SENDER_PASSWORD;
  session.login.user_domain = "smtp.web.de";

  //Wenn keine Verbindung aufgebaut werden konnte soll die Funktion verlassen werden.
  if (!smtp.connect(&session)) {
    Serial.println("Aufbau der SMTPSession nicht erfolgreich!");
    return;
  }
  //Aufbau der E-Mail
  SMTP_Message message;
  //Im Header kann man recht einfach dem Absender Faken
  message.sender.name = "ESP32 Mailer"; //steht bei "gesendet von"
  message.sender.email = SENDER_EMAIL; //der Absender (oder eine Fake E-Mail)
  message.subject = "Achtung! unbekannter Zugriffsversuch mit RFID-ID [" + rfidId + "]!"; //der Betreff der E-Mail
  message.addRecipient("Max Mustermann", "max.mustermann@mustermann.de"); //der Empfänger
  //Aufbau des Contents der E-Mail
  String textMsg = "Es wurde versucht mit der RFID-ID " + rfidId + " den Zugang zu erlangen.";
  message.text.content = textMsg.c_str();
  //Encoding für die E-Mail
  message.text.charSet = "utf-8";
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
  message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;
  //Eine eindeutige ID welche die Mail kennzeichnet
  //zwischen den spitzen Klammern kann ein Wert xyz@domain.com eingetragen werden
  message.addHeader("Message-ID: <esp32_mailer@web.de>");
  if (!MailClient.sendMail(&smtp, &message)) {
    //Im Fehlerfall wird der Grund auf der seriellen Schnittstelle ausgegeben.
    Serial.println("Fehler beim senden der E-Mail, Grund: " + smtp.errorReason());
  }
}

void smtpCallback(SMTP_Status status) {
  ESP_MAIL_PRINTF("E-Mails erfolgreich versendet: %d\n", status.completedCount());
  ESP_MAIL_PRINTF("fehlerhafte E-Mails: %d\n", status.failedCount());
}
