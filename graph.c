#include "graph.h"

//#define GRAPH_PRINT_STEP 0

#define TRUE 1
#define FALSE 0
#define UNDEFINED -1

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

void getLowerPath(int startNode, int start, int end, int * lower, int * lowerKey) {
    int i;
    *lower = INT_MAX;
    *lowerKey = -1;

    for (i = start; i <= end; i++) {
        int w = getWeightFromIndex(startNode, i);
        if (w < *lower) {
            *lower = w;
            *lowerKey = i;
        }
    }
}

static void sequentialSolution(void);
static unsigned long finishTimestamp(void);
static void startTimestamp(void);

void sequentialSolution(void) {
    if (graph != NULL) {

        startTimestamp();
        createArtificialEdges();

        {
            int fact = factorialHashTable[graph->size - 1] / 2;
            int lower;
            int key;
            pPath p;

            printf("Start sequential run:\n");
            getLowerPath(0, 0, fact, &lower, &key);

            p = getPathFromIndex(0, key);

            //printPath(p);
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
    pPath ret = (pPath) malloc(sizeof (Path));
    pNode * others = (pNode*) malloc(sizeof (pNode) * (graph->size - 1));
    pPathNode pathNode;
    pPathNode lastPathNode = pathNode;
    int countNotUsed = graph->size - 1;
    int i, j;
    pNode startNode = graph->nodes[start];
    pNode src;
    pNode dst;

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

    pathNode = (pPathNode) malloc(sizeof (PathNode));

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

        pathNode = (pPathNode) malloc(sizeof (PathNode));

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

    pathNode = (pPathNode) malloc(sizeof (PathNode));

    if (pathNode == NULL) {
        printf("Error while allocating memory to create path\n");
        exit(-1);
    }

    pathNode->node = dst;
    pathNode->next = NULL;
    lastPathNode->next = pathNode;

    free(others);

    return ret;
}

int getWeightFromNodes(pNode src, pNode dst) {
    return graph->edges[(src->id - 'A') * graph->size + (dst->id - 'A')];
}

int getWeightFromIndex(int start, int idx) {
    pNode * others = (pNode*) malloc(sizeof (pNode) * (graph->size - 1));
    int ret = 0;
    int countNotUsed = graph->size - 1;
    int i, j;
    pNode startNode = graph->nodes[start];
    pNode src;
    pNode dst;

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

    free(others);

    return ret;
}

void createGraph(int size) {

    if (graph != NULL) {
        destroyGraph();
    }

    graph = (pGraph) malloc(sizeof (Graph));

    if (graph != NULL) {
        int i;
        graph->nodes = (pNode*) malloc(sizeof (pNode) * size);
        graph->size = size;

        if (graph->nodes != NULL) {
            int err = FALSE;

            for (i = 0; i < size && !err; i++) {
                graph->nodes[i] = (pNode) malloc(sizeof (Node));
                if (graph->nodes[i] == NULL) {
                    err = TRUE;
                } else {
                    graph->nodes[i]->id = 'A' + i;
                    graph->nodes[i]->step = FALSE;
                }
            }

            if (!err) {

                graph->edges = (int*) malloc(sizeof (int) * size * size);

                if (graph->edges == NULL) {
                    free(graph->nodes);
                    free(graph);
                    graph = NULL;
                    printf("Error while allocating memory to create graph\n");
                    exit(-1);
                } else {
                    factorialHashTable = (int*) malloc(sizeof (int) * size);
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
                    free(graph->nodes[j]);
                }

                free(graph->nodes);
                free(graph);
                graph = NULL;
                printf("Error while allocating memory to create graph\n");
                exit(-1);

            }

        } else {
            free(graph);
            graph = NULL;
            printf("Error while allocating memory to create graph\n");
            exit(-1);
        }

    }

}

void destroyGraph(void) {

    if (graph != NULL) {

        int i = 0;

        free(graph->edges);
        for (; i < graph->size; i++) {
            free(graph->nodes[i]);
        }

        free(graph->nodes);
        free(graph);

        free(factorialHashTable);
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
    pPathNode pathNode = (pPathNode) malloc(sizeof (PathNode));

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
        int * dist = (int*) malloc(sizeof (int) * graph->size);
        int * prev = (int*) malloc(sizeof (int) * graph->size);

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

        ret = (pPath) malloc(sizeof (Path));

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

        free(prev);
        free(dist);
    }

    return ret;
}

void createArtificialEdges(void) {
    if (graph != NULL && artificialEdges == NULL) {
        int i, j, size = graph->size;

        artificialEdges = (pPath *) malloc(sizeof (pPath) * size * size);

        if (artificialEdges == NULL) {
            printf("Error while creating artificial edges\n");
            exit(-1);
        }

        for (i = 0; i < size; i++) {
            for (j = 0; j < size; j++) {
                int edge = i * size + j;
                if (graph->edges[edge] == 0 && i != j) {
                    int reverseEdge = j * size + i;
                    pPath p = dijkstra(i, j);
                    graph->edges[edge] = graph->edges[reverseEdge] = p->totalWeight;
                    artificialEdges[edge] = artificialEdges[reverseEdge] = p;
                } else {
                    artificialEdges[edge] = NULL;
                }
            }
        }
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
        free(artificialEdges);
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
        free(old);
    }
    free(path);
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

void test(void) {

    /*
        createGraph(6);
        addEdge('A', 'B', 7);
        addEdge('A', 'C', 9);
        addEdge('A', 'F', 14);
        addEdge('B', 'C', 10);
        addEdge('B', 'D', 15);
        addEdge('C', 'D', 11);
        addEdge('C', 'F', 2);
        addEdge('D', 'E', 6);
        addEdge('F', 'E', 9);
     */

    // src: http://www.emsampa.com.br/xspxrjint.htm
    int graphSrc[11] = {
        159, 269, 122, 118, 182, 170, 127, 132, 35, 166, 338
    };

    int i;
    int tam = 1 + 11;

    createGraph(tam);

    for (i = 1; i < tam; i++) {
        int ini = 65; // A
        addEdge('A', ini + i, graphSrc[i - 1]);
    }

    sequentialSolution();

    destroyGraph();
}