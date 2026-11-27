#include "niv1.hpp"


void generer_habitants(struct ile* ile) {
    ile->liste_hab = (struct habitant**)malloc(ile->nb_hab * sizeof(struct habitant*));
    for (int j = 0; j < ile->nb_hab; j++) {
        ile->liste_hab[j] = creation_habitant(ile->nom, j);
    }
    maj_gini(ile);
}

void vieillir(struct ile* ile) {
    for (int i = 0; i < ile -> nb_hab; i++) {
       ile -> liste_hab[i] -> age++;
    }
}

void init_revenu(struct ile* ile) {
    for (int i = 0; i < ile -> nb_hab; i++) {
        if (ile->liste_hab[i] -> age == 18) {
            ile ->liste_hab[i] -> revenu = revenu_min +  ((float)rand()/ (float) RAND_MAX) * (revenu_max-revenu_min);
        }
    }
}

void maj_gini(struct ile* ile) {
    float e = 0.0;
    float m = 0.0;

    for (int i = 0; i < ile -> nb_hab; i++) {
        for (int j = 0; j < ile -> nb_hab; j++) {
            if (ile->liste_hab[i]->revenu > ile->liste_hab[j]->revenu) {
                e += ile->liste_hab[i]->revenu - ile->liste_hab[j]->revenu;
            } else {
                e += ile->liste_hab[j]->revenu- ile->liste_hab[i]->revenu;
            }
        }
        m += ile->liste_hab[i]->revenu; 
    }
    if (m == 0.0) {
        ile -> gini = 0.0;
    } else {
        e = e * (1.0/((ile->nb_hab)*(ile->nb_hab)));
        m = m * (1.0/(ile->nb_hab));
        ile->gini = e/(2*m);
    }
}

void index_revenu(struct ile* ile) {
    float inf = ile->inflation; 

    for (int i = 0; i < ile->nb_hab; i++) {
        if (ile->liste_hab[i]->age >= 18) {
            ile->liste_hab[i]->revenu = ile->liste_hab[i]->revenu*(1.0+(inf/100));
        }
    }
}

void renouv_population(struct ile* ile){
    int c = rand()%(6);
    int s = ile->nb_hab;
    int l = ((ile->nb_hab) * c) / 100;

    for (int i = 0; i < l; i++) {
        free(ile->liste_hab[i]);
        ile->liste_hab[i] = creation_habitant(ile->nom, s++);
        ile->liste_hab[i]->age = 0;
        ile->liste_hab[i]->revenu = 0;

    }
}
void evolution_niv1(struct ile* ile) {
    vieillir(ile);
    init_revenu(ile);
    index_revenu(ile); //on prend d'abord en compte l'inflation puis on calcule le coefficient de gini
    maj_gini(ile);
    
    renouv_population(ile);
}