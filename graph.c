#include "graph.h"

#define TRUE 1
#define FALSE 0
#define UNDEFINED -1

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct {
    char id;
    int step; // dijkstra
} Node, *pNode;

typedef struct {
    int * edges;
    pNode * nodes;
    int size;
} Graph, *pGraph;

typedef struct StructLinkedNode {
    struct StructLinkedNode * next;
    pNode node;
} PathNode, *pPathNode;

static pGraph graph = NULL;

pGraph createGraph(int size, int createNodes) {

    pGraph ret = (pGraph) malloc(sizeof (Graph));

    if (ret != NULL) {
        int i;
        ret->nodes = (pNode*) malloc(sizeof (pNode) * size);
        ret->size = size;

        if (ret->nodes != NULL) {
            int err = FALSE;

            if (createNodes) {
                for (i = 0; i < size && !err; i++) {
                    ret->nodes[i] = (pNode) malloc(sizeof (Node));
                    if (ret->nodes[i] == NULL) {
                        err = TRUE;
                    } else {
                        ret->nodes[i]->id = 'A' + i;
                        ret->nodes[i]->step = FALSE;
                    }
                }
            }

            if (!err) {

                ret->edges = (int*) malloc(sizeof (int) * size * size);

                if (ret->edges == NULL) {
                    free(ret->nodes);
                    free(ret);
                    ret = NULL;
                }

            } else {

                int j;

                for (j = 0; j < i; j++) {
                    free(ret->nodes[j]);
                }

                free(ret->nodes);
                free(ret);
                ret = NULL;

            }

        } else {
            free(ret);
            ret = NULL;
        }

    }

    if (createNodes) {
        graph = ret;
    }

    return ret;
}

void destroyGraph(pGraph graphArgument, int destroyNodes) {
    pGraph target;

    if (graphArgument == NULL) {
        target = graph;
    } else {
        target = graphArgument;
    }

    free(target->edges);
    if (destroyNodes) {
        int i = 0;
        for (; i < target->size; i++) {
            free(target->nodes[i]);
        }
    }

    free(target->nodes);
    free(target);

    if (graphArgument == NULL) {
        graph = NULL;
    }
}

typedef struct {
    pPathNode first;
    int totalWeight;
} Path, *pPath;

static void initDijkstra(int * pathSizes, int * previous) {
    int i;
    for (i = 0; i < graph->size; i++) {
        graph->nodes[i]->step = FALSE;
        pathSizes[i] = INT_MAX;
        previous[i] = UNDEFINED;
    }
}

static int allStepped() {
    int i = 0;
    for (; i < graph->size; i++) {
        if (!graph->nodes[i]->step) {
            return FALSE;
        }
    }
    return TRUE;
}

static int lowerPath(int * paths) {
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

static pPathNode addPathNode(int u, pPathNode lastPathNode) {
    pPathNode pathNode = (pPathNode) malloc(sizeof(PathNode));

    if (pathNode == NULL) {
        printf("Error while allocating memory for dijkstra\n");
        exit(-1);
    }

    pathNode->node = graph->nodes[u];
    pathNode->next = lastPathNode;
    
    return pathNode;
}

static pPath dijkstra(int src, int dst) {

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

        } while (!allStepped() && !found);

        ret = (pPath) malloc(sizeof(Path));

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

/*

// return true or false
int createArtificialEdges(void);
// return true or false
int destroyArtificialEdges(void);
// return -1 in case of failure, positive integer as the weight of the path
int getCircularPathWeight(int index);

 */

static void addEdge(char srcChar, char dstChar, int weight) {
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

static void printPath(pPath path) {
    pPathNode it = path->first;
    printf("Total weight: %d\n", path->totalWeight);
    while (it != NULL) {
        printf("%c ", it->node->id);
        it = it->next;
    }
    printf("\n");
}

static void destroyPath(pPath path) {
    pPathNode it = path->first;
    while (it != NULL) {
        pPathNode old = it;
        it = it->next;
        free(old);
    }
    free(path);
}

void test(void) {
    createGraph(6, TRUE);
    addEdge('A', 'B', 7);
    addEdge('A', 'C', 9);
    addEdge('A', 'F', 14);
    addEdge('B', 'C', 10);
    addEdge('B', 'D', 15);
    addEdge('C', 'D', 11);
    addEdge('C', 'F', 2);
    addEdge('D', 'E', 6);
    addEdge('F', 'E', 9);
    //printEdges();
    pPath p = dijkstra(4, 0);
    printPath(p);
    destroyPath(p);
    destroyGraph(NULL, TRUE);
}