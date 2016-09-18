/* Conversion d'un fichier XML Moodle en HTML pour impression
 * © Emmanuel CURIS, septembre 2016
 *
 * À la demande de l'équipe Moodle, de Chantal Guihenneuc
    & de Jean-Michel Delacotte
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ascii_utils.h"
#include "txt_utils.h"

#include "impression.h"

/* -------- Variables globales -------- */

/* -------- Fonctions locales -------- */

/* -------- Réalisation pratique -------- */

/* ———————— Création du fichier HTML associé au XML

   nom_fichier : le nom du fichier XML que l'on veut convertir

   Renvoie un descripteur de fichier vers le fichier HTML à compléter
           NULL en cas d'erreur, quelle qu'elle soit
 */

FILE *HTML_CommencerFichier( const char *nom_fichier )
{
  FILE *fichier_HTML;
  char *nom_fichier_HTML, *pos_point;

  if ( NULL == nom_fichier ) {
    return NULL;
  }

  /* ———— On construit le nom du fichier HTML ———— */
  nom_fichier_HTML = strdup( nom_fichier );
  if ( NULL == nom_fichier_HTML ) {
    ERREUR( "Impossible de copier le nom de fichier [strdup][%s]", nom_fichier );
    return NULL;
  }
  /* On enlève l'extension .xml, si elle existe */
  pos_point = rindex( nom_fichier_HTML, '.' );
  if ( ( pos_point != NULL ) &&
       ( strcmp( pos_point, ".xml" ) == 0 ) ) {
    *pos_point = 0;
  }
  /* On ajoute l'extension .html */
  nom_fichier_HTML = TXT_AjouteTexte( nom_fichier_HTML, ".html" );
 
  printf( "Fichier à écrire : %s\n", nom_fichier_HTML );

  /* ———— Ouverture du fichier ———— */
  fichier_HTML = fopen( nom_fichier_HTML, "w" );
  if ( NULL == fichier_HTML ) {
    ERREUR( "Impossible de créer le fichier HTML [%s] ; code erreur : %d",
	    nom_fichier_HTML, errno );
    free( nom_fichier_HTML ) ; nom_fichier_HTML = NULL;
    return NULL;
  }

  /* ———— On indique qu'il s'agit d'un fichier HTML 5 ———— */
  (void) fprintf( fichier_HTML,
		  "<!DOCTYPE html>\n" /* Selon spécifications W3C */
		  "<html>\n"
		  );

  /* ———— Écriture de l'en-tête du fichier HTML ———— */
  (void) fprintf( fichier_HTML,
		  "<head>\n"
		  " <meta charset=\"UTF-8\">\n" /* Codage du fichier en UTF-8, comme le XML Moodle */
		  " <title>Version HTML imprimable du fichier Moodle %s</title>\n"
		  " <link rel=\"stylesheet\" href=\"impression.css\">\n" /* Style CSS à utiliser… */
		  "</head>\n",
		  nom_fichier );

  /* ———— Écriture du début du contenu du fichier HTML ———— */
  (void) fprintf( fichier_HTML,
		  " <body>\n"
		  "  <h1 class=\"titre\">Version imprimable d'une base de questions Moodle</h1>\n"
		  "   <p class=\"en_tete\">"
		  "Fichier XML Moodle original : "
		  "<span class=\"nom_fichier\">%s</span>"
		  "<br />\n"
		  "Date de conversion :"
		  "<br />\n"
		  "<i>Conversion par le logiciel développé"
		  " pour l'équipe TICE/Moodle de la faculté de pharmacie de Paris"
		  " — © Emmanuel C<small>URIS</small>, septembre 2016 [version %s].</i>"
		  "</p>\n",
		  nom_fichier, VERSION );

  /* Plus besoin du nom de fichier… */
  free( nom_fichier_HTML ) ; nom_fichier_HTML = NULL;

  /* On renvoie le fichier ouvert… */
  return fichier_HTML;
}

int HTML_TerminerFichier(FILE *fichier_HTML, int complet)
{
  if ( NULL == fichier_HTML ) {
    ERREUR( "Pas de fichier HTML indiqué !" );
    return -1;
  }

  if ( FAUX == complet ) {
    (void) fprintf( fichier_HTML,
		    " <div class=\"erreur\">\n"
		    "  <p class=\"erreur\">"
		    "<b>Attention&thinsp;!</b>"
		    " Une erreur a eu lieu pendant la conversion,"
		    " toutes les questions n'ont pas été converties."
		    "</p>\n"
		    " </div>\n" );
  }

  /* ———— Écriture de la fin du contenu du fichier HTML ———— */
  (void) fprintf( fichier_HTML,
		  " </body>\n"
		  "</html>" );

  /* On peut fermer le fichier… */
  (void) fclose( fichier_HTML ); fichier_HTML = NULL;

  return 0;
}


