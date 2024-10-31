/* CLIENT. Donner l'adresse IP et un argv[2] en paramètre */

#include <stdio.h>             /* fichiers d'en-tête classiques */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/select.h>
#include <errno.h>

#include <sys/socket.h>        /* fichiers d'en-tête "réseau" */
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT_SERVEUR 5015      /* Numéro de port pour le serveur */
#define BUFFER_SIZE  1024      /* Taille maximum des tampons */


int main(int argc, char *argv[]) {

	if (argc != 3) {
		perror("Utilisation : ./chat_client IP_serveur argv[2]");
		return 1;
	}

	char tampon[BUFFER_SIZE];
    fd_set ensemble, temp;


	/* 1. On crée la socket du client. */
	int sclient = socket(AF_INET, SOCK_STREAM, 0);
	if (sclient==-1) {
		perror("Error socket()");
		return 1;
	}

	/* 2. On prépare l'adnbLusse du serveur. */
	struct sockaddr_in sa = {0};
	sa.sin_family = AF_INET;
	sa.sin_port = htons(PORT_SERVEUR);
	sa.sin_addr.s_addr = inet_addr(argv[1]);


	/* 3. On demande une connexion au serveur, tant qu'on y arrive pas. */
	while ( connect(sclient, (struct sockaddr*)&sa, sizeof(sa)) == -1 );
	printf("Client de chat ouvert :\n");


	/* 4. On communique. */
    printf("%s > ", argv[2]);
    fflush(stdout);

    FD_ZERO(&ensemble); 
    FD_SET(0,&ensemble);
    FD_SET(sclient,&ensemble);

    while (1) {
        temp = ensemble;
        if (select(sclient + 1, &temp, NULL, NULL, NULL) == -1) {
            perror("Error select()");
            break;
        }
        
        //Envoi de messages
        if (FD_ISSET(0, &temp)) {
            memset(tampon, 0, BUFFER_SIZE);
            strcpy(tampon, argv[2]);
            strcat(tampon, " : ");

            int lus = read(0, tampon + strlen(tampon), BUFFER_SIZE - strlen(tampon) - 1);
            if (lus > 0) {
                if (write(sclient, tampon, lus + strlen(tampon)) == -1) {
                    perror("Error write()");
                    break;
                }
                printf("%s > ", argv[2]);
                fflush(stdout);
            }
            else break;
        }


		//Réception de messages
        if (FD_ISSET(sclient, &temp)) {
            int lus = read(sclient, tampon, BUFFER_SIZE - 1);
            if (lus > 0) {
                printf("\n%s", tampon);
                fflush(stdout);
                printf("%s > ", argv[2]); 
                fflush(stdout); 
            }
            else {
                printf("Déconnexion du serveur.\n");
                break;
            }
        }
    }


	/* 5. On termine et libère les ressources. */
	shutdown(sclient,SHUT_RDWR);
	close(sclient);

	return 0;
}
