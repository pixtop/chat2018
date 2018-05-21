/* version 0 (PM, 16/4/17) :
	Le client de conversation
	- crée deux tubes (fifo) d'E/S, nommés par le nom du client, suffixés par _C2S/_S2C
	- demande sa connexion via le tube d'écoute du serveur (nom supposé connu),
		 en fournissant le pseudo choisi (max TAILLE_NOM caractères)
	- attend la réponse du serveur sur son tube _C2S
	- effectue une boucle : lecture sur clavier/S2C.
	- sort de la boucle si la saisie au clavier est "au revoir"
	Protocole
	- les échanges par les tubes se font par blocs de taille fixe TAILLE_MSG,
	- le texte émis via C2S est préfixé par "[pseudo] ", et tronqué à TAILLE_MSG caractères
Notes :
	-le  client de pseudo "fin" n'entre pas dans la boucle : il permet juste d'arrêter
		proprement la conversation.
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>

#define TAILLE_MSG 128		/* nb caractères message complet (nom+texte) */
#define TAILLE_NOM 25		/* nombre de caractères d'un pseudo */
#define NBDESC FD_SETSIZE-1  /* pour le select (macros non definies si >= FD_SETSIZE) */
#define TAILLE_TUBE 512		/*capacité d'un tube */
#define NB_LIGNES 20
#define TAILLE_SAISIE 1024

char pseudo [TAILLE_NOM];
char buf [TAILLE_TUBE];
char discussion [NB_LIGNES] [TAILLE_MSG]; /* derniers messages reçus */
char deconnexion[]="au revoir"; /* message pour se déconnecter */
char tubeC2S [TAILLE_NOM+7] = "./";	/* pour le nom du tube C2S */
char tubeS2C [TAILLE_NOM+7] = "./";	/* pour le nom du tube S2C */

void afficher(int depart) {
    int i;
    for (i=1; i<=NB_LIGNES; i++) {
        printf("%s\n", discussion[(depart+i)%NB_LIGNES]);
    }
    printf("=========================================================================\n");
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

/* fonction qui ajoute le nom en préfixe d'un messsage. */
char *prefixe(char *pfmessage, const char *message) {
  char prefixe[TAILLE_MSG] = "[";
  strcat(prefixe,pseudo);
  strcat(prefixe,"] ");
  strntrunc(pfmessage,message,TAILLE_MSG-strlen(prefixe));
  strcat(prefixe,pfmessage);
  return pfmessage = prefixe;
}

void nettoyage(const char *message) {
  unlink(tubeC2S);
  unlink(tubeS2C);
  printf("%s\n", message);
}

void sigint_handler(int sig) {
  nettoyage("\nfin client");
  exit(0);
}

int main (int argc,char const *argv[]) {
    int i,nlus,necrits;

    int ecoute, S2C, C2S;			/* descripteurs tubes */
    int curseur = 0;				/* position dernière ligne reçue */

    fd_set readfds; 				/* ensemble de descripteurs écoutés par le select */

    char saisie [TAILLE_SAISIE];
    char message [TAILLE_MSG];

    signal(SIGINT, sigint_handler);

    if (!((argc == 2) && (strlen(argv[1]) < TAILLE_NOM*sizeof(char)))) {
        printf("utilisation : %s <pseudo>\n", argv[0]);
        printf("Le pseudo ne doit pas dépasser 25 caractères\n");
        exit(1);
    }
    strncpy(pseudo, argv[1], TAILLE_NOM);

    /* ouverture du tube d'écoute */
    ecoute = open("./ecoute", O_WRONLY);
    if (ecoute == -1) {
        printf("Le serveur doit être lance, et depuis le meme repertoire que le client\n");
        exit(2);
    }

    /* création des tubes de service */
    strcat(tubeC2S, pseudo);strcat(tubeC2S, "_C2S");
    strcat(tubeS2C, pseudo);strcat(tubeS2C, "_S2C");
    mkfifo(tubeC2S, S_IRUSR|S_IWUSR);
    mkfifo(tubeS2C, S_IRUSR|S_IWUSR);

    /* connexion */
    write(ecoute, strntrunc(message,pseudo,TAILLE_MSG), TAILLE_MSG);

    if (strcmp(pseudo,"fin") != 0) {
    	/* client " normal " */
		  /* initialisations */
      C2S = open(tubeC2S, O_WRONLY);
      S2C = open(tubeS2C, O_RDONLY);
      nlus = read(S2C, buf, TAILLE_TUBE);
      strntrunc(discussion[curseur%NB_LIGNES], buf,TAILLE_MSG);
      afficher(curseur++);

      while (strncmp(saisie,deconnexion,strlen(deconnexion)-1) != 0) {
      /* boucle principale */
        FD_ZERO(&readfds);
        FD_SET(0,&readfds);
        FD_SET(S2C,&readfds);
        select(NBDESC, &readfds, NULL, NULL, NULL);
        if (FD_ISSET(0, &readfds) != 0) {
          nlus = read(0,saisie,TAILLE_SAISIE);
          necrits = write(C2S,prefixe(message,saisie),TAILLE_MSG);
        }
        if (FD_ISSET(S2C, &readfds) != 0) {
          if (nlus = read(S2C, buf, TAILLE_TUBE) > 0) {
            strntrunc(discussion[curseur%NB_LIGNES],buf,TAILLE_MSG);
            afficher(curseur++);
          }
          else {
            nettoyage("connexion interrompu");
            exit(1);
          }
        }
      }
    }
    /* nettoyage */
    nettoyage("fin client");
    exit(0);
}
