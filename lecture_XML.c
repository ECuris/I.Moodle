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

/* -------- Fonctions locales -------- */

char *ConvertirQuestion( char *ligne );

char *CQ_ChoixMultiple( void );

char *XML_LitQuestion ( FILE *fichier, char *balise, int *code_erreur, FILE *fichier_HTML );

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
  printf( "Fichier à lire     : %s\n", nom_fichier );

  /* On ouvre le fichier à lire */
  fichier = fopen( nom_fichier, "r" );
  if ( NULL == fichier ) {
    ERREUR( "Erreur en ouvrant le fichier %s\n", nom_fichier );
    return -2;
  }
  /* printf( "On a ouvert le fichier…\n" ); */

  /* Lecture du premier caractère : doit être un < */
  c = fgetc( fichier );
  if ( c != '<' ) {
    ERREUR( "Le fichier ne commence par par < [%s]\n", nom_fichier );
    (void) fclose( fichier ); fichier = NULL;
    return -3;
  }
  ungetc( c, fichier );   /* On remet le caractère pour être tranquille */
  /* printf( "C'est un fichier qui commence bien par < …\n" ); */

  /* Lecture de la première balise : est-ce bien du XML Moodle ? */
  balise = XML_LitBalise( fichier, &code_erreur, VRAI );
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
    printf( "Balise lue : %s\n", balise );
    free( balise ); balise = NULL;

    balise = XML_LitBalise( fichier, &code_erreur, VRAI );
    if ( NULL == balise ) {
      ERREUR( "Impossible de trouver le début des questions (balise <quiz>) [code %d]",
	      code_erreur );
      (void) fclose( fichier ); fichier = NULL;
      return -5;
    }
    /* printf( "Comparaison : %d [%s]\n", strcmp( "<quiz>", balise ), balise ); */
  } while( strcmp( "<quiz>", balise ) != 0 );
  /* printf( "C'est bien un fichier qui ressemble à du Moodle…\n" ); */
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

    balise = XML_LitBalise( fichier, &code_erreur, VRAI );
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


/* ———————————————————————————————————————————————————————————————————— *
 |                                                                      |
 | 		   Traitement générique XML (simpliste)                 |
 |                                                                      |
 * ———————————————————————————————————————————————————————————————————— */

/* ———————— Lecture d'une balise XML (ou HTML) ———————— 
   
   fichier       : le fichier à lire
   code_erreur   : pointeur sur un entier qui recevra le code d'erreur éventuel
   avertissement : VRAI = on veut lire une balise,
                          on affiche si texte ignoré avant de trouver la balise
		   FAUX = on cherche une balise,
		          on a donc des chances d'avoir du texte ignoré
   
   Renvoie NULL en cas d'erreur, quelle qu'elle soit
                et remet alors le curseur du fichier là où il était

	   une chaîne contenant la balise, <> inclus, sinon. À libérer avec free()
 */

char *XML_LitBalise( FILE *fichier, int *code_erreur, int avertissement )
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

  while( c != '<' ) {
    /* On n'est pas sur une ouverture de balise : on lit jusqu'à en trouver une… */
    if ( 0 == isspace( c ) ) {
      /* Les retours à la ligne, espaces… sont tolérés */
      if ( VRAI == avertissement ) {
	(void) fprintf( stderr, "%s,%s l.%u : Attention, on n'est pas sur une balise…"
			" [1er caractère non-espace trouvé : %c]\n",
			__FILE__, __FUNCTION__, __LINE__, c );
	avertissement = FAUX;	/* Un seul message suffit ! */
      }
    }

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
  ungetc( c, fichier );	/* On remet le caractère pour être sûr qu'il soit inclus dans la balise */

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
  /* printf( "Balise lue : %s\n", balise ); */
  return balise;
}


/* ———————— Lecture d'une balise XML (ou HTML) ———————— 
   
   fichier     : le fichier à lire
   code_erreur : pointeur sur un entier qui recevra le code d'erreur
   
   Renvoie NULL en cas d'erreur, quelle qu'elle soit
                et remet alors le curseur du fichier là où il était

	   une chaîne contenant le contenu de la balise, balises exclues, sinon.
	   À libérer avec free()
 */

char *XML_LitContenuBalise( FILE *fichier, int *code_erreur )
{
  char *texte;
  fpos_t position, debut_texte;
  int c, retour;
  unsigned long lg_texte = 0, i, n_lu;

  /* Vérifications initiales */
  if ( NULL == fichier ) {
    ERREUR( "Pas de descripteur de fichier" );
    if ( code_erreur != NULL ) *code_erreur = -70;
    return NULL;
  }

  /* On mémorise la position initiale */
  retour = fgetpos( fichier, &position );
  if ( retour != 0 ) {
    ERREUR( "Problème dans fgetpos [code : %d / %s] — lecture annulée",
	    errno, strerror( errno ) );
    if ( code_erreur != NULL ) *code_erreur = -71;
    return NULL;
  }

  /* Lecture du premier caractère (hors espace, retour à la ligne…) */
  c = fgetc( fichier );
  while( isspace( c ) ) {
    c = fgetc( fichier );
    if ( EOF == c ) {
      ERREUR( "Fin de fichier atteinte en voulant lire une balise…" );
      if ( code_erreur != NULL ) *code_erreur = -72;

      /* On se remet au début… */
      retour = fsetpos( fichier, &position );
      if ( retour != 0 ) {
	ERREUR( "Problème dans fsetpos [code : %d / %s]",
		errno, strerror( errno ) );
      }

      return NULL;
    }
  }

  /* La, on est bien sur le début du contenu proprement dit */
  if ( '<' == c ) {
    int c_avant, c_avant_avant;

    c = fgetc( fichier );
    if ( '/' == c ) {
      /* </ : balise fermante, normalement ne peut être que celle de l'ouvrante ici
	  ⇒ la balise ne contient rien…
       */
      /* On recule de deux caractères… */
      retour = fseek( fichier, -2, SEEK_CUR );
      if ( retour != 0 ) {
	ERREUR( "Problème de rembobinage du fichier [code %d : %s]",
		errno, strerror( errno ) );
	if ( code_erreur != NULL ) *code_erreur = +1;
      }

      /* On mémorise une chaîne d'un seul caractère : chaîne vide… */
      texte = (char *) malloc( 1 );
      if ( NULL == texte ) {
	ERREUR( "Impossible de réserver une chaîne vide !" );
	if ( code_erreur != NULL ) *code_erreur = -73;
	return NULL;
      }
      *texte = 0;

      /* On renvoie la chaîne vide que l'on a construite */
      if ( code_erreur != NULL ) *code_erreur = 0;
      return texte;
    } else if ( '!' != c ) {
      /* Ouverture de balise : cas non-géré
	  ⇒ on ignore ce début de balise et on renvoie une erreur
       */
      ERREUR( "Balise au sein d'une balise Moodle finale — chargement problématique" );
      if ( code_erreur != NULL ) *code_erreur = +10;
      return NULL;
    }

    /* Si on est là, commence par <! : début de balise CDATA, que l'on lit */
    /* On recule de deux caractères pour avoir toute la balise… */
    retour = fseek( fichier, -2, SEEK_CUR );
    if ( retour != 0 ) {
      ERREUR( "Problème de rembobinage du fichier [code %d : %s]",
	      errno, strerror( errno ) );
      if ( code_erreur != NULL ) *code_erreur = +1;
    }

    /* On mémorise la position du début de la balise [peut être différente de celle du début… */
    retour = fgetpos( fichier, &debut_texte );

    /* On lit les trois premiers caractères [devraient être <![] */
    c_avant_avant = fgetc( fichier );
    c_avant       = fgetc( fichier );
    c             = fgetc( fichier );
    if ( ( c_avant_avant != '<' ) || ( c_avant != '!' ) || ( c != '[' ) ) {
      ERREUR( "Début de balise <![CDATA[ incorrect : %c%c%c\n",
	      c_avant_avant, c_avant, c );

      /* On se remet au début… */
      retour = fsetpos( fichier, &position );
      if ( retour != 0 ) {
	ERREUR( "Problème dans fsetpos [code : %d / %s]",
		errno, strerror( errno ) );
      }

      return NULL;
    }
    lg_texte = 3;		/* Déjà trois caractères… */

    do {
      c_avant_avant = c_avant;
      c_avant       = c;
      c             = fgetc( fichier );
      lg_texte++;
      if ( EOF == c ) {
      }
    } while ( ( c_avant_avant != ']' ) || ( c_avant != ']' ) || ( c != '>' ) );

  } else {
    /* On annule la lecture du 1er caractère utile */
    ungetc( c, fichier );

    /* On mémorise la position du début de la balise [peut être différente de celle du début… */
    retour = fgetpos( fichier, &debut_texte );
    if ( retour != 0 ) {
      ERREUR( "Problème dans fgetpos [code : %d / %s] — lecture annulée",
	      errno, strerror( errno ) );
      if ( code_erreur != NULL ) *code_erreur = -53;
      return NULL;
    }
    
    /* On lit jusqu'à trouver le début de la balise fermante
       (postule qu'il n'y ait pas de balise ouvrante dans ce lot <balise>XXX</balise>)
     */
    c = fgetc( fichier );	/* On relie le 1er caractère de la balise… */
    while ( c != '<' ) {
      lg_texte++;		/* Le caractère lu précédemment compte… */

      c = fgetc( fichier );
      if ( EOF == c ) {
	ERREUR( "Fin de fichier atteinte en voulant lire une balise…" );
	if ( code_erreur != NULL ) *code_erreur = -75;

	/* On se remet au début… */
	retour = fsetpos( fichier, &position );
	if ( retour != 0 ) {
	  ERREUR( "Problème dans fsetpos [code : %d / %s]",
		  errno, strerror( errno ) );
	}

	return NULL;
      }
    }
    /* On annule la lecture du dernier caractère : début de la balise suivante */
    ungetc( c, fichier );

    /* printf( "Contenu brut de %lu caractères à lire\n", lg_texte ); */
  }

  /* On réserve & initialise la mémoire pour le contenu */
  texte = (char *) malloc( sizeof( char ) * ( lg_texte + 1 ) );
  for ( i = 0 ; i <= lg_texte ; i++ ) texte[ i ] = 0;

  /* On se replace au début de la chaîne à lire */
  retour = fsetpos( fichier, &debut_texte );
  if ( retour != 0 ) {
    ERREUR( "Problème dans fsetpos [code : %d / %s] — Lecture annulée",
	    errno, strerror( errno ) );
    free( texte ) ; texte = NULL;
    if ( code_erreur != NULL ) *code_erreur = -57;
    return NULL;
  }

  /* … et on relit tous en une fois */
  n_lu = fread( texte, sizeof( char ), lg_texte, fichier );
  if ( n_lu != lg_texte ) {
    (void) fprintf( stderr, "%s,%s l.%u : Discordance de longueur dans la lecture de la balise [%lu vs %lu]\n",
		    __FILE__, __FUNCTION__, __LINE__, n_lu, lg_texte );
  }

  /* On renvoie la chaîne que l'on a construite */
  /* printf( "Texte lu : %s\n", texte ); */
  if ( code_erreur != NULL ) *code_erreur = 0;
  return texte;
}

/* ———————— Lecture d'une balise booléenne ————————
   
   fichier     : le fichier à lire
   
   Renvoie -1 en cas d'erreur, quelle qu'elle soit
            0 si la balise vaut FAUX (0, false…)
	    1 si la balise vaut VRAI (1, true…)
 */

int XML_LitBaliseBinaire(FILE *fichier)
{
  char *texte_balise;
  unsigned long i;
  int code_erreur;

  if ( NULL == fichier ) return -1;

  /* On lit le contenu de la balise */
  texte_balise = XML_LitContenuBalise( fichier, &code_erreur );
  if ( NULL == texte_balise ) {
    (void) fprintf( stderr, "%s,%s l.%u : Impossible de lire la balise booléenne [code %d]\n",
		    __FILE__, __FUNCTION__, __LINE__, code_erreur );
    return -1;
  }

  /* La balise est-elle numérique ? */
  if ( ( *texte_balise >= '0' ) && ( *texte_balise <= '9' ) ) {
    int code_erreur;

    code_erreur = atoi( texte_balise );
    free( texte_balise ); texte_balise = NULL;

    return ( code_erreur == 0 ) ? 0 : 1;
  }
 
  /* Balise textuelle : conversion en minuscules, pour comparaison ensuite… */
  for ( i = 0 ; i < strlen( texte_balise ) ; i++ ) {
    texte_balise[ i ] = tolower( texte_balise[ i ] );
  }

  /* Quelle est la valeur de la balise ? */
  if ( strcmp( "true", texte_balise ) == 0 ) {
    free( texte_balise ); texte_balise = NULL;

    return 1;
  }
  if ( strcmp( "false", texte_balise ) == 0 ) {
    free( texte_balise ); texte_balise = NULL;

    return 0;
  }

  /* Ici : variante inconnue */
  ERREUR( "Balise booléenne à contenu inconnu [%s]",
	  texte_balise );

  /* Plus besoin */
  free( texte_balise ); texte_balise = NULL;

  /* Problème… */
  return -1;
}

/* ———————————————————————————————————————————————————————————————————— *
 |                                                                      |
 |	  	     Traitement des questions Moodle                    |
 |                                                                      |
 * ———————————————————————————————————————————————————————————————————— */

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

	balise = XML_LitBalise( fichier, code_erreur, FAUX );
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

	balise = XML_LitBalise( fichier, code_erreur, FAUX );
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
  /* printf( " Type de la question : %s\n", position ); */
  type_question = MOODLE_TypeQuestion( position );
  switch( type_question ) {
  case QUESTION_CATEGORIE:
    balise = XML_LitCategorie( fichier, balise, code_erreur, fichier_HTML );
    break;

  case QUESTION_QCM:
    balise = XML_LitQCM( fichier, balise, code_erreur, fichier_HTML );
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

    balise = XML_LitBalise( fichier, code_erreur, FAUX );
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



/* ———————————————————————————————————————————————————————————————————— *
 |                                                                      |
 |       Morceaux fréquents de lecture d'une question XML Moodle        |
 |                                                                      |
 * ———————————————————————————————————————————————————————————————————— */


/* ———————— Compter le nombre de réponses d'une question ———————— 
 
   fichier     : le fichier XML en cours d'analyse
   code_erreur : pointeur vers un entier, code erreur si erreur
 */

unsigned int XML_Q_CompterReponses(FILE *fichier, int *code_erreur)
{
  char *balise = NULL;
  int retour;
  fpos_t position_debut;
  unsigned int nombre_reponses = 0;

  if ( NULL == fichier ) return 0;

  /* On repère la position actuelle */
  retour = fgetpos( fichier, &position_debut );
  if ( retour != 0 ) {
    ERREUR( "Problème dans fgetpos [code : %d / %s] — lecture annulée",
	    errno, strerror( errno ) );
    if ( code_erreur != NULL ) *code_erreur = -81;
    return 0;
  }

  /* On lit les balises jusqu'à la fin de la question
      (sans avertissement car leur contenu sera sauté aussi…)
   */
  do {
    free( balise ); balise = NULL;
    balise = XML_LitBalise( fichier, code_erreur, FAUX );
    if ( NULL == balise ) {
      ERREUR( "Erreur en cherchant les réponses [%d]", 
	      ( NULL == code_erreur ) ? 0 : *code_erreur );
      break;
    }

    if ( strncmp( "<answer", balise, 7 ) == 0 ) {
      /* On a trouvé une balise de réponse */
      nombre_reponses++;
    }
  } while ( strcmp( "</question>", balise ) != 0 );

  /* On se remet au début… */
  retour = fsetpos( fichier, &position_debut );
  if ( retour != 0 ) {
    if ( code_erreur != NULL ) *code_erreur = -82;
    ERREUR( "Problème dans fsetpos [code : %d / %s] — lecture ultérieure attendue problématique",
	    errno, strerror( errno ) );
  }

  /* … et on renvoie le nombre de réponses lues */
  return nombre_reponses;
}

/* ———————— Traiter une réponse (à un format classique) ————————

   fichier        : le fichier XML en cours de traitement
   fichier_HTML   : le fichier HTML qui doit contenir le résultat
   balise         : la balise de réponse qui vient d'être lue
   numero_reponse : le numéro de la réponse, dans la liste
   code_erreur    : pointeur vers un entier, code erreur si erreur
 */

char *XML_TraiterReponse(FILE *fichier, FILE *fichier_HTML, char *balise,
			 unsigned int numero_reponse, int *code_erreur)
{
  char *reponse;
  double fraction;		/* La note de la réponse, en % */
  int retour;

  /* Vérifications initiales */
  if ( NULL == balise ) return NULL;
  if ( strncmp( "<answer", balise, 7 ) != 0 ) {
    ERREUR( "La balise n'est pas un type de réponse reconnu"
	    "[%s, on attend <answer]\n", balise );
    return balise;
  }

  /* On analyse le début de la balise : format <answer fraction="..."> attendu */
  reponse = balise + 7;
  if ( *reponse == '>' ) {
    (void) fprintf( stderr, "%s,%s l.%u : pas de fraction associée, supposée à 100%%\n",
		    __FILE__, __FUNCTION__, __LINE__ );
    fraction = 100;
  } else {
    if ( strncmp( " fraction=", reponse, 10 ) != 0 ) {
      (void) fprintf( stderr, "%s,%s l.%u : format de balise inconnu — ignoré, fraction nulle\n",
		      __FILE__, __FUNCTION__, __LINE__ );
      fraction = 0;
    } else {
      char *fin_fraction;

      reponse += 10;
      /* Rien n'assure que le guillemet soit obligatoire */
      if ( *reponse == '\"' ) reponse++;

      errno = 0;		/* Doc : mettre errno à 0 avant… */
      fraction = strtod( reponse, &fin_fraction );
      if ( errno != 0 ) {
	(void) fprintf( stderr, "%s,%s l.%u : ERREUR en voulant convertir la fraction en nombre [code %u, %s]\n",
			__FILE__, __FUNCTION__, __LINE__, errno, strerror( errno ) );
      }
      if ( fin_fraction == reponse ) {
	(void) fprintf( stderr, "%s,%s l.%u : Pas de fraction à convertir [balise : %s]\n",
			__FILE__, __FUNCTION__, __LINE__, balise );
      }
    }
  }

  /* On commence la réponse */
  retour = HTML_CommencerReponse( fichier_HTML, fraction, numero_reponse );
  if ( retour != 0 ) {
    (void) fprintf( stderr, "%s,%s l.%u : Problème en voulant commencer la réponse [code %d]\n",
		    __FILE__, __FUNCTION__, __LINE__, retour );
  }

  /* On analyse les balises de la réponse */
  do {
    /* Plus besoin de cette balise */
    free( balise ); balise = NULL;

    balise = XML_LitBalise( fichier, code_erreur, VRAI );
    if ( NULL == balise ) {
      ERREUR( "Balise <answer> non fermée ? [code erreur : %d]\n",
	      ( NULL == code_erreur ) ? 1987 : *code_erreur );
      return NULL;
    }

    if ( strncmp( "<text", balise, 5 ) == 0 ) {
      char *texte_reponse;

      /* On lit le contenu de cette réponse */
      texte_reponse = XML_LitContenuBalise( fichier, code_erreur );
      if ( NULL == texte_reponse ) {
	ERREUR( "Balise <text> non fermée ? [code erreur : %d]\n",
	      ( NULL == code_erreur ) ? 1987 : *code_erreur );
      }

      /* Et on l'affiche… */
      retour = HTML_TexteReponse( fichier_HTML, texte_reponse );
      if ( retour != 0 ) {
	(void) fprintf( stderr, "%s,%s l.%u : ERREUR en voulant écrire la réponse [code %d]\n",
			__FILE__, __FUNCTION__, __LINE__, retour );
      }

      /* Plus besoin d'elle */
      free( texte_reponse ) ; texte_reponse = NULL;

      /* On lit la balise suivante... qui devrait être </text> */
      free( balise ); balise = NULL;

      balise = XML_LitBalise( fichier, code_erreur, FAUX );
      if ( NULL == balise ) {
	ERREUR( "Balise <text> non fermée ? [code erreur : %d]\n",
		( NULL == code_erreur ) ? 1987 : *code_erreur );
	return NULL;
      }
    } else if ( strcmp( "</text>", balise ) == 0 ) {
      /* Balise </text> fermante, normale : rien à faire */
    } else if ( strcmp( "</answer>", balise ) != 0 ) {
      (void) fprintf( stderr, "%s,%s l.%u : Balise inconnue — ignorée [%s]\n",
		      __FILE__, __FUNCTION__, __LINE__, balise );
    }
  } while ( strcmp( "</answer>", balise ) != 0 );
  
  /* On termine la réponse… */
  retour = HTML_FinirReponse( fichier_HTML );

  /* On renvoie la dernière balise lue */
  return balise;
}


/* ———————— Trouver le titre d'une question XML Moodle ———————— 
 */

char *XML_Q_TrouverTitre(FILE *fichier, int *code_erreur)
{
  char *balise, *titre;
  int premiere_balise = VRAI, retour;
  fpos_t position_debut;

  /* On mémorise la position du début de la question */
  retour = fgetpos( fichier, &position_debut );
  if ( retour != 0 ) {
    ERREUR( "Problème dans fgetpos [code : %d / %s] — lecture annulée",
	    errno, strerror( errno ) );
    if ( code_erreur != NULL ) *code_erreur = -81;
    return NULL;
  }

  /* On lit la 1re balise de la question */
  balise = XML_LitBalise( fichier, code_erreur, VRAI );
  if ( NULL == balise ) {
    ERREUR( "Impossible de trouver une balise" );
    return NULL;
  }

  /* Tant que cette balise n'est pas la bonne… */
  while( strncmp( "<name", balise, 5 ) != 0 ) {
    premiere_balise = FAUX;	/* C'est donc que le nom est au milieu ou absent */
    free( balise ); balise = NULL;

    /* On récupère la balise suivante… */
    balise = XML_LitBalise( fichier, code_erreur, FAUX );
    if ( NULL == balise ) {
      ERREUR( "Problème en cherchant le nom de la question [code %d]\n",
	      *code_erreur );
      return NULL;
    }
    if ( strcmp( "</question>", balise ) == 0 ) {
      (void) fprintf( stderr, "%s,%s l.%u : Question sans balise « nom »\n",
		      __FILE__, __FUNCTION__, __LINE__ );
      /* On se remet au début de la question */
      retour = fsetpos( fichier, &position_debut );
      if ( retour != 0 ) {
	ERREUR( "Problème dans fsetpos [code : %d / %s] — lecture ultérieure attendue problématique",
		errno, strerror( errno ) );
      }

      /* On renvoie NULL, mais sans erreur */
      if ( code_erreur != NULL ) *code_erreur = 0;
      return NULL;
    }
  }

  /* Si on est là : on a lu la balise <name*> */
  do {
    free( balise ); balise = NULL;

    /* On doit lire donc une balise texte */
    balise = XML_LitBalise( fichier, code_erreur, FAUX );
    if ( NULL == balise ) {
      ERREUR( "Problème en voulant lire le contenu de la balise <name> [code %d]\n",
	      *code_erreur );
      return NULL;
    }
    if ( strcmp( "</name>", balise ) == 0 ) {
      /* Balise vide ou sans texte */
      if ( FAUX == premiere_balise ) {
	retour = fsetpos( fichier, &position_debut );
	if ( retour != 0 ) {
	  ERREUR( "Problème dans fsetpos [code : %d / %s] — lecture ultérieure attendue problématique",
		  errno, strerror( errno ) );
	}
      }

      ERREUR( "Balise <name> sans balise <text>. Format inconnu." );

      /* On libère la balise */
      free( balise ); balise = NULL;

      /* On renvoie NULL, mais sans erreur */
      if ( code_erreur != NULL ) *code_erreur = 0;
      return NULL;
    }
  } while ( strcmp( "<text>", balise ) != 0 );

  /* On libère la balise : plus besoin d'elle… */
  free( balise ); balise = NULL;

  /* Si on est là, on n'a plus qu'à lire le texte du nom */
  titre = XML_LitContenuBalise( fichier, code_erreur );
  if ( NULL == titre ) {
    ERREUR( "Erreur en voulant lire le titre d'une question (après balise <text>)" );
    return NULL;
  }

  /* Si première balise [cas le lus fréquent] :
       inutile de rembobiner, autant continuer à lire directement.
       Pour être tranquille, on lit les deux balises </text> et </name>
   */
  if ( VRAI == premiere_balise ) {
    balise = XML_LitBalise( fichier, code_erreur, VRAI ); /* Normalement, </text> */
    if ( NULL == balise ) {
      ERREUR( "Problème en voulant lire la balise </text> [code %d]\n",
	      *code_erreur );
      return NULL;
    }
    if ( strcmp( balise, "</text>" ) != 0 ) {
      ERREUR( "Balise inattendue : %s (au lieu de </text>)", balise );
    }

    do {
      free( balise ); balise = NULL;

      balise = XML_LitBalise( fichier, code_erreur, VRAI ); /* Normalement, </text> */
      if ( NULL == balise ) {
	ERREUR( "Problème en cherchant la balise </name> [code %d]\n",
		*code_erreur );
	return NULL;
      }
    } while( strcmp( balise, "</name>" ) != 0 );

    /* On libère la dernière balise : </base>, inutile */
    free( balise ); balise = NULL;
  } else {
    /* Sinon, on se remet au début de la question */
    retour = fsetpos( fichier, &position_debut );
    if ( retour != 0 ) {
      ERREUR( "Problème dans fsetpos [code : %d / %s] — lecture ultérieure attendue problématique",
	      errno, strerror( errno ) );
    }
  }

  /* On renvoie le titre de la question */
  return titre;
}

/* ———————— Trouver l'énoncé d'une question XML Moodle ———————— 
 */

char *XML_Q_TrouverEnonce(FILE *fichier, int *code_erreur)
{
  char *balise, *enonce;
  int premiere_balise = VRAI, retour;
  fpos_t position_debut;

  /* On mémorise la position du début de la question */
  retour = fgetpos( fichier, &position_debut );
  if ( retour != 0 ) {
    ERREUR( "Problème dans fgetpos [code : %d / %s] — lecture annulée",
	    errno, strerror( errno ) );
    if ( code_erreur != NULL ) *code_erreur = -81;
    return NULL;
  }

  /* On lit la 1re balise de la question */
  balise = XML_LitBalise( fichier, code_erreur, VRAI );
  if ( NULL == balise ) {
    ERREUR( "Impossible de trouver une balise" );
    return NULL;
  }

  /* Tant que cette balise n'est pas la bonne… */
  while( strncmp( "<questiontext", balise, 13 ) != 0 ) {
    (void) fprintf( stderr, "%s,%s l.%u : Balise ignorée: %s\n",
		    __FILE__, __FUNCTION__, __LINE__, balise );
    premiere_balise = FAUX;	/* C'est donc que le nom est au milieu ou absent */
    free( balise ); balise = NULL;

    /* On récupère la balise suivante… */
    balise = XML_LitBalise( fichier, code_erreur, FAUX );
    if ( NULL == balise ) {
      ERREUR( "Problème en cherchant le nom de la question [code %d]\n",
	      *code_erreur );
      return NULL;
    }
    if ( strcmp( "</question>", balise ) == 0 ) {
      (void) fprintf( stderr, "%s,%s l.%u : Question sans balise « énoncé »\n",
		      __FILE__, __FUNCTION__, __LINE__ );
      /* On se remet au début de la question */
      retour = fsetpos( fichier, &position_debut );
      if ( retour != 0 ) {
	ERREUR( "Problème dans fsetpos [code : %d / %s] — lecture ultérieure attendue problématique",
		errno, strerror( errno ) );
      }

      /* On renvoie NULL, mais sans erreur */
      if ( code_erreur != NULL ) *code_erreur = 0;
      return NULL;
    }
  }

  /* Si on est là : on a lu la balise <questiontext*> */
  do {
    free( balise ); balise = NULL;

    /* On doit lire donc une balise texte */
    balise = XML_LitBalise( fichier, code_erreur, VRAI );
    if ( NULL == balise ) {
      ERREUR( "Problème en voulant lire le contenu de la balise <questiontext> [code %d]\n",
	      *code_erreur );
      return NULL;
    }
    if ( strcmp( "</questiontext>", balise ) == 0 ) {
      /* Balise vide ou sans texte */
      if ( FAUX == premiere_balise ) {
	retour = fsetpos( fichier, &position_debut );
	if ( retour != 0 ) {
	  ERREUR( "Problème dans fsetpos [code : %d / %s] — lecture ultérieure attendue problématique",
		  errno, strerror( errno ) );
	}
      }

      ERREUR( "Balise <questiontext> sans balise <text>. Format inconnu." );

      /* On libère la balise */
      free( balise ); balise = NULL;

      /* On renvoie NULL, mais sans erreur */
      if ( code_erreur != NULL ) *code_erreur = 0;
      return NULL;
    }
  } while ( strcmp( "<text>", balise ) != 0 );

  /* On libère la balise : plus besoin d'elle… */
  free( balise ); balise = NULL;

  /* Si on est là, on n'a plus qu'à lire le texte du nom */
  enonce = XML_LitContenuBalise( fichier, code_erreur );
  if ( NULL == enonce ) {
    ERREUR( "Erreur en voulant lire le titre d'une question (après balise <text>)" );
    return NULL;
  }

  /* Si première balise [cas le lus fréquent] :
       inutile de rembobiner, autant continuer à lire directement.
       Pour être tranquille, on lit les deux balises </text> et </name>
   */
  if ( VRAI == premiere_balise ) {
    balise = XML_LitBalise( fichier, code_erreur, VRAI ); /* Normalement, </text> */
    if ( NULL == balise ) {
      ERREUR( "Problème en voulant lire la balise </text> [code %d]\n",
	      *code_erreur );
      return NULL;
    }
    if ( strcmp( balise, "</text>" ) != 0 ) {
      ERREUR( "Balise inattendue : %s (au lieu de </text>)", balise );
    }

    do {
      free( balise ); balise = NULL;

      balise = XML_LitBalise( fichier, code_erreur, VRAI ); /* Normalement, </text> */
      if ( NULL == balise ) {
	ERREUR( "Problème en cherchant la balise </questiontext> [code %d]\n",
		*code_erreur );
	return NULL;
      }
    } while( strcmp( balise, "</questiontext>" ) != 0 );

    /* On libère la dernière balise : </questiontext>, inutile */
    free( balise ); balise = NULL;
  } else {
    /* Sinon, on se remet au début de la question */
    retour = fsetpos( fichier, &position_debut );
    if ( retour != 0 ) {
      ERREUR( "Problème dans fsetpos [code : %d / %s] — lecture ultérieure attendue problématique",
	      errno, strerror( errno ) );
    }
  }

  /* On renvoie le titre de la question */
  return enonce;
}
