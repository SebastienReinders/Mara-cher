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
#include <string>
using namespace std;

#include "protocole.h" // contient la cle et la structure d'un message

void handlerSIGUSR1(int sig);

  MYSQL_RES  *resultat;
  MYSQL_ROW  Tuple;
  MYSQL* connexion;
  char requete[200];
  int nbArticles = 0;
  int ret = 0;
  int idQ;
  int qudemande;
  int qudispo;
  int newqu;
  char Prix[20];
  char NewPrix[20];
  int senti = 0;
  float TestPrix = -1;


int main(int argc,char* argv[])
{
  // Masquage de SIGINT
  sigset_t mask;
  sigaddset(&mask,SIGINT);
  sigprocmask(SIG_SETMASK,&mask,NULL);

    struct sigaction A;
    A.sa_handler = handlerSIGUSR1;
    sigemptyset(&A.sa_mask);
    A.sa_flags = 0;

    if (sigaction(SIGUSR1, &A, NULL) == -1)
    {
      perror ("Erreur de Sigaction\n");
      exit (1);
    }

  // Recuperation de l'identifiant de la file de messages
  fprintf(stderr,"(ACCESBD %d) Recuperation de l'id de la file de messages\n",getpid());
  if ((idQ = msgget(CLE,0)) == -1)
  {
    perror("(ACCESBD) Erreur de msgget");
    exit(1);
  }

  // Récupération descripteur lecture du pipe
  int fdRpipe = atoi(argv[1]);

  // Connexion à la base de donnée
  // TO DO
    connexion = mysql_init(NULL);
  if (mysql_real_connect(connexion,"localhost","Student","PassStudent1_","PourStudent",0,0,0) == NULL)
  {
    fprintf(stderr,"(SERVEUR) Erreur de connexion à la base de données...\n");
    exit(1);  
  }

  MESSAGE m;
  MESSAGE reponse;

  int l = 0;

  while(1)
  {
    // Lecture d'une requete sur le pipe
         // TO DO - E4 
    
    ret = read(fdRpipe, &m, sizeof(MESSAGE));

    switch(m.requete)
    {
      case CONSULT :  // TO DO
                      fprintf(stderr,"(ACCESBD %d) Requete CONSULT reçue de %d\n",getpid(),m.expediteur);
                      // Acces BD

                      // Preparation de la reponse

                      sprintf(requete,"select * from UNIX_FINAL where id = %d", m.data1);
                      if (mysql_query(connexion, requete) != 0)
                      {
                        fprintf (stderr, "Erreur de Mysql-query");
                      }

                      if((resultat = mysql_store_result(connexion)) == NULL)
                      {
                        fprintf (stderr,"Erreur de mysql store");
                      }

                      if ((Tuple = mysql_fetch_row(resultat)) != NULL)
                      {

                            //Données "systeme" :
                        reponse.type = m.expediteur;
                        reponse.expediteur = getpid();
                        reponse.requete = CONSULT;
   
                            //Données utiles :
                        reponse.data1 = atoi(Tuple[0]);
                        strcpy(reponse.data2, Tuple[1]);
                        strcpy(reponse.data4, Tuple[4]);
                        strcpy(reponse.data3, Tuple[3]);


                        /*on recupere le prix de la bd dans un tuple au format char.
                        on veut renvoyer ce prix au client au format float.*/
// Début du code qui marche ici mais pas au gérant
                        
                        strcpy(Prix, Tuple[2]);
                        string tmp4(Prix);
                        size_t x = tmp4.find(",");
                        if (x != string::npos) tmp4.replace(x,1,".");
                        
                        reponse.data5 = atof(tmp4.c_str());


             // Envoi de la reponse au bon caddie
                          if(msgsnd(idQ,&reponse,sizeof(MESSAGE)-sizeof(long),0) == -1)
                          {
                            perror("(AccesBD) Erreur de msgsnd");
                            msgctl(idQ,IPC_RMID,NULL);
                            exit(1);
                          }
                        }

                      break;

      case ACHAT :    // TO DO
                      fprintf(stderr,"(ACCESBD %d) Requete ACHAT reçue de %d\n",getpid(),m.expediteur);

                      // Preparation de la reponse

                      sprintf(requete,"select * from UNIX_FINAL where id = %d", m.data1);
                      if (mysql_query(connexion, requete) != 0)
                      {
                        fprintf (stderr, "Erreur de Mysql-query");
                      }

                      if((resultat = mysql_store_result(connexion)) == NULL)
                      {
                        fprintf (stderr, "Erreur de mysql store");
                      }

                      if ((Tuple = mysql_fetch_row(resultat)) != NULL)
                      {
                            //Données "systeme" :


                      qudispo = atoi(Tuple[3]);
                      qudemande = atoi(m.data2);

                        reponse.type = m.expediteur;
                        reponse.expediteur = getpid();
                        reponse.requete = ACHAT;
   
                            //Données utiles :
                        reponse.data1 = atoi(Tuple[0]);
                        strcpy(reponse.data2, Tuple[1]);
                        strcpy(reponse.data4, Tuple[4]);


                      if (qudemande > qudispo)
                      {
                        strcpy(reponse.data3, "0");
                      }
                      else
                      {
                        newqu = qudispo - qudemande;
                        //MAJ BASE DE DONNEES A FAIRE
                        sprintf(reponse.data3, "%d", qudemande);


                        sprintf(requete,"UPDATE UNIX_FINAL SET stock = %d where id = %d", newqu, reponse.data1);
                        if (mysql_query(connexion, requete) != 0)
                        {
                          fprintf (stderr, "Erreur de Mysql-query");
                        }

                          sprintf(reponse.data3, "%d", qudemande);
                      }


                      // Finalisation et envoi de la reponse
                          if(msgsnd(idQ,&reponse,sizeof(MESSAGE)-sizeof(long),0) == -1)
                          {
                            perror("(AccesBD) Erreur de msgsnd");
                            msgctl(idQ,IPC_RMID,NULL);
                            exit(1);
                          }

                        }
                      break;

      case CANCEL :   // TO DO


                      // Acces BD

                      newqu = atoi(m.data2);

                      sprintf(requete,"UPDATE UNIX_FINAL SET stock = stock + %d where id = %d", newqu, m.data1);

                      if (mysql_query(connexion, requete) != 0)
                      {
                        fprintf (stderr, "Erreur de Mysql-query");
                      }

                      if((resultat = mysql_store_result(connexion)) == NULL)
                      {
                        fprintf (stderr, "Erreur de mysql store");
                      }


                      break;

    }
  }
}

void handlerSIGUSR1(int sig)
{
  mysql_close(connexion);
  exit(0);
}