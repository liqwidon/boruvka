#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "timer.h"
#include <map>
#include <vector>
#include <iostream>
#include <typeinfo>
#include <algorithm>
#include <set>
#include "graph_gen.hpp"
#include "boruvkasequential.hpp"

using namespace std;

namespace parallel {
    //METHODS
    vector<Edge> boruvkaParallel(vector<Edge*>,int);
    void getAsMap(vector<Edge*> *);
    void initializeRoot();
    void initializeMaps();
    void resetMaps();
    vector<int> getKeyset();
    vector<int> getNewKeyset();
    void print(vector<int>);
    void setCheapestEdgesForEachComponent();
    void* setCheapestEdgesForEachComponentThreadBody(void *);
    void print(map<int,Edge>);
    bool edgeCompare(Edge, Edge);
    void setNewGroups();
    void setNewGroupsThreadBody(void *);
    void updateNewGroups(int, int);
    void updateInEdges(int, int, Edge);
    int getRoot(int);
    void unionFind(int, int);
    void setNewOutEdges();
    void* setNewOutEdgesThreadBody(void *);
    void deleteAll(vector<Edge>*, Edge);
    void deleteAllCycles(int);
    bool contains(map<int, vector<int> >, int);
    bool contains(vector<int>, int);
    void print(vector<Edge>);
    void print(map<int,vector<int> >);
    void print(map<int,vector<Edge> >);
    void print(Edge *);

    //VARIABLES
    int num_threads;
    int N;
    int * root;
    map<int, vector<Edge> > graph;
    map<int, vector<int> > groups;
    vector<Edge> * outEdges;

    map<int, vector<Edge> > inEdges;
    Edge* cheapest;
    map<int, vector<int> > newGroups;
    vector<Edge> * newOutEdges;

    vector<int> keyset;

    vector<Edge> boruvkaParallel(vector<Edge*> edges, int n){
        N = n;
        getAsMap(&edges);
        initializeRoot();
        initializeMaps();
        int i = 0;
        while(groups.size() > 1){
            resetMaps();
            setCheapestEdgesForEachComponent();
            setNewGroups();
            if (newGroups.size() == 1) break;
            setNewOutEdges();
        }

        return inEdges[0];
    }

    void getAsMap(vector<Edge*> *edges){

        for(int i = 0; i<N; i++){
            graph[i] = *(new vector<Edge>());
        }
        vector<Edge*>::iterator it;
        for(it = edges->begin(); it != edges->end(); it++){
            int x = (*it)->x;
            int y = (*it)->y;
            graph[x].insert(graph[x].end(), (**it));
            graph[y].insert(graph[y].end(), (**it));
        }
    }

    void initializeRoot(){
        root = new int[N];
        for(int i = 0; i<N; i++){
            root[i] = i;
        }
    }


    void initializeMaps(){
        groups = *(new map<int, vector<int> >());
        inEdges = *(new map<int, vector<Edge> >());
        outEdges = new vector<Edge>[N];
        newGroups = *(new map<int, vector<int> >());
        newOutEdges = new vector<Edge>[N];
        cheapest = new Edge[N];

        for(int i = 0; i<N; i++){
            groups[i] = *(new vector<int>());
            groups[i].insert(groups[i].end(),i);
            outEdges[i] = *(new vector<Edge>(graph[i]));
            inEdges[i] = *(new vector<Edge>());
        }
    }

    void resetMaps(){
        newOutEdges = new vector<Edge>[N];
        newGroups = groups;
        keyset = getKeyset();
    }

    vector<int> getKeyset(){
        vector<int> keys;
        map<int,vector<int> >::iterator it;
        for(it = groups.begin(); it != groups.end(); it++){
            keys.insert(keys.end(),it->first);
        }
        return keys;
    }

    vector<int> getNewKeyset(){
        vector<int> keys;
        map<int,vector<int> >::iterator it;
        for(it = newGroups.begin(); it != newGroups.end(); it++){
            keys.insert(keys.end(),it->first);
        }
        return keys;

    }

    void setCheapestEdgesForEachComponent(){
        pthread_t thread[num_threads];
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

        for (int i=0; i<num_threads; ++i)
        {
            pthread_create(&thread[i], &attr,
                   setCheapestEdgesForEachComponentThreadBody, (void *) i);
        }

        for (int i=0; i<num_threads; ++i)
        {
            pthread_join(thread[i], NULL);
        }
    }

    void* setCheapestEdgesForEachComponentThreadBody(void * args){
        int tid = reinterpret_cast<intptr_t>(args);

        int num_components = keyset.size();
        int start = tid * ((num_components+(num_threads-1))/num_threads);
        int end = min(start + ((num_components+(num_threads-1))/num_threads),num_components);

        for(int i = start; i<end; i++){
            int id = keyset.at(i);
            vector<Edge> *compEdges = &(outEdges[id]);
            sort (compEdges->begin(), compEdges->end(), edgeCompare);
            cheapest[id] = compEdges->at(0);
        }
    }

    bool edgeCompare (Edge e1, Edge e2) {
        return (e1.w < e2.w);
    }

    void setNewGroups(){
        for(int i = 0; i<keyset.size(); i++){
            int id = keyset.at(i);
            Edge picked = cheapest[id];

            int root1 = getRoot(picked.x);
            int root2 = getRoot(picked.y);

            if (root1 == root2) {
                continue;
            }
            if (root1 < root2){
                updateNewGroups(root1,root2);
                updateInEdges(root1,root2,picked);
            }
            else{
                updateNewGroups(root2,root1);
                updateInEdges(root2,root1,picked);
            }
            unionFind(root1,root2);
        }
    }

    void updateNewGroups(int root1, int root2){
        newGroups[root1].insert(newGroups[root1].end(),newGroups[root2].begin(),newGroups[root2].end());
        newGroups.erase(root2);
    }

    void updateInEdges(int root1, int root2, Edge picked){
        inEdges[root1].insert(inEdges[root1].end(),inEdges[root2].begin(),inEdges[root2].end());
        inEdges[root1].insert(inEdges[root1].end(),picked);
        inEdges.erase(root2);
    }

    int getRoot(int comp){
        while(root[comp] != comp){
            comp = root[comp];
        }
        return comp;
    }

    void unionFind(int comp1, int comp2) {
        if (comp1 < comp2) {
            root[comp2] = comp1;
        } else {
            root[comp1] = comp2;
        }
        return;
    }

    void setNewOutEdges(){

        pthread_t thread[num_threads];
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        keyset = getNewKeyset();
        for (int i=0; i<num_threads; i++)
        {
            pthread_create(&thread[i], &attr,
                   setNewOutEdgesThreadBody, (void *) i);
        }

        for (int i=0; i<num_threads; i++)
        {
            pthread_join(thread[i], NULL);
        }
        outEdges = newOutEdges;
        groups = newGroups;

    }

    void* setNewOutEdgesThreadBody(void * args){

        int tid = reinterpret_cast<intptr_t>(args);


        int num_components = keyset.size();
        int start = tid * ((num_components+(num_threads-1))/num_threads);
        int end = min(start + ((num_components+(num_threads-1))/num_threads),num_components);

        for(int i = start; i<end; i++){
            int newComponentID = keyset.at(i);

            vector<int> subcomponents = newGroups[newComponentID];
            newOutEdges[newComponentID] = *(new vector<Edge>());

            vector<int>::iterator sub_it;
            for(sub_it = subcomponents.begin(); sub_it != subcomponents.end(); sub_it++){
                if(!contains(groups,(*sub_it))){
                    continue;
                }

                newOutEdges[newComponentID].insert(newOutEdges[newComponentID].end(),outEdges[*sub_it].begin(), outEdges[*sub_it].end());
                outEdges[*sub_it].clear();
            }

            vector<Edge>::iterator e_it;
            for(e_it = inEdges[newComponentID].begin(); e_it != inEdges[newComponentID].end(); e_it++){
                deleteAll(&newOutEdges[newComponentID],*e_it);
            }
            deleteAllCycles(newComponentID);\
        }

    }

    void deleteAll(vector<Edge> *v, Edge e){
        vector<Edge>::iterator it = v->begin();
        while(it != v->end()){
            if(edgeEquals(&(*it),&e)){
                *it = v->back();
                v->pop_back();
                continue;
            }
            it++;
        }
    }

    void deleteAllCycles(int id){
        vector<Edge> *v = &newOutEdges[id];
        vector<int> subs = groups[id];

        vector<Edge>::iterator it = v->begin();
        while(it != v->end()){
            int x = (*it).x;
            int y = (*it).y;
            if(contains(subs,x) && contains(subs,y)){
                *it = v->back();
                v->pop_back();
                continue;
            }
            it++;
        }
    }

    bool contains(map<int, vector<int> > m, int val){
        return m.find(val) != m.end();
    }

    bool contains(vector<int> v, int val){
        return find(v.begin(),v.end(),val) != v.end();
    }
    void print(vector<Edge> v){
        vector<Edge>::iterator it;
        for(it = v.begin(); it != v.end(); it++){
            cout<<it->x<<" "<<it->y<<" "<<it->w<<endl;
        }
        cout<<endl;

    }
    void print(map<int,vector<int> > g){
        map<int, vector<int> >::iterator it;
        for(it = g.begin(); it != g.end(); it++){
            int id = it->first;
            vector<int> vec = it->second;

            cout<<"compid "<<id<<"....";
            print(vec);
        }
        cout<<endl;

    }
    void print(map<int,vector<Edge> > g){
        map<int, vector<Edge> >::iterator it;
        for(it = g.begin(); it != g.end(); it++){
            int id = it->first;
            vector<Edge> vec = it->second;

            cout<<"compid "<<id<<"....";
            vector<Edge>::iterator vit;
            for(vit = vec.begin(); vit != vec.end(); vit++){
                cout<<(*vit).x<<" "<<(*vit).y<<" "<<(*vit).w <<", ";
            }
            cout<<endl;
        }
        cout<<endl;
    }
    void print(Edge * g){
        for(int i = 0; i<keyset.size(); i++){
            Edge e = cheapest[keyset.at(i)];
            cout<<"compid.."<<keyset.at(i)<<" edge "<<e.x<<" "<<e.y<<" "<<e.w<<endl;
        }
        cout<<endl;
    }
    void print(map<int,Edge> g){
        for(int i = 0; i<keyset.size(); i++){
            Edge e = cheapest[keyset.at(i)];
            cout<<"compid.."<<keyset.at(i)<<" edge "<<e.x<<" "<<e.y<<" "<<e.w<<endl;
        }
        cout<<endl;
    }
    void print(vector<int> v){
        vector<int>::iterator it;
        for(it = v.begin(); it != v.end(); it++){
            cout<<*it<<" ";
        }
        cout<<endl;

    }
}
int N,E;
int nodes[7] = {100,200,400,500,700,1000,2000};
bool areEqual(vector<Edge*>, vector<Edge> );
int main(){

    int t;
    cout<<"Enter number of threads: "<<endl;
    scanf("%d",&t);
    cout<<"MATCH(?) #Nodes\t#Edges\tSequential time\tParallel time\t#threads  Speedup"<<endl;
    for(int i = 0; i<7; i++){
        for(int j = 0; j<3; j++){
            N = nodes[i];
            parallel::num_threads = t;
            vector<Edge*> g;

            vector<Edge*> gb = *(new vector<Edge*>());
            vector<Edge> gbparallel = *(new vector<Edge>());

            g = generateGraph(N);

            long long timeboruvka = timer();
            gb = boruvka::boruvka(g,N);
            timeboruvka = timer() - timeboruvka;

            long long timeparallel = timer();
            gbparallel = parallel::boruvkaParallel(g,N);
            timeparallel = timer() - timeparallel;
            bool ok = areEqual(gb,gbparallel);

            cout<<(ok?"MATCH\t":"ERROR\t")<<N<<"\t"<<g.size()<<"\t"<<(timeboruvka/1000000.0)<<"\t"<<(timeparallel/1000000.0)<<"\t"<<t<<"\t  "<<timeboruvka*1.0/timeparallel<<endl;
        }
    }
    return 0;
}
bool areEqual(vector<Edge*> v1, vector<Edge> v2){
    sort(v1.begin(),v1.end(),boruvka::edgeCompare);
    sort(v2.begin(),v2.end(),parallel::edgeCompare);

    vector<Edge*>::iterator it1 = v1.begin();
    vector<Edge>::iterator it2 = v2.begin();

    bool ok = true;
    int sum1 = 0;
    int sum2 = 0;
    while(it1 != v1.end() && it2 != v2.end()){
        if(((*it2).x != (**it1).x)){
            ok = false;
            break;
        }
        if(((*it2).y != (**it1).y)){
            ok = false;
            break;
        }
        if(((*it2).w != (**it1).w)){
            ok = false;
            break;
        }
        sum1 += (**it1).w;
        sum2 += (*it2).w;
        it1++;
        it2++;
    }
    return ok;
}
