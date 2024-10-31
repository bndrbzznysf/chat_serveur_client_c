/* SERVEUR. Lancer ce programme en premier (pas d'arguments). */
#include <stdio.h>                  /* fichiers d'en-tête classiques */
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/select.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/socket.h>             /* fichiers d'en-tête "réseau" */
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT_SERVEUR 5015           /* Numéro de port pour le serveur */
#define MAX_CLIENTS   128           /* Nombre maximum de clients */
#define BUFFER_SIZE  1024           /* Taille maximum des messages */


int main(int argc, char *argv[]) {

  int sservice[MAX_CLIENTS] = {0};
  int nbClients = 0;
  char tampon[BUFFER_SIZE];
  fd_set ensemble, temp;

  /* 1. On déroute les signaux */
  signal(SIGPIPE, SIG_IGN);  //Pour la déconnexion du client

  /* 2. On crée la socket d'écoute. */
  int secoute = socket(AF_INET, SOCK_STREAM, 0);
  if (secoute == -1){
    perror("Error socket()");
    return 1;
  }

  /* 3. On prépare l'adresse du serveur. */
  struct sockaddr_in saddr = {0};
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(PORT_SERVEUR);
  saddr.sin_addr.s_addr = htonl(INADDR_ANY);

  /* 4. On attache la socket a l'adresse du serveur. */
  if (bind(secoute, (struct sockaddr*) &saddr, sizeof(saddr)) == -1){
    perror("Error bind()");
    return 1;
  }

  /* 5. Enregistrement auprès du système. */
  if (listen(secoute, 5) == -1){
    perror("Error listen()");
    return 1;
  }


  FD_ZERO(&ensemble);
  FD_SET(secoute, &ensemble);
  int max_fd = secoute;


  while (1) {

    printf("Serveur en attente de nouveaux clients ou messages.\n");
    temp = ensemble;

    if (select(max_fd + 1, &temp, NULL, NULL, NULL) == -1) {
      perror("Error select()");
      break;
    }

    /* 6. Si on a reçu une demande sur la socket d'écoute... */
    if (FD_ISSET(secoute, &temp)) {
        struct sockaddr_in caddr;
        int calen = sizeof(caddr);
        int client = accept(secoute, (struct sockaddr*) &caddr, &calen);
        
        if (client == -1) {
            perror("Error accept()");
            continue;
        }
        
        int i;
        for(int i = 0; i < MAX_CLIENTS; i++) {
          if(sservice[i] == 0) {
            sservice[i] = client;
            FD_SET(client, &ensemble);
            if (client > max_fd) {
                max_fd = client;
            }
            nbClients++;
            printf("Nouveau client connecté.\n");
            break;
          }
        }

        if(i == MAX_CLIENTS) {
          printf("Erreur : plus de place pour de nouveaux clients.\n");
          close(client);
        }
    }

    /* 7. Si on a reçu des données sur une socket de service... */
        for(int i = 0; i < MAX_CLIENTS; i++) {
            if(sservice[i] > 0 && FD_ISSET(sservice[i], &temp)) {
                memset(tampon, 0, BUFFER_SIZE);
                int lus = read(sservice[i], tampon, BUFFER_SIZE - 1);
                
                if(lus <= 0) {
                    // Client déconnecté
                    close(sservice[i]);
                    FD_CLR(sservice[i], &ensemble);
                    sservice[i] = 0;
                    nbClients--;
                    printf("Client déconnecté.\n");
                }
                else {
                    // Broadcast du message à tous les autres clients
                    printf("Message reçu : %s\n", tampon);
                    for(int j = 0; j < MAX_CLIENTS; j++) {
                        if(sservice[j] > 0 && j != i) {
                            write(sservice[j], tampon, lus);
                        }
                    }
                }
            }
        }

  }

  /* 8. On termine et libère les ressources. */
  close(secoute);
  for(int i = 0; i < MAX_CLIENTS; i++) {
      if(sservice[i] > 0) 
        close(sservice[i]);
  }

  return 0;
}
