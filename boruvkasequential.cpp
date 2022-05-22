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
using namespace std;

namespace boruvka{

    vector<Edge*> boruvka(vector<Edge*>,int);
    void getAsMap(vector<Edge*> *);
    void initializeRoot();
    void initializeMaps();
    bool edgeCompare(Edge*, Edge*);
    void resetMaps();
    void setCheapestEdgesForEachComponent();
    void updateNewGroups(int, int);
    void updateInEdges(int, int, Edge*);
    int getRoot(int);
    void unionFind(int, int);
    void setNewGroupsAndNewInEdges();
    void setNewOutEdges();
    void deleteAll(vector<Edge*>*, Edge* );
    void deleteAllCycles(int);
    bool contains(map<int, vector<int> > *, int);
    bool contains(vector<int> *, int );
    void print(map<int,vector<int> >);
    void print(map<int,vector<Edge*> >);
    void print(vector<Edge*> );


    int N;
    int * root;

    map<int, vector<Edge*> > graph;
    map<int, vector<int> > groups;
    map<int, vector<Edge*> > inEdges;
    map<int, vector<Edge*> > outEdges;
    map<int, Edge*> cheapest;

    map<int, vector<int> > newGroups;
    map<int, vector<Edge*> > newOutEdges;


    vector<Edge*> boruvka(vector<Edge*> edges,int n){
        N = n;
        getAsMap(&edges);
        initializeRoot();
        initializeMaps();
        while(groups.size() > 1){
            resetMaps();
            setCheapestEdgesForEachComponent();
            setNewGroupsAndNewInEdges();
            setNewOutEdges();
            if(newGroups.size() == 1) break;
            outEdges = newOutEdges;
            groups = newGroups;
        }

        return inEdges[0];
    }


    void getAsMap(vector<Edge*> *edges){

        for(int i = 0; i<N; i++){
            graph[i] = *(new vector<Edge*>());
        }
        vector<Edge*>::iterator it;
        for(it = edges->begin(); it != edges->end(); it++){
            int x = (*it)->x;
            int y = (*it)->y;
            graph[x].insert(graph[x].end(), (*it));
            graph[y].insert(graph[y].end(), (*it));
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
        inEdges = *(new map<int, vector<Edge*> >());
        outEdges = *(new map<int, vector<Edge*> >());
        newGroups = *(new map<int, vector<int> >());
        newOutEdges = *(new map<int, vector<Edge*> >());

        for(int i = 0; i<N; i++){
            groups[i] = *(new vector<int>());
            groups[i].insert(groups[i].end(),i);
            outEdges[i] = *(new vector<Edge*>(graph[i]));
            inEdges[i] = *(new vector<Edge*>());
        }
    }

    bool edgeCompare (Edge* e1, Edge* e2) {
        return (e1->w < e2->w);
    }

    void resetMaps(){
        newGroups = groups;
        newOutEdges.clear();
    }


    void setCheapestEdgesForEachComponent(){
        map<int, vector<Edge*> >::iterator comp_it;
        for(comp_it = outEdges.begin(); comp_it != outEdges.end(); comp_it++){
            int componentID = comp_it->first;
            vector<Edge*> compEdges = outEdges[componentID];
            sort (compEdges.begin(), compEdges.end(), edgeCompare);
            cheapest[componentID] = compEdges.at(0);
        }
    }

    void updateNewGroups(int root1, int root2){
        newGroups[root1].insert(newGroups[root1].end(),newGroups[root2].begin(),newGroups[root2].end());
        newGroups.erase(root2);
    }

    void updateInEdges(int root1, int root2, Edge* picked){
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

    void setNewGroupsAndNewInEdges(){
        map<int, Edge*>::iterator ch_it;
        for(ch_it = cheapest.begin(); ch_it != cheapest.end(); ch_it++){

            Edge * picked = ch_it->second;

            int root1 = getRoot(picked->x);
            int root2 = getRoot(picked->y);

            if (root1 == root2)
                continue;
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

    void setNewOutEdges(){

        map<int, vector<int> >::iterator ng_it;
        for(ng_it = newGroups.begin(); ng_it != newGroups.end(); ng_it++){
            int newComponentID = ng_it->first;
            vector<int> subcomponents = ng_it->second;

            newOutEdges[newComponentID] = *(new vector<Edge*>());
            vector<int>::iterator sub_it;
            for(sub_it = subcomponents.begin(); sub_it != subcomponents.end(); sub_it++){

                if(!contains(&groups,(*sub_it))){
                    continue;
                }

                newOutEdges[newComponentID].insert(newOutEdges[newComponentID].end(),outEdges[*sub_it].begin(), outEdges[*sub_it].end());
                outEdges.erase(*sub_it);
            }
            vector<Edge*>::iterator e_it;
            for(e_it = inEdges[newComponentID].begin(); e_it != inEdges[newComponentID].end(); e_it++){
                deleteAll(&newOutEdges[newComponentID],*e_it);
            }
            deleteAllCycles(newComponentID);
        }
    }

    void deleteAll(vector<Edge*> *v, Edge *e){
        vector<Edge*>::iterator it = v->begin();
        while(it != v->end()){
            if(edgeEquals(*it,e)){
                *it = v->back();
                v->pop_back();
                continue;
            }
            it++;
        }
    }

    void deleteAllCycles(int id){
        vector<Edge*> *v = &newOutEdges[id];
        vector<int> subs = groups[id];

        vector<Edge*>::iterator it = v->begin();
        while(it != v->end()){
            int x = (**it).x;
            int y = (**it).y;
            if(contains(&subs,x) && contains(&subs,y)){
                *it = v->back();
                v->pop_back();
                continue;
            }
            it++;
        }
    }

    bool contains(map<int, vector<int> > *m, int val){
        return m->find(val) != m->end();
    }

    bool contains(vector<int> * v, int val){
        return find(v->begin(),v->end(),val) != v->end();
    }
    void print(map<int,vector<int> > g){
        map<int, vector<int> >::iterator it;
        for(it = g.begin(); it != g.end(); it++){
            int id = it->first;
            vector<int> vec = it->second;

            cout<<"compid "<<id<<"....";
            vector<int>::iterator vit;
            for(vit = vec.begin(); vit != vec.end(); vit++){
                cout<<*vit<<", ";
            }
            cout<<endl;
        }
        cout<<endl;
    }
    void print(map<int,vector<Edge*> > g){
        map<int, vector<Edge*> >::iterator it;
        for(it = g.begin(); it != g.end(); it++){
            int id = it->first;
            vector<Edge*> vec = it->second;

            cout<<"compid "<<id<<"....";
            vector<Edge*>::iterator vit;
            for(vit = vec.begin(); vit != vec.end(); vit++){
                cout<<(**vit).x<<" "<<(**vit).y<<" "<<(**vit).w <<", ";
            }
            cout<<endl;
        }
        cout<<endl;
    }
    void print(vector<Edge*> g){
        vector<Edge*>::iterator it;
        for(it = g.begin(); it != g.end(); it++){
            Edge e = **it;
            cout<<e.x<<" "<<e.y<<" "<<e.w <<endl;;
        }
        cout<<endl;
    }
}
