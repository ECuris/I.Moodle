/* Conversion d'un fichier XML Moodle en HTML pour impression
 * © Emmanuel CURIS, septembre 2016
 *
 * À la demande de l'équipe Moodle, de Chantal Guihenneuc
    & de Jean-Michel Delacotte
 *
 * Fichier résumant les différentes fonctions utilisables
 */

#ifndef IMPRESSION_H_
# define IMPRESSION_H_		/* Protection contre l'inclusion multiple */

# define VERSION "0.1 bêta, 17-09-2016" /* Numéro de version… */

# ifndef VRAI
#  define FAUX (0)
#  define VRAI (1)
# endif

/* -------- Les fonctions pour la lecture en XML ————————— */

int XML_ImprimerFichier( const char *nom_fichier ); /* Traitement d'un fichier */

char *XML_LitBalise       ( FILE *fichier, int *code_erreur );
char *XML_LitContenuBalise( FILE *fichier, int *code_erreur );

char *XML_Q_TrouverTitre  ( FILE *fichier, int *code_erreur );

/* ———— Les différents types de question possibles ———— */

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

char *XML_LitCategorie( FILE *fichier, char *balise, int *code_erreur, FILE *fichier_HTML );
char *XML_LitQCM      ( FILE *fichier, char *balise, int *code_erreur, FILE *fichier_HTML );

/* ———————— Les fonctions pour l'écriture en HTML ———————— */

FILE *HTML_CommencerFichier( const char *nom_fichier );
int   HTML_TerminerFichier ( FILE *fichier_HTML, int complet );

char *HTML_PreparerTexte   ( char *texte );

int HTML_CreerCategorie ( FILE *fichier_HTML, char *nom_categorie );
int HTML_FermerCategorie( FILE *fichier_HTML );

int HTML_CreerQuestion( FILE *fichier_HTML, int type, char *titre );
int HTML_FinirQuestion( FILE *fichier_HTML );

/* ———————— Les fonctions pour l'interface ——————————————— */

#define ERREUR(message, ...) ERR_Message( message, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__ )
void ERR_Message(const char *message, const char *fichier, const char *fonction, unsigned int ligne, ...);

#endif	/* IMPRESSION_H_  */
