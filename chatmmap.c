/* version 0.1 (PM, 23/4/17) :
	La discussion est un tableau de messages, couplé en mémoire partagée.
	Un message comporte un auteur, un texte et un numéro d'ordre (croissant).
	Le numéro d'ordre permet à chaque participant de détecter si la discussion a évolué
	depuis la dernière fois qu'il l'a affichée.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h> /* définit mmap  */
#include <signal.h>

#define TAILLE_AUTEUR 25
#define TAILLE_TEXTE 128
#define TAILLE_TUBE 512
#define NB_LIGNES 20

/* message : numéro d'ordre, auteur (25 caractères max), texte (128 caractères max) */
struct message {
  char auteur [TAILLE_AUTEUR];
  char texte [TAILLE_TEXTE];
};

/* discussion (20 derniers messages) */
struct message *discussion;

/* afficher la discussion */
void afficher() {
	int i;
	for (i=0; i<NB_LIGNES; i++) {
		printf("[%s] : %s\n", discussion[i].auteur, discussion[i].texte);
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

/* traitant : rafraichir la discussion, s'il y a lieu */
void traitant (int sig) {
	afficher();
  alarm(1);
}

int main (int argc, char *argv[]) {
	struct message m;
	int i,taille,fdisc;
 	char fin;
  char buf[TAILLE_TUBE];

	if (argc > 3) {
		printf("usage: %s <discussion> <participant>\n", argv[0]);
		exit(1);
	}

  signal(SIGALRM, traitant);
  alarm(1);

	 /* ouvrir et coupler discussion */
	if ((fdisc = open (argv[1], O_RDWR | O_CREAT, 0666)) == -1) {
		printf("erreur ouverture discussion\n");
		exit(2);
	}

  strncpy(m.auteur, argv[2],TAILLE_AUTEUR);

	/*	mmap ne spécifie pas quel est le resultat d'une ecriture *apres* la fin d'un
		fichier couple (SIGBUS est une possibilite, frequente). Il faut donc fixer la
		taille du fichier destination à la taille du fichier source *avant* le couplage.
		On utilise ici lseek (a la taille du fichier source) + write d'un octet,
		qui sont deja connus.
	*/
  fin = '\0';
	taille = sizeof(struct message)*NB_LIGNES;
 	lseek (fdisc, taille, SEEK_SET);
 	write (fdisc, &fin, sizeof fin);
  discussion = mmap(0,taille,PROT_READ|PROT_WRITE, MAP_SHARED, fdisc, 0);

 	/* à compléter : saisie des messages, gestion de la discussion*/
  for(;;) {
    read(0, buf, TAILLE_TUBE);
    strntrunc(m.texte, buf, TAILLE_TEXTE);
    for (i=0; i < NB_LIGNES-1; i++)
      discussion[i] = discussion[i+1];
    discussion[NB_LIGNES-1] = m;
  }

	close(fdisc);
 	exit(0);
}
