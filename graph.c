#include "graph.h"

#define TRUE 1
#define FALSE 0

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
    int currWeight;
    int currentIdx;
    struct StructLinkedNode * next;
    pNode current;
} LinkedNode, *pLinkedNode;

static pGraph graph = NULL;

pGraph createGraph(int size, int createNodes) {
    
    pGraph ret = (pGraph) malloc(sizeof(Graph));

    if(ret != NULL) {
        int i;
        ret->nodes = (pNode*) malloc(sizeof(pNode) * size);

        if(ret->nodes != NULL) {
            int err = FALSE;

            if(createNodes) {
                for(i = 0; i < size && !err; i++) {
                    ret->nodes[i] = (pNode) malloc(sizeof(Node));
                    if(ret->nodes[i] == NULL) {
                        err = TRUE;
                    } else {
                        ret->nodes[i]->id = 'A' + i;
                        ret->nodes[i]->step = FALSE;
                    }
                }
            }

            if(!err) {

                ret->edges = (int*) malloc(sizeof(int) * size);

                if(ret->edges == NULL) {
                    free(ret->nodes);
                    free(ret);
                    ret = NULL;
                }

            } else {

                int j;

                for(j = 0; j < i; j++) {
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

    if(createNodes) {
        graph = ret;
    }
    
    return ret;
}

void destroyGraph(pGraph graphArgument, int destroyNodes) {
    pGraph target;
    
    if(graphArgument == NULL) {
        target = graph;
    } else {
        target = graphArgument;
    }
    
    free(target->edges);
    if(destroyNodes) {
        int i = 0;
        for(;i < target->size; i++) {
            free(target->nodes[i]);
        }
    }
    
    free(target->nodes);
    free(target);
    
    if(graphArgument == NULL) {
        graph = NULL;
    }
}

typedef struct {
    pGraph graph;
    int totalWeight;
} Path, *pPath;

static pPath recDijkstra(int srcIdx, int dstIdx, int * pathSizes, int previousPath, int idx) {

    pPath path = NULL;
    int j;

    if(srcIdx == dstIdx) {
        path = (pPath) malloc(sizeof(Path));
        path->graph = createGraph(idx, FALSE);
        path->graph->nodes[idx - 1] = graph->nodes[srcIdx];
        path->totalWeight = previousPath;
    } else {
        int lowerEdge = INT_MAX;
        int initialLower = 0;
        pLinkedNode nextPath = NULL;

        for(j = 0; j < graph->size; j++) {
            pNode node = graph->nodes[j];
            int pathIdx = srcIdx * graph->size + j;
            int edge = graph->edges[pathIdx] + previousPath;
            if(node->step == FALSE && edge != 0 && edge < pathSizes[pathIdx]) {
                pLinkedNode linkedNode = (pLinkedNode) malloc(sizeof(LinkedNode));
                pathSizes[pathIdx] = edge;
                linkedNode->currWeight = edge;
                linkedNode->next = NULL;
                linkedNode->current = node;
                linkedNode->currentIdx = j;
                if(nextPath != NULL) {
                    pLinkedNode it = nextPath, lastIt = NULL;
                    while(it != NULL) {
                        if(it->currWeight > linkedNode->currWeight) {
                            if(lastIt != NULL) {
                                lastIt->next = linkedNode;
                            } else {
                                nextPath = linkedNode;
                            }
                            linkedNode->next = it;
                        }
                        lastIt = it;
                        it = it->next;
                    }

                    if(linkedNode->next == NULL) {
                        lastIt->next = linkedNode;
                    }
                } else {
                    nextPath = linkedNode;
                }
            }
        }

        graph->nodes[srcIdx]->step = TRUE;

        while(nextPath != NULL) {
            pLinkedNode temp = nextPath;
            if(nextPath->current->step == FALSE && path == NULL) {
                path = recDijkstra(nextPath->currentIdx, dstIdx, pathSizes, nextPath->currWeight, idx + 1);
                path->graph->nodes[idx - 1] = graph->nodes[srcIdx];
            }
            nextPath = nextPath->next;
            free(temp);
        }
    }

    return path;
}

static void initDijkstra(int * pathSizes) {
    int i;
    for(i = 0; i < graph->size; i++) {
        graph->nodes[i]->step = FALSE;
        pathSizes[i] = INT_MAX;
    }
}

static pPath dijkstra(int src, int dst) {
    pPath ret = NULL;
    if(graph != NULL) {
        int * pathSizes = (int*) malloc(sizeof(int) * graph->size);
        if(pathSizes == NULL) {
            printf("Error while allocating memory to run dijkstra...\n");
            exit(-1);
        }
        initDijkstra(pathSizes);
        pathSizes[src * graph->size] = 0;
        ret = recDijkstra(src, dst, pathSizes, 0, 1);
        free(pathSizes);
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

void test(void) {
    createGraph(1, TRUE);
    destroyGraph(NULL, TRUE);
}