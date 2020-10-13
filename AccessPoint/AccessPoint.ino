#include <SPI.h>
#include <RF24.h>

String radioId = "AP"; /* Identifies Access Point */
String players[2] = {"P1", "P2"}; /* Identifies the players */
String protocolId = "CAP"; /* Identifies the protocol called CAP - Controlled Access Protocol */
char response[64];

RF24 radio(7, 8);
const uint64_t pipes[2] = { 0xF0F0F0F0D2LL, 0xF0F0F0F0E1LL };

int p1Score = 0, p2Score = 0, finish = 11, numberOfTips = 0;
String playerWithCard = players[0], selectedCard, selectedTip;
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
      String package = buildPackage("st", players[0]); // Sends to player the start token (st)
      checkForInterference();
      sendPackage(package);
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
    if ((millis() - timer) > 15000) { /* Checks for a while that no package has arrived */
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

void getNextStep(String package) {
  if (p1Score >= finish) {
    package = buildPackage("yw", players[0]);
  } else if (p2Score >= finish) {
    package = buildPackage("yw", players[1]);
  } else {
    String answeringPlayer = getAnsweringPlayer();
    playerWithCard = answeringPlayer; // Changes the player with card
    package = buildPackage("st", playerWithCard);
  }
  sendPackage(package);
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
  String package = response;
  String answeringPlayer = getAnsweringPlayer();

  if (formattedResponse.endsWith(protocolId)) {
    String message = formattedResponse.substring(4, formattedResponse.length() - 2); // Extract the message of the package

    if (message.startsWith("c")) { // A player select a card with tips
      selectedCard = message.substring(message.length() - 1);
      package = buildPackage("nt", answeringPlayer);
      sendPackage(package);
    } else if (message.startsWith("t")) { // A player select a tip
      String index = message.substring(message.length() - 2, message.length() - 1);
      selectedTip = getTipText(index, selectedCard);

      numberOfTips += 1;
      package = buildPackage("0" + selectedTip, answeringPlayer);
      sendPackage(package);
    } else if (message.startsWith("1")) { // A player sends the answer
      String answer = message.substring(1, message.length() - 1); // Extract the answer
      String person = cards[selectedCard.toInt()].person;

      if (answer.compareTo(person) == 0) { // If it is the correct answer
        if (answeringPlayer == players[0]) { // The answering player is the one who scores
          p1Score += (4 - numberOfTips);
        } else {
          p2Score += (4 - numberOfTips);
        }

        getNextStep(package);
      } else { // If it not is the correct answer
        if (numberOfTips > 4) {
          if (playerWithCard == players[0]) { // The player with card is the one who scores
            p1Score += 4;
          } else {
            p2Score += 4;
          }

          getNextStep(package);
        } else {
          String score = "nt";

          if (answeringPlayer == players[0]) {
            score += p1Score;
          } else {
            score += p2Score;
          }

          package = buildPackage(score, answeringPlayer);
          sendPackage(package);
        }
      }
    }
  } else {
    Serial.println("**** The received packet is from another network ****");
  }
}

/*
   Build a package with origin `radioId` and a `target`,
   where a request will be taken through a protocol `protocolId`
*/
String buildPackage(String request, String target) {
  return radioId + target + request + protocolId;
}

/*
   Radio sends a packet
*/
void sendPackage(String package) {
  radio.stopListening(); /* Stop listening, then messages can be sent */

  Serial.print("**** AP is sending the packet: ");
  Serial.println(package);
  delay(500);

  radio.startWrite(package.c_str(), package.length(), false); /* write packet */
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
