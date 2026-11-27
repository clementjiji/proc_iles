#ifndef NIVEAU2_HPP
#define NIVEAU2_HPP

#include "monde.hpp"

#include <mpi.h>


void evolution_niv2(struct monde* m, const int* owner, int annee, MPI_Comm comm);
float mediane(struct ile* ile);
void regle1(struct monde* m, const int* owner, MPI_Comm comm);
void regle2(struct monde* m, const int* owner, MPI_Comm comm);

#endif