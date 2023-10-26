#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <mysql.h>
#include "protocole.h" // contient la cle et la structure d'un message

int idQ;

ARTICLE articles[10];
int nbArticles = 0;

int fdWpipe;
int pidClient;
int okko;
int j = 0;

char mot[20];


MYSQL* connexion;

void handlerSIGALRM(int sig);

int main(int argc,char* argv[])
{
  // Masquage de SIGINT
  sigset_t mask;
  sigaddset(&mask,SIGINT);
  sigprocmask(SIG_SETMASK,&mask,NULL);

  // Armement des signaux
  // TO DO
    struct sigaction A;
    A.sa_handler = handlerSIGALRM;
    sigemptyset(&A.sa_mask);
    A.sa_flags = 0;

    if (sigaction(SIGALRM, &A, NULL) == -1)
    {
      perror ("Erreur de Sigaction\n");
      exit (1);
    }

  // Recuperation de l'identifiant de la file de messages
  fprintf(stderr,"(CADDIE %d) Recuperation de l'id de la file de messages\n",getpid());
  idQ = msgget(CLE,0);
  if (idQ == -1)
  {
    fprintf(stderr,"(CADDIE) Erreur de msgget");
    exit(1);
  }

  // Connexion à la base de donnée
  connexion = mysql_init(NULL);
  if (mysql_real_connect(connexion,"localhost","Student","PassStudent1_","PourStudent",0,0,0) == NULL)
  {
    fprintf(stderr,"(SERVEUR) Erreur de connexion à la base de données...\n");
    exit(1);  
  }


  MESSAGE m;
  MESSAGE reponse;
  
  char requete[200];
  char newUser[20];
  char maqu[20];
  char Prix[20];
  MYSQL_RES  *resultat;
  MYSQL_ROW  Tuple;


  // Récupération descripteur écriture du pipe
  fdWpipe = atoi(argv[1]);
  
  while(1)
  {
    alarm(60);

    if (msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long),getpid(),0) == -1)
    {
      perror("(CADDIE) Erreur de msgrcv");
      exit(1);
    }
    alarm(0);

    switch(m.requete)
    {
      case LOGIN :    // TO DO
                      fprintf(stderr,"(CADDIE %d) Requete LOGIN reçue de %d\n",getpid(),m.expediteur);
                      pidClient = m.expediteur;
                      break;

      case LOGOUT :   // TO DO
                      fprintf(stderr,"(CADDIE %d) Requete LOGOUT reçue de %d\n",getpid(),m.expediteur);

                      exit(0);

                      break;

      case CONSULT :  // TO DO
      
                      fprintf(stderr,"(CADDIE %d) Requete CONSULT reçue de %d\n",getpid(),m.expediteur);

                          /* Comment savoir si c'est une requete vers Acces ou vers le client ?
                                - Le PID de l'expéditeur vaut 1 : C'est une DEMANDE
                                - Le PID ne vaut pas 1 c'est une réponse d'Acces*/

                      if (m.expediteur == 1)
                      {
                          // On va pipe le message vers Access

                        m.expediteur = getpid();
                        write(fdWpipe, &m, sizeof(MESSAGE));

                      }
                      else
                      {
                          // On va envoyer le message vers le client + le prévenir.

                        m.type = pidClient;
                        m.expediteur = getpid();
                        if ((okko = atoi(m.data3)) > 0);
                          {
                            if(msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long),0) == -1)
                            {
                              perror("(Caddie) Erreur de msgsnd");
                              msgctl(idQ,IPC_RMID,NULL);
                              exit(1);
                            }
                            kill(pidClient, SIGUSR1);
                          }


                      }

                      
                      break;

      case ACHAT :    // TO DO
                      fprintf(stderr,"(CADDIE %d) Requete ACHAT reçue de %d\n",getpid(),m.expediteur);

                          /* Comment savoir si c'est une requete vers Acces ou vers le client ?
                                - Le PID de l'expéditeur vaut 1 : C'est une DEMANDE
                                - Le PID ne vaut pas 1 c'est une réponse d'Acces*/

                      if (m.expediteur == 1)
                      {
                          // On va pipe le message vers Access

                        m.expediteur = getpid();
                        write(fdWpipe, &m, sizeof(MESSAGE));

                      }
                      else
                      {
                          // On va envoyer le message vers le client + le prévenir.

                        m.type = pidClient;
                        m.expediteur = getpid();
                        m.requete = ACHAT;



                          if (atoi(m.data3) != 0 && nbArticles < 10)
                          {

                              articles[nbArticles].id = m.data1;
                              strcpy(articles[nbArticles].intitule, m.data2);
                              articles[nbArticles].prix = m.data5;
                              articles[nbArticles].stock = atoi(m.data3);
                              strcpy(articles[nbArticles].image, m.data4);
  
                              nbArticles ++;

                              if(msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long),0) == -1)
                              {
                                perror("(Caddie) Erreur de msgsnd");
                                msgctl(idQ,IPC_RMID,NULL);
                                exit(1);
                              }

                          }
                          else  // L'achat ne s'est pas effectué
                          {
                              strcpy(m.data3, "0");
                              if(msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long),0) == -1)
                              {
                                perror("(Caddie) Erreur de msgsnd");
                                msgctl(idQ,IPC_RMID,NULL);
                                exit(1);
                              }
                          }
                          kill(pidClient, SIGUSR1);

                      }
                      break;

      case CADDIE :   // TO DO
                      fprintf(stderr,"(CADDIE %d) Requete CADDIE reçue de %d\n",getpid(),m.expediteur);

                     while (j < nbArticles)
                      {
                        reponse.type = pidClient;
                        reponse.expediteur = getpid();
                        reponse.requete = CADDIE;
                              reponse.data1 = articles[j].id;
                              strcpy(reponse.data2,articles[j].intitule);
                              reponse.data5 = articles[j].prix;
                              sprintf (reponse.data3, "%d", articles[j].stock);
                              strcpy(reponse.data4, articles[j].image);

                          if(msgsnd(idQ,&reponse,sizeof(MESSAGE)-sizeof(long),0) == -1)
                          {
                            perror("(Caddie) Erreur de msgsnd");
                            msgctl(idQ,IPC_RMID,NULL);
                            exit(1);
                          }
                          kill(pidClient, SIGUSR1);

                          j++;
                      };
                      j = 0;
                      break;

      case CANCEL :   // TO DO
                      fprintf(stderr,"(CADDIE %d) Requete CANCEL reçue de %d\n",getpid(),m.expediteur);

                      // on transmet la requete à AccesBD

                      if (m.expediteur == 1)
                      {
                          // On va pipe le message vers Access
                        reponse.expediteur = getpid();
                        reponse.data1 = articles[m.data1].id;
                        reponse.requete = CANCEL;
                        sprintf (reponse.data2, "%d", articles[m.data1].stock); 

                        write(fdWpipe, &reponse, sizeof(MESSAGE));

                      }

                      // Suppression de l'aricle du panier

                      for (m.data1; m.data1 < nbArticles; m.data1++)
                      {
                        articles[m.data1].id = articles[(m.data1 + 1)].id;
                        strcpy(articles[m.data1].intitule ,articles[(m.data1 + 1)].intitule);
                        articles[m.data1].prix = articles[(m.data1 + 1)].prix;
                        articles[m.data1].stock = articles[(m.data1 + 1)].stock;
                        strcpy(articles[m.data1].image ,articles[(m.data1 + 1)].image);
                      }

                      nbArticles--;


                      break;

      case CANCEL_ALL : // TO DO
                      fprintf(stderr,"(CADDIE %d) Requete CANCEL_ALL reçue de %d\n",getpid(),m.expediteur);

                      // On envoie a AccesBD autant de requeres CANCEL qu'il y a d'articles dans le panier
                      if (m.expediteur == 1)
                      {
                          // On va pipe le message vers Access autant de fois que d'articles
                        for (int i = 0; i < nbArticles; i++)
                        {
                          reponse.expediteur = getpid();
                          reponse.data1 = articles[i].id;
                          reponse.requete = CANCEL;
                          sprintf (reponse.data2, "%d", articles[i].stock); 

                          write(fdWpipe, &reponse, sizeof(MESSAGE));
                        }
                        nbArticles = 0;

                      }

                      // On vide le panier
                      break;

      case PAYER :    // TO DO
                      fprintf(stderr,"(CADDIE %d) Requete PAYER reçue de %d\n",getpid(),m.expediteur);

                      // On vide le panier
                      nbArticles = 0;
                      break;
    }
  }
}

void handlerSIGALRM(int sig)
{
  fprintf(stderr,"(CADDIE %d) Time Out !!!    !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n",getpid());
MESSAGE reponse;
  // Annulation du caddie et mise à jour de la BD


  // On envoie a AccesBD autant de requetes CANCEL qu'il y a d'articles dans le panier

      for (int i = 0; i < nbArticles; i++)
      {
      reponse.expediteur = getpid();
      reponse.data1 = articles[i].id;
      reponse.requete = CANCEL;
      sprintf (reponse.data2, "%d", articles[i].stock); 

      write(fdWpipe, &reponse, sizeof(MESSAGE));
      }

  // Envoi d'un Time Out au client (s'il existe toujours)
    reponse.type = pidClient;
    reponse.expediteur = getpid();
    reponse.requete = TIME_OUT;
    if(msgsnd(idQ,&reponse,sizeof(MESSAGE)-sizeof(long),0) == -1)
    {
    perror("(Caddie) Erreur de msgsnd");
    msgctl(idQ,IPC_RMID,NULL);
    exit(1);
    }

    kill (pidClient, SIGUSR1);
         
  exit(0);
}