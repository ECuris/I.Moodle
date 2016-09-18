/* Conversion d'un fichier XML Moodle en HTML pour impression
 * © Emmanuel CURIS, septembre 2016
 *
 * À la demande de l'équipe Moodle, de Chantal Guihenneuc
    & de Jean-Michel Delacotte
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "impression.h"

/* -------- Variables globales -------- */

/* -------- Fonctions locales -------- */

/* -------- Réalisation pratique -------- */

void ERR_Message(const char *message, const char *fichier, const char *fonction, unsigned int ligne, ...)
{
  va_list parametres;

  /* On récupère la liste d'arguments variables */
  va_start( parametres, ligne );

  /* On commence par l'afficher sur la sortie standard, quoi qu'il arrive… */
  (void) fprintf( stderr, "%s,%s l.%u : ",
		  fichier, fonction, ligne );
  (void) vfprintf( stderr, message, parametres );

  /* Terminé avec cette liste, on la libère… */
  va_end( parametres );
  
  /* Faut-il une boîte d'alerte ? */
  #ifdef BIB_X_
  #endif  /* BIB_X_ */

  
 
  /* Terminé */
}
