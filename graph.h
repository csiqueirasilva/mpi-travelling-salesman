#ifndef GUARD_C_MPI_GRAPH
#define GUARD_C_MPI_GRAPH

void test(void);

void createGraph(int size);
void destroyGraph(void);

/*
// return true or false
int createGraph(int size);
// return true or false
int destroyGraph(void);
// return true or false
int createArtificialEdges(void);
// return true or false
int destroyArtificialEdges(void);
// return -1 in case of failure, positive integer as the weight of the path
int getCircularPathWeight(int index);
*/

#undef GUARD_C_MPI_GRAPH
#endif
