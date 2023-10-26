#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <setjmp.h>
#include "protocole.h" // contient la cle et la structure d'un message
#include "FichierClient.h" // Contient les fonctions liés à la gestion des clients


int ret;
int idFils, idFils2, idFils3;                   //NEWWW
int idQ,idShm,idSem;
int fdPipe[2];
TAB_CONNEXIONS *tab;
char param[10];
char param2[10];
sigjmp_buf contexte;

union semun
{
int val;
struct semid_ds *buf;
unsigned short *array;
} arg;

struct sembuf operations[1];



void afficheTab();
void handlerSIGINT(int sig);
void handlerSIGCHLD(int sig2);
//void handledSIGUSR1(int sig1);

int main()
{
  // Armement des signaux
      // TO DO - Fait E2


    //Armement SIGINT

    struct sigaction A;
    A.sa_handler = handlerSIGINT;
    sigemptyset(&A.sa_mask);
    A.sa_flags = 0;

    if (sigaction(SIGINT, &A, NULL) == -1)
    {
      perror ("Erreur de Sigaction\n");
      exit (1);
    }


    //Armement SICHLD

    struct sigaction B;
    B.sa_handler = handlerSIGCHLD;
    sigemptyset(&B.sa_mask);
    B.sa_flags = 0;

    if (sigaction(SIGCHLD, &B, NULL) == -1)
    {
      perror ("Erreur de Sigaction\n");
      exit (1);
    }

  
  // Creation des ressources
  // Creation de la file de message
  fprintf(stderr,"(SERVEUR %d) Creation de la file de messages(1)\n",getpid());
  if ((idQ = msgget(CLE,IPC_CREAT | IPC_EXCL | 0600)) == -1)  // CLE definie dans protocole.h
  {
    perror("(SERVEUR) Erreur de msgget(2)\n");
    exit(1);
  }
    printf("\nidQ : %d\n", idQ);
  // TO BE CONTINUED

  // Creation memoire partagee

  if((idShm = shmget(CLE, 52, IPC_CREAT | IPC_EXCL | 0600)) == -1)    //NEWWWWWW
  {
    perror("Erreur de creation de memoire partagee\n");
    exit(1);
  }
  else
  {
    printf ("SERVEUR : Memoire partagée créer correctement\n");
  }
  printf("(SERVEUR : idShm = %d\n", idShm );

  // Creation du pipe
      // TO DO - E4
    if (pipe(fdPipe) == -1)
    {
      fprintf (stderr, "Erreur de pipe !");
      exit(1);
    }

  // Création semaphore & configuration
  idSem = semget(CLE, 1,IPC_CREAT | IPC_EXCL | 0600);
  if (idSem == -1)
  {
  perror("Erreur de semget");
  exit(1);
  }


    arg.val = 1;

    if(semctl(idSem, 0, SETVAL, arg) == -1)
    {
      fprintf (stderr, "Erreur de mise à jour du semaphore");
      exit(1);
    }
            
 

  // Initialisation du tableau de connexions
  tab = (TAB_CONNEXIONS*) malloc(sizeof(TAB_CONNEXIONS)); 

  for (int i=0 ; i<6 ; i++)
  {
    tab->connexions[i].pidFenetre = 0;
    strcpy(tab->connexions[i].nom,"");
    tab->connexions[i].pidCaddie = 0;
  }
  tab->pidServeur = getpid();
  tab->pidPublicite = 0;

  afficheTab();

  // Creation du processus Publicite (étape 2)
  // TO DO

  if((idFils = fork()) == -1)                                  //NEWWWW
  {
    perror("Erreur de fork ");
    exit(1);
  }
  tab->pidPublicite = idFils;

  if(idFils == 0)
  {
      if(execlp("Publicite", "Publicite", NULL, NULL) == -1)       //NEWWWW
      {
        perror("Erreur d execution de Publicite\n");
        exit(1);
      }
  }



  // Creation du processus AccesBD (étape 4)
  // TO DO
  if((idFils3 = fork()) == -1)                                  //NEWWWW
  {
    perror("Erreur de fork ");
    exit(1);
  }
  
  if(idFils3 == 0)
  {
      sprintf(param, "%d", fdPipe[0]);
      if(execlp("AccesBD", "AccesBD", param, NULL) == -1)       //NEWWWW
      {
        perror("Erreur d execution de Publicite\n");
        exit(1);
      }
  }
  tab->pidAccesBD = idFils3;
  MESSAGE m;
  MESSAGE reponse;

  //Le saut siglongjmp revient ici après son handler. 

  ret = sigsetjmp(contexte, 1);


  while(1)
  {

  	fprintf(stderr,"(SERVEUR %d) Attente d'une requete...\n",getpid());
    if (msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long),1,0) == -1)
    {
      perror("(SERVEUR) Erreur de msgrcv");
      msgctl(idQ,IPC_RMID,NULL);
      exit(1);
    }
    switch(m.requete)
    {
      case CONNECT : for (int i=0 ; i<6 ; i++)
                        {
                          if (tab->connexions[i].pidFenetre == 0)
                          {
                            tab->connexions[i].pidFenetre = m.expediteur;
                            i = 6;
                          }
                        }

                      fprintf(stderr,"(SERVEUR %d) Requete CONNECT reçue de %d\n",getpid(),m.expediteur);
                      break;

      case DECONNECT : // TO DO
                      for (int i=0 ; i<6 ; i++)
                        {
                          if (tab->connexions[i].pidFenetre == m.expediteur)
                          {
                            tab->connexions[i].pidFenetre = 0;
                            
                            if (tab->connexions[i].pidCaddie != 0)
                            {
                              reponse.type = tab->connexions[i].pidCaddie;
                              reponse.requete = LOGOUT;


                              if (msgsnd(idQ,&reponse,sizeof(MESSAGE)-sizeof(long),0) == -1)
                              {
                                perror("Erreur de msgsnd - 1");
                                msgctl(idQ,IPC_RMID,NULL);
                                exit(1);
                              }
                            }


                            i = 6;
                          }
                        }

                      fprintf(stderr,"(SERVEUR %d) Requete DECONNECT reçue de %d\n",getpid(),m.expediteur);
                      break;
      case LOGIN :    // TO DO -> DO OK
                  operations[0].sem_num = 0;
                  operations[0].sem_flg = IPC_NOWAIT;

                  operations[0].sem_op = -1 ; //-> oN DIMINUE DE 1 POUR LE METTRE A 0
                  if (semop(idSem, operations, 1) == -1)
                  {               // ON A PAS LE CONTROLE 
                      //***123456
                          reponse.requete = BUSY;
                          reponse.type = m.expediteur;

                          if (msgsnd(idQ,&reponse,sizeof(MESSAGE)-sizeof(long),0) == -1)
                          {
                              perror("Erreur de msgsnd - 2");
                              msgctl(idQ,IPC_RMID,NULL);
                              exit(1);
                          }
                          kill(m.expediteur, SIGUSR1);
                                 
                  }
                  else
                  {
                      if(m.data1 == 1)
                      {
                        int res = 0;

                        res = estPresent(m.data2);
                        if(res > 0)
                        {

                          reponse.data1 = 0;
                          strcpy(reponse.data4, "Client deja existant !");
                          
                        }
                        else
                        {
                          ajouteClient(m.data2, m.data3);
                          reponse.data1 = 1;
                          strcpy(reponse.data4, "Nouveau client cree : Bienvenue !");
                          
                        }
                      }
                      else
                      {
                        int pos;

                        if((pos = estPresent(m.data2)) > 0)
                        {
                          int res;

                          if((res = verifieMotDePasse(pos, m.data3)) == 1)
                          {
                            reponse.data1 = 1;
                            strcpy(reponse.data4, "Re-bonjour cher Client !");
                            
                          }

                          else
                          {
                            if(res == 0)
                            {
                              reponse.data1 = 0;
                              strcpy(reponse.data4, "Mot de passe incorrect...");
                              
                            }

                          }
                        }
                        else
                        {
                          if(pos == 0)
                          {
                            reponse.data1 = 0;
                            strcpy(reponse.data4, "Client inconnu...");
                            
                          }
                        }
                      }


                      // Envoyer le signal vers le client.

                      reponse.type = m.expediteur;
                      
                      reponse.requete = LOGIN;

                      if (msgsnd(idQ,&reponse,sizeof(reponse)-sizeof(long),0) == -1)
                        {
                          fprintf (stderr, "Erreur de msgctl - 7");
                            msgctl(idQ,IPC_RMID,NULL);
                            exit(1);
                        }
                        else
                        {
                          for (int i=0 ; i<6 ; i++)
                          {
                            if (tab->connexions[i].pidFenetre == m.expediteur && reponse.data1 == 1)
                            {
                              strcpy(tab->connexions[i].nom, m.data2);
//On lance le Caddie
                              if((idFils2 = fork()) == -1)                                  //NEWWWW
                              {
                                perror("Erreur de fork ");
                                exit(1);
                              }
                              tab->connexions[i].pidCaddie = idFils2;

                              if(idFils2 == 0)
                              {
                                sprintf (param2, "%d", fdPipe[1]);
                                if(execlp("Caddie", "Caddie", param2, NULL) == -1)       //NEWWWW
                                {
                                  perror("Erreur d execution de Caddie\n");
                                  exit(1);
                                }
                              }

                                  /*  Mnt que le caddie est lancé, on va l'informer à quel client
                                      il est lié. On lui transfère donc ce qu'on a recu (ca ne sert
                                      à rien de copié/coller vers reponse) en spécifiant le type vers
                                      l'id du caddie.
                                  */
                              m.type = tab->connexions[i].pidCaddie;
                              m.requete = LOGIN;
                              if (msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long),0) == -1)
                              {
                                  fprintf (stderr, "Erreur de msgsnd - 6");
                                  msgctl(idQ,IPC_RMID,NULL);
                                  exit(1);
                              }

                              i = 6;
                            }
                          }
                          
                          kill(m.expediteur, SIGUSR1);
                          
                        }
                    operations[0].sem_op = +1 ;
                    if (semop(idSem, operations, 1) == -1)
                    {
                        fprintf (stderr, "ON A PAS REUSSI A RENDRE \n");
                    }
                  }

                      


                      fprintf(stderr,"(SERVEUR %d) Requete LOGIN reçue de %d : --%d--%s--%s--\n",getpid(),m.expediteur,m.data1,m.data2,m.data3);
                      break; 

      case LOGOUT :   // TO DO

                  fprintf(stderr,"(SERVEUR %d) Requete LOGOUT reçue de %d\n",getpid(),m.expediteur);
                  operations[0].sem_num = 0;
                  operations[0].sem_flg = IPC_NOWAIT;

                  operations[0].sem_op = -1 ; //-> oN DIMINUE DE 1 POUR LE METTRE A 0
                  if (semop(idSem, operations, 1) == -1)
                  {               // ON A PAS LE CONTROLE 
                      //***123456
                          reponse.requete = BUSY;
                          reponse.type = m.expediteur;

                          if (msgsnd(idQ,&reponse,sizeof(MESSAGE)-sizeof(long),0) == -1)
                          {
                              perror("Erreur de msgsnd - 2");
                              msgctl(idQ,IPC_RMID,NULL);
                              exit(1);
                          }
                          kill(m.expediteur, SIGUSR1);
                                 
                  }
                  else
                  {
                      for (int i=0 ; i<6 ; i++)
                      {
                          if (tab->connexions[i].pidFenetre == m.expediteur)
                          {
                            reponse.type = tab->connexions[i].pidCaddie;
                            reponse.requete = LOGOUT;


                          if (msgsnd(idQ,&reponse,sizeof(MESSAGE)-sizeof(long),0) == -1)
                          {
                              perror("Erreur de msgsnd - 2");
                              msgctl(idQ,IPC_RMID,NULL);
                              exit(1);
                          }
                            i = 6;
                          }
                      }
                    operations[0].sem_op = +1 ;
                    if (semop(idSem, operations, 1) == -1)
                    {
                        fprintf (stderr, "ON A PAS REUSSI A RENDRE\n");
                    }

                  }

                      break;

      case UPDATE_PUB :  // TO DO
                      fprintf(stderr,"(SERVEUR %d) Requete UPDATE_PUB reçue de %d\n",getpid(),m.expediteur);
                      for (int i = 0; i < 6; i++)
                      {
                        if (tab->connexions[i].pidFenetre != 0)
                        {
                          kill(tab->connexions[i].pidFenetre, SIGUSR2);
                        }
                      }
                      break;

      case CONSULT :  // TO DO
                      fprintf(stderr,"(SERVEUR %d) Requete CONSULT reçue de %d\n",getpid(),m.expediteur);
                      operations[0].sem_num = 0;
                      operations[0].sem_flg = IPC_NOWAIT;

                      operations[0].sem_op = -1 ; //-> oN DIMINUE DE 1 POUR LE METTRE A 0
                      if (semop(idSem, operations, 1) == -1)
                      {               // ON A PAS LE CONTROLE 
                      //***123456
                          reponse.requete = BUSY;
                          reponse.type = m.expediteur;

                          if (msgsnd(idQ,&reponse,sizeof(MESSAGE)-sizeof(long),0) == -1)
                          {
                              perror("Erreur de msgsnd - 2");
                              msgctl(idQ,IPC_RMID,NULL);
                              exit(1);
                          }
                          kill(m.expediteur, SIGUSR1);
                                 
                      }
                      else
                      { 
                        for (int i = 0; i < 6; i++)
                        {
                          if (tab->connexions[i].pidFenetre == m.expediteur)
                          {
                            m.type = tab->connexions[i].pidCaddie;
                            i = 6;
                          }
                        }
                        m.requete = CONSULT;
                        m.expediteur = 1;
                        if (msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long),0) == -1)
                        {
                          perror("(Serveur) Erreur de msgsnd - 3");
                          msgctl(idQ,IPC_RMID,NULL);
                          exit(1);
                        }
                        operations[0].sem_op = +1 ;
                        if (semop(idSem, operations, 1) == -1)
                        {
                          fprintf (stderr, "ON A PAS REUSSI A RENDRE \n");
                        }
                      }

                      break;

      case ACHAT :    // TO DO
                      fprintf(stderr,"(SERVEUR %d) Requete ACHAT reçue de %d\n",getpid(),m.expediteur);
                      operations[0].sem_num = 0;
                      operations[0].sem_flg = IPC_NOWAIT;

                      operations[0].sem_op = -1 ; //-> oN DIMINUE DE 1 POUR LE METTRE A 0
                      if (semop(idSem, operations, 1) == -1)
                      {               // ON A PAS LE CONTROLE 
                      //***123456
                          reponse.requete = BUSY;
                          reponse.type = m.expediteur;

                          if (msgsnd(idQ,&reponse,sizeof(MESSAGE)-sizeof(long),0) == -1)
                          {
                              perror("Erreur de msgsnd - 2");
                              msgctl(idQ,IPC_RMID,NULL);
                              exit(1);
                          }
                          kill(m.expediteur, SIGUSR1);
                                 
                      }
                      else
                      { 
                        for (int i = 0; i < 6; i++)
                        {
                          if (tab->connexions[i].pidFenetre == m.expediteur)
                          {
                            m.type = tab->connexions[i].pidCaddie;
                            i = 6;
                          }
                        }
                        m.expediteur = 1;
                        m.requete = ACHAT;
                        if (msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long),0) == -1)
                        {
                          perror("(Serveur) Erreur de msgsnd - 4");
                          msgctl(idQ,IPC_RMID,NULL);
                          exit(1);
                        }

                        operations[0].sem_op = +1 ;
                        if (semop(idSem, operations, 1) == -1)
                        {
                          fprintf (stderr, "ON A PAS REUSSI A RENDRE\n");
                        }
                    }

                      break;

      case CADDIE :   // TO DO
                      fprintf(stderr,"(SERVEUR %d) Requete CADDIE reçue de %d\n",getpid(),m.expediteur);
                      operations[0].sem_num = 0;
                      operations[0].sem_flg = IPC_NOWAIT;

                      operations[0].sem_op = -1 ; //-> oN DIMINUE DE 1 POUR LE METTRE A 0
                      if (semop(idSem, operations, 1) == -1)
                      {               // ON A PAS LE CONTROLE 
                      //***123456
                          reponse.requete = BUSY;
                          reponse.type = m.expediteur;

                          if (msgsnd(idQ,&reponse,sizeof(MESSAGE)-sizeof(long),0) == -1)
                          {
                              perror("Erreur de msgsnd - 2");
                              msgctl(idQ,IPC_RMID,NULL);
                              exit(1);
                          }
                          kill(m.expediteur, SIGUSR1);
                                 
                      }
                      else
                      { 
                        for (int i = 0; i < 6; i++)
                        {
                          if (tab->connexions[i].pidFenetre == m.expediteur)
                          {
                            m.type = tab->connexions[i].pidCaddie;
                            m.requete = CADDIE;
                        if (msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long),0) == -1)
                        {
                            perror("(Serveur) Erreur de msgsnd - 5");
                            msgctl(idQ,IPC_RMID,NULL);
                            exit(1);
                        }

                            i = 6;
                          }
                        }
                        operations[0].sem_op = +1 ;
                        if (semop(idSem, operations, 1) == -1)
                        {
                          fprintf (stderr, "ON A PAS REUSSI A RENDRE\n");
                        }
                      }
                      break;

      case CANCEL :   // TO DO
                      fprintf(stderr,"(SERVEUR %d) Requete CANCEL reçue de %d\n",getpid(),m.expediteur);
                      operations[0].sem_num = 0;
                      operations[0].sem_flg = IPC_NOWAIT;

                      operations[0].sem_op = -1 ; //-> oN DIMINUE DE 1 POUR LE METTRE A 0
                      if (semop(idSem, operations, 1) == -1)
                      {               // ON A PAS LE CONTROLE 
                      //***123456
                          reponse.requete = BUSY;
                          reponse.type = m.expediteur;

                          if (msgsnd(idQ,&reponse,sizeof(MESSAGE)-sizeof(long),0) == -1)
                          {
                              perror("Erreur de msgsnd - 2");
                              msgctl(idQ,IPC_RMID,NULL);
                              exit(1);
                          }
                          kill(m.expediteur, SIGUSR1);
                                 
                      }
                      else
                      { 
                        for (int i = 0; i < 6; i++)
                        {
                          if (tab->connexions[i].pidFenetre == m.expediteur)
                          {
                            m.type = tab->connexions[i].pidCaddie;
                            m.expediteur = 1;
                            m.requete = CANCEL;
                            if (msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long),0) == -1)
                            {
                                perror("(Serveur) Erreur de msgsnd - 5");
                                msgctl(idQ,IPC_RMID,NULL);
                                exit(1);
                            }

                            i = 6;
                          }
                        }
                        operations[0].sem_op = +1 ;
                        if (semop(idSem, operations, 1) == -1)
                        {
                          fprintf (stderr, "ON A PAS REUSSI A RENDRE \n");
                        }
                      }
                      break;

      case CANCEL_ALL : // TO DO
                      fprintf(stderr,"(SERVEUR %d) Requete CANCEL_ALL reçue de %d\n",getpid(),m.expediteur);
                      operations[0].sem_num = 0;
                      operations[0].sem_flg = IPC_NOWAIT;

                      operations[0].sem_op = -1 ; //-> oN DIMINUE DE 1 POUR LE METTRE A 0
                      if (semop(idSem, operations, 1) == -1)
                      {               // ON A PAS LE CONTROLE 
                      //***123456
                          reponse.requete = BUSY;
                          reponse.type = m.expediteur;

                          if (msgsnd(idQ,&reponse,sizeof(MESSAGE)-sizeof(long),0) == -1)
                          {
                              perror("Erreur de msgsnd - 2");
                              msgctl(idQ,IPC_RMID,NULL);
                              exit(1);
                          }
                          kill(m.expediteur, SIGUSR1);
                                 
                      }
                      else
                      {                       
                        for (int i = 0; i < 6; i++)
                        {
                          if (tab->connexions[i].pidFenetre == m.expediteur)
                          {
                            m.type = tab->connexions[i].pidCaddie;
                            m.expediteur = 1;
                            m.requete = CANCEL_ALL;
                            if (msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long),0) == -1)
                            {
                              perror("(Serveur) Erreur de msgsnd - 5");
                              msgctl(idQ,IPC_RMID,NULL);
                              exit(1);
                            }

                            i = 6;
                          }
                          operations[0].sem_op = +1 ;
                          if (semop(idSem, operations, 1) == -1)
                          {
                            fprintf (stderr, "ON A PAS REUSSI A RENDRE\n");
                          }
                        }
                      }
                      break;

      case PAYER : // TO DO
                      fprintf(stderr,"(SERVEUR %d) Requete PAYER reçue de %d\n",getpid(),m.expediteur);
                      operations[0].sem_num = 0;
                      operations[0].sem_flg = IPC_NOWAIT;

                      operations[0].sem_op = -1 ; //-> oN DIMINUE DE 1 POUR LE METTRE A 0
                      if (semop(idSem, operations, 1) == -1)
                      {               // ON A PAS LE CONTROLE 
                      //***123456
                          reponse.requete = BUSY;
                          reponse.type = m.expediteur;

                          if (msgsnd(idQ,&reponse,sizeof(MESSAGE)-sizeof(long),0) == -1)
                          {
                              perror("Erreur de msgsnd - 2");
                              msgctl(idQ,IPC_RMID,NULL);
                              exit(1);
                          }
                          kill(m.expediteur, SIGUSR1);
                                 
                      }
                      else
                      {                       
                        for (int i = 0; i < 6; i++)
                        {
                          if (tab->connexions[i].pidFenetre == m.expediteur)
                          {
                            m.type = tab->connexions[i].pidCaddie;
                            i = 6;
                          }
                        }
                        m.expediteur = 1;
                        m.requete = PAYER;
                        if (msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long),0) == -1)
                        {
                            perror("(Serveur) Erreur de msgsnd - 4");
                            msgctl(idQ,IPC_RMID,NULL);
                            exit(1);
                        }
                          operations[0].sem_op = +1 ;
                          if (semop(idSem, operations, 1) == -1)
                          {
                            fprintf (stderr, "ON A PAS REUSSI A RENDRE\n");
                          }
                      }
                      break;

      case NEW_PUB :  // TO DO
                      fprintf(stderr,"(SERVEUR %d) Requete NEW_PUB reçue de %d\n",getpid(),m.expediteur);
                      m.type = idFils;
                      m.requete = NEW_PUB;
                      m.expediteur = 1;
                      if (msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long),0) == -1)
                      {
                          perror("(Serveur) Erreur de msgsnd - 4");
                          msgctl(idQ,IPC_RMID,NULL);
                          exit(1);
                      }
                      kill(idFils, SIGUSR1);

                      break;
    }   //fin switch

    afficheTab();


  }   //fin while



}   //fin main

void afficheTab()
{
  fprintf(stderr,"Pid Serveur   : %d\n",tab->pidServeur);
  fprintf(stderr,"Pid Publicite : %d\n",tab->pidPublicite);
  fprintf(stderr,"Pid AccesBD   : %d\n",tab->pidAccesBD);
  for (int i=0 ; i<6 ; i++)
  fprintf(stderr,"%6d -%20s- %6d\n",tab->connexions[i].pidFenetre,
                                                      tab->connexions[i].nom,
                                                      tab->connexions[i].pidCaddie);
  fprintf(stderr,"\n");
}




//Handler SIGINT

void handlerSIGINT(int sig)
{
      //On tue pub.
  kill(idFils,SIGKILL);

      // On tue AccessBD
  kill(idFils3,SIGUSR1);
  //idFils3 = wait(NULL);

      // On supprime la file
  if (msgctl(idQ,IPC_RMID,NULL) == -1)
  {
    perror("Erreur de msgctl(3)");

  }
  
    // On supprime la mémoire partagée
  if (shmctl(idShm, IPC_RMID, NULL) == -1)
    {
        perror("(SERVEUR)Erreur de shmctl(4)\n");

    }

    // On supprime le semaphore
  if (semctl(idSem, 0, IPC_RMID) == -1)
    {
        perror("(SERVEUR)Erreur de semctl(4)\n");

    }



    // On ferme le pipe
      if (close(fdPipe[1]) == -1)
  {
    printf("Erreur de fermeture en ecriture de pipe\n");
  }

  if (close(fdPipe[0]) == -1)
  {
    printf("Erreur de fermeture en lecture de pipe\n");
  }

}


//Handler SIGCHLD

void handlerSIGCHLD(int sig2)
{
  printf ("Je suis dans SIGCHLD\n\n\n\n\n\n\n");
  //Ici on elimine les fils CADDIE qui ont été exit dans le caddie.
  idFils2 = wait(NULL);

  //On supprime aussi du tableau le pid du caddie qu on vient de terminer


  for (int i = 0; i < 6; i++)
  {
    if (tab->connexions[i].pidCaddie == idFils2)
    {
      tab->connexions[i].pidCaddie = 0;
      strcpy(tab->connexions[i].nom, "\0");
      i = 6;
    }
  }




  //Maintenant, on doit revenir dans la boucle du Serveur car il doit continuer de tourner pour faire son travail

  siglongjmp(contexte, 406);
}


