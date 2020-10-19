#include <SPI.h>
#include <RF24.h>

String radioId = "P2"; /* Identifies P2 */
String target = "P1"; /* Identifies the target */
String protocolId = "CAP"; /* Identifies the protocol called CAP - Controlled Access Protocol */
char response[64];

RF24 radio(7, 8);
const uint64_t pipes[2] = { 0xF0F0F0F0D2LL, 0xF0F0F0F0E1LL };
String selectedCard, selectedTip;

struct card {
  String person;
  String tipA;
  String tipB;
  String tipC;
} cards[] = {
  {
    "Ada Lovelace",
    "I'm the first programmer in history",
    "I was a mathematician and writer",
    "I worked with Charles Babbage"
  },
  {
    "Radia Perlman",
    "I created the STP",
    "I was called the mother of the internet",
    "I created the TRILL protocol",
  },
  {
    "Grace Hopper",
    "I created the first compiler",
    "I contributed to the creation of the Cobol language",
    "I made the term bug popular",
  },
};

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

String getTipText(String index, String selectedCard) {
  if (index == "1") {
    return cards[selectedCard.toInt()].tipA;
  } else if (index == "2") {
    return cards[selectedCard.toInt()].tipB;
  }
  return cards[selectedCard.toInt()].tipC;
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
  
  if (formattedResponse.endsWith(protocolId)) {
    String t = formattedResponse.substring(2, 4); // Extract the target
    
    if (formattedResponse.startsWith("AP") && t.compareTo(radioId) == 0) {
      String message = formattedResponse.substring(4, formattedResponse.length() - 3); // Extract the message of the packet

      if (message.compareTo("st") == 0) { // If receives the message to start, this player select a card
        Serial.println("\n**** Select a card between 1 and 3: ");
        while (Serial.available() == 0) {}

        String index = Serial.readString();
        Serial.println(index);

        selectedCard = "c" + String(index.toInt() - 1);
        String packet = buildPacket(selectedCard, target);
        sendPacket(packet);
      } else if (message.startsWith("nt")) { // If receives the new tip message, this player select a tip
        String sc = "ca" + message.substring(4, 5);
  
        Serial.print("\n**** Select a tip between 1 and 3: ");
        while (Serial.available() == 0) {}

        String index = Serial.readString();
        Serial.println(index);

        selectedTip = "t" + String(index.toInt());
        String packet = buildPacket(selectedTip + sc, target);
        sendPacket(packet);
      } else if (message.startsWith("t")) { // The answering player selected a tip, so, build packet with the tip that will be shown
        Serial.println("\n**** Sending the tip...");
        String packet = buildPacket("0" + message, target);
        sendPacket(packet);
      } else if (message.startsWith("0")) { // In this case, a tip will be display
        String index = message.substring(2, 3); // Extract the tip index
        String sc = message.substring(5, 6); // Extract the card index
        String tip = getTipText(index, sc);

        Serial.print("\n**** Tip: ");
        Serial.println(tip);
        delay(2000);

        Serial.println("\n**** Who is the person? ****");
        while (Serial.available() == 0) {}

        String answer = Serial.readString();
        Serial.println(answer);
        answer.replace("\n", "");
        String packet = buildPacket("1" + answer, target);
        sendPacket(packet);
      } else if (message.startsWith("sr")) { // The score will be display
        String score;

        if (message.length() < 4) {
          score = message.substring(message.length() - 1);
        } else {
          score = message.substring(message.length() - 2, message.length() - 1);
        }

        Serial.print("\n**** Your score: ");
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
    Serial.println("**** The received packet is from another network ****");
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
  checkForInterference(); // First, check for interference
  
  radio.stopListening(); /* Stop listening, then messages can be sent */

  Serial.print("**** P2 is sending the packet: ");
  Serial.println(packet);
  delay(500);

  radio.startWrite(packet.c_str(), packet.length(), false); /* write packet */
  delay(500);
  radio.startListening();
  delay(3000);
}
