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
 
  printf( "Fichier à écrire   : %s\n", nom_fichier_HTML );

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


/* ———————————————————————————————————————————————————————————————————— *
 |                                                                      |
 |       Morceaux fréquents d'écriture d'une question XML Moodle        |
 |                                                                      |
 * ———————————————————————————————————————————————————————————————————— */

/* ———————— Création d'une question en HTML ———————— */

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
    (void) fputs( titre, fichier_HTML );
  }
  (void) fprintf( fichier_HTML, "</h3>\n"
		  "    </div>\n" );

  /* Le début du contenant de l'énoncé et des réponses */
  (void) fprintf( fichier_HTML,
		  "    <table class=\"contenu question\">\n" );
  return 0;
}

/* ———————— Préparer l'énoncé d'une question en HTML ————————
     
   fichier_HTML    : celui où écrire le code HTML produit
   enonce          : la chaîne brute contenant l'énoncé
   nombre_reponses : le nombre de réponses connues
   titres          : si VRAI, indiquer les titres
 */

int HTML_EnonceQuestion(FILE *fichier_HTML, char *enonce, unsigned int nombre_reponses, int titres)
{
  if ( NULL == fichier_HTML ) return -1; /* Pas de fichier HTML indiqué… */

  /* Ligne avec les titres des morceaux */
  if ( VRAI == titres ) {
    (void) fprintf( fichier_HTML,
		    "     <tr>\n"
		    "      <th class=\"titre énoncé\">Énoncé</th>\n"
		    "      <th class=\"titre réponse\" colspan=3>Réponse" );
    if ( nombre_reponses > 1 ) {
      (void) fprintf( fichier_HTML,
		      "s<br /><span class=\"nbr_réponses\">%u réponses enregistrées</span>",
		      nombre_reponses );
    }
    (void) fprintf( fichier_HTML,
		    "</th>\n" 
		    "     </tr>\n" );
  }

  /* Ligne avec l'énoncé (en comptant une ligne par réponse ensuite */
  (void) fprintf( fichier_HTML,
		  "     <tr>\n"
		  "      <td class=\"enonce\"" );
  if ( nombre_reponses > 1 ) {
    (void) fprintf( fichier_HTML,
		    " rowspan=%u",
		    nombre_reponses );
  }
  (void) fprintf( fichier_HTML, ">" );

  /* L'énoncé proprement dit */
  if ( NULL == enonce ) {
    (void) fputs( "<span class=\"avertissement\">Sans énoncé</span>", fichier_HTML );
  } else {
    enonce = HTML_PreparerTexte( enonce );
    (void) fputs( enonce, fichier_HTML );
  }

  /* Tout s'est bien passé */
  return 0;  
}

/* ———————— Sortir une réponse à une question en HTML ————————
               1) Commencer la réponse…

   fichier_HTML   : celui où écrire le code HTML produit
   fraction       : la (fraction de) note de cette réponse, en %
   numero_reponse : le numéro de la réponse
 */

int HTML_CommencerReponse(FILE *fichier_HTML, double fraction,
			  unsigned int numero_reponse)
{
  int fraction_i;
  int retour;

  if ( NULL == fichier_HTML ) return -1; /* Pas de fichier HTML indiqué… */

  /* Si pas la 1re réponse, on commence une nouvelle ligne dans le tableau */
  if ( numero_reponse > 1 ) {
    retour = fputs( "     <tr>", fichier_HTML );
    if ( retour <= 0 ) {
      (void) fprintf( stderr, "%s,%s l.%u : ERREUR en voulant commencer une réponse [code %d]\n",
		      __FILE__, __FUNCTION__, __LINE__, retour );
    }
  }

  /* Début de la colonne… */
  retour = fprintf( fichier_HTML,
		    "      <td class=\"reponse numero\">%u</td>\n",
		    numero_reponse );
  if ( retour < 0 ) {
    (void) fprintf( stderr, "%s,%s l.%u : ERREUR en voulant numéroter la réponse [code %d]\n",
		    __FILE__, __FUNCTION__, __LINE__, retour );
  }

  /* Affichage de la note */
  retour = fputs( "      <td class=\"reponse note ", fichier_HTML );
  if ( retour <= 0 ) {
    (void) fprintf( stderr, "%s,%s l.%u :"
		    " ERREUR en voulant commencer la fraction de note [code %d]\n",
		    __FILE__, __FUNCTION__, __LINE__, retour );
  }
  fraction_i = (int) fraction;

  /* La classe suivant le type de réponse */
  if ( 100 == fraction_i ) {
    retour = fputs( "juste", fichier_HTML );
  } else if ( fraction > 0.0 ) {
    retour = fputs( "juste partielle", fichier_HTML );
  } else if ( 0 == fraction ) {
    retour = fputs( "fausse", fichier_HTML );
  } else if ( fraction < 0 ) {
    retour = fputs( "fausse negatif", fichier_HTML );
  }
  if ( retour <= 0 ) {
    (void) fprintf( stderr, "%s,%s l.%u :"
		    " ERREUR en voulant écrire la classe CSS de la note [code %d]\n",
		    __FILE__, __FUNCTION__, __LINE__, retour );
  }

  retour = fputs( "\">", fichier_HTML );
  if ( retour <= 0 ) {
    (void) fprintf( stderr, "%s,%s l.%u :"
		    " ERREUR en voulant écrire la classe CSS de la note [code %d]\n",
		    __FILE__, __FUNCTION__, __LINE__, retour );
  }


  /* Le signe de la note */
  retour = 0;
  if ( fraction_i < 0 ) {
    retour = fputs( "&minus;", fichier_HTML );
    fraction_i = -fraction_i;
  } else if ( fraction_i > 0 ) {
    retour = fputc( '+', fichier_HTML );
  }
  if ( retour < 0 ) {
    (void) fprintf( stderr, "%s,%s l.%u :"
		    " ERREUR en voulant écrire le signe de la note [code %d]\n",
		    __FILE__, __FUNCTION__, __LINE__, retour );
  }

  /* La note */
  if ( ( fraction - fraction_i ) < 0.01 ) {
    if ( 0 == fraction_i ) {
      retour = fputs( "0", fichier_HTML );
    } else {
      retour = fprintf( fichier_HTML, "%d&thinsp;%%", fraction_i );
    }
  } else {
    /* Cas possibles : 1/9 de la note = 11,11 %
                       1/8 de la note = 12,50 %
                       1/7 de la note = 14,28 %
                       1/6 de la note = 16,66 %
                       1/3 de la note = 33,33 %
                       2/3 de la note = 66,66 %
                       5/6 de la note = 83,33 %
     */
    retour = fprintf( fichier_HTML, "%.2f&thinsp;%%", fraction );
  }
  if ( retour < 0 ) {
    (void) fprintf( stderr, "%s,%s l.%u :"
		    " ERREUR en voulant écrire la note [code %d]\n",
		    __FILE__, __FUNCTION__, __LINE__, retour );
  }

  (void) fprintf( fichier_HTML,
		  "</td>\n"
		  "      <td class=\"reponse texte\">" );

  /* Tout s'est bien passé */
  return 0;
}


/* ———————— Sortir une réponse à une question en HTML ————————
               2) Le texte de la réponse…

   fichier_HTML   : celui où écrire le code HTML produit
   reponse        : le texte brut de la réponse
 */

int HTML_TexteReponse(FILE *fichier_HTML, char *reponse)
{
  int retour;

  if ( NULL == fichier_HTML ) return -1; /* Pas de fichier HTML indiqué… */

  /* On traduit le texte de la réponse */
  if ( NULL == reponse ) {
    retour = fputs( "<span class=\"avertissement\">Aucune réponse indiquée</span>",
		    fichier_HTML );
  } else {
    reponse = HTML_PreparerTexte( reponse );
    retour = fputs( reponse, fichier_HTML );
  }
  if ( retour <= 0 ) {
    ERREUR( "Problème en voulant écrire la réponse [code %d ; réponse %s]\n",
	    retour, reponse );

    return -10;
  }

  /* Tout s'est bien passé */
  return 0;
}


/* ———————— Sortir une réponse à une question en HTML ————————
               3) Terminer la réponse…

   fichier_HTML   : celui où écrire le code HTML produit
 */

int HTML_FinirReponse(FILE *fichier_HTML)
{
  if ( NULL == fichier_HTML ) return -1; /* Pas de fichier HTML indiqué… */

   /* Fin de la réponse */
  (void) fprintf( fichier_HTML,
		  "</td>\n"
		  "     </tr>\n" );

  /* Tout s'est bien passé */
  return 0;
}


/* ———————— Terminer une question en HTML ———————— */

int HTML_FinirQuestion(FILE *fichier_HTML)
{
  if ( NULL == fichier_HTML ) return -1; /* Pas de fichier HTML indiqué… */

  (void) fprintf( fichier_HTML,
		  "    </table> <!-- Contenu de la question -->\n"
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
