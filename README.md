# Implementação de um protocolo de acesso ao meio em Arduino

Este repositório apresenta a implementação de um protocolo de acesso ao meio com controle de acesso, o qual foi desenvolvido em Arduino utilizando a biblioteca [nRF24](http://tmrh20.github.io/RF24/).

Para demonstrar o funcionamento do protocolo, foi desenvolvida uma versão simplificada do jogo *20 Questions*. Esta versão conta com 5 perguntas por carta e cada carta corresponde a uma personalidade influente da Ciência da Computação. A seguir, são apresentadas a estrutura do protocolo e as regras do jogo, respectivamente.

### Estrutura

Foram utilizadas 3 placas de rádio baseadas no Arduino Nano, sendo que:
* 2 placas correspondem aos jogadores;
* 1 placa corresponde ao AP (*Access Point*), ela é a responsável por gerenciar a comunicação entre os jogadores, isto é, ela é quem destina os pacotes de um jogador para outro;
* O AP conhece o endereço de todas as placas;
* Um jogador não envia um pacote diretamente para outro jogador, como mencionado anteriormente, o pacote deve passar pelo AP e o AP encaminha o pacote para o destino;
* A relação do protocolo com o jogo está documentada no código;
* Podem ser enviados diferentes tipos de pacotes e cada um possui algumas sinalizações que podem ser interpretadas:
  * `st` *(start token)*: AP sinaliza para um jogador que ele pode começar escolhendo uma carta.
  * `cnumero`: Indica que um jogador escolheu uma carta, o *numero* corresponde ao número da carta;
  * `nt` *(new tip)*: Pacote enviado para um jogador para ele saber que pode enviar uma dica;
  * `tnumero`: Indica que um jogador escolheu uma dica, o *numero* corresponde ao número da dica;
  * `0mensagem`: Indica que uma dica deve ser exibida;
  * `1mensagem`: Indica que um jogador enviou a resposta para uma pergunta, onde *mensagem* corresponde à resposta;
  * `srnumero`: Indica que uma pontuação deve ser exibida, onde *numero* é o valor da pontuação do jogador.
* Um pacote tem o seguinte formato:
  * `origin + target + content + protocol`
* Assim, considerando as sinalizações mencionadas anteriormente, um exemplo de pacote seria:
  * `P2P1t1ca1CAP`: P2 solicita a P1 a dica 1 (t1) da carta 1 (ca1) por meio do protocolo CAP.

### Regras do jogo

A aplicação que faz uso do protocolo de acesso ao meio é baseada no jogo *20 Questions*, também conhecido como *Perfil*. Ela funciona da seguinte forma:

* O jogo é composto por 5 "cartas", onde cada carta possui 4 dicas;
* O jogador 1 escolhe uma carta;
* O jogador 2 escolhe uma dica da carta;
* Após o jogador 1 apresentar a dica, o jogador 2 dá um palpite de pessoa;
  * Se o jogador 2 acertar, os papéis se invertem, então, o jogador 2 passa a perguntar e o jogador 1 tenta acertar quem é a pessoa;
  * Se o jogador 2 não acertar, ele continua escolhendo dicas e o jogador 1 apresentando-as, até elas acabarem ou o jogador 2 acertar;
* Se o jogador que está tentando acertar não conseguir adivinhar quem é a pessoa após as 4 dicas, o jogador que estava perguntando ganha 4 pontos;
* Se o jogador conseguir acertar quem é a pessoa, então ele pontua conforme o número de dicas que usou. Exemplo: usou 3 dicas de 4, então ele faz 2 pontos, 2 dicas = 3 pontos, 1 dica = 4 pontos.
* O jogo termina quando um jogador marcar 12 pontos;
* Se as cartas acabarem, vence o jogador com maior pontuação.
