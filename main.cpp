#include <iostream>
#include <vector>
#include <ctime>

#include "monde.hpp"
#include "niv1.hpp"
#include "niv2.hpp"

/*
 * Lecture du fichier par le root, sans créer les habitants (liste_hab = NULL).
 * Les ponts vers une île inexistante (ex : "1 10") sont ignorés.
 */
struct monde* lecture_monde(const char* filename) {

    struct monde* m = (struct monde*)malloc(sizeof(struct monde));

    ifstream f(filename);
    if(!f){
        std::cout << "Erreur : impossible d'ouvrir " << filename << "\n";
        MPI_Abort(MPI_COMM_WORLD,1);
    }

    f >> m->nb_iles;
    int nb = m->nb_iles;
    m->mes_iles = (struct ile**)malloc(m->nb_iles*sizeof(struct ile*));


    for (int i=0; i<nb; i++) {
        m->mes_iles[i] = (struct ile*)malloc(sizeof(struct ile));
        f>>(m->mes_iles[i])->num>>(m->mes_iles[i])->nom >> (m->mes_iles[i])->nb_hab;
        m->mes_iles[i]->liste_hab = NULL;
        m->mes_iles[i]->inflation = inflation_initiale;
        m->mes_iles[i]->gini = 0.0;
        m->mes_iles[i]->surface = 0;
    }

    m->mat_adj = (int*)calloc(nb*nb,sizeof(int));

    int nb_arete;
    f>>nb_arete;

    int a, b;
    for (int i=0; i<nb_arete; i++) {
        f>>a>>b;
        if (a < 0 || a >= nb || b < 0 || b >= nb || a == b) {
            continue;
        }
        m->mat_adj[a*nb+b] = 1;
        m->mat_adj[b*nb+a] = 1;
    }

    m->inflation = inflation_initiale;

    return m;

}


/*
 * Construit le monde sur un processus autre que le root, à partir des données reçues.
 */
struct monde* construire_monde(int nb, std::vector<char>& noms, std::vector<int>& nb_hab, std::vector<int>& adj){
    struct monde* m = (struct monde*)malloc(sizeof(struct monde));
    m->nb_iles = nb;
    m->inflation = inflation_initiale;
    m->mes_iles = (struct ile**)malloc(nb*sizeof(struct ile*));
    for(int i=0; i<nb;i++){
        m->mes_iles[i] = (struct ile*)malloc(sizeof(struct ile));
        m->mes_iles[i]->num = i;
        strncpy(m->mes_iles[i]->nom,&noms[i*10],10);
        m->mes_iles[i]->nb_hab = nb_hab[i];
        m->mes_iles[i]->liste_hab = NULL;
        m->mes_iles[i]->inflation = inflation_initiale;
        m->mes_iles[i]->gini = 0.0;
        m->mes_iles[i]->surface = 0;
    }
    m->mat_adj = (int*)malloc(nb*nb*sizeof(int));
    for(int i=0; i<nb*nb;i++){
        m->mat_adj[i] = adj[i];
    }
    return m;
}


/*
 * Le root lit le fichier et envoie à tous (Bcast) : le nombre d'îles, les noms,
 * le nombre d'habitants et mat_adj. Renvoie le monde sur chaque processus.
 */
struct monde* diffuser_monde(const char* nom_fichier, int rang, int root){
    struct monde* m = NULL;
    int nb = 0;

    if(rang == root){
        m = lecture_monde(nom_fichier);
        nb = m->nb_iles;
    }

    MPI_Bcast(&nb,1,MPI_INT,root,MPI_COMM_WORLD);

    std::vector<char> noms(nb*10);
    std::vector<int> nb_hab(nb);
    std::vector<int> adj(nb*nb);

    if(rang == root){
        for(int i=0; i<nb;i++){
            strncpy(&noms[i*10],m->mes_iles[i]->nom,10);
            nb_hab[i] = m->mes_iles[i]->nb_hab;
        }
        for(int i=0; i<nb*nb;i++){
            adj[i] = m->mat_adj[i];
        }
    }

    MPI_Bcast(noms.data(),nb*10,MPI_CHAR,root,MPI_COMM_WORLD);
    MPI_Bcast(nb_hab.data(),nb,MPI_INT,root,MPI_COMM_WORLD);
    MPI_Bcast(adj.data(),nb*nb,MPI_INT,root,MPI_COMM_WORLD);

    if(rang != root){
        m = construire_monde(nb,noms,nb_hab,adj);
    }
    return m;
}


/*
 * Répartition round robin : l'île i est gérée par le processus i % size.
 */
std::vector<int> repartition(int nb, int size){
    std::vector<int> owner(nb);
    for(int i=0; i<nb;i++){
        owner[i] = i % size;
    }
    return owner;
}


/*
 * Chaque processus génère les habitants de ses îles uniquement.
 */
void generer_iles_locales(struct monde* m, std::vector<int>& owner, int rang){
    srand(time(NULL) + rang);
    for(int i=0; i<m->nb_iles;i++){
        if(owner[i] == rang){
            generer_habitants(m->mes_iles[i]);
        }
    }
}


/*
 * Une itération = une année : niveau 1 sur les îles locales, puis niveau 2.
 */
void simulation(struct monde* m, std::vector<int>& owner, int rang, int annees){
    for(int annee=1; annee<=annees;annee++){
        for(int i=0; i<m->nb_iles;i++){
            if(owner[i] == rang){
                evolution_niv1(m->mes_iles[i]);
            }
        }
        evolution_niv2(m,owner.data(),annee,MPI_COMM_WORLD);
    }
}


/*
 * Libère le monde : les habitants seulement pour les îles locales (NULL ailleurs).
 */
void liberer_monde_local(struct monde* m){
    for(int i=0; i<m->nb_iles;i++){
        if(m->mes_iles[i]->liste_hab != NULL){
            liberer_habitant(m->mes_iles[i]);
        }
        free(m->mes_iles[i]);
    }
    free(m->mes_iles);
    free(m->mat_adj);
    free(m);
}


/*
 * ./projet <fichier> <root> [annees]
 * Mise en place MPI, simulation, affichage du temps (max sur les processus), libération.
 */
int main(int argc, char* argv[]){
    MPI_Init(&argc, &argv);
    int rang;
    int size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rang);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if(argc < 3){
        if(rang == 0){
            std::cout << "Erreur argument inssufisant : ./projet <fichier> <root> [annees]\n";
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    char* nom_fichier = argv[1];
    int root = std::atoi(argv[2]);
    int annees = 10;
    if(argc > 3){
        annees = std::atoi(argv[3]);
    }

    if(root < 0 || root >= size){
        if(rang == 0){
            std::cout << "Erreur : root doit etre entre 0 et " << size-1 << "\n";
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    double debut = MPI_Wtime();

    struct monde* m = diffuser_monde(nom_fichier,rang,root);
    std::vector<int> owner = repartition(m->nb_iles,size);
    generer_iles_locales(m,owner,rang);
    simulation(m,owner,rang,annees);

    double temps = MPI_Wtime() - debut;
    double temps_max = 0;
    MPI_Reduce(&temps,&temps_max,1,MPI_DOUBLE,MPI_MAX,root,MPI_COMM_WORLD);
    if(rang == root){
        std::cout << m->nb_iles << " iles, " << size << " processus, " << annees << " ans : " << temps_max << " s\n";
    }

    liberer_monde_local(m);
    MPI_Finalize();
    return 0;
}
