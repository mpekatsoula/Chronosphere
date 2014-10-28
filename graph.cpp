#include <iostream>
#include <fstream>
#include <vector>
#include <cassert>
#include <string>
#include <unistd.h>
#include <algorithm>
#include "parser_helper.h"
#include "graph.h"

extern std::unordered_map <string, LibParserCellInfo> Cells;
extern std::unordered_map <string, VerParserPinInfo> Pins;
extern std::unordered_map <string, NetParserInfo> Nets;
std::unordered_map <string, pi_values> PIs;
std::unordered_map <string, po_values> POs;

int create_graph() {

  // Iterate through all nets and create connections on Pins hash table
  for ( auto it = Nets.begin(); it != Nets.end(); ++it ) {
    
    string key = (it->second).output.instance_name + (it->second).output.pinName;

    /* Store primary inputs and outputs and connect them *
     * to their corresponding pins.                      */
    if ( (it->second).isPrimaryOut ) {
      POs[it->first].linkedBy = (it->second).output;
      NetPin newPin;
      newPin.pinName = it->first;
      newPin.instance_name.clear();
      Pins[key].linksTo.push_back( newPin );
    }

    if ( (it->second).isPrimaryIn ) {
      PIs[it->first].linksTo = (it->second).inputs; // link primary inputs
      for( std::vector<NetPin>::const_iterator i = (it->second).inputs.begin(); i != (it->second).inputs.end(); ++i) {
        string key3 = i->instance_name + i->pinName;
        NetPin newPin;
        newPin.pinName = it->first;
        newPin.instance_name.clear();
        Pins[key3].linkedBy.push_back(newPin);
      }
    }
    
    if ( !key.empty() )
      Pins[key].linksTo.insert( Pins[key].linksTo.end(), (it->second).inputs.begin(), (it->second).inputs.end() );



    /* To store linkedBy information, we need to iterate through every input item and store the output to them *
     * Reminder: Since we create PIs and POs above, we make sure that they are not included in Pins table      */
    for( std::vector<NetPin>::const_iterator i = (it->second).inputs.begin(); i != (it->second).inputs.end() && !(it->second).isPrimaryIn && !(it->second).isPrimaryOut; ++i) {
      string key2 = i->instance_name + i->pinName;
      Pins[key2].linkedBy.insert(Pins[key2].linkedBy.end(), it->second.output);
    }
   

  }

  return 1;

}

/* Print graph to graphviz format, using BFS */
int print_graph() {


  string Colors[] = {"aliceblue", "powderblue", "lightskyblue","deepskyblue", "cornflowerblue","royalblue", \
                     "darkslateblue", "lightblue", "mediumblue", "midnightblue", "navy", "navyblue", "skyblue"} ;

  vector<NetPin> fwdLevel0;
  vector<string> Visited, Visited2;
  cout << "digraph fwd {" << endl ;
  cout << "graph [rankdir=LR,fontsize=10];" << endl;
  /* Create subgraphs */
  for ( auto it = Pins.begin(); it != Pins.end(); ++it ) {
    for ( std::vector<NetPin>::const_iterator j = (it->second).linksTo.begin(); j != (it->second).linksTo.end(); j++ ) {
      /* For each instance name create subgraph. Stupid implementation maybe fix it later */

      if ( std::find(Visited.begin(), Visited.end(), j->instance_name) == Visited.end() && !j->instance_name.empty() ) {
        cout << "subgraph cluster" << j->instance_name << "{" << endl;
        Visited.insert(Visited.end(), j->instance_name );

        for ( auto i = Nets.begin(); i != Nets.end(); i++ ) {
          for ( auto i2 = (i->second).inputs.begin(); i2 != (i->second).inputs.end(); i2++ ) {
            if ( i2->instance_name == j->instance_name && !(i->second).output.instance_name.empty() ) {    
              string key = i2->instance_name + i2->pinName;
              
              if ( std::find(Visited2.begin(), Visited2.end(), key) == Visited2.end() ) {
                cout << "\t" << key << ";" <<  endl;          
                Visited2.insert(Visited2.end(), key );
              }
            }
          }
          if ( (i->second).output.instance_name  == j->instance_name && !(i->second).output.instance_name.empty() ) {
            
              string key = (i->second).output.instance_name + (i->second).output.pinName;
              
              if ( std::find(Visited2.begin(), Visited2.end(), key) == Visited2.end() ) {
                cout << "\t" << key << ";" <<  endl;          
                Visited2.insert(Visited2.end(), key );
              }
          }
        }
        cout << "\tlabel = \"" << j->instance_name << "\";" << endl;
        cout << "}" << endl;
      }
      
    }
  }

  /* Clear Visited */
  Visited.clear();

  unsigned int level = 0;

  /* Primary inputs subgraph */
  cout << "subgraph clusterPIs {" << endl;
  cout << "color=palegreen;" << endl;
  cout << "node [shape=box,color=palegreen,fontsize=10];" << endl;
  cout << "label=\"Primary Inputs\";" << endl;
  for ( auto it = PIs.begin(); it != PIs.end(); ++it )
    cout << "\t" << it->first << ";" << endl;
  cout << "}" << endl;

  /* Primary outputs subgraph */
  cout << "subgraph clusterPOs {" << endl;
  cout << "color=salmon;" << endl;
  cout << "node [shape=box,color=salmon,fontsize=10];" << endl;
  cout << "label=\"Primary Outputs\";" << endl;
  for ( auto it = POs.begin(); it != POs.end(); ++it )
    cout << "\t" << it->first << ";" << endl;
  cout << "}" << endl;

  for ( auto it = PIs.begin(); it != PIs.end(); ++it ) {

    fwdLevel0.insert( fwdLevel0.end(), (it->second).linksTo.begin(), (it->second).linksTo.end() );



    for ( std::vector<NetPin>::const_iterator j = (it->second).linksTo.begin(); j != (it->second).linksTo.end(); j++ ) {
      cout << "\t" << it->first << "->" <<  j->instance_name + j->pinName << ";" << endl; 
      cout << "\t" << j->instance_name + j->pinName << "[color=" << Colors[level] << ",style=filled,fontsize=10]" << endl;    
    }
  }

  bool isFinalLevel = false;
  vector<NetPin> currLevel = fwdLevel0;
  vector<NetPin> nextLevel;
  nextLevel.clear();
  level++;

  while ( !isFinalLevel ) {
    
    isFinalLevel = true;
  
    /* Start iterating current level */
    for (std::vector<NetPin>::const_iterator i = currLevel.begin(); i != currLevel.end(); ++i) {

      string key =  i->instance_name + i->pinName;

      /* Build next level. Check if pin is in visited list. */
      if ( std::find(Visited.begin(), Visited.end(), key) == Visited.end() ) {
        isFinalLevel = false;

        // Insert pin to visited
        Visited.insert(Visited.end(), key );

        for ( std::vector<NetPin>::const_iterator j = Pins[key].linksTo.begin(); j != Pins[key].linksTo.end(); j++ )  {
          
          /* Print outputs with red color */
          if ( !j->instance_name.empty() )
            cout << "\t" << j->instance_name + j->pinName << "[color=" << Colors[level % NUM_OF_COLORS] << ",style=filled,fontsize=10]" << endl;

          cout << "\t" << key << "->" <<  j->instance_name + j->pinName << ";" << endl; 
        }
    
        nextLevel.insert( nextLevel.end(), Pins[key].linksTo.begin(), Pins[key].linksTo.end() );
      }
    }

    /* Clear nextLevel for next iteration */
    currLevel = nextLevel;
    nextLevel.clear();
    level++;
  }


  cout << "}" << endl;
/*
  cout << "\nLinked By (backwards traversal): \n" ;
  for ( auto it = POs.begin(); it != POs.end(); ++it )
    cout << "Primary: " + it->first + "  ----> " + it->second.linkedBy.instance_name + it->second.linkedBy.pinName << endl;

  for ( auto it = Pins.begin(); it != Pins.end(); ++it ) {

    cout << it->first + " ----> " ;
  
    for( std::vector<NetPin>::const_iterator i = (it->second).linkedBy.begin(); i != (it->second).linkedBy.end(); ++i)
      if ( i->instance_name.empty()  )
        cout << "primary: " + i->pinName << " ";
      else
        cout << i->instance_name + i->pinName << " ";

      cout << endl;
   
  }
*/
}

void search_indicies() {



}

int bfs_on_graph_fwd() {

  /* For forward traversal level 0 is PIs' connections */
  vector<NetPin> fwdLevel0;
  vector<string> Visited;
  for ( auto it = PIs.begin(); it != PIs.end(); ++it )
    fwdLevel0.insert( fwdLevel0.end(), (it->second).linksTo.begin(), (it->second).linksTo.end() );

  bool isFinalLevel = false;
  vector<NetPin> currLevel = fwdLevel0;
  vector<NetPin> nextLevel;
  nextLevel.clear();
  cout << "Printing PIs' connections" << endl;
  while ( !isFinalLevel ) {
    
    isFinalLevel = true;
    cout << " ---------------------------------------------------------- " << endl;
  
    /* Start iterating current level */
    for (std::vector<NetPin>::const_iterator i = currLevel.begin(); i != currLevel.end(); ++i) {

      string key =  i->instance_name + i->pinName;
      
      /* Build next level. Check if pin is in visited list. TODO: make list bitmap */
      if ( std::find(Visited.begin(), Visited.end(), key) == Visited.end() ) {
        isFinalLevel = false;

        // Insert pin to visited
        Visited.insert(Visited.end(), key );

        /* If pin is input, that means that it is connected in-cell. *
         * So we need to call interpolation/extrapolation functions, *
         * else do  */
        if ( Pins[key].isInput )
          cout << "Calling interpolation" << endl;
        else
          cout << "Do something else!" << endl;

        int number_of_arcs = Cells[i->cellType].timingArcs.size();
  /*      if ( number_of_arcs ) {
          for (  int j = 0; j < number_of_arcs; j++ ) {
            cout << "From ---> To" << endl;
            cout << Cells[i->cellType].timingArcs[j].fromPin << endl;
            cout << Cells[i->cellType].timingArcs[j].toPin << endl << endl;
//            if ( Cells[i->cellType].timingArcs[j].fromPin == i->pinName && !Cells[i->cellType].timingArcs[j].fallDelay.loadIndices.empty() )
//              cout << "Cell type: " << Cells[i->cellType].timingArcs[j].fallDelay.loadIndices[4] << endl; // Binary search here
          }
        }
*/
        cout << key << endl << endl << endl;
        nextLevel.insert( nextLevel.end(), Pins[key].linksTo.begin(), Pins[key].linksTo.end() );
      }
    }

    /* Clear nextLevel for next iteration */
    currLevel = nextLevel;
    nextLevel.clear();
    
  }
  return 1;

}

int bfs_on_graph_bwd() {




}
