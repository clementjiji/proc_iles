#include "niv2.hpp"
#include <algorithm>
#include <vector>

float mediane(struct ile* ile){

    if(ile->nb_hab == 0){
        return 0;
    }

    std::vector<float>tab;

    for(int i = 0; ile->nb_hab>i;i++){
        tab.push_back(ile->liste_hab[i]->revenu);
    }

    std::sort(tab.begin(),tab.end());

    float milieu = 0;

    if(tab.size()%2 != 0){
        milieu = tab[tab.size()/2];
    }
    else{
        milieu = (tab[tab.size()/2]+tab[tab.size()/2-1])/2;
    }

    return milieu;


}



void regle1(m, owner, comm){
    /****
        1. Échanges : pour chaque île locale i et chaque voisine j gérée par un AUTRE
 *      processus, on lance un MPI_Isend du Gini de i vers owner[j] (tag = i) et un
 *      MPI_Irecv du Gini de j (tag = j), rangé dans recu[i*nb + j].
 *      Les Isend/Irecv non bloquants évitent les interblocages et l'ordre des
 *      messages n'a pas d'importance grâce au tag (= numéro de l'île émettrice).
 *      Chaque requête est stockée dans "requetes".
 * 
 *      2. MPI_Waitall : on attend que tous les échanges soient terminés.
 * 
 *      3. Règle 1 : pour chaque île locale i, on compare son Gini à celui de ses voisines
 *      (lu directement si la voisine est locale, dans recu sinon), puis on applique
 *      le +5 % si au moins une voisine a un Gini supérieur.
     * ***/
    int rang;
    MPI_Comm_rank(comm,&rang);
    std::vector<MPI_Request> requetes;
    int nb = m->nb_iles;
    std::vector<float> recu(nb*nb);

    // Boucle 1 : lancer les échanges de Gini avec les voisines des autres processus
    for(int i=0; i<nb;i++){
        if(owner[i] == rang){
            for(int j=0;j<nb;j++){
                if(m->mat_adj[i*nb + j] == 1 && j!=i && owner[j]!=rang){
                    MPI_Request r;
                    MPI_Isend(&m->mes_iles[i]->gini,1,MPI_FLOAT,owner[j],i,comm,&r);
                    requetes.push_back(r);
                    MPI_Irecv(&recu[i*nb + j],1,MPI_FLOAT,owner[j],j,comm,&r);
                    requetes.push_back(r);
                }
            }
        }
    }

    MPI_Waitall(requetes.size(),requetes.data(),MPI_STATUSES_IGNORE);

    // Boucle 2 : règle 1
    for(int i=0; i<nb;i++){
        if(owner[i] == rang){
            bool plus = false;
            for(int j=0;j<nb;j++){
                if(m->mat_adj[i*nb + j] == 1 && j!=i){
                    float gini_j;
                    if(owner[j] == rang){
                        gini_j = m->mes_iles[j]->gini;
                    }
                    else{
                        gini_j = recu[i*nb + j];
                    }
                    if(m->mes_iles[i]->gini < gini_j){
                        plus = true;
                    }
                }
            }
            if(plus){
                for(int h=0;h<m->mes_iles[i]->nb_hab;h++){
                    if(m->mes_iles[i]->liste_hab[h]->age<25){
                        m->mes_iles[i]->liste_hab[h]->revenu *= 1.05;
                    }
                }
            }
        }
    }
}


void evolution_niv2(struct monde* m, const int* owner, int annee, MPI_Comm comm) {
    regle1(m, owner, comm);

}