#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <signal.h>
#include <string.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include "protocole.h" // contient la cle et la structure d'un message
#include <errno.h>

int idQ, idShm;
char *pShm;                         //pointeur sur memoire partagee (pour attachement processus)
void handlerSIGUSR1(int sig);
char pub[51];

int fd;
MESSAGE m;
sigjmp_buf contexte;
int ret;

int main()
{

  // Armement des signaux
          // TO DO
    struct sigaction A;
    A.sa_handler = handlerSIGUSR1;
    sigemptyset(&A.sa_mask);
    A.sa_flags = 0;

    if (sigaction(SIGUSR1, &A, NULL) == -1)
    {
      perror ("Erreur de Sigaction\n");
      exit (1);
    }

  // Masquage des signaux
  sigset_t mask;
  sigfillset(&mask);
  sigdelset(&mask,SIGUSR1);
  sigprocmask(SIG_SETMASK,&mask,NULL);

  // Recuperation de l'identifiant de la file de messages
  fprintf(stderr,"(PUBLICITE %d) Recuperation de l'id de la file de messages\n",getpid());

  if ((idQ = msgget(CLE,0)) == -1)
  {
    perror("(PUBLICITE) Erreur de msgget");
    exit(1);
  }

  // Recuperation de l'identifiant de la mémoire partagée
  idShm = shmget(CLE, 52, 0);
  if(idShm == -1)             //NEWWWWWW
  {
    perror("Erreur de recuperation de memoire partagee");
    exit(1);
  }


  // Attachement à la mémoire partagée
  pShm = (char*)shmat(idShm, NULL, 0);


  // Mise en place de la publicité en mémoire partagée

  strcpy(pub,"Bienvenue sur le site du M Sebarby en ligne !");



  // retour du saut apres handler SIGUSR1
  ret = sigsetjmp(contexte, 1);


  for (int i=0 ; i<=50 ; i++) 
    pShm[i] = ' ';
  pShm[51] = '\0';
  int indDebut = 25 - strlen(pub)/2;
  for (int i=0 ; i<strlen(pub) ; i++) 
    pShm[indDebut + i] = pub[i];

  
  MESSAGE m;          //NEWWW  creation structure pour envoi mise a jour au serveur (toutes les secondes)
  while(1)
  {
    // Envoi d'une requete UPDATE_PUB au serveur
    m.type = 1;                 // a destination du serveur   NEWWW
    m.expediteur = getpid();
    m.requete = UPDATE_PUB;


    if (msgsnd(idQ,&m,sizeof(m)-sizeof(long),0) == -1)
    {
      perror("Erreur d envoi de la mise a jour au serveur.");
      exit(1);
    }
  

    sleep(1); 

    // Decallage vers la gauche
    char prov[1];
    prov[0] = pShm[0];     // Copie du 1er element

    for (int i = 0; i < 50; i++)
    {
        pShm[i] = pShm[i+1];  // Remplace le 1er et déccalle tout d'un.
    }
    pShm[50] = prov[0]; // Le premier devient le dernier
    //strcpy(pub,"Bienvenue sur le site du M Se12345 en ligne !");
    //strcpy(pShm, pub);
  }
}




void handlerSIGUSR1(int sig)
{
  fprintf(stderr,"(PUBLICITE %d) Nouvelle publicite !\n",getpid());


  // Lecture message NEW_PUB
  if (msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long),0,0) == -1)
  {
    perror("(PUB) Erreur de msgrcv - 4");
    msgctl(idQ,IPC_RMID,NULL);
    exit(1);
   }

  // Mise en place de la publicité en mémoire partagée

   for (int i = 0; i < 52; i++)
   {
    pub[i] = m.data4[i];
   }
   siglongjmp(contexte, 406);
}
