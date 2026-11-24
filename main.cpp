#include <iostream>

#include "monde.hpp"

int main(void) {

    struct monde* mm = generation("monde1.txt");
    for (int i = 0; i < mm->nb_iles; i++) {
        mm->mes_iles[i]->inflation = mm->inflation; 
        mm->mes_iles[i]->gini = 0.0;
    }
    affichage_monde(mm);

    liberer_monde(&mm);

    return EXIT_SUCCESS;
}
