/* version 0 (PM, 16/4/17) :
	Le serveur de conversation
	- crée un tube (fifo) d'écoute (avec un nom fixe : ./ecoute)
	- gère un maximum de maxParticipants conversations :
		* accepter les demandes de connexion tube d'écoute) de nouveau(x) participant(s)
			 si possible
			-> initialiser et ouvrir les tubes de service (entrée/sortie) fournis
				dans la demande de connexion
		* messages des tubes (fifo) de service en entrée
			-> diffuser sur les tubes de service en sortie
	- détecte les déconnexions lors du select
	- se termine à la connexion d'un client de pseudo "fin"
	Protocole
	- suppose que les clients ont créé les tube d'entrée/sortie avant
		la demande de connexion, nommés par le nom du client, suffixés par _C2S/_S2C.
	- les échanges par les tubes se font par blocs de taille fixe, dans l'idée d'éviter
	  le mode non bloquant.
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#include <stdbool.h>
#include <sys/stat.h>

#define NBPARTICIPANTS 5 	/* seuil au delà duquel la prise en compte de nouvelles
								 connexions sera différée */
#define TAILLE_MSG 128		/* nb caractères message complet (nom+texte) */
#define TAILLE_NOM 25		/* nombre de caractères d'un pseudo */
#define NBDESC FD_SETSIZE-1	/* pour un select éventuel
								 (macros non definies si >= FD_SETSIZE) */
#define TAILLE_TUBE 512		/*capacité d'un tube */

typedef struct ptp {
    bool actif;
    char nom [TAILLE_NOM];
    int in;		/* tube d'entrée */
    int out;	/* tube de sortie */
} participant;

typedef struct dde {
    char nom [TAILLE_NOM];
} demande;

static const int maxParticipants = NBPARTICIPANTS+1+TAILLE_TUBE/sizeof(demande);

participant participants [NBPARTICIPANTS+sizeof(char)1+TAILLE_TUBE/sizeof(demande)]; //maxParticipants
char buf[TAILLE_TUBE];
int nbactifs = 0;

void effacer(int i) {
    participants[i].actif = false;
    bzero(participants[i].nom, TAILLE_NOM*sizeof(char));
    participants[i].in = -1;
    participants[i].out = -1;
}

void diffuser(char *dep) {
  int i = 0;
  while (!strcmp(participants[i].nom, dep)) i++;

}

void desactiver (int p) {
/* traitement d'un participant déconnecté */
for (i=0; i<maxParticipants; i++) {
    effacer(i);
}
}

void ajouter(char *dep) {
/*  Pré : nbactifs < maxParticpants
	Ajoute un nouveau participant de pseudo dep.
	Si le participant est "fin", termine le serveur.
*/
  if (!strcmp(dep,"fin")) {
    int i = 0;
    char path[strlen(dep)+4] = dep;
    while (participants[i].actif == true) i++;
    participants[i].actif = true;participants[i].nom = dep;
    participants[i].nom = dep;
    participants[i].in = pen(strcat(path,));
  }
  else {

  }

}

int main (int argc, char *argv[]) {
    int i,nlus,necrits,res;
    int ecoute;		/* descripteur d'écoute */
    fd_set readfds; /* ensemble de descripteurs écoutés par un select éventuel */
    char * buf0;   /* pour parcourir le contenu d'un tube, si besoin */
    char pseudo[TAILLE_NOM];

    /* création (puis ouverture) du tube d'écoute */
    mkfifo("./ecoute",S_IRUSR|S_IWUSR); // mmnémoniques sys/stat.h: S_IRUSR|S_IWUSR = 0600
    ecoute=open("./ecoute",O_RDONLY);

    FD_ZERO(&readfds);
    FD_SET(ecoute, &readfds);

    for (i=0; i<maxParticipants; i++) {
        effacer(i);
    }

    while (true) {
      printf("participants actifs : %d\n",nbactifs);
		/* boucle du serveur : traiter les requêtes en attente
			sur le tube d'écoute et les tubes d'entrée
		*/
      res = select(NBDESC, &readfds, NULL, NULL)
      if (res != -1) {
          if (FD_ISSET(ecoute, &readfds) != 0) {
            nlus = read(ecoute, buf0, TAILLE_TUBE);
            ajouter(strncpy(pseudo, buf0, nlus));
            nbactifs++;
            write(open(pseudo+"_s2c"))
          }
      }
    }
}
