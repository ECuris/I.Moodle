# Impression d'XML Moodle en HTML
# © Emmanuel CURIS, septembre 2016

INCLUSION = -I/home/curis/Programmes/Lase/Utilitaires
INCLUSION += -D_POSIX_C_SOURCE=200112L -D_DEFAULT_SOURCE

CC_OPT  = -std=c99 -pipe -D__USE_FIXED_PROTOTYPES__ 
CC_OPT += -g -Wall -W -x c -Wno-implicit-int -Werror -D_DEVELOPPE_ -O

CC_OPT +=  $(INCLUSION)

LIBS = -L/home/curis/Programmes/Lase/Utilitaires -lutils -lm

CFLAGS = $(CC_OPT)

OBJETS = categorie.o erreurs.o lecture_XML.o principal.o qcm.o sortie_HTML.o

imprime_Moodle: $(OBJETS)
	$(CC) $(OBJETS) -o imprime_Moodle $(LIBS)

categorie.o   : categorie.c impression.h
erreurs.o     : erreurs.c impression.h
principal.o   : principal.c impression.h
qcm.o         : qcm.c impression.h
lecture_XML.o : lecture_XML.c impression.h
sortie_HTML.o : sortie_HTML.c impression.h
