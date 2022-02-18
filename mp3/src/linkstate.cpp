#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <map>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <set>
#include "Graph.h"
#include <bits/stdc++.h>
#include <algorithm>
#include <queue>
typedef pair<int, int> pi;

map<int, map<int, int> > distanceTable;  // one, the other and distance
map<int, map<int, int> > prevTable;
ofstream outfile;

// int VertexDistance(Vertex u, Vertex v){ // given u and v, find D(V)
//     int result = INT_MAX;
//     map<int, map<int, int> >::iterator itr;
//     map<int, int>::iterator ptr;
//     for(itr = distanceTable.begin(); itr != distanceTable.end(); itr++){
//         for(ptr = itr->second.begin(); ptr != itr->second.end(); ptr++){
//             if(itr->first == u.id && ptr->first == v.id){ // u is first, v is second
//                 result = ptr->second;
//             }else if(itr->first == v.id && ptr->first == u.id){
//                 result = ptr->second;
//             }
//         }
//     }
//     return result;
// }
//
// int cost(Graph g, Vertex u, Vertex v){// given u and v, find cost(u,v)
//     int result = INT_MAX;
//     for(int i = 0; i < g.Edges.size(); i++){
//         if(g.Edges[i].one == u.id && g.Edges[i].theOther == v.id){
//             result = g.Edges[i].dis;
//         }else if(g.Edges[i].one == v.id && g.Edges[i].theOther == u.id){
//             result = g.Edges[i].dis;
//         }
//     }
//     if(result == INT_MAX){
//         cout << "did not find cost" << endl;
//     }
//     return result;
// }

// vector<int> findClosestVertexOfU(set<int> Nprime, Vertex u){
//     int cst = INT_MAX; // closest vertex id
//     int minDis = INT_MAX;
//     // in order to iterate the nested map
//     // can be simplified by distance(u,v)有点晕就没写
//     map<int, map<int, int> >::iterator itr;
//     map<int, int>::iterator ptr;
//     for(itr = distanceTable.begin(); itr != distanceTable.end(); itr++){
//         for(ptr = itr->second.begin(); ptr != itr->second.end(); ptr++){
//             cout << "a" << endl;
//             if(itr->first == u.id){ // u is first
//                 cout << "b" << endl;
//                 cout << "Nprime size is" <<Nprime.size() << endl;
//                 set<int>::iterator it = Nprime.begin();
//                 while (it != Nprime.end()){
//                     cout << (*it) << " , ";
//                     it++;
//                 }
//                 cout << endl;
//                 cout << "cst is" << ptr->first << endl;
//                 if(Nprime.count(ptr->first) == 0){ // make sure cst is not in Nprime
//                     cout << "c" << endl;
//                     if(ptr->second <= minDis && ptr->first < cst){ // distance min and cst min
//                         minDis = ptr->second;
//                         cst = ptr->first;
//                     }
//
//                 }
//             }else if(ptr->first == u.id){ // u is second
//                 if(Nprime.count(itr->first) == 0){
//                     if(ptr->second <= minDis && itr->first < cst){
//                         minDis = ptr->second;
//                         cst = itr->first;
//                     }
//                 }
//             }
//         }
//     }
//     if(cst == INT_MAX){
//         cout << "failed to find cloest" << endl;
//         exit(1);
//     }
//     vector<int> result; //?????
//     //cout << "cloest vertex is" << cst << endl;
//     //cout << "cloest distance is" << minDis << endl;
//     result.clear();
//     result.push_back(cst);
//     result.push_back(minDis);
//     //cout << "cloest vertex is" << result[0] << endl;
//     //cout << "cloest distance is" << result[1] << endl;
//     return result;
// }
//
// void Dijkstra(Graph g, Vertex u) { // u is the start Vertex
//     // map<int, Vertex> allVertices = g.Vertices;
//     // for (auto itr = allVertices.begin(); itr != allVertices.end(); ++itr) {}
//
//     // initialization
//     set<int> Nprime;
//     Nprime.insert(u.id);
//     for(int i = 0; i < u.Neighbors.size(); i++){
//         int distance = -1;
//         for(int j = 0; j < g.Edges.size(); j++){
//             if(g.Edges[j].one == u.id && g.Edges[j].theOther == u.Neighbors[i]){
//                 distance = g.Edges[j].dis;
//             }
//             if(g.Edges[j].one == u.Neighbors[i] && g.Edges[j].theOther == u.id){
//                 distance = g.Edges[j].dis;
//             }
//         }
//         if(distance == -1){
//             cout << "failed to find distance" << endl;
//             exit(1);
//         }
//         distanceTable[u.id].insert(make_pair(u.Neighbors[i], distance));
//         // only add neighbor distance, if cannot find {u,v,dis} in distancetable
//         // which means the distancebetween u and v is infinite
//     }
//     // loop
//     while(Nprime.size() < g.Vertices.size()){
//         vector<int> cst = findClosestVertexOfU(Nprime, u);
//         Nprime.insert(cst[0]); // add w tp N'
//         Vertex w = g.Vertices[cst[0]];// get value by key from a map
//         int distance = VertexDistance(u, w); // D(v)
//         for(int i = 0; i < w.Neighbors.size(); i++){
//             if(Nprime.count(w.Neighbors[i]) == 0){ //make sure v is not in Nprime
//                 int cwv = cost(g, w, w.Neighbors[i]);
//                 distance = min(distance, cst[1] + cwv);
//             }
//         }
//     }
// }


int deleteMin(priority_queue<pi, vector<pi>, greater<pi> > & pq){
    int min = pq.top().first;
    int min_node = pq.top().second;
    priority_queue<pi> temp;
    temp.push(pq.top());
    pq.pop();
    while(pq.size() != 0 && pq.top().first == min){
        if(pq.top().second < min_node){
            min_node = pq.top().second;
        }
        temp.push(pq.top());
        pq.pop();
    }
    while(temp.size() != 0){
        if(temp.top().second != min_node){
            pq.push(temp.top());
            temp.pop();
        }else{
            temp.pop();
        }
    }
    return min_node;
}

void deleteFromPq(priority_queue<pi, vector<pi>, greater<pi> > & pq, int v){
    priority_queue<pi, vector<pi>, greater<pi> > temp;
    temp.push(pq.top());
    pq.pop();
    while(pq.size() != 0){
        if(pq.top().second != v){
            temp.push(pq.top());
        }
        pq.pop();
    }
    while(temp.size() != 0){
        pq.push(temp.top());
        temp.pop();
    }
}

void Dijkstra(Graph g, int s, map<int, int> & distance, map<int, int> & prev) { // u is the start Vertex
    map<int, int> visted;
    visted.clear();
    map<int, int> inpq;
    inpq.clear();
    for ( auto it = g.Vertices.begin(); it != g.Vertices.end(); it++ ){ //
        distance[it->first] = INT_MAX;//
        prev[it->first] = -1;
        visted[it->first] = -1;
        inpq[it->first] = -1;
    }
    distance[s] = 0;
    priority_queue<pi, vector<pi>, greater<pi> > pq;
    pq.push(make_pair(distance[s], s));
    inpq[s] = 1;
    while(!pq.empty()){
        int min_node = deleteMin(pq);
        visted[min_node] = 1;
        inpq[min_node] = -1;
        Vertex u = g.Vertices[min_node];
        for(int i = 0; i < u.Edges.size(); i++){
            int v = u.Edges[i].theOther;
            if(visted[v] == 1){
                continue;
            }
            int cost = u.Edges[i].dis;
            if(distance[v] > distance[min_node] + cost){
                if(inpq[v] == 1){
                    deleteFromPq(pq, v);
                    inpq[v] = -1;
                }
                distance[v] = distance[min_node] + cost;
                prev[v] = min_node;
                if(inpq[v] != 1){
                    pq.push(make_pair(distance[v], v));
                    inpq[v] = 1;
                }
            }else if(distance[v] == distance[min_node] + cost){
                if(prev[v] > min_node){
                    prev[v] = min_node;
                }
            }
        }
    }
    distanceTable[s] = distance;
    prevTable[s] = prev;
}

void output_topofile(Graph g, int s, map<int, int> & distance, map<int, int> & prev){
    // generate forwording table
    priority_queue <int, vector<int>, greater<int> > pq;
    for(int i = 1; i <= g.Vertices.size(); i++){
        pq.push(g.Vertices[i].id);
    }
    vector<int> path;
    vector<int> vertexOrder;
    while(!pq.empty()){
        vertexOrder.push_back(pq.top());
        pq.pop();
    }
    // output topology entries
    for(int i = 0; i < vertexOrder.size(); i++){
        int dest = vertexOrder[i];
        if(distance[dest] == INT_MAX){
            continue;
        }
        int cost = distance[dest];
        path.clear();
        int temp = dest;
        path.push_back(dest);
        while(temp != s){
            path.push_back(prev[temp]);
            temp = prev[temp];
        }
        if(path.size() != 1){
            outfile << dest << " " << path[path.size()-2] << " " << cost << endl;
        }else{
            outfile << dest << " " << path[0] << " " << cost << endl;
        }
    }
}

void output_messagefile(vector<messageInfo> messagePair){
    map<int,int> distance,prev;
    for(int i = 0; i < messagePair.size(); i++){
        distance.clear();
        prev.clear();
        distance = distanceTable[messagePair[i].start];
        prev = prevTable[messagePair[i].start];
        int cost = distance[messagePair[i].end];

        outfile << "from " << messagePair[i].start;
        outfile << " to " << messagePair[i].end;
        outfile << " cost ";
        if(cost == INT_MAX){
            outfile << "infinite hops unreachable message " << messagePair[i].message;
        }else{
            stack <int> path;

            int prev_hop = prev[messagePair[i].end];
            while(prev_hop != -1){
                path.push(prev_hop);
                prev_hop = prev[prev_hop];
            }
            outfile << cost << " hops ";
            while(path.size() != 0){
                int temp = path.top();
                path.pop();
                outfile << temp << " ";
            }
            outfile << "message " << messagePair[i].message;
        }
        outfile << endl;
    }
}


int main(int argc, char** argv) {
    //printf("Number of arguments: %d", argc);
    if (argc != 4) {
        printf("Usage: ./linkstate topofile messagefile changesfile\n");
        return -1;
    }

    Graph graph;
    // FILE *fpOut;
    // fpOut = fopen("output.txt", "w");
    outfile.open("output.txt");
    ifstream messagefile(argv[2]);
    ifstream changesfile(argv[3]);

    ifstream intopofile(argv[1]);

    int start, end, dis;
    while(intopofile >> start >> end >> dis){
        Edge e(dis, start, end);
        graph.buildGraph(e);
    }
    intopofile.close();

    // read message file
    vector<messageInfo> messagePair; // record message file
    string curLine;
    while (getline(messagefile, curLine)) {
        sscanf(curLine.c_str(), "%d %d %*s", &start, &end);
        string message = curLine.substr(curLine.find(" ") + 1);
        message = message.substr(message.find(" ") + 1);
        messageInfo temp(start, end, message);
        messagePair.push_back(temp);
    }

    distanceTable.clear();
    prevTable.clear();
    map<int,int> distance,prev;
    for(int i = 1; i <= graph.Vertices.size(); i++){
        distance.clear();
        prev.clear();
        Dijkstra(graph, graph.Vertices[i].id, distance, prev);
        output_topofile(graph, graph.Vertices[i].id, distance, prev);
    }
    output_messagefile(messagePair);

    // read changes file
    vector<Edge> updateEdges;
    string curEdge;
    while (getline(changesfile, curEdge)){
        sscanf(curEdge.c_str(), "%d %d %d", &start, &end, &dis);
        Edge temp(dis, start, end);
        updateEdges.push_back(temp);
    }

    for(int i = 0; i < updateEdges.size(); i++){
        graph.updateEdge(updateEdges[i]);
        distanceTable.clear();
        prevTable.clear();
        for(int i = 1; i <= graph.Vertices.size(); i++){
            distance.clear();
            prev.clear();
            Dijkstra(graph, graph.Vertices[i].id, distance, prev);
            output_topofile(graph, graph.Vertices[i].id, distance, prev);
        }
        // call message output
        output_messagefile(messagePair);
    }
    outfile.close();

    // cout << "vertex Number is" << graph.Vertices.size() << endl;
    // cout << "edge Number is" << graph.Edges.size() << endl;
    // cout << "distance table size is" << distanceTable.size() << endl;
    // map<int,int> distance,prev;
    // distance.clear();
    // prev.clear();
    // Dijkstra(graph, graph.Vertices[1], distance, prev);
    // cout << distanceTable[1][3] << endl;


    return 0;
}
