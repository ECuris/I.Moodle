/* Conversion d'un fichier XML Moodle en HTML pour impression
 * © Emmanuel CURIS, septembre 2016
 *
 * À la demande de l'équipe Moodle, de Chantal Guihenneuc
    & de Jean-Michel Delacotte
 *
 * Traitement des questions de type « catégorie »
 */

#include <ctype.h>		/* Pour tolower */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "impression.h"

/* ~-~-~-~-~-~-~-~-~ Variables globales ~-~-~-~-~-~-~-~-~-~ */

extern unsigned int numero_question;

/* ~-~-~-~-~-~-~-~-~ Fonctions locales ~-~-~-~-~-~-~-~-~-~- */

/* ~-~-~-~-~-~-~-~-~ Réalisation pratique ~-~-~-~-~-~-~-~-~ */

/* ———————— Lecture d'une catégorie dans le fichier XML Moodle ———————— */

char *XML_LitQCM( FILE *fichier, char *balise, int *code_erreur, FILE *fichier_HTML )
{
  fpos_t position_debut;
  char *nom_question = NULL;
  int retour;

  /* Vérifications initiales */
  if ( NULL == balise ) {
    ERREUR( "Balise manquante !" );
    if ( NULL != code_erreur ) *code_erreur = -10;
    return NULL;
  }
  if ( ( NULL == fichier ) || ( NULL == fichier_HTML ) ) {
    ERREUR( "Problème avec l'un des deux fichiers transmis" );
    if ( NULL != code_erreur ) *code_erreur = -1;
    return balise;
  }

  /* On note le début de ce bloc… */
  retour = fgetpos( fichier, &position_debut );
  if ( retour != 0 ) {
    ERREUR( "Problème dans fgetpos [code : %d / %s] — lecture annulée",
	    errno, strerror( errno ) );
    if ( code_erreur != NULL ) *code_erreur = -51;
    return NULL;
  }

  /* —————— Recherche du nom de la question —————— */
  nom_question = XML_Q_TrouverTitre( fichier, code_erreur );
  printf( "Nom de question lu : %s\n", nom_question );

  /* On crée donc la question */
  retour = HTML_CreerQuestion( fichier_HTML, QUESTION_QCM, nom_question );
  if ( retour != 0 ) {
    ERREUR( "Problème en voulant créer la question [titre : %s]\n",
	    nom_question );
  }

  /* On a terminé avec cette question */
  retour = HTML_FinirQuestion( fichier_HTML );
  if ( retour != 0 ) {
    ERREUR( "Problème en voulant finir la question [titre : %s]\n",
	    nom_question );
  }

  free( nom_question ); nom_question = NULL;

  /* On renvoie la dernière balise lue pour continuer */
  return balise;
}
