#include <SPI.h>
#include <RF24.h>

String radioId = "AP"; /* Identifies Access Point */
String players[2] = {"P1", "P2"}; /* Identifies the players */
String protocolId = "CAP"; /* Identifies the protocol called CAP - Controlled Access Protocol */
char response[32];

RF24 radio(7, 8);
const uint64_t pipes[2] = { 0xF0F0F0F0D2LL, 0xF0F0F0F0E1LL };

int p1Score = -1, p2Score = -1, finish = 11, numberOfTips = 0;
String playerWithCard = players[0], selectedCard, selectedTip;
struct card {
  String person;
  String tipA;
  String tipB;
  String tipC;
  String tipD;
} cards[] = {
  {
    "Ada Lovelace",
    "Sou considerada a primeira programadora da história",
    "Nasci em dezembro de 1815",
    "Também fui matemática e escritora",
    "Trabalhei com Charles Babbage"
  },
  {
    "Alan Turing",
    "Trabalhei para a inteligência britânica na Segunda Guerra",
    "Fui uma grande influência da Ciência da Computação",
    "Projetei a máquina de Turing",
    "Nasci em junho de 1912"
  },
  {
    "Radia Perlman",
    "Criei o protocolo STP",
    "Já fui chamada de mãe da internet",
    "Criei o protocolo TRILL",
    "Recebi o prêmio National Inventors Hall of Fame em 2016"
  },
  {
    "Bill gates",
    "Sou um das pessoas mais ricas do mundo",
    "Fundei uma famosa empresa de software em parceria com Paul Allen",
    "Criei uma fundação de caridade com minha esposa",
    "Estudei em Harvard"
  },
  {
    "Grace Hopper",
    "Criei o primeiro compilador",
    "Contribuí para a criação da linguagem Cobol",
    "Graças a mim, o termo bug se popularizou",
    "Fui analista de sistemas  da marinha dos EUA"
  },
};

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

  Serial.print("AP is sending the packet: ");
  delay(100);
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
  while (radio.available()) { /* Checks whether there are bytes available to be read */
    radio.read(&response, sizeof(response)); /* Then, read the available payload */
  }
  delay(1000);

  String formattedResponse = String(response);
  String package = response;
  String answeringPlayer;

  if (playerWithCard == players[0]) {
        answeringPlayer = players[1];
      } else {
        answeringPlayer = players[0];
      }

  if (formattedResponse.endsWith(protocolId)) {
    String message = formattedResponse.substring(4, formattedResponse.length() - 2); // Extract the message of the package

    if (message.startsWith("c")) { // A player select a card with tips
      selectedCard = message.substring(message.length() - 1);
    } else if (message.startsWith("t")) { // A player select a tip
      String index = message.substring(message.length() - 1);

      if (index == "1") {
        selectedTip = cards[selectedCard.toInt()].tipA;
      } else if (index == "2") {
        selectedTip == cards[selectedCard.toInt()].tipB;
      } else if (index == "3") {
        selectedTip = cards[selectedCard.toInt()].tipC;
       } else if (index == "4") {
        selectedTip= cards[selectedCard.toInt()].tipD;
      }

      numberOfTips += 1;
      package = buildPackage("0" + selectedTip, answeringPlayer);
    } else if (message.startsWith("a")) { // A player sends the answer
      String answer = formattedResponse.substring(1, formattedResponse.length() - 1); // Extract the answer

      if (answer.compareTo(selectedCard) == 0) { // If it is the correct answer
        if (answeringPlayer == players[0]) { // The answering player is the one who scores
          p1Score += (4 - numberOfTips);
        } else {
          p2Score += (4 - numberOfTips);
        }

        if (p1Score == finish) {
          package = buildPackage("yw", players[0]);
        } else if (p2Score == finish) {
          package = buildPackage("yw", players[1]);
        } else {
          playerWithCard = answeringPlayer; // Changes the player with card
          package = buildPackage("st", playerWithCard);
        }
      } else { // If it not is the correct answer
        if (numberOfTips > 4) {
          if (playerWithCard == players[0]) { // The player with card is the one who scores
            p1Score += 4;
          } else {
            p2Score += 4;
          }
        
          if (p1Score == finish) {
            package = buildPackage("yw", players[0]);
          } else if (p2Score == finish) {
            package = buildPackage("yw", players[1]);
          } else {
            playerWithCard = answeringPlayer; // Changes the player with card
            package = buildPackage("st", playerWithCard);
          }
        } else {
          package = buildPackage("nt", answeringPlayer);
        }
      }
    }

    sendPackage(package);
  } else {
    Serial.println("**** The received packet is from another network ****");
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
    if ((millis() - timer) > 5000) { /* Checks for a while that no package has arrived */
      Serial.println("**** Timeout: No packets to route ****");
      return false;
    }
  }

  return true;
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

/*
   Manage the communication network between AP and players
*/
void manageNetwork() {
//  Serial.println("**** Welcome to the game 4 QUESTIONS! ****");
//  Serial.println("** You have 4 chances to guess the person **");

  for (int no = 0; no < 2; no++) {
    Serial.print(players[no]);
    Serial.println(" It's your turn");
    playerWithCard  = players[no];
    String package = buildPackage("st", players[no]); // Sends to player the start token (st)
    
    // Try sends the token 5 times
    for (int retry = 0; retry < 5; retry++) {
      Serial.print("\n**** Attempt nº: ");
      Serial.println(retry);
      delay(1000);

      if (package.startsWith("AP")) {
        Serial.println("a");
      }

      checkForInterference();
      sendPackage(package);

      if (hasReceivedPackets()) {
        routePackage();
        break;
      }
    }
  }
}

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
