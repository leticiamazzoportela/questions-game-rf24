#include <SPI.h>
#include <RF24.h>

String radioId = "AP"; /* Identifies Access Point */
String players[2] = {"P1", "P2"}; /* Identifies the players */
String protocolId = "CAP"; /* Identifies the protocol called CAP - Controlled Access Protocol */
char response[64];

RF24 radio(7, 8);
const uint64_t pipes[2] = { 0xF0F0F0F0D2LL, 0xF0F0F0F0E1LL };

int p1Score = 0, p2Score = 0, finish = 12, numberOfTips = 0;
String playerWithCard = players[0], selectedCard, selectedTip, sc;
//struct card {
//  String person;
//  String tipA;
//  String tipB;
//  String tipC;
//  String tipD;
//} cards[] = {
//  {
//    "Ada Lovelace",
//    "Sou considerada a primeira programadora da historia",
//    "Nasci em dezembro de 1815",
//    "Tambem fui matematica e escritora",
//    "Trabalhei com Charles Babbage"
//  },
//  {
//    "Radia Perlman",
//    "Criei o protocolo STP",
//    "Já fui chamada de mae da internet",
//    "Criei o protocolo TRILL",
//    "Recebi o premio National Inventors Hall of Fame em 2016"
//  },
//  {
//    "Grace Hopper",
//    "Criei o primeiro compilador",
//    "Contribuí para a criação da linguagem Cobol",
//    "Graças a mim, o termo bug se popularizou",
//    "Fui analista de sistemas  da marinha dos EUA"
//  },

struct card {
  String person;
  String tipA;
  String tipB;
  String tipC;
  String tipD;
} cards[] = {
  {
    "Ad",
    "S",
    "N",
    "T",
    "Tr"
  },
  {
    "Ra",
    "C",
    "J",
    "C",
    "R"
  },
  {
    "Gr",
    "Cr",
    "C",
    "G",
    "F"
  },
};

void setup() {
  Serial.begin(115200);

  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.setAutoAck(false);
  radio.setChannel(1);
  radio.openWritingPipe(pipes[1]);
  radio.openReadingPipe(1, pipes[0]);
}

void loop() {
  manageNetwork();
  delay(5000);
}

/*
   Manage the communication network between AP and players
*/
void manageNetwork() {
  // Try sends the token 5 times
  for (int attempt = 0; attempt < 15; attempt++) {
    Serial.print("\n**** Attempt nº: ");
    Serial.println(attempt);
    delay(1000);

    if (hasReceivedPackets()) {
      routePackets();
      break;
    } else {
      playerWithCard = players[0];
      String packet = buildPacket("st", players[0]); // Sends to player the start token (st)
      checkForInterference();
      sendPacket(packet);
      break;
    }
  }
}

/*
   Check received packets
   return true if it has packets, false otherwise
*/
bool hasReceivedPackets() {
  radio.startListening();
  unsigned long timer = millis();

  while (!radio.available()) {
    if ((millis() - timer) > 15000) { /* Checks for a while that no packet has arrived */
      Serial.println("**** Timeout: No packets to route ****");
      return false;
    }
  }

  return true;
}

String getAnsweringPlayer() {
  if (playerWithCard == players[0]) {
    return players[1];
  }

  return players[0];
}

String getTipText(String index, String selectedCard) {
  if (index == "1") {
    return cards[selectedCard.toInt()].tipA;
  } else if (index == "2") {
    return cards[selectedCard.toInt()].tipB;
  } else if (index == "3") {
    return cards[selectedCard.toInt()].tipC;
  }
  return cards[selectedCard.toInt()].tipD;
}

void routeWinnerLosePackets(String winner, String loser) {
  numberOfTips = 0;

  String winPacket = buildPacket("YOOOOU WIIIN!!!", winner);
  sendPacket(winPacket);

  String losePacket = buildPacket("YOOOOU LOOOSE =(", loser);
  sendPacket(losePacket);
}

void getNextStep(String packet) {
  if (p1Score >= finish) {
    routeWinnerLosePackets(players[0], players[1]);
  } else if (p2Score >= finish) {
    routeWinnerLosePackets(players[1], players[0]);
  } else {
    String answeringPlayer = getAnsweringPlayer();
    playerWithCard = answeringPlayer; // Changes the player with card
    numberOfTips = 0;
    packet = buildPacket("st", playerWithCard);
    sendPacket(packet);
  }
}

/*
   Checks if it has packets to route, if so,
   forwards them to the destination
*/
void routePackets() {

  while (radio.available()) { /* Checks whether there are bytes available to be read */
    radio.read(&response, sizeof(response)); /* Then, read the available payload */
  }
  delay(500);

  String formattedResponse = String(response);
  String packet = response;
  String answeringPlayer = getAnsweringPlayer();

  if (formattedResponse.endsWith(protocolId)) {
    String message = formattedResponse.substring(4, formattedResponse.length() - 3); // Extract the message of the packet

    if (message.startsWith("c")) { // A player select a card with tips
      selectedCard = message.substring(message.length() - 1);
      String cardPacket = "ca" + selectedCard;
      packet = buildPacket("nt" + cardPacket, answeringPlayer); // Identifies new tip and the card id

      sendPacket(packet);
    } else if (message.startsWith("t")) { // A player select a tip
      String index = message.substring(message.length() - 4, message.length() - 3);
      sc = message.substring(message.length() - 1);
      selectedTip = getTipText(index, sc);

      numberOfTips += 1;
      packet = buildPacket("0" + selectedTip, answeringPlayer);
      sendPacket(packet);
    } else if (message.startsWith("1")) { // A player sends the answer
      String answer = message.substring(1, message.length()); // Extract the answer
      String person = cards[sc.toInt()].person;

      if (answer.compareTo(person) == 0) { // If it is the correct answer
        String content = "sr";

        if (answeringPlayer == players[0]) { // The answering player is the one who scores
          p1Score += (5 - numberOfTips);
          content += p1Score;
        } else {
          p2Score += (5 - numberOfTips);
          content += p2Score;
        }

        String scorePacket = buildPacket(content, answeringPlayer);
        sendPacket(scorePacket); // Send to answering player your score

        getNextStep(packet);
      } else { // If it not is the correct answer
        if (numberOfTips > 4) {
          String content = "sr";

          if (playerWithCard == players[0]) { // The player with card is the one who scores
            p1Score += 4;
            content += p1Score;
          } else {
            p2Score += 4;
            content += p1Score;
          }

          String scorePacket = buildPacket(content, answeringPlayer);
          sendPacket(scorePacket); // Send to player with card your score

          getNextStep(packet);
        } else {
          String score = "ntca" + selectedCard; // Identifies new tip and the card id

          if (answeringPlayer == players[0]) {
            score += p1Score;
          } else {
            score += p2Score;
          }

          packet = buildPacket(score, answeringPlayer);
          sendPacket(packet);
        }
      }
    }
  } else {
    Serial.println("**** The received packet is from another network ****");
  }
}

/*
   Build a packet with origin `radioId` and a `target`,
   where a request will be taken through a protocol `protocolId`
*/
String buildPacket(String request, String target) {
  return radioId + target + request + protocolId;
}

/*
   Radio sends a packet
*/
void sendPacket(String packet) {
  radio.stopListening(); /* Stop listening, then messages can be sent */

  Serial.print("**** AP is sending the packet: ");
  Serial.println(packet);
  delay(500);

  radio.startWrite(packet.c_str(), packet.length(), false); /* write packet */
  delay(500);
  radio.startListening();
  delay(5000);
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
