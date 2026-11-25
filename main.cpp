#include <iostream>

#include "monde.hpp"
#include "niv1.hpp"

int main(void) {

    struct monde* mm = generation("monde1.txt");
    for (int i = 0; i < mm->nb_iles; i++) {
        mm->mes_iles[i]->inflation = mm->inflation; 
        mm->mes_iles[i]->gini = 0.0;
    }

    int annee = 5;
    for (int i = 1; i <= annee; i++) {
        for (int j = 0; j < mm->nb_iles; j++) {
            evolution_niv1(mm->mes_iles[i]);
        }
        cout <<"Annee " << i << endl;
        for (int k = 0; k < mm->nb_iles; k++) {
            cout << " " << mm->mes_iles[k]->nom << " : gini" <<mm->mes_iles[k]->gini << endl;
        }
    }

    affichage_monde(mm);

    liberer_monde(&mm);

    return EXIT_SUCCESS;
}
