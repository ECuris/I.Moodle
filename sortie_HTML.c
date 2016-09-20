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

/* ~-~-~-~-~-~-~-~-~ Variables globales ~-~-~-~-~-~-~-~-~-~ */

int          categorie_ouverte = FAUX; /* VRAI si l'on a ouvert une catégorie */
unsigned int numero_categorie  = 0;    /* Décompte des catégories */

unsigned int numero_question   = 0;    /* Décompte des questions dans la catégorie */

/* ~-~-~-~-~-~-~-~-~ Fonctions locales ~-~-~-~-~-~-~-~-~-~- */

static inline const char *HTML_TexteTypeQuestion( int type );

/* ~-~-~-~-~-~-~-~-~ Réalisation pratique ~-~-~-~-~-~-~-~-~ */

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
		  "</p>\n"
		  "  <hr class=\"separateur\">\n",
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

  /* On ferme la catégorie ouverte, s'il y en a une */
  HTML_FermerCategorie( fichier_HTML );

  /* On affiche un message en cas de problème de conversion */
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

/* ———————— Préparer un texte pour sa sortie HTML ———————— */

char *HTML_PreparerTexte(char *texte)
{
  unsigned long lg_texte;

  if ( NULL == texte ) return texte ;

  /* On enlève la partie CDATA, au besoin */
  if ( strncmp( "<![CDATA[", texte, 9 ) == 0 ) {
    texte += 9;

    /* Le ]]> final */
    lg_texte = strlen( texte );
    texte[ lg_texte - 1 ] = 0;
    texte[ lg_texte - 2 ] = 0;
    texte[ lg_texte - 3 ] = 0;
  }

  /* On renvoie le texte modifié */
  return texte;
}

/* ———————— Routines communes aux différentes questions ———————— */

int HTML_CreerQuestion(FILE *fichier_HTML, int type, char *titre)
{
  if ( NULL == fichier_HTML ) return -1; /* Pas de fichier HTML indiqué… */

  /* Une question supplémentaire */
  numero_question++;

  /* Le conteneur de la question */
  (void) fprintf( fichier_HTML,
		  "   <div class=\"question\">\n" );

  /* Le bandeau avec le titre de la question */
  (void) fprintf( fichier_HTML,
		  "    <div class=\"en_tete question\">\n"
		  "     <p class=\"numero_question\">Q %03u [%s]</p>\n"
		  "     <h3 class=\"titre_question\">",
		  numero_question, HTML_TexteTypeQuestion( type ) );
  if ( NULL == titre ) {
    (void) fprintf( fichier_HTML, "<i>Sans titre</i>" );
  } else {
    titre = HTML_PreparerTexte( titre );
    (void) fprintf( fichier_HTML, titre );
  }
  (void) fprintf( fichier_HTML, "</h3>\n"
		  "    </div>\n" );

  /* Le début du contenant de l'énoncé et des réponses */
  (void) fprintf( fichier_HTML,
		  "    <div class=\"contenu question\">\n" );
  return 0;
}

int HTML_FinirQuestion(FILE *fichier_HTML)
{
  if ( NULL == fichier_HTML ) return -1; /* Pas de fichier HTML indiqué… */

  (void) fprintf( fichier_HTML,
		  "    </div> <!-- Contenu de la question -->\n"
		  "   </div> <!-- Question -->\n\n" );

  return 0;
}


static inline const char *HTML_TexteTypeQuestion(int type)
{
  switch( type ) {
  case QUESTION_ERREUR:
    return "<span class=\"erreur\">ERREUR</span>";

  case QUESTION_INCONNUE:
    return "<span class=\"avertissement\">Type inconnu</span>";

  case QUESTION_CATEGORIE:
    return "Catégorie";

  case QUESTION_QCM:
    return "QCM";

  case QUESTION_VF:
    return "V/F";

  case QUESTION_QROC:
    return "QROC";

  case QUESTION_PAIRES:
    return "Paires";

  case QUESTION_LIBRE:
    return "Complexe";

  case QUESTION_OUVERTE:
    return "Ouverte";

  case QUESTION_NUMERIQUE:
    return "Numérique";

  case QUESTION_TEXTE:
    return "— description —";

  default:
    return "<span class=\"erreur\">À IMPLÉMENTER</span>";
  }

  return "<span class=\"erreur\">Impossible</span>";
}
