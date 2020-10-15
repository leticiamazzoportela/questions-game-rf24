#include <SPI.h>
#include <RF24.h>

String radioId = "P1"; /* Identifies P1 */
String target = "P2"; /* Identifies the target */
String protocolId = "CAP"; /* Identifies the protocol called CAP - Controlled Access Protocol */
char response[64];

RF24 radio(7, 8);
const uint64_t pipes[2] = { 0xF0F0F0F0D2LL, 0xF0F0F0F0E1LL };

String selectedCard, selectedTip;

void setup() {
  Serial.begin(115200);

  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.setAutoAck(false);
  radio.setChannel(1);
  radio.openWritingPipe(pipes[0]);
  radio.openReadingPipe(1, pipes[1]);
  radio.startListening();
}

void loop() {
  managePackets();
  delay(5000);
}

/*
   Manages the received packets
*/
void managePackets() {
  radio.startListening();
  delay(100);

  while (radio.available()) { /* Checks whether there are bytes available to be read */
    radio.read(&response, sizeof(response)); /* Then, read the available payload */
  }

  String formattedResponse = String(response);
  String packet;

  if (formattedResponse.endsWith(protocolId)) {
    String t = formattedResponse.substring(2, 4); // Extract the target

    if (formattedResponse.startsWith("AP") && t.compareTo(radioId) == 0) {
      String message = formattedResponse.substring(4, formattedResponse.length() - 3); // Extract the message of the packet
      
      if (message.compareTo("st") == 0) { // If receives the message to start, this player select a card
        Serial.println("**** Select a card between 1 and 3: ");
        while (Serial.available() == 0) {}

        String index = Serial.readString();
        Serial.println(index);

        selectedCard = "c" + String(index.toInt() - 1);
        packet = buildPacket(selectedCard, target);

        checkForInterference();
        sendPacket(packet);
      } else if (message.startsWith("nt")) { // If receives the new tip message, this player select a tip
        String sc = "ca" + message.substring(message.length() - 1);

        Serial.print("**** Select a tip between 1 and 4: ");
        while (Serial.available() == 0) {}

        String index = Serial.readString();
        Serial.println(index);

        selectedTip = "t" + String(index.toInt());
        packet = buildPacket(selectedTip + sc, target);

        checkForInterference();
        sendPacket(packet);
      } else if (message.startsWith("0")) { // In this case, a tip will be display
        String tip = message.substring(1, message.length()); // Extract the tip text
        Serial.print("**** Tip: ");
        Serial.println(tip);
        delay(1000);

        Serial.println("**** Who is the person? ****");
        while (Serial.available() == 0) {}

        String answer = Serial.readString();
        answer.replace("\n", "");
        packet = buildPacket("1" + answer, target);

        checkForInterference();
        sendPacket(packet);
      } else if (message.startsWith("sr")) { // The score will be display
        String score;

        if (message.length() < 4) {
          score = message.substring(message.length() - 1);
        } else {
          score = message.substring(message.length() - 2, message.length() - 1);
        }

        Serial.print("**** Your score: ");
        Serial.println(score);
      } else {
        Serial.println(message);
      }
    } else {
      Serial.print("**** The packet will be discarted: ");
      Serial.println(formattedResponse);
      delay(1000);
    }
  } else {
    delay(1000);
  }
}

/*
   Build a packet with origin `radioId` and a `target`,
   where a content will be taken through a protocol `protocolId`
*/
String buildPacket(String content, String target) {
  return radioId + target + content + protocolId;
}

/*
   Check for interference in the channel
*/
void checkForInterference() {
  do {
    radio.startListening();
    delay(500);
    radio.stopListening();
  } while (radio.testCarrier());
}

/*
   Radio sends a packet
*/
void sendPacket(String packet) {
  radio.stopListening(); /* Stop listening, then messages can be sent */

  Serial.print("**** P1 is sending the packet: ");
  Serial.println(packet);
  delay(500);

  radio.startWrite(packet.c_str(), packet.length(), false); /* write packet */
  delay(500);
  radio.startListening();
  delay(1000);
}
