/* Conversion d'un fichier XML Moodle en HTML pour impression
 * © Emmanuel CURIS, septembre 2016
 *
 * À la demande de l'équipe Moodle, de Chantal Guihenneuc
    & de Jean-Michel Delacotte
 */

#include <ctype.h>		/* Pour tolower */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ascii_utils.h"
#include "txt_utils.h"

#include "impression.h"

/* -------- Variables globales -------- */

#define QUESTION_ERREUR   -2
#define QUESTION_INCONNUE -1

#define QUESTION_CATEGORIE 1	/* Catégorie      , « category »    */
#define QUESTION_QCM       2	/* Choix multiples, « multichoice » */
#define QUESTION_VF        3	/* Vrai/Faux      , « truefalse »   */
#define QUESTION_QROC      4	/* Réponse courte , « shortanswer » */
#define QUESTION_PAIRES    5	/* Appariement    , « matching »    */
#define QUESTION_LIBRE     6	/* Format libre   , « cloze »       */
#define QUESTION_OUVERTE   7	/* Texte libre    , « essay »       */
#define QUESTION_NUMERIQUE 8	/* Numérique      , « numerical »   */
#define QUESTION_TEXTE     9	/*                , « description » */

/* -------- Fonctions locales -------- */

char *ConvertirQuestion( char *ligne );

char *CQ_Categorie( void );
char *CQ_ChoixMultiple( void );

char *XML_LitBalise   ( FILE *fichier, int *code_erreur );

char *XML_LitQuestion ( FILE *fichier, char *balise, int *code_erreur, FILE *fichier_HTML );

char *XML_LitCategorie( FILE *fichier, char *balise, int *code_erreur, FILE *fichier_HTML );

int MOODLE_TypeQuestion( char *texte_type );

/* -------- Réalisation pratique -------- */

/* ———————— Ouverture d'un fichier XML proposé, vérifications
            Traitement du fichier

   nom_fichier : le nom [complet] du fichier à traiter

   Renvoie     : 0 si aucune erreur, code erreur sinon
   ———————— */

int XML_ImprimerFichier( const char *nom_fichier )
{
  FILE *fichier, *fichier_HTML;
  int c, code_erreur;
  char *balise;

  if ( NULL == nom_fichier ) {
    ERREUR( "Aucun nom de fichier indiqué" );
    return -1;
  }
  printf( "Fichier à lire   : %s\n", nom_fichier );

  /* On ouvre le fichier à lire */
  fichier = fopen( nom_fichier, "r" );
  if ( NULL == fichier ) {
    ERREUR( "Erreur en ouvrant le fichier %s\n", nom_fichier );
    return -2;
  }
  printf( "On a ouvert le fichier…\n" );

  /* Lecture du premier caractère : doit être un < */
  c = fgetc( fichier );
  if ( c != '<' ) {
    ERREUR( "Le fichier ne commence par par < [%s]\n", nom_fichier );
    (void) fclose( fichier ); fichier = NULL;
    return -3;
  }
  ungetc( c, fichier );   /* On remet le caractère pour être tranquille */
  printf( "C'est un fichier qui commence bien par < …\n" );

  /* Lecture de la première balise : est-ce bien du XML Moodle ? */
  balise = XML_LitBalise( fichier, &code_erreur );
  if ( NULL == balise ) {
    ERREUR( "Impossible de lire la 1re balise… [Fichier : %s ; code : %d]\n",
	    nom_fichier, code_erreur );
    (void) fclose( fichier ); fichier = NULL;
    return code_erreur;
  }
  if ( strncmp( "<?xml ", balise, 6 ) ) {
    ERREUR( "Début de fichier incorrect, devrait être <?xml mais est %s", balise );
    free( balise ); balise = NULL;
    
    (void) fclose( fichier ); fichier = NULL;
    return -4;
  }

  /* On cherche le début des questions [balise <quiz>] */
  do {
    free( balise ); balise = NULL;

    balise = XML_LitBalise( fichier, &code_erreur );
    if ( NULL == balise ) {
      ERREUR( "Impossible de trouver le début des questions (balise <quiz>) [code %d]",
	      code_erreur );
      (void) fclose( fichier ); fichier = NULL;
      return -5;
    }
    printf( "Comparaison : %d [%s]\n", strcmp( "<quiz>", balise ), balise );
  } while( strcmp( "<quiz>", balise ) != 0 );
  printf( "C'est bien un fichier qui ressemble à du Moodle…\n" );
  printf( "On a trouvé le début du quiz…\n" );

  /* ———— On ouvre donc le fichier de sortie HTML ———— */
  fichier_HTML = HTML_CommencerFichier( nom_fichier );
  if ( NULL == fichier_HTML ) {
    ERREUR( "Impossible de créer le fichier HTML. Conversion annulée" );
    (void) fclose( fichier ); fichier = NULL;
    return -10;
  }

  /* ———— On traite les balises qui suivent, jusqu'à la fin du quiz ———— */
  do {
    free( balise ); balise = NULL;

    balise = XML_LitBalise( fichier, &code_erreur );
    if ( NULL == balise ) {
      ERREUR( "Fin de fichier pendant la lecture des questions (balise </quiz> manquante ?) [code %d]",
	      code_erreur );
      (void) fclose( fichier ); fichier = NULL;
      return -5;
    }

    if ( strncmp( "<question", balise, 9 ) == 0 ) {
      balise = XML_LitQuestion( fichier, balise, &code_erreur, fichier_HTML );
    } else if ( strcmp( "</quiz>", balise ) != 0 ) {
      ERREUR( "Balise inconnue ou mal placée [%s] — ignorée", balise );
    }
    
  } while ( strcmp( "</quiz>", balise ) != 0 );

  /* On a finit : on ferme le fichier */
  (void) fclose( fichier ); fichier = NULL;

  /* On ferme aussi le fichier HTML… */
  (void) HTML_TerminerFichier( fichier_HTML, VRAI );

  /* Et tout s'est bien passé, apparemment */
  return 0;
}

/* ———————— Lecture d'une balise XML (ou HTML) ———————— 
   
   fichier     : le fichier à lire
   code_erreur :
   
   Renvoie NULL en cas d'erreur, quelle qu'elle soit
                et remet alors le curseur du fichier là où il était

	   une chaîne contenant la balise, <> inclus, sinon. À libérer avec free()
 */

char *XML_LitBalise( FILE *fichier, int *code_erreur )
{
  char *balise;
  fpos_t position, debut_balise;
  int c, retour;
  unsigned long lg_balise, i, n_lu;

  if ( NULL == fichier ) {
    ERREUR( "Pas de descripteur de fichier" );
    if ( code_erreur != NULL ) *code_erreur = -50;
    return NULL;
  }

  /* On mémorise la position initiale */
  retour = fgetpos( fichier, &position );
  if ( retour != 0 ) {
    ERREUR( "Problème dans fgetpos [code : %d / %s] — lecture annulée",
	    errno, strerror( errno ) );
    if ( code_erreur != NULL ) *code_erreur = -51;
    return NULL;
  }

  /* On lit jusqu'à trouver une balise ouvrante */
  c = fgetc( fichier );
  if ( c != '<' ) {
    (void) fprintf( stderr, "%s,%s l.%u : Attention, on n'est pas sur une balise…\n",
		    __FILE__, __FUNCTION__, __LINE__ );

    /* On n'est pas sur une ouverture de balise : on lit jusqu'à en trouver une… */
    while( c != '<' ) {
      c = fgetc( fichier );
      if ( EOF == c ) {
	ERREUR( "Fin de fichier atteinte sans trouver de balise…" );
	if ( code_erreur != NULL ) *code_erreur = -52;

	/* On se remet au début… */
	retour = fsetpos( fichier, &position );
	if ( retour != 0 ) {
	  ERREUR( "Problème dans fsetpos [code : %d / %s]",
		  errno, strerror( errno ) );
	}
	return NULL;
      }
    }
  }
  ungetc( c, fichier );		/* On remet le caractère pour être sûr qu'il soit inclus dans la balise */

  /* On mémorise la position du début de la balise [peut être différente de celle du début… */
  retour = fgetpos( fichier, &debut_balise );
  if ( retour != 0 ) {
    ERREUR( "Problème dans fgetpos [code : %d / %s] — lecture annulée",
	    errno, strerror( errno ) );
    if ( code_erreur != NULL ) *code_erreur = -53;
    return NULL;
  }

  /* ———— On cherche le > correspondant ———— */
  lg_balise = 0;
  do {
    c = fgetc( fichier );

    if ( EOF == c ) {
      ERREUR( "Fin de fichier atteinte en lisant une balise… > final manquant ?" );
      if ( code_erreur != NULL ) *code_erreur = -55;

      /* On se remet au début… */
      retour = fsetpos( fichier, &position );
      if ( retour != 0 ) {
	ERREUR( "Problème dans fsetpos [code : %d / %s]",
		errno, strerror( errno ) );
      }
      return NULL;
    }

    if ( ( lg_balise > 1 ) & ( '<' == c ) ) {
      /* Balise imbriquée dans une balise : interdit normalement ! */
      ERREUR( "Code d'ouverture de balise au sein d'une balise… > final manquant ?" );
      if ( code_erreur != NULL ) *code_erreur = -56;
      return NULL;
    }

    /* Tous les autres caractères sont valides */
    lg_balise++;
  } while ( c != '>' );


  /* ———— On réserve pour lire la balise ———— */
  balise = (char *) malloc( sizeof( char ) * ( lg_balise + 1 ) ); /* +1 : 0 final */
  if ( NULL == balise ) {
    ERREUR( "Mémoire insuffisante pour lire une balise de %lu caractères",
	    lg_balise );

    /* On se remet au début… */
    retour = fsetpos( fichier, &position );
    if ( retour != 0 ) {
      ERREUR( "Problème dans fsetpos [code : %d / %s]",
	      errno, strerror( errno ) );
    }

    if ( code_erreur != NULL ) *code_erreur = -60;
    return NULL;
  }
  /* Initialisation à 0 */
  for ( i = 0 ; i <= lg_balise ; i++ ) {
    balise[ i ] = 0;
  }

  /* On se remet au début de la balise… */
  retour = fsetpos( fichier, &debut_balise );
  if ( retour != 0 ) {
    ERREUR( "Problème dans fsetpos [code : %d / %s] — Lecture annulée",
	    errno, strerror( errno ) );
    free( balise ) ; balise = NULL;
    if ( code_erreur != NULL ) *code_erreur = -57;
    return NULL;
  }

  /* … et on relit tous en une fois */
  n_lu = fread( balise, sizeof( char ), lg_balise, fichier );
  if ( n_lu != lg_balise ) {
    (void) fprintf( stderr, "%s,%s l.%u : Discordance de longueur dans la lecture de la balise [%lu vs %lu]\n",
		    __FILE__, __FUNCTION__, __LINE__, n_lu, lg_balise );
  }

  /* On renvoie la balise lue… */
  printf( "Balise lue : %s\n", balise );
  return balise;
}

/* ———————— Lecture et traitement d'une question ———————— 
 
   fichier     : le fichier XML en cours d'analyse
   balise      : la balise qui contient le type de question
                 [doit avoir été créé avec malloc()
		  pour pouvoir être libéré avec free()]
   code_erreur : contiendra le code d'erreur si problème

   Renvoie la balise d'origine en cas d'erreur [mais on aura éventuellement avancé dans le fichier]
              sauf si elle a été libérée : renvoie NULL dans ce cas
           la balise qui suit la fin de la question sinon
 */

char *XML_LitQuestion( FILE *fichier, char *balise, int *code_erreur, FILE *fichier_HTML )
{
  char *position;
  int type_question;

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

  if ( strncmp( "<question", balise, 9 ) != 0 ) {
    ERREUR( "Balise inattendue [%s] — ignorée" );
    return balise;
  }

  position = balise + 9;
  if ( *position != ' ' ) {
    if ( '>' == *position ) {
      ERREUR( "Question sans type — ignorée" );
      if ( code_erreur != NULL ) *code_erreur = +1;
      
      do {
	free( balise ); balise = NULL;

	balise = XML_LitBalise( fichier, code_erreur );
	if ( NULL == balise ) {
	  ERREUR( "Impossible de trouver la balise fermante </question>" );
	  return NULL;
	}
      } while( strcmp( "</question>", balise ) != 0 );

      /* On renvoie cette balise */
      return balise;
    }
  }

  /* La, on peut chercher le type */
  while( strncmp( "type=\"", position, 6 ) != 0 ) {
    position++;
    if ( 0 == *position ) {	/* Fin de chaîne atteinte */
      ERREUR( "Question sans type — ignorée" );
      if ( code_erreur != NULL ) *code_erreur = +2;
      
      do {
	free( balise ); balise = NULL;

	balise = XML_LitBalise( fichier, code_erreur );
	if ( NULL == balise ) {
	  ERREUR( "Impossible de trouver la balise fermante </question>" );
	  return NULL;
	}
      } while( strcmp( "</question>", balise ) != 0 );

      /* On renvoie cette balise */
      return balise;
    }
  }
  
  position += 6;
  printf( " Type de la question : %s\n", position );
  type_question = MOODLE_TypeQuestion( position );
  switch( type_question ) {
  case QUESTION_CATEGORIE:
    balise = XML_LitCategorie( fichier, balise, code_erreur, fichier_HTML );
    break;

  case QUESTION_ERREUR:
    ERREUR( "Type de question inconnu [%s] — question ignorée", position );
    if ( code_erreur != NULL ) *code_erreur = +3;
    break;

  case QUESTION_INCONNUE:
    ERREUR( "Type de question inconnu [%s] — question ignorée", position );
    if ( code_erreur != NULL ) *code_erreur = +4;
    break;
  }

  /* On saute toutes les balises jusqu'à celle de fin de question, si on n'y est pas déjà */
  while ( strcmp( "</question>", balise ) != 0 ) {
    free( balise ); balise = NULL;

    balise = XML_LitBalise( fichier, code_erreur );
    if ( NULL == balise ) {
      ERREUR( "Impossible de trouver la balise fermante </question>" );
      return NULL;
    }
  }

  /* On renvoie cette balise fermante */
  return balise;
}

/* ———————— Décodage du type de question à partir ———————— */

int MOODLE_TypeQuestion( char *texte_type )
{
  char *pos_fin;
  int code_question = QUESTION_INCONNUE;
  unsigned long i;

  if ( NULL == texte_type ) return QUESTION_ERREUR;

  /* On cherche le guillement fermant : le premier… */
  pos_fin = strchr( texte_type, '"' );
  if ( NULL == pos_fin ) {
    ERREUR( "Balise incorrecte, type de question sans guillement fermant [%s]",
	    texte_type );
  } else {
    *pos_fin = 0;		/* On remettra le guillemet à la fin, pour la suite… */
  }

  /* On convertit en minuscules pour être tranquille */
  for ( i = 0 ; i < strlen( texte_type ) ; i++ ) {
    texte_type[ i ] = tolower( texte_type[ i ] );
  }

  /* On fait des comparaisons directes… */
  if ( strcmp( "category"   , texte_type ) == 0 ) code_question = QUESTION_CATEGORIE;
  if ( strcmp( "multichoice", texte_type ) == 0 ) code_question = QUESTION_QCM;
  if ( strcmp( "truefalse"  , texte_type ) == 0 ) code_question = QUESTION_VF;
  if ( strcmp( "shortanswer", texte_type ) == 0 ) code_question = QUESTION_QROC;
  if ( strcmp( "matching"   , texte_type ) == 0 ) code_question = QUESTION_PAIRES;
  if ( strcmp( "cloze"      , texte_type ) == 0 ) code_question = QUESTION_LIBRE;
  if ( strcmp( "essay"      , texte_type ) == 0 ) code_question = QUESTION_OUVERTE;
  if ( strcmp( "numerical"  , texte_type ) == 0 ) code_question = QUESTION_NUMERIQUE;
  if ( strcmp( "description", texte_type ) == 0 ) code_question = QUESTION_TEXTE;

  /* On remet le guillemet, pour ne pas perdre la fin de la balise */
  if ( pos_fin != NULL ) *pos_fin = '"';

  /* On renvoie le code de la question */
  return code_question;
}

/* ——————————————————————————————————————————————————————————————————————
	   Traitement des différents types de question de Moodle
 * —————————————————————————————————————————————————————————————————————— */

/* ———————— Catégorie ———————— */

char *XML_LitCategorie( FILE *fichier, char *balise, int *code_erreur, FILE *fichier_HTML )
{
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
      ERREUR( "Impossible de trouver la balise de texte [<text>]" );
      return NULL;
    }
  } while ( strcmp( "<text>", balise ) != 0 );

  /* On lit le texte de cette balise */

  /* On cherche la balise de catégorie fermante */

  /* On renvoie la dernière balise lue : sera traitée par la fonction appelante */
  return balise;
}