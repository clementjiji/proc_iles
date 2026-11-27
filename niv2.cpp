#include "niv2.hpp"
#include <algorithm>
#include <vector>
#include <cmath>

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



void regle1(struct monde* m, const int* owner, MPI_Comm comm){
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


void regle2(struct monde* m, const int* owner, MPI_Comm comm){
    /*
    * Règle 2, tous les 5 ans.
    * 1. Échange des médianes avec les voisines (Isend/Irecv, tag = n° de l'île), rangées dans recu.
    * 2. Si l'écart des médianes est d'au moins 5 % : la plus riche lance 4 dés, l'autre 2,
    *    on garde le meilleur dans de[], puis échange des dés avec les voisines (de_recu).
    * 3. Si la plus riche a un dé strictement supérieur, le pont est supprimé dans mat_adj
    *    (les deux processus du pont prennent la même décision).
    */
    int rang;
    int nb = m->nb_iles;
    MPI_Comm_rank(comm,&rang);
    std::vector<MPI_Request> requetes;
    std::vector<float> med(nb);
    
    std::vector<float> recu(nb*nb);

    std::vector<int>de(nb*nb, 0);
    std::vector<int>de_recu(nb*nb, 0);

    for(int i=0;i<nb;i++){
        if(owner[i] == rang){
            med[i] = mediane(m->mes_iles[i]);
        }
        
    }


    for(int i=0;i<nb;i++){
        if(owner[i] == rang){
            for(int j=0;j<nb;j++){
                if(m->mat_adj[i*nb + j] == 1 && j!=i && owner[j]!=rang){
                    MPI_Request r;
                    MPI_Isend(&med[i],1,MPI_FLOAT,owner[j],i,comm,&r);
                    requetes.push_back(r);
                    MPI_Irecv(&recu[i*nb + j],1,MPI_FLOAT,owner[j],j,comm,&r);
                    requetes.push_back(r);
                }
                else if (m->mat_adj[i*nb + j] == 1 && j != i && owner[j] == rang){
                    recu[i*nb+j] = med[j];
                }
            }
        }
    }
    
    MPI_Waitall(requetes.size(),requetes.data(),MPI_STATUSES_IGNORE);
    requetes.clear();

    for(int i=0;i<nb;i++){
        if(owner[i] == rang){
            for(int j = 0;j<nb;j++){
                if(m->mat_adj[i*nb + j] == 1 && j != i){
                    float med_i = med[i];
                    float med_j = recu[i*nb+j];
                    float ecart = 0;
                    if(std::max(med_i,med_j) > 0){
                        ecart = std::abs(med_i - med_j)/std::max(med_i,med_j);
                    }
                    if(ecart<0.05){
                        continue;
                    }
                    else{
                        int nb_des = 0;
                        if(med_i<med_j){
                            nb_des = 2;
                        }
                        else{
                            nb_des = 4;
                        }
                        for(int d=0;d<nb_des;d++){
                            int des = rand() % 6 + 1;
                            if(des > de[i*nb + j]){
                                de[i*nb + j] = des;
                            }
                        }
                        if(owner[j] != rang){
                            MPI_Request r;
                            MPI_Isend(&de[i*nb+j],1,MPI_INT,owner[j],i,comm,&r);
                            requetes.push_back(r);
                            MPI_Irecv(&de_recu[i*nb+j],1,MPI_INT,owner[j],j,comm,&r);
                            requetes.push_back(r);
                        }
                    }
                }
            }
        }
    }

    MPI_Waitall(requetes.size(),requetes.data(),MPI_STATUSES_IGNORE);
    requetes.clear();

    int des_j = 0;
    int des_i = 0;

    for(int i=0;i<nb;i++){
        if(owner[i]==rang){
            for(int j=0;j<nb;j++){
                if(m->mat_adj[i*nb + j] == 1 && j != i && de[i*nb + j] != 0){
                    if(owner[j] != rang){
                        des_j = de_recu[i*nb + j];
                    }
                    else{
                        des_j = de[j*nb + i];
                    }
                    des_i = de[i*nb + j];

                    bool riche_gagne;
                    if(med[i] > recu[i*nb + j]){
                        riche_gagne = des_i > des_j;
                    }
                    else{
                        riche_gagne = des_j > des_i;
                    }

                    if(riche_gagne){
                        m->mat_adj[i*nb + j] = 0;
                        m->mat_adj[j*nb + i] = 0;
                    }
                }
            }
        }
    }

}


void evolution_niv2(struct monde* m, const int* owner, int annee, MPI_Comm comm) {
    regle1(m, owner, comm);
    if(annee % 5 == 0){
        regle2(m, owner, comm);
    }

}