#include <iostream>
#include <vector>
#include <string>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <set>
#include <map>

using namespace std;

class Edge {
public:
    int dis;
    int one;
    int theOther;

    Edge(int distance, int start, int end){
        dis = distance;
        one = start;
        theOther = end;
    }

};

class Vertex {
public:
    int id;
    //Vertex prev;
    vector<Edge> Edges;    //不确定需不需要
    vector<int> Neighbors;
    //map<int, map<int, int>> fowardingTable; // one, the other and next-hop

    Vertex(int idNo){
        id = idNo;
        //prev = NULL;
        Edges.clear();//可能需要
        Neighbors.clear();
        //fowardingTable.clear();
    }

    Vertex(){
        Edges.clear();
        Neighbors.clear();
    }

    void insertEdge(Edge e){
        if(e.one == id){
            for(int i = 0; i < Neighbors.size(); i++){
                if(Neighbors[i] == e.theOther){
                    return;
                }
            }
            Edges.push_back(e);
            Neighbors.push_back(e.theOther);
        }
    }

    void deleteEdge(int index){
        vector<Edge> temp_Edges;
        for(int i = 0; i < Edges.size(); i++){
            if(i != index){
                temp_Edges.push_back(Edges[i]);
            }
        }
        Edges.clear();
        for(int i = 0; i < temp_Edges.size(); i++){
            Edges.push_back(temp_Edges[i]);
        }
    }

    void deleteNeighbor(int theOther){
        vector<int> temp_Neighbors;
        for(int i = 0; i < Neighbors.size(); i++){
            if(Neighbors[i] != theOther){
                temp_Neighbors.push_back(Neighbors[i]);
            }
        }
        Neighbors.clear();
        for(int i = 0; i < temp_Neighbors.size(); i++){
            Neighbors.push_back(temp_Neighbors[i]);
        }
    }

    void updateEdge(Edge e){
        for(int i = 0; i < Edges.size(); i++){
            if(Edges[i].one == e.one && Edges[i].theOther == e.theOther){
                if(e.dis <= 0){
                    deleteEdge(i);
                    deleteNeighbor(e.theOther);
                    return;
                }else{
                    Edges[i].dis = e.dis;
                    return;
                }
            }
        }
        //insertEdge(e);
    }

    // void UpdateEdge(Edge e){
    //     for(int i = 0; i < Edges.size(); i++){
    //         if(Edges[i].one == e.one && Edges[i].theOther == e.theOther){
    //             Edges[i].dis = e.dis;
    //         }
    //         if(Edges[i].theOther == e.one && Edges[i].one == e.theOther){
    //             Edges[i].dis = e.dis;
    //         }
    //     }
    // }
};

class Graph {
public:
    vector<Edge> Edges;
    map<int, Vertex> Vertices;

    Graph(){
        Vertices.clear();
        Edges.clear();
    }

    void buildGraph(Edge e){
        Edges.push_back(e);
        Edge eRever = Edge(e.dis, e.theOther, e.one);
        if(Vertices.find(e.one) == Vertices.end()){ //does not find one
            // operation on Vertex
            Vertex one(e.one);
            one.insertEdge(e);
            // operation on graph
            Vertices.insert(make_pair(e.one, one));
        }else{
            Vertices[e.one].insertEdge(e);
        }
        if(Vertices.find(e.theOther) == Vertices.end()){//does not find theOther
            // operation on Vertex
            Vertex theOther(e.theOther);
            theOther.insertEdge(eRever);
            // operation on graph
            Vertices.insert(make_pair(e.theOther, theOther));
        }else{
            Vertices[e.theOther].insertEdge(eRever);
        }
    }

    void deleteEdge(Edge e){
        vector<Edge> temp_Edges;
        for(int i = 0; i < Edges.size(); i++){ // double direction check
            if(Edges[i].one == e.one && Edges[i].theOther == e.theOther){
                continue;
            }else if(Edges[i].theOther == e.one && Edges[i].one == e.theOther){
                continue;
            }
            temp_Edges.push_back(Edges[i]);
        }
        Edges.clear();
        for(int i = 0; i < temp_Edges.size(); i++){ // double direction check
            Edges.push_back(temp_Edges[i]);
        }
    }

    void updateEdge(Edge e){
        // update edge for Vertices
        if(Vertices.find(e.one) != Vertices.end()){
            Vertices[e.one].updateEdge(e);
        }
        if(Vertices.find(e.theOther) != Vertices.end()){
            Edge eRever = Edge(e.dis, e.theOther, e.one);
            Vertices[e.theOther].updateEdge(eRever);
        }
        // update edge for graph
        if(e.dis <= 0){
            deleteEdge(e);
            return;
        }
        for(int i = 0; i < Edges.size(); i++){ // double direction check
            if(Edges[i].one == e.one && Edges[i].theOther == e.theOther){
                Edges[i].dis = e.dis;
                return;
            }else if(Edges[i].theOther == e.one && Edges[i].one == e.theOther){
                Edges[i].dis = e.dis;
                return;
            }
        }
        buildGraph(e);
    }
};

class messageInfo {
public:
    int start;
    int end;
    string message;

    messageInfo(){
        start = -1;
        end = -1;
        message = "";
    }

    messageInfo(int s, int e, string m){
        start = s;
        end = e;
        message = m;
    }
};
