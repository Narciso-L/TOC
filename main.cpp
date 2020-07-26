/*Copyright (C) <2017>  <Narciso López-López>
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#Authors: Narciso López & Andrea Vázquez

#include <algorithm>
#include <chrono>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <list>
#include <sstream>
#include <tgmath.h>


#define NUM_SECONDS_TO_WAIT 10// Time limit para ATB

using namespace std;
using namespace std::chrono;


struct node{
    unsigned short id;                 // Id del nodo
    unsigned short numConexiones = 0;  //Número de conexiones del nodo (Parte individual)
};


struct modelo{
    unsigned short id;  // Id del nodo
    float price = 0;    // Precio del router
    float pfail = 0;    // Probabilidad de fallo del router
};


struct estado{
    vector<unsigned short> solution;   // Vector que contiene los modelos asignados a los routers en un estado
    double f;
};


float probTotal = 0;    // Probabilidad de fallo total de los modelos de routers


//Ordenar 2 nodos en función del número de conexiones (Parte individual)
bool orderNodesByConnections (node i, node j){
    return (i.numConexiones > j.numConexiones);
}


//Ordenar 2 modelos en función de la probabilidad de fallo (Parte individual)
bool orderModelsBypFail (modelo i, modelo j){
    return (i.pfail < j.pfail);
}


// BFS modificado para encontrar el número de islas teniendo en cuenta nodos desconectados
void BFS(vector< list< int > > adj,vector< int > &res, estado est, int s, unsigned short modelDesc){
    int V = adj.size();
    bool *visited = new bool[V];

    for(unsigned short i = 0; i < V; i++)
        visited[i] = false;

    // Cola del bfs
    list<int> queue;

    // Añadir el primer nodo a la lista de visitados
    visited[s] = true;
    queue.push_back(s);

    list<int>::iterator i;  // Iterador para recorrer la lista de adyacencia

    while(!queue.empty()){
        s = queue.front();
        res[s] = 1;
        queue.pop_front();  // Se saca el primer elemento
        // Se recorren todos los vértices adyacentes que no hayan sido visitados
        for(i = adj[s].begin(); i != adj[s].end(); ++i){
            if (!visited[*i]) { // Si no está en la lista de visitados y si su modelo es diferente al modelo eliminado
                visited[*i] = true;
                if (est.solution[*i]!=modelDesc)
                    queue.push_back(*i);        // Se añade a la cola
            }
        }
    }
}


// Obtener el número de componentes conexas
unsigned short compConex(vector< list< int > > adj, estado e, unsigned short modelDesc){
    unsigned short ini = 0; // Primer valor desde el que se realiza el bfs
    vector<int> res(adj.size(),0);  // Resultado del bfs
    unsigned short ccn = 1; // Número de componentes conexas

    // Este bucle busca un nodo de inicio que no esté desconectado
    if(e.solution[ini]!=-1){ // Comenzar el recorrido
        while (ini < adj.size() && e.solution[ini] == modelDesc){
            ini++;
        }
    }

    BFS (adj,res,e,ini,modelDesc);  // Primer BFS

    for (unsigned short i = 0; i<res.size(); i++){  // Comprueba nodos marcados para contar componentes conexas
        if (res[i] == 0 && e.solution[i]!= modelDesc){
            BFS(adj, res, e, i, modelDesc);
            ccn++;
        }
    }
    return ccn;
}


// Contar el número de conexiones de cada nodo (Parte individual)
void countConnections (vector< list< int > > &adj,vector< node > &nodes){
    for (unsigned short i = 0; i<nodes.size(); i++){
        nodes[i].numConexiones = adj[i].size();
    }
}


// Función auxiliar para usar en look4IsolatedNodes
// Devuelve true si encuentra los nodos que están aislados (Parte individual)
bool isoContains(int it,vector<int> &iso){
    unsigned short i = 0;
    if (!iso.empty()) {
        while (i < iso.size() && iso[i] != it)
            i++;
        if (iso[i] == it)
            return true;
        else
            return false;
    }
    return false;
}


// Buscar los nodos aislados (en principio éstos deberían de tener como enlace el mejor router) (Parte individual)
vector<int> look4IsolatedNodes(vector< list< int > > &adj,vector<node> &nodos){
    vector<int> iso;
    for (unsigned short i = 0; i<adj.size(); i++){
        if (adj[i].size() == 1){                    // Comprueba que la lista de ady tiene una conexión
            list<int>::iterator it = adj[i].begin();
            if (!isoContains (*it,iso))             // Si encuentra nodos aislados los almacena
                iso.push_back(*it);
        }
    }
    return iso;
}


// Estado inicial con inicialización de los routers con más probabilidad de fallo (Parte individual)
unsigned int initWorstRouter(unsigned short modelosOrd, vector<modelo>&modelos, estado &estado){
    unsigned int coste = 0;
    for (unsigned short i = 0; i<estado.solution.size(); i++){
        estado.solution[i] = modelosOrd;
        coste += modelos[modelosOrd].price;
    }
    return coste;
}


// Inicialización teniendo en cuenta el número de conexiones (Parte individual)
estado initNumConnections(vector< list< int > > &adj,vector<modelo> &modelos,vector< node > &nodes, unsigned int budget){
    unsigned short tam = nodes.size();
    vector<unsigned short> sol(tam);   // Vector de la solución
    estado est;
    est.solution = sol;
    vector<int> isolatedRouters = look4IsolatedNodes(adj,nodes);    // Buscar nodos aislados
    vector<modelo>modelosOrd = modelos;                             // Vector de modelos ord por pFail
    sort(modelosOrd.begin(), modelosOrd.end(), orderModelsBypFail); // Ordenar modelos por probabilidad de fallo

    // Inicializar con los routers con más probabilidad de fallo
    unsigned int precio = initWorstRouter(modelosOrd[modelosOrd.size()-1].id,modelos,est);

    // Asignar los mejores routers a los nodos aislados
    for (unsigned short i = 0; i<isolatedRouters.size() && precio<=budget; i++){
        if(est.solution[isolatedRouters[i]]!=modelosOrd[0].id){
            est.solution[isolatedRouters[i]] = modelosOrd[0].id;
            precio += modelosOrd[0].price;                      // Se incrementa el coste del modelo del router más caro
            precio -= modelosOrd[modelosOrd.size()-1].price;    // Se decrementa el coste del router más barato
        }
    }

    if (precio<=budget) {
        countConnections(adj, nodes);   // Contar número de conexiones
        vector<node> nodesOrd = nodes;  // Nodos ordenados en función del número de conexiones
        sort(nodesOrd.begin(), nodesOrd.end(), orderNodesByConnections);    // Ord nodos por # conexiones
        // Calcular el # conexiones del nodo más conectado que supera el 80% de conexiones
        unsigned short probConexiones = (unsigned char) (ceil((float) nodesOrd[0].numConexiones * 0.8));
        unsigned short i = 0;

        // Cambiar modelo de router más malo por el mejor teniendo en cuenta el presupuesto
        while(nodesOrd[i].numConexiones >= probConexiones && precio <= budget ){
            if (est.solution[nodesOrd[i].id] != modelosOrd[0].id ){
                est.solution[nodesOrd[i].id] = modelosOrd[0].id;
                precio += modelosOrd[0].price;                     // Se incrementa el coste del modelo del router más caro
                precio -= modelosOrd[modelosOrd.size() - 1].price; // Se decrementa el coste del router más barato
            }
            i++;
        }
        // Calcular el # conexiones del nodo más conectado que supera el 40% de conexiones
        unsigned short probConexiones2 = (unsigned char) (ceil((float) nodesOrd[0].numConexiones * 0.4));
        unsigned short j = 0;

        // Cambiar modelo de router más malo por el 2º mejor teniendo en cuenta el presupuesto
        while(nodesOrd[j].numConexiones >= probConexiones2 && precio <= budget ){
            if (est.solution[nodesOrd[j].id] != modelosOrd[0].id ){
                if (nodesOrd[j].numConexiones >= probConexiones2 && nodesOrd[j].numConexiones < probConexiones){
                    est.solution[nodesOrd[j].id] = modelosOrd[1].id;
                    precio += modelosOrd[1].price;         // Se incrementa el coste del 2º modelo del router más caro
                    precio -= modelosOrd[modelosOrd.size() - 1].price; // Se decrementa el coste del router más barato
                }
            }
            j++;
        }

        if (precio > budget) {  // Si se ha sobrepasado el presupuesto se quita el más caro por el más barato
            est.solution[i] = modelosOrd[modelosOrd.size()-1].id;
            precio -= modelosOrd[0].price;
            precio += modelosOrd[modelosOrd.size()-1].price;
        }
    }

    return est;
}


// Inicialización aleatoria (Parte común)
estado iniRand(unsigned short numModelos,vector< node > &nodes){
    unsigned short tam = nodes.size();
    vector<unsigned short> sol(tam);   // Vector de la solución
    estado est;
    est.solution = sol;

    for (unsigned short i = 0; i < tam; i++) {
        unsigned short r = rand() % (((numModelos - 1) - 0) + 1) + 0;  // Número aleatorio entre 0 y el último valor de modelos
        est.solution[i] = r;
    }

    return est;
}


// Función de evaluación (Parte común)
float fitness(vector< list< int > > &adj,estado &est, vector<modelo> &modelos, unsigned int budget){
    unsigned int cost = 0;      // budget
    unsigned short i = 0;
    float pfail;                // probabilidad de fallo
    unsigned short ccn;         // componentes conexas
    float resultado = 0;

    //Se va calculando el precio de todos los routers hasta que sobrepase el presupuesto
    while ( (i < est.solution.size()) && (cost <= budget) ){
        cost += modelos[est.solution[i]].price;
        i++;
    }

    if (cost <= budget){
        //Número de islas sin fallo
        //Se calcula el número de componentes conexas desconectando cada grupo de modelos
        for (unsigned short i = 0; i<modelos.size(); i++){
            ccn = compConex(adj,est,i);
            pfail = modelos[i].pfail;
            resultado += (pfail/probTotal)*ccn;    //(ccn*pfail);//*(ccnNoFail*npfail);
        }
        //  (P1/P_total)*#islas_si_falla_modelo1 + (P2/P_total)*#islas_si_falla_modelo2 + (P3/P_total)*#islas_si_falla_modelo3
    }

    est.f = resultado;
    return resultado;
}


// Función para visualizar la lista de adyacencia (Parte común)
void printAdjList(vector< list< int > > adjList){
    for (unsigned short i = 0; i < adjList.size(); ++i){
        list< int >::iterator itr = adjList[i].begin();

        while (itr != adjList[i].end()) {
            cout <<" -> "<< (*itr);
            ++itr;
        }
        cout << endl;
    }
}


// Función que crea un vector de nodos  (Parte común)
vector<node> createNodesVector(unsigned short numNodes){
    vector<node> nodes;
    for (unsigned short i = 0; i<numNodes; i++){
        node n;
        n.id=i;
        nodes.push_back(n);
    }
    return nodes;
}


// Generación de vecindario con un router aleatorio de la solución actual y
// 2 routers del estado mínimo para intercambiarlos en la nueva solución (Parte individual)
estado generateNeighborhood(estado s, estado sMin, unsigned short models ){
    estado newNeighborhood = s;
    unsigned short sizeNeighborhood = s.solution.size();

    // Posiciones aleatorias
    unsigned short pos1 = rand()%(( (sizeNeighborhood - 1) - 0) + 1) + 0;
    unsigned short pos2 = rand()%(( (sizeNeighborhood - 1) - 0) + 1) + 0;
    unsigned short pos3 = rand()%(( (sizeNeighborhood - 1) - 0) + 1) + 0;

    while ( pos1 == pos2 ) //Comprobar que sean posiciones distintas
        pos2 = rand()%(( (sizeNeighborhood - 1) - 0) + 1) + 0;
    while ( pos1 == pos3 || pos2 == pos3 )
        pos3 = rand()%(( (sizeNeighborhood - 1) - 0) + 1) + 0;

    unsigned short model1 = rand()%(((models - 1) - 0) + 1) + 0;
    while ( model1 == s.solution[pos1]) // Comprobar que sean modelos distintos
        model1 = rand()%(( (models - 1) - 0) + 1) + 0;

    unsigned short model2 = sMin.solution[pos2];
    unsigned short model3 = sMin.solution[pos3];
    //   cout<<endl <<pos1 <<" "<<pos2<<" "<<pos3<<" "<<model1<<" "<<model2<<" "<<model3 <<" " << endl;
    newNeighborhood.solution[pos1] = model1;
    newNeighborhood.solution[pos2] = model2;
    newNeighborhood.solution[pos3] = model3;

    return newNeighborhood;
}


/* Simulated Annealing
*  Entradas:
*  adj: Lista de adyacencia
*  modelos: Vector de modelos
*  sO: El estado inicial inicializado aleatoriamente
*  kmax: El máximo número de pasos
*  budget: Presupuesto
*  nodos: vector de nodos
*  t1: ATB
*
*  Salida: Vector de estados*/  // (Parte común)
estado SA(vector< list< int > > &adj, vector<modelo> &modelos, estado &s0, float kmax, unsigned int budget,vector<node> &nodos, time_t t1){
    estado s = s0;
    estado snew;
    estado sMin = s0;
    float Es;       // Función de evaluación de s
    float Esnew;    // Función de evaluación de snew
    float EsMin = INTMAX_MAX; // Evaluación mínima
    float T = 1;      // Temperatura
    float tmin = 0.001;
    float alfa = 0.9;
    float P = 0;    // Probabilidad de intercambiar el estado

    cout << "Iterando, por favor espere..."<< endl;
    while (T>tmin){
        for (unsigned short i = 0; i<kmax; i++) {

            Es = fitness(adj, s, modelos, budget);  // Función de evaluación del estado
            // Generar una configuración inicial (vecino) correcta
            while (Es == 0 ) {                       // Si la función de evaluación es 0 --> sobrepasa el coste
                //s = iniRand(modelos.size(), nodos);
                s = generateNeighborhood(s ,sMin, modelos.size());
                Es = fitness(adj, s, modelos, budget);
            }

            cout << "Eval Es: "<< Es << endl;
            if (Es < EsMin) {   // Si la f.eval es menor que la eval mín, se asigna como mín, y se asigna el estado
                EsMin = Es;
                sMin = s;
            }

            if (EsMin <= 1) break;
            // Generar un vecino que no tenga coste = 0
            //snew = iniRand(modelos.size(),nodos);
            snew = generateNeighborhood(s, sMin, modelos.size());

            Esnew = fitness(adj, snew, modelos, budget);    // Función de evaluación del estado nuevo
            while (Esnew == 0) {                            // Si la función de evaluación es 0 --> sobrepasa el coste
                //snew = iniRand(modelos.size(), nodos);
                snew = generateNeighborhood(s, sMin, modelos.size());
                Esnew = fitness(adj, snew, modelos, budget);
            }

            if (Esnew<Es)   // Comparamos las funciones de evaluación
                s = snew;   // Asignamos el nuevo estado como inicial
            else {
                P = exp((Es - Esnew) / T);  // Función exp de SA
                float r = ((double) rand() / (double) (RAND_MAX));  //Se genera un aleatorio entre 0 1

                if (P > r)      // Si la probabilidad sobrepasa el # aleatorio
                    s = snew;   // Asignamos en nuevo estado como inicial
            }

            time_t t2 = time(0);    // Cálculo de tiempo para ATB
            if ((t2 - t1) >= NUM_SECONDS_TO_WAIT) {
                break;
            }
        }
        if (EsMin <= 1) break;
        T *= alfa;  // Decrementar temperatura

    }
    cout << endl << "Evaluación mínima: " << EsMin;
    return sMin;
}


int main(int argc, char *argv[]){

    srand(time(NULL));      // Inicialización semilla aleatoria

    ifstream infile;

    infile.open(argv[1]);   // Fichero de entrada

    unsigned short totalNodes;         // Número total de nodos
    unsigned short numE;               // Número de aristas
    unsigned int budget;             // Presupuesto total
    unsigned short numModels;          // Tipos de modelos de routers

    string line;            // Cada línea del fichero

    // Cálculo de tiempos para ATB
    high_resolution_clock::time_point tt1 = high_resolution_clock::now();   // HRC
    clock_t tStart = clock();
    time_t t1 = time(0);

    /******************************** LEER FICHERO ENTRADA ***************************************/
    // Se leen el número total de nodos
    getline(infile, line);
    istringstream iss(line);
    iss >> totalNodes;
    cout << "Número de nodos: " << totalNodes << endl;

    // Se crea el vector de nodos
    vector<node> nodes = createNodesVector(totalNodes);

    // Se crea la lista de adyacencia que es un vector de listas, cada elemento es una lista de <int>
    vector<list<int> > adjacencyList(totalNodes);

    // Exportar a fichero .csv para cargar en Gephi las aristas
    ofstream fileEdges;
    fileEdges.open (argv[2]);    // Cambiar la ruta según el pc
    fileEdges << "Source,Target\n";

    // Se leen el número de aristas
    getline(infile, line);
    istringstream iss4(line);
    iss4 >> numE;
    cout << "Número de aristas: " << numE << endl;

    // Se lee el grafo
    for (unsigned short i = 0; i < numE; i++) {
        getline(infile, line);
        istringstream iss(line);
        int v1, v2;
        iss >> v1;
        iss >> v2;
        adjacencyList[v1].push_back(v2);
        adjacencyList[v2].push_back(v1);
        fileEdges << v1 << "," << v2 << endl;   // .csv edges
    }

    fileEdges.close();

    // Se lee el presupuesto
    getline(infile, line);
    istringstream iss5(line);
    iss5 >> budget;
    cout << "Presupuesto total: " << budget << endl;

    // Se lee el número total de modelos (tipos de router)
    getline(infile, line);
    istringstream iss6(line);
    iss6 >> numModels;
    cout << "Tipos de router: " << numModels << endl << endl;

    // Vector de modelos del router
    vector<modelo> modelos(numModels);

    // Leer precio y probabilidad de fallo
    for (unsigned short i = 0; i < numModels; i++) {
        getline(infile, line);
        istringstream iss(line);
        iss >> modelos[i].id;
        iss >> modelos[i].price;
        iss >> modelos[i].pfail;
    }

    infile.close(); // Cerrar fichero

    // Cálculo de la probabilidad de fallo total de los modelos de routers
    for(unsigned short i = 0; i<modelos.size(); i++){
        probTotal += modelos[i].pfail;
    }

    // Mostrar el grafo completo (lista de adyacencia)
    cout << "Grafo" << endl;
    printAdjList(adjacencyList);
    cout << endl;

    // Se muestra la información de cada uno de los nodos del grafo
    /* cout << "Información de los nodos" << endl;
     for (int i = 0; i < nodes.size(); i++) {
         cout << "Nodo " << nodes[i].id << " --> ";
         if (nodes[i].isRouter == false)
             cout << "Es un cliente" << endl;
         else {
             cout << "Es un router " << endl;
         }
     }

     for (int i = 0; i < modelos.size(); i++) {
         cout << "Modelo número: " << modelos[i].id << endl;
         cout << "Precio: " << modelos[i].price << endl;
         cout << "Probabilidad de fallo: " << modelos[i].pfail << endl;
     }*/


    /***************************** BEGIN ***********************************/

    // Inicialización
    //estado ini = iniRand(numModels, nodes);
    estado ini = initNumConnections(adjacencyList,modelos,nodes,budget);

    // Imprimir el resultado por pantalla
    cout << endl << "Asignaciones después de la inicialización aleatoria" << endl;
    for (unsigned short i = 0; i < ini.solution.size(); i++) {
        cout << "Router: " << i << " -> ";
        cout << "Modelo: " << ini.solution[i] << endl;
    }
    cout << endl;

    // Simulated Annealing
    estado e = SA(adjacencyList, modelos, ini, 200, budget, nodes, t1);

    // Visualizar solución final
    cout << endl << endl;
    for (unsigned short i = 0; i < e.solution.size(); i++) {
        cout << "Nodo: " << i << " -->" << e.solution[i] << endl;
    }
    cout << endl;

    for(unsigned short i = 0; i< modelos.size() ; i++){
        cout << "Número de islas si se cae el modelo de router: " << i << "  " << compConex(adjacencyList, e, i) << endl;
    }
    cout << endl;

    unsigned short rojos, azules, verdes, cianes = 0;
    for(unsigned short i = 0; i < e.solution.size(); i++) {
        if(e.solution[i] == 0) rojos++;
        if(e.solution[i] == 1) azules++;
        if(e.solution[i] == 2) verdes++;
        if(e.solution[i] == 3) cianes++;
    }

    cout << "# Modelo 0 (rojos): " << rojos << endl;
    cout << "# Modelo 1 (azules): " << azules<< endl;
    cout << "# Modelo 2 (verdes): " << verdes<< endl;
    cout << "# Modelo 3 (cianes): " << cianes<< endl<<endl;

    // Cálculo de tiempos
    clock_t tEnd = clock();
    printf("Time taken: %.2fs\n", (double)(tEnd - tStart)/CLOCKS_PER_SEC);

    high_resolution_clock::time_point tt2 = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>( tt2 - tt1 ).count();
    cout << "High Res Clk: " << duration;

    // Exportar a fichero .csv para cargar en Gephi los nodos
    ofstream fileNodes;
    fileNodes.open (argv[3]);    // Cambiar la ruta según el pc
    fileNodes << "Id,Label,colour\n";
    for (unsigned short i = 0; i < e.solution.size(); i++) {   // Asignar colores según modelos
        // if(e.solution[i] == -1) fileNodes << i << "," << i << "," << "#FFFFFF" << endl; //Blanco
        if(e.solution[i] == 0) fileNodes << i << "," << i << "," <<  "#FF0000" << endl; //Rojo
        if(e.solution[i] == 1) fileNodes << i << "," << i << "," <<  "#2E64FE" << endl; //Azul
        if(e.solution[i] == 2) fileNodes << i << "," << i << "," <<  "#00FF00" << endl; //Verde
        if(e.solution[i] == 3) fileNodes << i << "," << i << "," <<  "#58FAF4" << endl; //Cian
        if(e.solution[i] == 4) fileNodes << i << "," << i << "," <<  "#FFFF00" << endl; //Amarillo
    }
    fileNodes.close();


    return 0;
}
