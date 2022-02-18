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
#include <queue>
typedef pair<int, int> pi;

map<int, map<int, int> > distanceTable;
map<int, map<int, int> > prevTable;

ofstream outfile;

void DV(Graph g, int s, map<int, int> & distance, map<int, int> & prev){
    for (auto it = g.Vertices.begin(); it != g.Vertices.end(); it++){
        distance[it->first] = INT_MAX - 1000; //max
        prev[it->first] = -1;
    }
    distance[s] = 0;
    for(int i = 1; i < g.Vertices.size(); i++){
        for(int j = 0; j < g.Edges.size(); j++){
            int cost = g.Edges[j].dis;
            int u = g.Edges[j].one;
            int v = g.Edges[j].theOther;
            if(distance[u] + cost < distance[v]){
                distance[v] = distance[u] + cost;
                prev[v] = u;
            } else if(distance[u] + cost == distance[v]){
                //tiebreak
                if(u < prev[v]){
                    prev[v] = u;
                }
            }
            if(distance[v] + cost < distance[u]){
                distance[u] = distance[v] + cost;
                prev[u] = v;
            }else if (distance[v] + cost == distance[u]){
                //tiebreak;
                if(v < prev[u]){
                    prev[u] = v;
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
        if(distance[dest] == INT_MAX - 1000){
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
        if(cost == INT_MAX - 1000){
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
        DV(graph, graph.Vertices[i].id, distance, prev);
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
            DV(graph, graph.Vertices[i].id, distance, prev);
            output_topofile(graph, graph.Vertices[i].id, distance, prev);
        }
        // call message output
        output_messagefile(messagePair);
    }
    outfile.close();


    return 0;
}
