#include "windowclient.h"
#include "ui_windowclient.h"
#include <QMessageBox>
#include <string>
using namespace std;

#include "protocole.h"

#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <signal.h>

extern WindowClient *w;

int idQ, idShm;
bool logged = 0;
char* pShm;
char affiche[100];
MESSAGE message;
ARTICLE articleEnCours;
float totalCaddie = 0.0;

void handlerSIGUSR1(int sig);
void handlerSIGUSR2(int sig);

#define REPERTOIRE_IMAGES "images/"

WindowClient::WindowClient(QWidget *parent) : QMainWindow(parent), ui(new Ui::WindowClient)
{
    ui->setupUi(this);

    // Configuration de la table du panier (ne pas modifer)
    ui->tableWidgetPanier->setColumnCount(3);
    ui->tableWidgetPanier->setRowCount(0);
    QStringList labelsTablePanier;
    labelsTablePanier << "Article" << "Prix à l'unité" << "Quantité";
    ui->tableWidgetPanier->setHorizontalHeaderLabels(labelsTablePanier);
    ui->tableWidgetPanier->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableWidgetPanier->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidgetPanier->horizontalHeader()->setVisible(true);
    ui->tableWidgetPanier->horizontalHeader()->setDefaultSectionSize(160);
    ui->tableWidgetPanier->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidgetPanier->verticalHeader()->setVisible(false);
    ui->tableWidgetPanier->horizontalHeader()->setStyleSheet("background-color: lightyellow");

    // Recuperation de l'identifiant de la file de messages
    fprintf(stderr,"(CLIENT %d) Recuperation de l'id de la file de messages\n",getpid());
            // TO DO -> Fait Etape 1
    if ((idQ = msgget(CLE,0)) == -1)
    {
        perror("(Client) Erreur de msgget(1)");
        exit(1);
    }
    printf("idQ = %d\n",idQ);

    // Recuperation de l'identifiant de la mémoire partagée
    fprintf(stderr,"(CLIENT %d) Recuperation de l'id de la mémoire partagée\n",getpid());
    // TO DO
      idShm = shmget(CLE, 52, 0);
      if(idShm == -1)            //NEWWWWWW
      {
        perror("Erreur de recuperation de memoire partagee");
        exit(1);
      }
    printf("idShm = %d", idShm);

    // Attachement à la mémoire partagée
    // TO DO

    pShm = (char*)shmat(idShm, NULL, 0);
    if(pShm == (char*)-1)
      {
        perror("Erreur d attachement a la memoire partagee"); //NEWWWW
        exit(1);
      }

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

    struct sigaction B;
    B.sa_handler = handlerSIGUSR2;
    sigemptyset(&B.sa_mask);
    B.sa_flags = 0;

    if (sigaction(SIGUSR2, &B, NULL) == -1)
    {
      perror ("Erreur de Sigaction\n");
      exit (1);
    }

    // Envoi d'une requete de connexion au serveur
            // TO DO - Fait -> E1
    message.expediteur = getpid();
    message.type = 1;
    message.requete = CONNECT;



    if (msgsnd(idQ,&message,sizeof(MESSAGE)-sizeof(long),0) == -1)
    {
        perror("(Client) Erreur de msgsnd");
        msgctl(idQ,IPC_RMID,NULL);
        exit(1);
    }



    // Exemples à supprimer
   // setPublicite("Promotions sur les concombres !!!");
   // setArticle("pommes",5.53,18,"pommes.jpg");
//    ajouteArticleTablePanier("Tu n'as encore rien acheté",0,0);

}

WindowClient::~WindowClient()
{
    delete ui;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions utiles : ne pas modifier /////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setNom(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditNom->clear();
    return;
  }
  ui->lineEditNom->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getNom()
{
  strcpy(nom,ui->lineEditNom->text().toStdString().c_str());
  return nom;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setMotDePasse(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditMotDePasse->clear();
    return;
  }
  ui->lineEditMotDePasse->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
const char* WindowClient::getMotDePasse()
{
  strcpy(motDePasse,ui->lineEditMotDePasse->text().toStdString().c_str());
  return motDePasse;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setPublicite(const char* Text)
{
  if (strlen(Text) == 0 )
  {
    ui->lineEditPublicite->clear();
    return;
  }
  ui->lineEditPublicite->setText(Text);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setImage(const char* image)
{
  // Met à jour l'image
  char cheminComplet[80];
  sprintf(cheminComplet,"%s%s",REPERTOIRE_IMAGES,image);
  QLabel* label = new QLabel();
  label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
  label->setScaledContents(true);
  QPixmap *pixmap_img = new QPixmap(cheminComplet);
  label->setPixmap(*pixmap_img);
  label->resize(label->pixmap()->size());
  ui->scrollArea->setWidget(label);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
int WindowClient::isNouveauClientChecked()
{
  if (ui->checkBoxNouveauClient->isChecked()) return 1;
  return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setArticle(const char* intitule,float prix,int stock,const char* image)
{
  ui->lineEditArticle->setText(intitule);
  if (prix >= 0.0)
  {
    char Prix[20];
    sprintf(Prix,"%.2f",prix);
    ui->lineEditPrixUnitaire->setText(Prix);
  }
  else ui->lineEditPrixUnitaire->clear();
  if (stock >= 0)
  {
    char Stock[20];
    sprintf(Stock,"%d",stock);
    ui->lineEditStock->setText(Stock);
  }
  else ui->lineEditStock->clear();
  setImage(image);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
int WindowClient::getQuantite()
{
  return ui->spinBoxQuantite->value();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::setTotal(float total)
{
  if (total >= 0.0)
  {
    char Total[20];
    sprintf(Total,"%.2f",total);
    ui->lineEditTotal->setText(Total);
  }
  else ui->lineEditTotal->clear();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::loginOK()
{
  ui->pushButtonLogin->setEnabled(false);
  ui->pushButtonLogout->setEnabled(true);
  ui->lineEditNom->setReadOnly(true);
  ui->lineEditMotDePasse->setReadOnly(true);
  ui->checkBoxNouveauClient->setEnabled(false);

  ui->spinBoxQuantite->setEnabled(true);
  ui->pushButtonPrecedent->setEnabled(true);
  ui->pushButtonSuivant->setEnabled(true);
  ui->pushButtonAcheter->setEnabled(true);
  ui->pushButtonSupprimer->setEnabled(true);
  ui->pushButtonViderPanier->setEnabled(true);
  ui->pushButtonPayer->setEnabled(true);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::logoutOK()
{
  ui->pushButtonLogin->setEnabled(true);
  ui->pushButtonLogout->setEnabled(false);
  ui->lineEditNom->setReadOnly(false);
  ui->lineEditMotDePasse->setReadOnly(false);
  ui->checkBoxNouveauClient->setEnabled(true);

  ui->spinBoxQuantite->setEnabled(false);
  ui->pushButtonPrecedent->setEnabled(false);
  ui->pushButtonSuivant->setEnabled(false);
  ui->pushButtonAcheter->setEnabled(false);
  ui->pushButtonSupprimer->setEnabled(false);
  ui->pushButtonViderPanier->setEnabled(false);
  ui->pushButtonPayer->setEnabled(false);

  setNom("");
  setMotDePasse("");
  ui->checkBoxNouveauClient->setCheckState(Qt::CheckState::Unchecked);

  setArticle("",-1.0,-1,"");

  w->videTablePanier();
  totalCaddie = 0.0;
  w->setTotal(-1.0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions utiles Table du panier (ne pas modifier) /////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::ajouteArticleTablePanier(const char* article,float prix,int quantite)
{
    char Prix[20],Quantite[20];

    sprintf(Prix,"%.2f",prix);
    sprintf(Quantite,"%d",quantite);

    // Ajout possible
    int nbLignes = ui->tableWidgetPanier->rowCount();
    nbLignes++;
    ui->tableWidgetPanier->setRowCount(nbLignes);
    ui->tableWidgetPanier->setRowHeight(nbLignes-1,10);

    QTableWidgetItem *item = new QTableWidgetItem;
    item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    item->setTextAlignment(Qt::AlignCenter);
    item->setText(article);
    ui->tableWidgetPanier->setItem(nbLignes-1,0,item);

    item = new QTableWidgetItem;
    item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    item->setTextAlignment(Qt::AlignCenter);
    item->setText(Prix);
    ui->tableWidgetPanier->setItem(nbLignes-1,1,item);

    item = new QTableWidgetItem;
    item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    item->setTextAlignment(Qt::AlignCenter);
    item->setText(Quantite);
    ui->tableWidgetPanier->setItem(nbLignes-1,2,item);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::videTablePanier()
{
    ui->tableWidgetPanier->setRowCount(0);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
int WindowClient::getIndiceArticleSelectionne()
{
    QModelIndexList liste = ui->tableWidgetPanier->selectionModel()->selectedRows();
    if (liste.size() == 0) return -1;
    QModelIndex index = liste.at(0);
    int indice = index.row();
    return indice;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions permettant d'afficher des boites de dialogue (ne pas modifier ////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::dialogueMessage(const char* titre,const char* message)
{
   QMessageBox::information(this,titre,message);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::dialogueErreur(const char* titre,const char* message)
{
   QMessageBox::critical(this,titre,message);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////// CLIC SUR LA CROIX DE LA FENETRE /////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::closeEvent(QCloseEvent *event)
{
  // TO DO (étape 1)
  // Envoi d'une requete DECONNECT au serveur


    message.expediteur = getpid();
    message.type = 1;
    message.requete = DECONNECT;

    if (logged == 1)
    {
        w->on_pushButtonLogout_clicked();
    }


    if (msgsnd(idQ,&message,sizeof(MESSAGE)-sizeof(long),0) == -1)
    {
        perror("Erreur de msgsnd");
        msgctl(idQ,IPC_RMID,NULL);
        exit(1);
    }


  // envoi d'un logout si logged

  // Envoi d'une requete de deconnexion au serveur

  exit(0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Fonctions clics sur les boutons ////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonLogin_clicked()
{
    // Envoi d'une requete de login au serveur
            // TO DO Fait - E1
    message.expediteur = getpid();
    message.type = 1;
    message.requete = LOGIN;
        // Si nouveau client : 1 si non 0 :
    message.data1 = isNouveauClientChecked();
        // Le login va ici :
    strcpy(message.data2,getNom());

        // Le mot de passe va ici :
    strcpy(message.data3,getMotDePasse());


    if (msgsnd(idQ,&message,sizeof(MESSAGE)-sizeof(long),0) == -1)
    {
        perror("Erreur de msgsnd");
        msgctl(idQ,IPC_RMID,NULL);
        exit(1);
    }

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonLogout_clicked()
{
    // Envoi d'une requete CANCEL_ALL au serveur (au cas où le panier n'est pas vide)
    // TO DO
    w->on_pushButtonViderPanier_clicked();
    // Envoi d'une requete de logout au serveur
    // TO DO

    message.expediteur = getpid();
    message.type = 1;
    message.requete = LOGOUT;


    if (msgsnd(idQ,&message,sizeof(MESSAGE)-sizeof(long),0) == -1)
    {
        perror("Erreur de msgsnd");
        msgctl(idQ,IPC_RMID,NULL);
        exit(1);
    }
    w->logoutOK();

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonSuivant_clicked()
{
    // TO DO (étape 3)
    // Envoi d'une requete CONSULT au serveur
                    message.type = 1;
                    message.requete = CONSULT;
                    message.expediteur = getpid();
                    message.data1 = (articleEnCours.id + 1);


    if (msgsnd(idQ,&message,sizeof(MESSAGE)-sizeof(long),0) == -1)
    {
        perror("(Client) Erreur de msgsnd");
        msgctl(idQ,IPC_RMID,NULL);
        exit(1);
    }

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonPrecedent_clicked()
{
    // TO DO (étape 3)
    // Envoi d'une requete CONSULT au serveur
                    message.type = 1;
                    message.requete = CONSULT;
                    message.expediteur = getpid();
                    message.data1 = (articleEnCours.id - 1);


    if (msgsnd(idQ,&message,sizeof(MESSAGE)-sizeof(long),0) == -1)
    {
        perror("(Client) Erreur de msgsnd");
        msgctl(idQ,IPC_RMID,NULL);
        exit(1);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonAcheter_clicked()
{
    // TO DO (étape 5)

    // Envoi d'une requete ACHAT au serveur
        if (getQuantite() > 0)
        {
                    message.type = 1;
                    message.requete = ACHAT;
                    message.expediteur = getpid();
                    message.data1 = articleEnCours.id;
                    sprintf(message.data2, "%d", getQuantite());

                    if (msgsnd(idQ,&message,sizeof(MESSAGE)-sizeof(long),0) == -1)
                    {
                        perror("(Client) Erreur de msgsnd");
                        msgctl(idQ,IPC_RMID,NULL);
                        exit(1);
                    }
        }
        else
        {
            w->dialogueMessage("Achat", "Je fais pas le biesse, je choisis une QU + grande que 0 !");
        }
        

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonSupprimer_clicked()
{
    // TO DO (étape 6)
    // Envoi d'une requete CANCEL au serveur
    
    if (getIndiceArticleSelectionne() == -1 )
    {
        w->dialogueMessage("Erreur :", "Aucun article séléctionné !");
    }
    else
    {
        // 1. On envoi la requete au serveur
            message.type = 1;
            message.requete = CANCEL;
            message.expediteur = getpid();
            message.data1 = getIndiceArticleSelectionne();

            if (msgsnd(idQ,&message,sizeof(MESSAGE)-sizeof(long),0) == -1)
            {
                perror("(Client) Erreur de msgsnd - Suppression article");
                msgctl(idQ,IPC_RMID,NULL);
                exit(1);
            }


        // On met à jour le caddie
            // On le vide
        w->videTablePanier();
        totalCaddie = 0.0;
        w->setTotal(0.0);
            // On demande le nouveau
        message.type = 1;
        message.requete = CADDIE;
        message.expediteur = getpid();

        if (msgsnd(idQ,&message,sizeof(MESSAGE)-sizeof(long),0) == -1)
        {
        perror("(Client) Erreur de msgsnd");
        msgctl(idQ,IPC_RMID,NULL);
        exit(1);
        }
    }
    // Mise à jour du caddie



    // Envoi requete CADDIE au serveur
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonViderPanier_clicked()
{
    // TO DO (étape 6)
    // Envoi d'une requete CANCEL_ALL au serveur
            message.type = 1;
            message.requete = CANCEL_ALL;
            message.expediteur = getpid();
            message.data1 = getIndiceArticleSelectionne();

            if (msgsnd(idQ,&message,sizeof(MESSAGE)-sizeof(long),0) == -1)
            {
                perror("(Client) Erreur de msgsnd - Suppression article");
                msgctl(idQ,IPC_RMID,NULL);
                exit(1);
            }

    // Mise à jour du caddie
    w->videTablePanier();
    totalCaddie = 0.0;
    w->setTotal(-1.0);

    // Envoi requete CADDIE au serveur
        message.type = 1;
        message.requete = CADDIE;
        message.expediteur = getpid();

        if (msgsnd(idQ,&message,sizeof(MESSAGE)-sizeof(long),0) == -1)
        {
        perror("(Client) Erreur de msgsnd");
        msgctl(idQ,IPC_RMID,NULL);
        exit(1);
        }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowClient::on_pushButtonPayer_clicked()
{
    // TO DO (étape 7)
    // Envoi d'une requete PAYER au serveur
            message.type = 1;
            message.requete = PAYER;
            message.expediteur = getpid();
            message.data1 = getIndiceArticleSelectionne();

            if (msgsnd(idQ,&message,sizeof(MESSAGE)-sizeof(long),0) == -1)
            {
                perror("(Client) Erreur de msgsnd - Suppression article");
                msgctl(idQ,IPC_RMID,NULL);
                exit(1);
            }

    char tmp[100];
    sprintf(tmp,"Merci pour votre paiement de %.2f ! Votre commande sera livrée tout prochainement.",totalCaddie);
    dialogueMessage("Payer...",tmp);

    // Mise à jour du caddie
    w->videTablePanier();
    totalCaddie = 0.0;
    w->setTotal(-1.0);

    // Envoi requete CADDIE au serveur
        message.type = 1;
        message.requete = CADDIE;
        message.expediteur = getpid();

        if (msgsnd(idQ,&message,sizeof(MESSAGE)-sizeof(long),0) == -1)
        {
        perror("(Client) Erreur de msgsnd");
        msgctl(idQ,IPC_RMID,NULL);
        exit(1);
        }

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///// Handlers de signaux ////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
void handlerSIGUSR1(int sig)
{
    MESSAGE m;
    while (msgrcv(idQ,&m,sizeof(MESSAGE)-sizeof(long),getpid(),IPC_NOWAIT) != -1)  // !!! a modifier en temps voulu !!!
    {
      switch(m.requete)
      {
        case LOGIN : 

                    if (m.data1 == 1)
                    {
                       w->loginOK();

                    }
                    w->dialogueMessage("Connection : ", m.data4);
                
                        // On récupère les articles.
                    message.type = 1;
                    message.requete = CONSULT;
                    message.expediteur = getpid();
                    message.data1 = 1;

                    if (msgsnd(idQ,&message,sizeof(MESSAGE)-sizeof(long),0) == -1)
                    {
                        perror("(Client) Erreur de msgsnd");
                        msgctl(idQ,IPC_RMID,NULL);
                        exit(1);
                    }

                    
                    break;

        case CONSULT : // TO DO (étape 3)


                    articleEnCours.id = m.data1;
                    strcpy(articleEnCours.intitule, m.data2);
                    articleEnCours.stock = atoi(m.data3);
                    articleEnCours.prix = m.data5;
                    strcpy(articleEnCours.image, m.data4);
                    w->setArticle(articleEnCours.intitule, articleEnCours.prix, articleEnCours.stock, articleEnCours.image);
                                //intitule prix     stock          image


                    break;

        case ACHAT : // TO DO (étape 5)
                    if (atoi(m.data3) == 0)
                    {
                        w->dialogueMessage("Achat", "Stock insuffisant !");
                    }
                    else
                    {
                        sprintf(affiche,"%s unités de %s achetees",m.data3, m.data2);
                        w->dialogueMessage("Achat", affiche);

                    message.type = 1;
                    message.requete = CADDIE;
                    message.expediteur = getpid();

                    if (msgsnd(idQ,&message,sizeof(MESSAGE)-sizeof(long),0) == -1)
                    {
                        perror("(Client) Erreur de msgsnd");
                        msgctl(idQ,IPC_RMID,NULL);
                        exit(1);
                    }
                        // On vide le panir pour qu'il puisse être mis à jour.
                    w->videTablePanier();
                    totalCaddie = 0;
                    
                    }

                    break;

         case CADDIE : // TO DO (étape 5)
                       // 


                        w->ajouteArticleTablePanier(m.data2,m.data5,atoi(m.data3));

                        totalCaddie +=  (m.data5 * (float)(atoi(m.data3)));

                        w->setTotal(totalCaddie);
                        
                    break;

         case TIME_OUT : // TO DO (étape 6)
                        w->logoutOK();
                        w->dialogueErreur("TIME-OUT", "Trop tard, pas dodo sur le clavier ! ");
                    break;

         case BUSY : // TO DO (étape 7)
                        w->dialogueErreur("ERREUR", "Serveur en maintenance, retentez plus tard.");
                    break;

         default :
                    break;
      }
    };
}




void handlerSIGUSR2(int sig)
{

    // recuperer le contenu ici
    printf ("MAJ PUB-> %s\n", pShm);
    w->setPublicite(pShm);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
