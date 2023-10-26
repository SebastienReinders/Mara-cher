#include "windowgerant.h"
#include "ui_windowgerant.h"
#include <iostream>
using namespace std;
#include <mysql.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include "protocole.h"
#include <string>
#include <string.h>
/////// Faire le controle de la taille de chaine de la pub dans la fonction.

int idArticleSelectionne = -1;
MYSQL *connexion;
MYSQL_RES  *resultat;
MYSQL_ROW  Tuple;
char requete[200];
char NewPub[51];
int idSem;
int idQ;
int idShm;
int taille = 0;
char Prix[20];

MESSAGE m;


union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
} arg;

struct sembuf operations[1];

WindowGerant::WindowGerant(QWidget *parent) : QMainWindow(parent),ui(new Ui::WindowGerant)
{
    ui->setupUi(this);

    // Configuration de la table du stock (ne pas modifer)
    ui->tableWidgetStock->setColumnCount(4);
    ui->tableWidgetStock->setRowCount(0);
    QStringList labelsTableStock;
    labelsTableStock << "Id" << "Article" << "Prix à l'unité" << "Quantité";
    ui->tableWidgetStock->setHorizontalHeaderLabels(labelsTableStock);
    ui->tableWidgetStock->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableWidgetStock->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidgetStock->horizontalHeader()->setVisible(true);
    ui->tableWidgetStock->horizontalHeader()->setDefaultSectionSize(120);
    ui->tableWidgetStock->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidgetStock->verticalHeader()->setVisible(false);
    ui->tableWidgetStock->horizontalHeader()->setStyleSheet("background-color: lightyellow");

    // Recuperation de la file de message
    // TO DO
    if ((idQ = msgget(CLE,0)) == -1)
    {
        perror("(GERANT) Erreur de msgget(1)");
        exit(1);
    }
    printf("idQ = %d\n",idQ);

    // Recuperation de l'identifiant de la mémoire partagée
    fprintf(stderr,"(GERANT %d) Recuperation de l'id de la mémoire partagée\n",getpid());
    // TO DO
      idShm = shmget(CLE, 52, 0);
      if(idShm == -1)            //NEWWWWWW
      {
        perror("Erreur de recuperation de memoire partagee");
        exit(1);
      }


    // Récupération du sémaphore
    // TO DO

    idSem = semget(CLE,0,0);
    if (idSem == -1)
    {
    perror("Erreur de semget");
    exit(1);
    }

    // Prise blocante du semaphore
    // TO DO
    operations[0].sem_num = 0;
    operations[0].sem_op = -1;
    operations[0].sem_flg = 0;

    if (semop(idSem, operations, 1) == -1)
    {
        printf ("Erreur de prise de semaphore\n");
        exit (1);
    }


    // Connexion à la base de donnée
    connexion = mysql_init(NULL);
    fprintf(stderr,"(GERANT %d) Connexion à la BD\n",getpid());
    if (mysql_real_connect(connexion,"localhost","Student","PassStudent1_","PourStudent",0,0,0) == NULL)
    {
      fprintf(stderr,"(GERANT %d) Erreur de connexion à la base de données...\n",getpid());
      exit(1);  
    }

    // Recuperation des articles en BD
    // TO DO
    sprintf(requete,"select * from UNIX_FINAL");
    if (mysql_query(connexion, requete) != 0)
    {
        printf ("Erreur de Mysql-query");
    }

    if((resultat = mysql_store_result(connexion)) == NULL)
    {
        printf ("Erreur de mysql store");
    }

    
    
    while ((Tuple = mysql_fetch_row(resultat)) != NULL)
    {
                        strcpy(Prix, Tuple[2]);
                        string tmp4(Prix);
                        size_t x = tmp4.find(".");
                        if (x != string::npos) tmp4.replace(x,1,",");

                        

                        ajouteArticleTablePanier(atoi(Tuple[0]),Tuple[1], atof(tmp4.c_str()), atoi(Tuple[3]));

    };




    // Exemples à supprimer
    /*ajouteArticleTablePanier(1,"pommes",2.53,25);
    ajouteArticleTablePanier(2,"oranges",5.83,1);
    ajouteArticleTablePanier(3,"bananes",1.85,12);
    ajouteArticleTablePanier(4,"cerises",5.44,17);*/
}

WindowGerant::~WindowGerant()
{
    delete ui;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions utiles Table du stock (ne pas modifier) //////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowGerant::ajouteArticleTablePanier(int id,const char* article,float prix,int quantite)
{
    char Id[20],Prix[20],Quantite[20];

    sprintf(Id,"%d",id);
    sprintf(Prix,"%.2f",prix);
    sprintf(Quantite,"%d",quantite);

    // Ajout possible
    int nbLignes = ui->tableWidgetStock->rowCount();
    nbLignes++;
    ui->tableWidgetStock->setRowCount(nbLignes);
    ui->tableWidgetStock->setRowHeight(nbLignes-1,10);

    QTableWidgetItem *item = new QTableWidgetItem;
    item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    item->setTextAlignment(Qt::AlignCenter);
    item->setText(Id);
    ui->tableWidgetStock->setItem(nbLignes-1,0,item);

    item = new QTableWidgetItem;
    item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    item->setTextAlignment(Qt::AlignCenter);
    item->setText(article);
    ui->tableWidgetStock->setItem(nbLignes-1,1,item);

    item = new QTableWidgetItem;
    item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    item->setTextAlignment(Qt::AlignCenter);
    item->setText(Prix);
    ui->tableWidgetStock->setItem(nbLignes-1,2,item);

    item = new QTableWidgetItem;
    item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    item->setTextAlignment(Qt::AlignCenter);
    item->setText(Quantite);
    ui->tableWidgetStock->setItem(nbLignes-1,3,item);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowGerant::videTableStock()
{
    ui->tableWidgetStock->setRowCount(0);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
int WindowGerant::getIndiceArticleSelectionne()
{
    QModelIndexList liste = ui->tableWidgetStock->selectionModel()->selectedRows();
    if (liste.size() == 0) return -1;
    QModelIndex index = liste.at(0);
    int indice = index.row();
    return indice;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowGerant::on_tableWidgetStock_cellClicked(int row, int column)
{
    //cerr << "ligne=" << row << " colonne=" << column << endl;
    ui->lineEditIntitule->setText(ui->tableWidgetStock->item(row,1)->text());
    ui->lineEditPrix->setText(ui->tableWidgetStock->item(row,2)->text());
    ui->lineEditStock->setText(ui->tableWidgetStock->item(row,3)->text());
    idArticleSelectionne = atoi(ui->tableWidgetStock->item(row,0)->text().toStdString().c_str());
    //cerr << "id = " << idArticleSelectionne << endl;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions utiles : ne pas modifier /////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
float WindowGerant::getPrix()
{
    return atof(ui->lineEditPrix->text().toStdString().c_str());
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
int WindowGerant::getStock()
{
    return atoi(ui->lineEditStock->text().toStdString().c_str());
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowGerant::getPublicite()
{
  strcpy(publicite,ui->lineEditPublicite->text().toStdString().c_str());
  return publicite;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////// CLIC SUR LA CROIX DE LA FENETRE /////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowGerant::closeEvent(QCloseEvent *event)
{
  fprintf(stderr,"(GERANT %d) Clic sur croix de la fenetre\n",getpid());
  // TO DO
  // Deconnexion BD
  mysql_close(connexion);

  // Liberation du semaphore
  // TO DO

    operations[0].sem_num = 0;
    operations[0].sem_op = 1;
    operations[0].sem_flg = 0;

    if (semop(idSem, operations, 1) == -1)
    {
        printf ("Erreur de relachement du semaphore\n");
        exit (1);
    }


  exit(0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions clics sur les boutons ////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowGerant::on_pushButtonPublicite_clicked()
{
  fprintf(stderr,"(GERANT %d) Clic sur bouton Mettre a jour\n",getpid());
  // TO DO (étape 7)
  taille = strlen(getPublicite());
  if (taille < 51 && taille > 0)
  {
      strcpy(NewPub, getPublicite());
      for (int i = 1; i < 51; i++)
      {
        if (NewPub[i] == '\0')
        {
            NewPub[i] = ' ';
            NewPub[i+1] = '\0';
        }
      }
        // Envoi d'une requete NEW_PUB au serveur

            m.type = 1;
            m.expediteur = getpid();
            m.requete = NEW_PUB;
            strcpy (m.data4, NewPub);

            if (msgsnd(idQ,&m,sizeof(MESSAGE)-sizeof(long),0) == -1)
            {
                perror("(GERANT) Erreur de msgsnd - 4");
                msgctl(idQ,IPC_RMID,NULL);
                exit(1);
            }

  }





}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowGerant::on_pushButtonModifier_clicked()
{
  fprintf(stderr,"(GERANT %d) Clic sur bouton Modifier\n",getpid());
  // TO DO
  //cerr << "Prix  : --"  << getPrix() << "--" << endl;
  //cerr << "Stock : --"  << getStock() << "--" << endl;

  char Prix[20];
  sprintf(Prix,"%f",getPrix());
  string tmp(Prix);
  size_t x = tmp.find(",");
  if (x != string::npos) tmp.replace(x,1,".");

  fprintf(stderr,"(GERANT %d) Modification en base de données pour id=%d\n",getpid(),idArticleSelectionne);

  // Mise a jour table BD
  // TO DO
  sprintf(requete,"UPDATE UNIX_FINAL SET prix = %s where id = %d", tmp.c_str(), idArticleSelectionne);
                        if (mysql_query(connexion, requete) != 0)
                        {
                          printf ("Erreur de Mysql-query");
                        }
  sprintf(requete,"UPDATE UNIX_FINAL SET stock = %d where id = %d", getStock(), idArticleSelectionne);
                        if (mysql_query(connexion, requete) != 0)
                        {
                          printf ("Erreur de Mysql-query");
                        }


    WindowGerant::videTableStock();
    sprintf(requete,"select * from UNIX_FINAL");
    if (mysql_query(connexion, requete) != 0)
    {
        printf ("Erreur de Mysql-query");
    }

    if((resultat = mysql_store_result(connexion)) == NULL)
    {
        printf ("Erreur de mysql store");
    }
    
    while ((Tuple = mysql_fetch_row(resultat)) != NULL)
    {
                        strcpy(Prix, Tuple[2]);
                        string tmp3(Prix);
                        size_t x = tmp3.find(".");
                        if (x != string::npos) tmp3.replace(x,1,",");

                      ajouteArticleTablePanier(atoi(Tuple[0]),Tuple[1], atof(tmp3.c_str()), atoi(Tuple[3]));

    };


}
