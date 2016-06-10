#include "graph.h"

#define GRAPH_PRINT_STEP
//#define USE_MPI_MALLOC

#define TRUE 1
#define FALSE 0
#define UNDEFINED -1

#ifdef USE_MPI_MALLOC
#include <mpi.h>
#endif

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

typedef struct {
    char id;
    int step; // dijkstra
} Node, *pNode;

typedef struct StructLinkedNode {
    struct StructLinkedNode * next;
    pNode node;
} PathNode, *pPathNode;

typedef struct {
    pPathNode first;
    int totalWeight;
} Path, *pPath;

typedef struct {
    int * edges;
    pNode * nodes;
    int size;
} Graph, *pGraph;

static unsigned long timestamp;
static pGraph graph = NULL;
static pPath * artificialEdges = NULL;
static int * factorialHashTable = NULL;

static void destroyPath(pPath path);
static void printPath(pPath path);
static void printRealPath(pPath p);
static void destroyArtificialEdges(void);
static void createArtificialEdges(void);
static void initDijkstra(int * pathSizes, int * previous);
static int allStepped(void);
static int lowerPath(int * paths);
static pPathNode addPathNode(int u, pPathNode lastPathNode);
static pPath dijkstra(int src, int dst);
static int getWeightFromIndex(int start, int idx);
static pPath getPathFromIndex(int start, int idx);
static unsigned int factorial(unsigned int n);
static int getWeightFromNodes(pNode src, pNode dst);
static void getLowerPath(int startNode, int start, int end, int * lower, int * lowerKey);
static void parallelSolution(int argc, char* argv[]);

#ifdef USE_MPI_MALLOC
static int taskDivision(int size, int qtt);

int taskDivision(int size, int qtt) {
    int i;
    int buffLimit = qtt;
    int divMaster = 0;

    if (size > 0) {

        divMaster = qtt / size;
        buffLimit -= divMaster;

        for (i = 1; i < size; i++) {
            int div = qtt / size;
            buffLimit -= div;
            if (buffLimit != 0 && i == (size - 1)) {
                div += buffLimit;
            }
            MPI_Send(&div, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
        }

    }

    return divMaster;
}
#endif

void getLowerPath(int startNode, int start, int end, int * lower, int * lowerKey) {
    int i;
    *lower = INT_MAX;
    *lowerKey = -1;

#ifdef GRAPH_PRINT_STEP
    printf("searching from %d to %d\n", start, end);
#endif

    for (i = start; i <= end; i++) {
        int w = getWeightFromIndex(startNode, i);
        if (w < *lower) {
            *lower = w;
            *lowerKey = i;
        }
    }

#ifdef GRAPH_PRINT_STEP
    printf("finished searching from %d to %d\n", start, end);
#endif
}

static void sequentialSolution(void);
static unsigned long finishTimestamp(void);
static void startTimestamp(void);

void sequentialSolution(void) {
    if (graph != NULL) {

        startTimestamp();
        createArtificialEdges();

        {
            int fact = factorialHashTable[graph->size - 1] / 2 - 1;
            int lower;
            int key;
            pPath p;

            printf("Start sequential run:\n");
            getLowerPath(0, 0, fact, &lower, &key);

            p = getPathFromIndex(0, key);

            printPath(p);
            printRealPath(p);

            destroyPath(p);
        }

        destroyArtificialEdges();
        finishTimestamp();

    }
}

void startTimestamp(void) {
    struct timespec spec;
    time_t s;
    unsigned long time_in_micros;
    clock_gettime(CLOCK_REALTIME, &spec);
    s = spec.tv_sec;
    time_in_micros = spec.tv_sec;
    timestamp = time_in_micros;
}

unsigned long finishTimestamp(void) {
    struct timespec spec;
    time_t s;
    unsigned long time_in_micros;
    unsigned long ret;
    clock_gettime(CLOCK_REALTIME, &spec);
    s = spec.tv_sec;
    time_in_micros = spec.tv_sec;
    ret = time_in_micros - timestamp;
    timestamp = ret;
    printf("Total time (seconds): %lu\n", timestamp);
    return ret;
}

unsigned int factorial(unsigned int n) {
    int i = 2;
    int ret = 1;
    for (; i <= n; i++) {
        ret *= i;
    }
    return ret;
}

pPath getPathFromIndex(int start, int idx) {
    pPath ret;
    pNode * others;

    pPathNode pathNode;
    pPathNode lastPathNode = pathNode;
    int countNotUsed = graph->size - 1;
    int i, j;
    pNode startNode = graph->nodes[start];
    pNode src;
    pNode dst;

#ifndef USE_MPI_MALLOC
    ret = (pPath) malloc(sizeof (Path));
    others = (pNode*) malloc(sizeof (pNode) * (graph->size - 1));
#else
    MPI_Alloc_mem(sizeof (Path), MPI_INFO_NULL, &ret);
    MPI_Alloc_mem(sizeof (pNode) * (graph->size - 1), MPI_INFO_NULL, &others);
#endif

    if (others == NULL || ret == NULL) {
        printf("ERROR WHILE ALLOCATING MEMORY TO GET WEIGHT FROM INDEX\n");
        exit(-1);
    }

    ret->first = NULL;
    ret->totalWeight = 0;

    for (j = 0, i = 0; i < graph->size; i++) {
        if (i != start) {
            others[j++] = graph->nodes[i];
        }
    }

    src = startNode;

#ifndef USE_MPI_MALLOC
    pathNode = (pPathNode) malloc(sizeof (PathNode));
#else
    MPI_Alloc_mem(sizeof (PathNode), MPI_INFO_NULL, &pathNode);
#endif

    if (pathNode == NULL) {
        printf("Error while allocating memory to create path\n");
        exit(-1);
    }

    pathNode->node = src;
    ret->first = pathNode;

    while (countNotUsed-- > 0) {
        lastPathNode = pathNode;
        int size = (graph->size - (graph->size - countNotUsed));
        int fact = factorialHashTable[size];
        int dstCount = 0;
        pNode selected = NULL;
        int dstN = idx / fact;
        idx %= fact;
        for (i = 0; i < (graph->size - 1) && selected == NULL; i++) {
            if (others[i] != NULL) {
                if (dstCount++ == dstN) {
                    selected = others[i];
                    others[i] = NULL;
                }
            }
        }

        dst = selected;

        ret->totalWeight += getWeightFromNodes(src, dst);
        src = dst;

#ifndef USE_MPI_MALLOC
        pathNode = (pPathNode) malloc(sizeof (PathNode));
#else
        MPI_Alloc_mem(sizeof (PathNode), MPI_INFO_NULL, &pathNode);
#endif

        if (pathNode == NULL) {
            printf("Error while allocating memory to create path\n");
            exit(-1);
        }

        pathNode->node = src;
        lastPathNode->next = pathNode;
    }

    lastPathNode = pathNode;
    dst = startNode;
    ret->totalWeight += getWeightFromNodes(src, dst);

#ifndef USE_MPI_MALLOC
    pathNode = (pPathNode) malloc(sizeof (PathNode));
#else
    MPI_Alloc_mem(sizeof (PathNode), MPI_INFO_NULL, &pathNode);
#endif

    if (pathNode == NULL) {
        printf("Error while allocating memory to create path\n");
        exit(-1);
    }

    pathNode->node = dst;
    pathNode->next = NULL;
    lastPathNode->next = pathNode;

#ifndef USE_MPI_MALLOC
    free(others);
#else
    MPI_Free_mem(others);
#endif

    return ret;
}

int getWeightFromNodes(pNode src, pNode dst) {
    return graph->edges[(src->id - 'A') * graph->size + (dst->id - 'A')];
}

int getWeightFromIndex(int start, int idx) {

    pNode * others;
    int ret = 0;
    int countNotUsed = graph->size - 1;
    int i, j;
    pNode startNode = graph->nodes[start];
    pNode src;
    pNode dst;

#ifndef USE_MPI_MALLOC
    others = (pNode*) malloc(sizeof (pNode) * (graph->size - 1));
#else
    MPI_Alloc_mem(sizeof (pNode) * (graph->size - 1), MPI_INFO_NULL, &others);
#endif

    if (others == NULL) {
        printf("ERROR WHILE ALLOCATING MEMORY TO GET WEIGHT FROM INDEX\n");
        exit(-1);
    }

    for (j = 0, i = 0; i < graph->size; i++) {
        if (i != start) {
            others[j++] = graph->nodes[i];
        }
    }

    src = startNode;

#ifdef GRAPH_PRINT_STEP
    printf("%d - %c", idx, src->id);
#endif

    while (countNotUsed-- > 0) {
        int size = (graph->size - (graph->size - countNotUsed));
        int fact = factorialHashTable[size];
        int dstCount = 0;
        pNode selected = NULL;
        int dstN = idx / fact;
        idx %= fact;
        for (i = 0; i < (graph->size - 1) && selected == NULL; i++) {
            if (others[i] != NULL) {
                if (dstCount++ == dstN) {
                    selected = others[i];
                    others[i] = NULL;
                }
            }
        }

        dst = selected;
#ifdef GRAPH_PRINT_STEP
        printf("%c", dst->id);
#endif
        ret += getWeightFromNodes(src, dst);
        src = dst;
    }

    dst = startNode;
    ret += getWeightFromNodes(src, dst);

#ifdef GRAPH_PRINT_STEP
    printf("%c - %d\n", dst->id, ret);
#endif

#ifndef USE_MPI_MALLOC
    free(others);
#else
    MPI_Free_mem(others);
#endif

    return ret;
}

void createGraph(int size) {

    if (graph != NULL) {
        destroyGraph();
    }

#ifndef USE_MPI_MALLOC
    graph = (pGraph) malloc(sizeof (Graph));
#else
    MPI_Alloc_mem(sizeof (Graph), MPI_INFO_NULL, &graph);
#endif

    if (graph != NULL) {
        int i;

#ifndef USE_MPI_MALLOC
        graph->nodes = (pNode*) malloc(sizeof (pNode) * size);
#else
        MPI_Alloc_mem(sizeof (pNode) * size, MPI_INFO_NULL, &graph->nodes);
#endif

        graph->size = size;

        if (graph->nodes != NULL) {
            int err = FALSE;

            for (i = 0; i < size && !err; i++) {
#ifndef USE_MPI_MALLOC
                graph->nodes[i] = (pNode) malloc(sizeof (Node));
#else
                MPI_Alloc_mem(sizeof (Node), MPI_INFO_NULL, &graph->nodes[i]);
#endif


                if (graph->nodes[i] == NULL) {
                    err = TRUE;
                } else {
                    graph->nodes[i]->id = 'A' + i;
                    graph->nodes[i]->step = FALSE;
                }
            }

            if (!err) {

#ifndef USE_MPI_MALLOC
                graph->edges = (int*) malloc(sizeof (int) * size * size);
#else
                MPI_Alloc_mem(sizeof (int) * size * size, MPI_INFO_NULL, &graph->edges);
#endif

                if (graph->edges == NULL) {

#ifndef USE_MPI_MALLOC
                    free(graph->nodes);
                    free(graph);

#else
                    MPI_Free_mem(graph->nodes);
                    MPI_Free_mem(graph);
#endif

                    graph = NULL;
                    printf("Error while allocating memory to create graph\n");
                    exit(-1);
                } else {

#ifndef USE_MPI_MALLOC
                    factorialHashTable = (int*) malloc(sizeof (int) * size);
#else
                    MPI_Alloc_mem(sizeof (int) * size, MPI_INFO_NULL, &factorialHashTable);
#endif

                    if (factorialHashTable == NULL) {
                        printf("Error while allocating memory to create factorial hash table\n");
                        exit(-1);
                    }

                    for (i = 0; i < size; i++) {
                        factorialHashTable[i] = factorial(i);
                    }
                }

            } else {

                int j;

                for (j = 0; j < i; j++) {
#ifndef USE_MPI_MALLOC
                    free(graph->nodes[j]);
#else
                    MPI_Free_mem(graph->nodes[j]);
#endif
                }

#ifndef USE_MPI_MALLOC
                free(graph->nodes);
                free(graph);
#else
                MPI_Free_mem(graph->nodes);
                MPI_Free_mem(graph);
#endif

                graph = NULL;
                printf("Error while allocating memory to create graph\n");
                exit(-1);

            }

        } else {
#ifndef USE_MPI_MALLOC
            free(graph);
#else
            MPI_Free_mem(graph);
#endif
            graph = NULL;
            printf("Error while allocating memory to create graph\n");
            exit(-1);
        }

    }

}

void destroyGraph(void) {

    if (graph != NULL) {

        int i = 0;

#ifndef USE_MPI_MALLOC
        free(graph->edges);
#else
        MPI_Free_mem(graph->edges);
#endif

        for (; i < graph->size; i++) {
#ifndef USE_MPI_MALLOC
            free(graph->nodes[i]);
#else
            MPI_Free_mem(graph->nodes[i]);
#endif
        }

#ifndef USE_MPI_MALLOC
        free(graph->nodes);
        free(graph);
        free(factorialHashTable);
#else
        MPI_Free_mem(graph->nodes);
        MPI_Free_mem(graph);
        MPI_Free_mem(factorialHashTable);
#endif

    }

    factorialHashTable = NULL;
    graph = NULL;
}

void initDijkstra(int * pathSizes, int * previous) {
    int i;
    for (i = 0; i < graph->size; i++) {
        graph->nodes[i]->step = FALSE;
        pathSizes[i] = INT_MAX;
        previous[i] = UNDEFINED;
    }
}

int allStepped(void) {
    int i = 0;
    for (; i < graph->size; i++) {
        if (!graph->nodes[i]->step) {
            return FALSE;
        }
    }
    return TRUE;
}

int lowerPath(int * paths) {
    int i;
    int lower = INT_MAX;
    int lowerKey = -1;
    for (i = 0; i < graph->size; i++) {
        if (paths[i] <= lower && !graph->nodes[i]->step) {
            lowerKey = i;
            lower = paths[i];
        }
    }
    return lowerKey;
}

pPathNode addPathNode(int u, pPathNode lastPathNode) {
    pPathNode pathNode;

#ifndef USE_MPI_MALLOC
    pathNode = (pPathNode) malloc(sizeof (PathNode));
#else
    MPI_Alloc_mem(sizeof (PathNode), MPI_INFO_NULL, &pathNode);
#endif

    if (pathNode == NULL) {
        printf("Error while allocating memory for dijkstra\n");
        exit(-1);
    }

    pathNode->node = graph->nodes[u];
    pathNode->next = lastPathNode;

    return pathNode;
}

pPath dijkstra(int src, int dst) {

    pPath ret = NULL;
    int v;
    int u;
    int found = FALSE;
    pPathNode lastPathNode = NULL;

    if (graph != NULL) {
        int * dist;
        int * prev;

#ifndef USE_MPI_MALLOC
        dist = (int*) malloc(sizeof (int) * graph->size);
        prev = (int*) malloc(sizeof (int) * graph->size);
#else
        MPI_Alloc_mem(sizeof (int) * graph->size, MPI_INFO_NULL, &dist);
        MPI_Alloc_mem(sizeof (int) * graph->size, MPI_INFO_NULL, &prev);
#endif

        if (dist == NULL || prev == NULL) {
            printf("Error while allocating memory to run dijkstra...\n");
            exit(-1);
        }

        initDijkstra(dist, prev);
        dist[src] = 0;

        do {

            u = lowerPath(dist);

            graph->nodes[u]->step = TRUE;

            if (u == dst) /* found */ {
                found = TRUE;
            } else {
                for (v = 0; v < graph->size; v++) {
                    int edgeIdx = u * graph->size + v;
                    if (graph->edges[edgeIdx] != 0) {
                        int alt = dist[u] + graph->edges[edgeIdx];
                        if (alt < dist[v]) {
                            dist[v] = alt;
                            prev[v] = u;
                        }
                    }
                }
            }

        } while (!found && !allStepped());

#ifndef USE_MPI_MALLOC
        ret = (pPath) malloc(sizeof (Path));
#else
        MPI_Alloc_mem(sizeof (Path), MPI_INFO_NULL, &ret);
#endif

        if (ret == NULL) {
            printf("Error while allocating memory for dijkstra\n");
            exit(-1);
        }

        ret->totalWeight = dist[u];

        while (prev[u] != UNDEFINED) {
            lastPathNode = addPathNode(u, lastPathNode);
            u = prev[u];
        }

        ret->first = addPathNode(u, lastPathNode);

#ifndef USE_MPI_MALLOC
        free(prev);
        free(dist);
#else
        MPI_Free_mem(prev);
        MPI_Free_mem(dist);
#endif

    }

    return ret;
}

void createArtificialEdges(void) {
    if (graph != NULL && artificialEdges == NULL) {
        int i, j, size = graph->size;
        int * buffer;
        
#ifndef USE_MPI_MALLOC
        artificialEdges = (pPath *) malloc(sizeof (pPath) * size * size);
        buffer = (int *) malloc(sizeof (int) * size * size);
        
#else
        MPI_Alloc_mem(sizeof (pPath) * size * size, MPI_INFO_NULL, &artificialEdges);
        MPI_Alloc_mem(sizeof (int) * size * size, MPI_INFO_NULL, &buffer);
#endif
        
        if (artificialEdges == NULL || buffer == NULL) {
            printf("Error while creating artificial edges\n");
            exit(-1);
        }

        for(i = 0; i < size * size; i++) {
            buffer[i] = 0;
        }
        
        for (i = 0; i < size; i++) {
            for (j = 0; j < size; j++) {
                int edge = i * size + j;
                if (graph->edges[edge] == 0 && artificialEdges[edge] == NULL && i != j) {
                    int reverseEdge = j * size + i;
                    pPath p = dijkstra(i, j);
                    buffer[edge] = buffer[reverseEdge] = p->totalWeight;
                    artificialEdges[edge] = artificialEdges[reverseEdge] = p;
                } else {
                    artificialEdges[edge] = NULL;
                }
            }
        }
        
        for(i = 0; i < size * size; i++) {
            if(buffer[i] != 0) {
                graph->edges[i] = buffer[i];
            }
        }
        
        free(buffer);
    }
}

void destroyArtificialEdges(void) {
    if (artificialEdges != NULL) {
        int size = graph->size;
        int i;
        int j;
        for (i = 0; i < size; i++) {
            for (j = 0; j < size; j++) {
                int e = i * size + j;
                if (artificialEdges[e] != NULL) {
                    destroyPath(artificialEdges[e]);
                    artificialEdges[j * size + i] = NULL;
                }
            }
        }

#ifndef USE_MPI_MALLOC
        free(artificialEdges);
#else
        MPI_Free_mem(artificialEdges);
#endif

    }
}

/*


// return true or false
// return -1 in case of failure, positive integer as the weight of the path
int getCircularPathWeight(int index);

 */

static void addEdge(int srcChar, int dstChar, int weight) {
    int src = srcChar - 'A';
    int dst = dstChar - 'A';
    graph->edges[src * graph->size + dst] =
            graph->edges[dst * graph->size + src] =
            weight;
}

static void printEdges() {
    int size = graph->size;
    int i;
    int j;

    printf(" ");
    for (i = 0; i < size; i++) {
        printf(" %c", 'A' + i);
    }

    printf("\n");

    for (i = 0; i < size; i++) {
        printf("%c", 'A' + i);
        for (j = 0; j < size; j++) {
            printf(" %d", graph->edges[i * graph->size + j]);
        }
        printf("\n");
    }
}

void printPath(pPath path) {
    pPathNode it = path->first;
    printf("Total weight: %d\n", path->totalWeight);
    while (it != NULL) {
        printf("%c ", it->node->id);
        it = it->next;
    }
    printf("\n");
}

void destroyPath(pPath path) {
    pPathNode it = path->first;
    while (it != NULL) {
        pPathNode old = it;
        it = it->next;
#ifndef USE_MPI_MALLOC
        free(old);
#else
        MPI_Free_mem(old);
#endif
    }
#ifndef USE_MPI_MALLOC
    free(path);
#else
    MPI_Free_mem(path);
#endif
}

void printRealPath(pPath p) {
    if (graph != NULL && artificialEdges != NULL) {
        pPathNode pathNode = p->first;
        while (pathNode != NULL) {
            if (pathNode->next == NULL) {
                printf("%c\n", pathNode->node->id);
            } else {
                int b = pathNode->node->id - 'A';
                int e = pathNode->next->node->id - 'A';
                pPath realPath = artificialEdges[b * graph->size + e];
                if (realPath != NULL) {
                    pPathNode it = realPath->first;
                    while (it != NULL && it->next != NULL) {
                        printf("%c ", it->node->id);
                        it = it->next;
                    }
                } else {
                    printf("%c ", pathNode->node->id);
                }
            }
            pathNode = pathNode->next;
        }
    }
}

void test(int argc, char* argv[]) {

    // src: http://www.emsampa.com.br/xspxrjint.htm
    int graphSrc[6] = {
        159, 269, 122, 118, 182, 170//, 127, 132, 35, 166, 338
    };

    int i;
    int tam = 1 + 6;
    int nCombinations;
    int taskSize;
    int taskIni;
    int taskEnd;
    int lower;
    int key;
    pPath p;

#ifdef USE_MPI_MALLOC

    int rank, size;
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

#endif
    
    nCombinations = factorial(tam - 1) / 2;

#ifdef USE_MPI_MALLOC
    
    if (rank == 0) {

        createGraph(tam);
        
#endif
        
#ifdef GRAPH_PRINT_STEP
        printf("nCombinations: %d\n", nCombinations);
#endif

        createGraph(6);
        addEdge('A', 'B', 700);
        addEdge('A', 'C', 119);
        addEdge('A', 'F', 14);
        addEdge('B', 'C', 109);
        addEdge('B', 'D', 15);
        addEdge('C', 'D', 11);
        addEdge('C', 'F', 2);
        addEdge('D', 'E', 6);
        addEdge('F', 'E', 9);

        
#ifdef USE_MPI_MALLOC
        
        for(i = 1; i < size; i++) {
            MPI_Send(&graph, 1, MPI_AINT, i, 0, MPI_COMM_WORLD);
        }

        for (i = 1; i < tam; i++) {
            int ini = 'A';
            addEdge('A', ini + i, graphSrc[i - 1]);
        }
        
#endif
        
        sequentialSolution();

#ifdef USE_MPI_MALLOC
        
        printf("Starting parallel run:\n");

        startTimestamp();
        createArtificialEdges();

        taskSize = taskDivision(size, nCombinations);

    } else {
        int divRecv;
        MPI_Recv(&graph, 1, MPI_AINT, 0, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(&divRecv, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        taskSize = divRecv;
    }

    taskIni = (nCombinations / size) * rank;
    taskEnd = taskIni + taskSize - 1;

#ifdef GRAPH_PRINT_STEP
    printf("%d %d %d %d\n", rank, taskSize, taskIni, taskEnd);
#endif

    getLowerPath(0, taskIni, taskEnd, &lower, &key);

    if (rank == 0) {
        for (i = 1; i < size; i++) {
            int l, k;
            MPI_Recv(&l, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(&k, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
            if (lower > l) {
                lower = l;
                key = k;
            }
        }

        printf("%d %d\n\n", lower, key);

        p = getPathFromIndex(0, key);

        printRealPath(p);

        destroyPath(p);

        destroyArtificialEdges();
        finishTimestamp();

#endif
        
        destroyGraph();

#ifdef USE_MPI_MALLOC
        
    } else {
        MPI_Send(&lower, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        MPI_Send(&key, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }

    MPI_Finalize();
    
#endif
    
}