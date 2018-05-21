Notes version Serveur
===

J'ai fini la version Serveur/Console. J'ai respecté les consignes suivantes :

Serveur
---
* Le serveur limite une conversation à un nombre de participants,   
il reporte les autres connexions jusqu'au départ de l'un d'eux.
* Les messages sont émis à des tailles fixes et tronqués si nécessaire.
* Les déconnexions sont traitées.
* Le serveur ne dépend d'aucun participant pour s'exécuter.
* Le serveur connaît le nom des deux fichiers de communication d'un client.
* Le serveur gère le client "fin" et le ctrl-c.

Client
---
* Un client se connecte au serveur en envoyant son pseudo.
* Les messages lus au clavier sont tronqués s'ils sont supérieurs à la taille  
d'un message.
* Un message spécifique de déconnexion permet de quitter proprement la  
conversation ("au revoir").
* Le ctrl-c est géré, il ferme proprement la conversation.

L'architecture de mon programme est exactement celle du squelette fourni.  
J'ai implanté uniquement une fonction qui permet de tronquer un message source  
dans chaîne destination. Ainsi, elle copie l'intégralité du message si la taille  
est inférieur à celle de destination, sinon la tronque à la taille indiquée.  
Voici son en-tête :  
`char *strntrunc(char *msg, const char *texte, size_t len) `
