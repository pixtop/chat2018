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
#include <signal.h>
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

participant participants [NBPARTICIPANTS+sizeof(char)+1+TAILLE_TUBE/sizeof(demande)]; //maxParticipants
char buf[TAILLE_TUBE];
int nbactifs = 0;

void sigint_handler(int sig) {
  unlink("./ecoute");
  printf("\n");
  exit(0);
}

void effacer(int i) {
  participants[i].actif = false;
  bzero(participants[i].nom, TAILLE_NOM*sizeof(char));
  participants[i].in = -1;
  participants[i].out = -1;
}

void diffuser(char *dep) {
  int i;
  for (i = 0; i < maxParticipants; i++)
    if (participants[i].actif == true)
      write(participants[i].out, dep, TAILLE_MSG);
}

/* traitement d'un participant déconnecté */
void desactiver (int p) {
  close(participants[p].in);
  close(participants[p].out);
  effacer(p);
  nbactifs--;
}

/*  Pré : nbactifs < maxParticpants
Ajoute un nouveau participant de pseudo dep.
Si le participant est "fin", termine le serveur.
*/
void ajouter(char *dep) {
  if (strcmp(dep,"fin") != 0) {
    int i;
    char connexion[TAILLE_MSG] = "[service] ";
    strcat(connexion, dep);
    strcat(connexion, " rejoint la conversation");
    char path[TAILLE_NOM+6] = "./";
    strcat(path, dep);
    for (i = 0; i < maxParticipants; i++) {
      if (participants[i].actif == false) {
        participants[i].actif = true;
        strcpy(participants[i].nom, dep);
        participants[i].in = open(strcat(strdup(path), "_C2S"), O_RDONLY);
        participants[i].out = open(strcat(strdup(path), "_S2C"), O_WRONLY);
        diffuser(connexion);
        nbactifs++;
        return;
      }
    }
  }
  unlink("./ecoute");
  exit(0);
}

/* Copie ou tronque le texte dans la chaine msg[len] */
char *strntrunc(char *msg, const char *texte, size_t len) {
  int i;
  for (i=0;i<len;i++) {
      if (texte[i] == '\n'||'\0') {
        msg[i]='\0';
        return strncpy(msg,texte,i);
      }
  }
  return strncpy(msg,texte,len-1);
}

int main (int argc, char *argv[]) {
  int i,nlus,res;
  int ecoute;		/* descripteur d'écoute */
  fd_set readfds; /* ensemble de descripteurs écoutés par un select éventuel */
  char msg[TAILLE_MSG];   /* pour parcourir le contenu d'un tube, si besoin */
  demande dde;
  signal(SIGINT, sigint_handler);

  /* création (puis ouverture) du tube d'écoute */
  mkfifo("./ecoute",S_IRUSR|S_IWUSR); // mmnémoniques sys/stat.h: S_IRUSR|S_IWUSR = 0600
  ecoute=open("./ecoute",O_RDONLY);

  /* initialisation de la liste des participants */
  for (i=0; i<maxParticipants; i++) effacer(i);

  while (true) {
    printf("participants actifs : %d\n", nbactifs);
  	/* boucle du serveur : traiter les requêtes en attente
  		sur le tube d'écoute et les tubes d'entrée
  	*/
    FD_ZERO(&readfds);
    if (nbactifs < NBPARTICIPANTS)
      FD_SET(ecoute, &readfds);
    for (i=0; i<maxParticipants; i++)
      if (participants[i].actif == true) FD_SET(participants[i].in, &readfds);
    if (res = select(NBDESC, &readfds, NULL, NULL, NULL) < 0) {
      printf("Erreur lors du select\n");
      exit(1);
    }
    //tube d'écoute
    if (FD_ISSET(ecoute, &readfds) != 0) {
      if (nlus = read(ecoute, buf, TAILLE_MSG) > 0)
        ajouter(strntrunc(dde.nom,buf,TAILLE_NOM));
      else {
        //cas particuliers où il n'y a plus personne dans la conversation.
        i=0;
        while (participants[i].actif != true) i++;
        desactiver(i);
        printf("participants actifs : %d\n", nbactifs);
        ecoute=open("./ecoute",O_RDONLY);
      }
    }
    //tubes d'entrée
    else {
      for (i=0; i<maxParticipants; i++) {
        if (FD_ISSET(participants[i].in, &readfds) != 0) {
          if (nlus = read(participants[i].in, buf, TAILLE_TUBE) < 1) {
            strcpy(buf, "[service] ");strcat(buf, participants[i].nom);
            strcat(buf, " quitte la conversation");
            desactiver(i);
          }
          diffuser(strntrunc(msg,buf,TAILLE_MSG));
        }
      }
    }
  }
}
