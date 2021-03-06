#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <string>
#include <queue>
#include <iostream>
#include "graph_gen.hpp"
#include "timer.h"
#include <time.h>

#define threshold 2000000
using namespace std;

vector<Edge*> generateGraph(int n){
    set<int> weights;
    vector<Edge*> graph;
    map<int, vector<Edge*> > g;
    for(int i = 0; i<n; i++){
        g[i] = *(new vector<Edge*>());
    }
    if (n==1) return graph;

    srand (time(NULL));
    long long rtime;
    while(true){
        if((timer() - rtime) > threshold){
            srand(time(NULL));
            rtime = timer();
        }
        int x = rand() % n;
        int y = rand() % n;
        int w = rand() % ((long)n*(long)n);

        Edge *edge = new Edge();
        edge->x = x;
        edge->y = y;
        edge->w = -1;
        if(x == y || contains(g,*edge) || contains(&weights,w)){
            continue;
        }
        edge->w = w;
        graph.insert(graph.end(),edge);
        g[x].insert(g[x].end(),edge);
        g[y].insert(g[y].end(),edge);
        weights.insert(w);
        if (isConnected(g,n) && (graph.size() >= (n * 6)))
            break;

        rtime = timer();

    }

    return graph;
}

bool contains(map<int, vector<Edge*> > m, Edge val){
    vector<Edge*>::iterator it;
    for(it = m[val.x].begin(); it != m[val.x].end(); it++){
        if(edgeEquals(*it,&val))
            return true;
    }
    return false;
}

bool contains(set<int>* myset, int val){
    return myset->find(val) != myset->end();
}

bool edgeEquals (Edge* e1, Edge* e2){
    return (e1->x == e2->x && e1->y == e2->y) || (e1->x == e2->y && e1->y == e2->x);
}

bool isConnected(map<int, vector<Edge*> > g, int n){
    bool *visited = new bool[n];
    for(int i = 0; i<n; i++){
        visited[i] = false;
    }

    queue<int> q;
    q.push(0);
    visited[0] = true;
    int count = 1;
    while(!q.empty()){
        int node = q.front();
        q.pop();
        vector<Edge*>::iterator it;
        for(it = g[node].begin(); it != g[node].end(); it++){
            int index = (*it)->getOtherEndpoint(node);
            if(index == -1)
                printf("error!!");
            if(visited[index]) continue;
            q.push(index);
            visited[index] = true;
            count++;
        }
    }
    return count == n;
}
void print(vector<Edge*> g){
    cout<<"graph begin"<<endl;
    vector<Edge*>::iterator it;
    for(it = g.begin(); it != g.end(); it++){
        cout<< (**it).x <<" "<<(**it).y<<" "<<(**it).w<<endl;
    }
    cout<<"graph end"<<endl;
}
void print(set<int> s){
    cout<<"weights begin"<<endl;
    set<int>::iterator it;
    for(it = s.begin(); it != s.end();it++){
        cout<<(*it)<<" ";
    }
    cout<<endl;
    cout<<"weights end"<<endl;
}
void print(vector<int> s){
    vector<int>::iterator it;
    for(it = s.begin(); it != s.end();it++){
        cout<<(*it)<<" ";
    }
    cout<<endl;
}


