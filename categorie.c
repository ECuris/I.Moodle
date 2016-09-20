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

extern int          categorie_ouverte;	/* Définies dans sortie_HTML.c */
extern unsigned int numero_categorie;
extern unsigned int numero_question;

/* ~-~-~-~-~-~-~-~-~ Fonctions locales ~-~-~-~-~-~-~-~-~-~- */

/* ~-~-~-~-~-~-~-~-~ Réalisation pratique ~-~-~-~-~-~-~-~-~ */

/* ———————— Lecture d'une catégorie dans le fichier XML Moodle ———————— */

char *XML_LitCategorie( FILE *fichier, char *balise, int *code_erreur, FILE *fichier_HTML )
{
  char *categorie;
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

  /* On cherche la balise qui commence une catégorie */
  do {
    free( balise ); balise = NULL;

    balise = XML_LitBalise( fichier, code_erreur );
    if ( NULL == balise ) {
      ERREUR( "Impossible de trouver la balise de catégorie [<category>]" );
      return NULL;
    }
  } while ( strcmp( "<category>", balise ) != 0 );

  /* On cherche la balise qui commence le texte de cette catégorie */
  do {
    free( balise ); balise = NULL;

    balise = XML_LitBalise( fichier, code_erreur );
    if ( NULL == balise ) {
      ERREUR( "Impossible de trouver la balise de texte [<text>] (code %d)",
	      code_erreur );
      return NULL;
    }
  } while ( strcmp( "<text>", balise ) != 0 );

  /* On lit le texte de cette balise */
  categorie = XML_LitContenuBalise( fichier, code_erreur );
  if ( NULL == categorie ) {
    if ( *code_erreur < 0 ) {
      /* Erreur grave… */
      ERREUR( "Impossible de lire le contenu de la balise <text> (code %d)",
	      code_erreur );
      return balise;
    }
  }
  printf( "Catégorie à faire : %s\n", categorie );

  /* On fait la catégorie dans le fichier HTML */
  retour = HTML_CreerCategorie( fichier_HTML, categorie );
  if ( retour != 0 ) {
    ERREUR( "Problème en préparant la catégorie %s\n",
	    categorie );
  }

  /* Plus besoin du titre de la catégorie */
  free( categorie ); categorie = NULL;

  /* On cherche la balise de catégorie fermante */
  balise = XML_LitBalise( fichier, code_erreur );
  if ( NULL == balise ) {
    ERREUR( "Impossible de trouver la balise de texte [</text>] (code %d)",
	    code_erreur );
    return NULL;
  }
  if ( strcmp( "</text>", balise ) != 0 ) {
    ERREUR( "Balise fermante incorrecte [%s au lieu de </text>]",
	    balise );
    return balise;
  }

  /* On renvoie la dernière balise lue : sera traitée par la fonction appelante */
  return balise;
}

/* ———————— Écriture d'une catégorie dans le fichier HTML pour impression ———————— */


/* ———————— Créer une catégorie ———————— */

int HTML_CreerCategorie( FILE *fichier_HTML, char *nom_categorie )
{
  /* Vérifications initiales */
  if ( NULL == fichier_HTML ) {
    ERREUR( "Pas de fichier HTML indiqué !" );
    return -1;
  }
  if ( NULL == nom_categorie ) {
    ERREUR( "Pas de nom de catégorie indiqué !" );
    return -2;
  }

  /* On ferme la catégorie précédente */
  HTML_FermerCategorie( fichier_HTML );

  /* On adapte le texte de la catégorie */
  nom_categorie = HTML_PreparerTexte( nom_categorie );

  /* On ouvre la catégorie */
  numero_categorie++;
  (void) fprintf( fichier_HTML,
		  "  <div class=\"categorie\">\n"
		  "   <h2 class=\"categorie\">Catégorie %u : %s</h2>\n"
		  "   <div class=\"contenu_categorie\">\n",
		  numero_categorie, nom_categorie );
  categorie_ouverte = VRAI;
  
  /* Terminé sans souci */
  return 0;
}

int HTML_FermerCategorie(FILE *fichier_HTML)
{
  if ( FAUX == categorie_ouverte ) return 0;

  /* Bilan sur l'analyse de cette catégorie */
  (void) fprintf( fichier_HTML,
		  "    <p class=\"bilan categorie\">" );
  if ( 0 == numero_question ) {
    (void) fprintf( fichier_HTML,
		    "<span style=\"color: Red;\">"
		    "<i>Aucune question dans cette catégorie&thinsp;!</i>"
		    "</span>" );
  } else if ( 1 == numero_question ) {
    (void) fprintf( fichier_HTML,
		    "Une seule question dans cette catégorie." );
  } else {
    (void) fprintf( fichier_HTML,
		    "%u questions dans cette catégorie.",
		    numero_question );
  }
  (void) fprintf( fichier_HTML, "</p>\n" );

  /* On cloture la catégorie */
  (void) fprintf( fichier_HTML,
		  "   </div> <!-- du contenu de la catégorie -->\n"
		  "  </div> <!-- de la catégorie -->\n" );
  categorie_ouverte = FAUX;
  return 0;
}
