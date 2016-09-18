/* Conversion d'un fichier XML Moodle en HTML pour impression
 * © Emmanuel CURIS, septembre 2016
 *
 * À la demande de l'équipe Moodle, de Chantal Guihenneuc
    & de Jean-Michel Delacotte
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ascii_utils.h"
#include "txt_utils.h"

#include "impression.h"

#define NOM_EXEMPLE "Exemples/ue18_2014_S1.xml"

/* -------- Variables globales -------- */

FILE *fichier = NULL; 		/* Le fichier XML traité */
/* FILE *html = NULL; */

/* -------- Fonctions locales -------- */

/* -------- Réalisation pratique -------- */

int main(int nbr_param, char *liste_param[])
{
  int retour;

  /* ———————— Message introductif ———————— */
  (void) printf( "————————————————————————————————————————————————————————————\n"
                 "          Conversion de fichier XML Moodle en HTML\n"
                 "           pour impression optimisée et relecture\n\n"
                 "© Emmanuel CURIS, septembre 2016 — pour le service TICE\n"
		 "Version : %s\n"
		 "————————————————————————————————————————————————————————————\n",
		 VERSION );

  /* 1) Choix du fichier 
        passé en paramètre ? ⇒ on regarde si l'on a indiqué un fichier… */
  if ( nbr_param > 1 ) {
    long i;
    char *nom_fichier;

    for ( i = 1 ; i < nbr_param ; i++ ) {
      /* On récupère le nom du fichier */
      nom_fichier = liste_param[ i ];

      /* Et on fait la conversion pour ce fichier… */
      retour = XML_ImprimerFichier( liste_param[ i ] );
      if ( retour != 0 ) {
	(void) fprintf( stderr, "\nERREUR en voulant convertir le fichier %s [code %d]\n",
			nom_fichier, retour );
      }
      (void) printf( "————————————————————————————————————————————————————————————\n" );
    }
  } else {
    retour = XML_ImprimerFichier( NOM_EXEMPLE );
    if ( retour != 0 ) {
      (void) fprintf( stderr, "\nERREUR en voulant convertir le fichier %s [code %d]\n",
		      NOM_EXEMPLE, retour );
    }
  }

  /* On a terminé, sans souci… */
  return 0;
}

/* -------- Traitement d'une question -------- */

char *ConvertirQuestion( char *ligne )
{
  char *pos_debut, *pos_fin;
  /* 1) repérer le type de la question */

  /* Format indiqué : <question type="XXX"> */
  pos_debut = strchr( ligne, '"' );
  if ( pos_debut == NULL ) {
  }

  pos_fin   = strrchr( ligne, '"' );
  if ( pos_fin == NULL ) {
  }

  *pos_fin = 0;
  printf( "Type de la question : %s\n", pos_debut );

  /* Suivant le type, on appelle la fonction appropriée */
  if ( strcmp( "category", pos_debut ) == 0 ) {
    free( ligne );
    /* ligne = CQ_Categorie( ); */
  } else if ( strcmp( "multichoice", pos_debut ) == 0 ) {
    free( ligne );
    /* ligne = CQ_ChoixMultiple( ); */
  } else {
  }

  /* On a fini : on renvoie ce que l'on a lu comme dernière ligne */
  return( ligne );
}
