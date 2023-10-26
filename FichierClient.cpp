#include "FichierClient.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>

int estPresent(const char* nom)
{
  int fd;

  fd = open(FICHIER_CLIENTS, O_RDONLY);
  if(fd == -1)
    return -1;
  else
  {
    CLIENT lireClient;
    int taille = 0, i = 0;


    taille = lseek(fd, 0, SEEK_END);
    taille /= sizeof(CLIENT);
    lseek(fd, 0, SEEK_SET);

    do
    {
      read(fd, &lireClient, sizeof(CLIENT));
      i++;

    }while(i < taille && strcmp(nom, lireClient.nom) != 0);

    if(strcmp(nom, lireClient.nom) == 0)
    {
      close(fd);
      return i;
    }
  }
  close(fd);

  return 0;
}

////////////////////////////////////////////////////////////////////////////////////
int hash(const char* motDePasse)
{
  int i = 1, mdp = 0;

  while(*motDePasse != '\0')
  {
      mdp += i * (*motDePasse);

      i++;
      motDePasse++;
  }
  mdp = mdp % 97;

  return mdp;
}

////////////////////////////////////////////////////////////////////////////////////
void ajouteClient(const char* nom, const char* motDePasse)
{
  int fd;

  if((fd = open(FICHIER_CLIENTS, O_WRONLY | O_CREAT | O_APPEND, 0644)) == -1)
  {
    perror("erreur open()");
    return;
  }
  else
  {
      CLIENT stockeNewClient;

      strcpy(stockeNewClient.nom, nom);
      stockeNewClient.hash = hash(motDePasse);

      write(fd, &stockeNewClient, sizeof(CLIENT));
  }
  close(fd);
}

////////////////////////////////////////////////////////////////////////////////////
int verifieMotDePasse(int pos, const char* motDePasse)
{
  int fd, i;

  if((fd = open(FICHIER_CLIENTS, O_RDONLY)) == -1)
    return -1;
  else
  {
    CLIENT lireClient;

    if(pos > 0)
      lseek(fd, (pos-1)*sizeof(CLIENT), SEEK_SET);

    read(fd, &lireClient, sizeof(CLIENT));

    if(lireClient.hash == hash(motDePasse))
      i = 1;
    else
      i = 0;
  }
  close(fd);

  return i;
}

////////////////////////////////////////////////////////////////////////////////////
int listeClients(CLIENT *vecteur) // le vecteur doit etre suffisamment grand
{
  int fd, i = 0;

  if((fd = open(FICHIER_CLIENTS, O_RDONLY)) == -1)
    return -1;
  else
  {
    CLIENT lireClient;
    int taille;

    taille = lseek(fd, 0, SEEK_END);
    taille /= sizeof(CLIENT);
    lseek(fd, 0, SEEK_SET);

    while(i < taille)
    {
      read(fd, &lireClient, sizeof(CLIENT));

      strcpy(vecteur->nom, lireClient.nom);
      vecteur->hash = lireClient.hash;

      i++;
      vecteur++;
    }

  }
  close(fd);

  return(i);
}