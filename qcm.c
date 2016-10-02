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
  char *nom_question = NULL, *enonce = NULL;
  int retour;
  unsigned int nombre_reponses, numero_reponse = 0;

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
  printf( " [QCM] Nom de question : %s\n", nom_question );

  /* On crée donc la question */
  retour = HTML_CreerQuestion( fichier_HTML, QUESTION_QCM, nom_question );
  if ( retour != 0 ) {
    ERREUR( "Problème en voulant créer la question [titre : %s]\n",
	    nom_question );
  }

  /* —————— Recherche de l'énoncé de la question — */
  enonce = XML_Q_TrouverEnonce( fichier, code_erreur );
  if ( NULL == enonce ) {
    ERREUR( "Question sans énoncé ! [titre : %s]\n",
	    nom_question );
  } else {
    printf( "       Énoncé lu       : %.30s{...}%s\n",
	    enonce, enonce + strlen( enonce ) - 21 );
  }

  /*  —————— Décompte du nombre de réponses à la question */
  nombre_reponses = XML_Q_CompterReponses( fichier, code_erreur );
  printf( "       %d réponse(s) détectée(s)\n", nombre_reponses );

  /* On affiche l'énoncé */
  retour = HTML_EnonceQuestion( fichier_HTML, enonce, nombre_reponses, VRAI );
  if ( retour != 0 ) {
    ERREUR( "Problème en préparant l'énoncé de la question [titre : %s]\n",
	    nom_question );
  }

  free( enonce ); enonce = NULL;

  /*  —————— On lit les balises suivantes et on les analyse ———————— */
  do {
    free( balise ) ; balise = NULL;

    balise = XML_LitBalise( fichier, code_erreur, VRAI );
    if ( NULL == balise ) {
      ERREUR( "Question incomplète ! [code erreur : ]\n",
	      ( NULL == code_erreur ) ? 1987 : *code_erreur );
      return NULL;
    }

    /* Analyse de la balise, au cas où… */
    if ( strncmp( "<answer", balise, 7 ) == 0 ) {
      numero_reponse++;

      balise = XML_TraiterReponse( fichier, fichier_HTML, balise, numero_reponse, code_erreur );
      if ( NULL == balise ) {
	ERREUR( "Question incomplète [réponse mal formulée ?] ! [code erreur : ]\n",
		( NULL == code_erreur ) ? 1987 : *code_erreur );
	return NULL;
      }
    } else if ( strcmp( "<shuffleanswers>", balise ) == 0 ) {
      int valeur;
      valeur = XML_LitBaliseBinaire( fichier );
      if ( valeur < 0 ) {
	ERREUR( "Balise <shuffleanswers> avec valeur inconnue — ignorée" );
      }

      printf( "       Balise <shuffleanswers> : %d\n", valeur );

      /* Une ligne précisant cela */
      (void) fprintf( fichier_HTML,
		      "     <tr>\n"
		      "      <td class=\"info.question\">%s</td>\n"
		      "     </tr>\n",
		      ( valeur ) ? "Les réponses seront mélangées à chaque utilisation" :
		                   "Les réponses seront dans l'ordre indiqué" );
    } else if ( strcmp( "<single>", balise ) == 0 ) {
      int valeur;

      valeur = XML_LitBaliseBinaire( fichier );
      if ( valeur < 0 ) {
	ERREUR( "Balise <single> avec valeur inconnue — ignorée" );
      }

      printf( "       Balise <single> : %d\n", valeur );

      /* Une ligne précisant cela */
      (void) fprintf( fichier_HTML,
		      "     <tr>\n"
		      "      <td class=\"info.question\">%s</td>\n"
		      "     </tr>\n",
		      ( valeur ) ? "Une seule des réponses peut être choisie" :
		                   "Plusieurs réponses peuvent être choisies" );
    } else if ( strcmp( "</question>", balise ) == 0 ) {
      /* Rien à faire : servira à finir la boucle… */
    } else if ( strcmp( "</single>", balise ) == 0 ) {
      /* Rien à faire : termine le <single>… */
    } else if ( strcmp( "</shuffleanswers>", balise ) == 0 ) {
      /* Rien à faire : termine le <shuffleanswers>… */
    } else {
      (void) fprintf( stderr, "%s,%s l.%u : balise inconnue [%s], ignorée\n",
		      __FILE__, __FUNCTION__, __LINE__, balise );
    }
  } while ( strcmp( "</question>", balise ) != 0 );

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
