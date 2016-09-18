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

/* ———————— Les fonctions pour l'écriture en HTML ———————— */

FILE *HTML_CommencerFichier( const char *nom_fichier );
int   HTML_TerminerFichier ( FILE *fichier_HTML, int complet );

/* ———————— Les fonctions pour l'interface ——————————————— */

#define ERREUR(message, ...) ERR_Message( message, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__ )
void ERR_Message(const char *message, const char *fichier, const char *fonction, unsigned int ligne, ...);

#endif	/* IMPRESSION_H_  */
