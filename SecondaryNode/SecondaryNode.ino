#include <SPI.h>
#include <RF24.h>

String radioId = "P2"; /* Identifies node2 */
String target = "P1"; /* Identifies the target */
String protocolId = "CAP"; /* Identifies the protocol called CAP - Controlled Access Protocol */
char response[32];

RF24 radio(7, 8);
const uint64_t pipes[2] = { 0xF0F0F0F0D2LL, 0xF0F0F0F0E1LL };

String selectedCard, selectedTip;

/*
   Build a package with origin `radioId` and a `target`,
   where a request will be taken through a protocol `protocolId`
*/
String buildPackage(String request, String target) {
  return radioId + target + request + protocolId ;
}

/*
   Radio sends a packet
*/
void sendPackage(String package) {
  radio.stopListening(); /* Stop listening, then messages can be sent */
  delay(1000);
  Serial.print("**** P2 is sending the packet: ****");
  delay(1000);
  Serial.println(package);
  delay(1000);

  radio.startWrite(package.c_str(), package.length(), false); /* write packet */
  delay(100);
  radio.startListening();
}

/*
   Checks if it has packets to route, if so,
   forwards them to the destination
*/
void routePackage() {
  radio.startListening();
  delay(100);

  while (radio.available()) { /* Verifies whether there are bytes available to be read */
    radio.read(&response, sizeof(response)); /* Then, read the available payload */
  }
  delay(1000);

  String formattedResponse = String(response);
  String package;

  if (formattedResponse.endsWith(protocolId)) {
    if (formattedResponse.startsWith("AP")) {
      String message = formattedResponse.substring(4, formattedResponse.length() - 2); // Extract the message of the package

      if (message.compareTo("st") == 0) { // If receives the message to start, this player select a card
        Serial.println("**** Select a card between 1 and 5: ");
        int index = Serial.read();
        selectedCard = "c" + String(index - 1);
        package = buildPackage(selectedCard, target);
      } else if (message.compareTo("nt") == 0) {
        Serial.println("**** Select a tip between 1 and 4: ");
        int index = Serial.read();
        selectedTip = "t" + String(index - 1);
        package = buildPackage(selectedTip, target);
      } else if (message.startsWith("0")) {
        String tip = formattedResponse.substring(1, formattedResponse.length() - 1); // Extract the tip text
        Serial.print("**** Tip: ");
        Serial.println(tip);
        delay(1000);
        Serial.println("**** Who is the person? ****");
        String answer = Serial.readString();
        package = buildPackage(answer, target);
      } else if (message.compareTo("nt") == 0) {
        Serial.println("**** YOOOOU WIIIIIN! ****");
      }

      checkForInterference();
      sendPackage(package);
    } else {
      Serial.print("**** Received Packet ****");
      delay(1000);
      Serial.println(response);
      delay(1000);
    }
  } else {
    Serial.println("**** The received packet is from another network ****");
    delay(1000);
  }
}

/*
   Check for interference in the channel
*/
void checkForInterference() {
  do {
    radio.startListening();
    delay(128);
    radio.stopListening();
  } while (radio.testCarrier());
}

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
  routePackage();
  delay(5000);
}
