#include <iostream>
#include <fstream>
#include <vector>
#include <cassert>
#include <string>
#include <unistd.h>
#include <algorithm>
#include "parser_helper.h"
#include "graph.h"

/* Hash tables */
extern std::unordered_map <string, LibParserCellInfo> Cells;
extern std::unordered_map <string, VerParserPinInfo> Pins;
extern std::unordered_map <string, NetParserInfo> NetsHelper;
extern std::unordered_map <string, SpefNet> SpefNets;
std::unordered_map <string, pi_values> PIs;
std::unordered_map <string, po_values> POs;
std::unordered_map <string, NetsInfo> Nets;

/* Create Graph and Nets hash table. */
int create_graph() {

  /* Iterate through all nets and create connections on Pins hash table * 
   * Also make Nets hash table. This will be our primary.               */
  for ( auto it = NetsHelper.begin(); it != NetsHelper.end(); ++it ) {
    
    string key = (it->second).output.instance_name + (it->second).output.pinName;
    string NetsKey;
    NetPin newPin;

    /* Store primary inputs and outputs and connect them *
     * to their corresponding pins.                      */
    if ( (it->second).isPrimaryOut ) {

      /* Set values */
      POs[it->first].linkedBy = (it->second).output;
      POs[it->first].tr_r_early = std::numeric_limits<double>::max();
      POs[it->first].tr_f_early = std::numeric_limits<double>::max();
      POs[it->first].tr_r_late = std::numeric_limits<double>::min();
      POs[it->first].tr_f_late = std::numeric_limits<double>::min();

      newPin.pinName = it->first;
      newPin.instance_name.clear();
      Pins[key].linksTo.push_back( newPin );

      /* Store primary outputs to Nets hash table */
      NetsKey = it->first; // Net name, output
      Nets[NetsKey].netName = NetsKey;
      Nets[NetsKey].fromPin = newPin;


    }

    if ( (it->second).isPrimaryIn ) {

      PIs[it->first].linksTo = (it->second).inputs; // link primary inputs
      for( std::vector<NetPin>::const_iterator i = (it->second).inputs.begin(); i != (it->second).inputs.end(); ++i) {
        string key3 = i->instance_name + i->pinName;

        /* Store info on Nets hash table */
        NetsKey = it->first + i->instance_name+ i->pinName; // Net name + Cell_instance name & net name of the next pin 
        Nets[NetsKey].toPin = *i;
        Nets[NetsKey].netName = it->first;

        /* Store info on Pins hash table */
        newPin.pinName = it->first;
        newPin.instance_name.clear();
        Pins[key3].linkedBy.push_back(newPin);
      }

    }
    
    if ( !key.empty() ) {
      Pins[key].linksTo.insert( Pins[key].linksTo.end(), (it->second).inputs.begin(), (it->second).inputs.end() );

      for ( auto i = (it->second).inputs.begin(); i != (it->second).inputs.end(); i++) {
        /* Store info on Nets hash table */
        NetsKey = it->first + i->instance_name + i->pinName; // Net name + (Cell_instance name & net name) of the next pin 
        Nets[NetsKey].toPin = *i;
        Nets[NetsKey].netName = it->first;
      }

    }

    /* To store linkedBy information, we need to iterate through every input item and store the output to them *
     * Reminder: Since we create PIs and POs above, we make sure that they are not included in Pins table      */
    for( std::vector<NetPin>::const_iterator i = (it->second).inputs.begin(); i != (it->second).inputs.end() && !(it->second).isPrimaryIn && !(it->second).isPrimaryOut; ++i) {

      string key2 = i->instance_name + i->pinName;
      Pins[key2].linkedBy.insert(Pins[key2].linkedBy.end(), it->second.output);

      /* Store Nets fromPin info */
      for ( auto i2 = Pins[key2].linkedBy.begin(); i2 != Pins[key2].linkedBy.end(); i2++) {
        NetsKey = it->first + i->instance_name + i->pinName;
        Nets[NetsKey].fromPin = *i2;
        Nets[NetsKey].netName = it->first;
      }

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

        for ( auto i = NetsHelper.begin(); i != NetsHelper.end(); i++ ) {
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

/* Find delay on Nets */
int find_nets_delay_() {

  for ( auto it = Nets.begin(); it != Nets.end(); ++it ) {

    /* Find information */
    string cellType = (it->second).fromPin.cellType;
    string pinName = (it->second).fromPin.pinName;
    string instance_name = (it->second).fromPin.instance_name;
    string netName = it->second.netName;

    /* If Cell type is empty, we are either connecting a primary input or a primary output with a pin */
    if ( cellType.empty() ) {
      double total_res = 0;
      double total_cap = 0;
      for ( auto j = SpefNets[netName].resistances.begin(); j != SpefNets[netName].resistances.end(); j++ ) {

        /* Maybe the net is input to two or more pins. We check this here */
        if ( j->toNodeName.n1 == netName || j->toNodeName.n1 == (it->second).toPin.instance_name ) {
          total_cap += j->resistance;
          /* Find next resistance */ 
          for ( auto j2 = SpefNets[netName].capacitances.begin(); j2 != SpefNets[netName].capacitances.end(); j2++ )
            if ( j2->nodeName.n1 == j->toNodeName.n1 && j2->nodeName.n2 == j->toNodeName.n2 ){
              total_res += total_cap*j2->capacitance;
              break;            
            }
        }
      }
      /* Store capacitance to Nets hash table */
      Nets[it->first].delay = total_res;

      continue;
    }

    auto tempPin = find_if( Cells[cellType].pins.begin(), Cells[cellType].pins.end(), findPinInfo( pinName ) );      
      
    /* Calculate nets capacitance */
    if ( !tempPin->isInput ) {
      if ( netName.empty() ) 
        continue;

      double total_res = 0;
      double total_cap = 0;
      for ( auto j = SpefNets[netName].resistances.begin(); j != SpefNets[netName].resistances.end(); j++ ) {

        /* Maybe the net is input to two or more pins. We check this here */
        if ( j->toNodeName.n1 == netName || j->toNodeName.n1 == (it->second).toPin.instance_name ) {
          total_cap += j->resistance;
          /* Find next resistance */ 
          for ( auto j2 = SpefNets[netName].capacitances.begin(); j2 != SpefNets[netName].capacitances.end(); j2++ )
            if ( j2->nodeName.n1 == j->toNodeName.n1 && j2->nodeName.n2 == j->toNodeName.n2 ){
              total_res += total_cap*j2->capacitance;
              break;            
            }
        }
      }
      /* Store delay to Nets hash table */
      Nets[it->first].delay = total_res;
    }
    else {
            
      string key = instance_name + pinName;
      for ( auto j = Pins[key].linksTo.begin(); j != Pins[key].linksTo.end(); j++ ) {
        
        /* Make key for Nets hash table. */
        string nextPkey = j->instance_name + j->pinName;
        string netkey = key + j->instance_name + j->pinName;

        /* C is the sum of all the nodes linked to, to the pin we are looking at */
        for ( auto j2 = Pins[nextPkey].linksTo.begin(); j2 != Pins[nextPkey].linksTo.end(); j2++ ) {

          /* If cellType is empty, then we are looking at a net that connects an primary output */
          if ( j2->cellType.empty() )
            continue;

          auto cellPin  = find_if( Cells[j2->cellType].pins.begin(), Cells[j2->cellType].pins.end(), findPinInfo( j2->pinName ) );             
          Nets[netkey].delay += cellPin->capacitance;
                
        }
      }
    }   
  }

#ifdef DEBUG
  for ( auto i = Nets.begin(); i != Nets.end(); i++ ) 
    cout << "Net: " << i->first << " Cap: " << (i->second).delay << endl;
#endif

  return 1;

}

double calculate_net_delay( string netName, string instance_name ) {

  double total_res = 0;
  double total_cap = 0;

  for ( auto j = SpefNets[netName].resistances.begin(); j != SpefNets[netName].resistances.end(); j++ ) {

    /* Maybe the net is input to two or more pins. We check this here */
    if ( j->toNodeName.n1 == netName || j->toNodeName.n1 == instance_name ) {

      total_cap += j->resistance;
      /* Find next resistance */ 
      for ( auto j2 = SpefNets[netName].capacitances.begin(); j2 != SpefNets[netName].capacitances.end(); j2++ )
        if ( j2->nodeName.n1 == j->toNodeName.n1 && j2->nodeName.n2 == j->toNodeName.n2 ){
          total_res += total_cap*j2->capacitance;
          break;            
      }

    }
  }

  return total_res;
}

/* Input: Next Pin key that is connected to the output pin we are connected to */
double calculate_fanout( string nextPkey ) {

  double total_capacitance = 0;

  /* C is the sum of all the nodes linked to, to the pin we are looking at */
  for ( auto j2 = Pins[nextPkey].linksTo.begin(); j2 != Pins[nextPkey].linksTo.end(); j2++ ) {

    /* If cellType is empty, then we are looking at a net that connects an primary output */
    if ( j2->cellType.empty() )
    continue;

    auto cellPin  = find_if( Cells[j2->cellType].pins.begin(), Cells[j2->cellType].pins.end(), findPinInfo( j2->pinName ) );             
    total_capacitance += cellPin->capacitance;
                      
  }

  return total_capacitance;

}

/* BFS algorithm implementation for forward traversal */
int find_nets_delay() {

  string cellType ;
  string pinName;
  string instance_name ;
  string NetsKey;

  for ( auto it = Nets.begin(); it != Nets.end(); ++it ) {
    cout << "Key: " << it->first << endl << "From instance name: " << (it->second).fromPin.instance_name << " From pinName: " << (it->second).fromPin.pinName << endl;
    cout << "To instance name: " << (it->second).toPin.instance_name << " to pinName: " << (it->second).toPin.pinName << endl <<endl;
  }

  /* For forward traversal level 0 is PIs' connections */
  vector<NetPin> fwdLevel0;
  vector<string> Visited;
  unsigned int level = 0;

  // ypologismos Net delay me spef
  for ( auto it = PIs.begin(); it != PIs.end(); it++ ) {
    fwdLevel0.insert( fwdLevel0.end(), (it->second).linksTo.begin(), (it->second).linksTo.end() );

    /* Calculate delay for all inputs */
    for ( auto k = (it->second).linksTo.begin(); k != (it->second).linksTo.end(); k++ ) {

      NetsKey = it->first + k->instance_name + k->pinName;
      /* Calculate and store delay to Nets hash table */
      Nets[NetsKey].delay = calculate_net_delay( it->first, k->instance_name);

    }
   
  }


  bool isFinalLevel = false;
  vector<NetPin> currLevel = fwdLevel0;
  vector<NetPin> nextLevel;
  nextLevel.clear();

  /* Begin traversing graph */
  while ( !isFinalLevel ) {
    
    isFinalLevel = true;

    /* Start iterating current level */
    for (std::vector<NetPin>::const_iterator i = currLevel.begin(); i != currLevel.end(); ++i) {

      string key =  i->instance_name + i->pinName;

      /* Build next level. Check if pin is in visited list. TODO: make list bitmap */
      if ( std::find(Visited.begin(), Visited.end(), key) == Visited.end() ) {
        isFinalLevel = false;

        // Insert pin to visited list
        Visited.insert(Visited.end(), key );

        std::vector<LibParserPinInfo>::iterator tempPin  = std::find_if( Cells[i->cellType].pins.begin(), Cells[i->cellType].pins.end(),
                                                                         findPinInfo( i->pinName ) );      
 

        /* If pin is input, that means that it is connected in-cell. *
         * So we need to call interpolation/extrapolation functions, *
         * else calculate delay with wire capacitances               */
        if ( Pins[key].isInput ) {

          /* For is not needed because we only have one in-cell *
           * connection but makes our life easier cause we      * 
           * don't have to check if list is empty manually      */
          for ( auto j = Pins[key].linksTo.begin(); j != Pins[key].linksTo.end(); j++ ) {
            
            /* Make key for Nets hash table. */
            string nextPkey = j->instance_name + j->pinName;
            string netkey = key + j->instance_name + j->pinName;

            double fan_out = calculate_fanout( nextPkey );
          }

        }
        else {

          /* Iterate through all connections and calculate total delay. */
          for ( auto k = Pins[key].linksTo.begin(); k != Pins[key].linksTo.end(); k++ ) {

            if ( k->instance_name.empty() ) // primary output
              NetsKey = Pins[key].connNetName ;
            else
              NetsKey = Pins[key].connNetName + k->instance_name + k->pinName;

            /* Calculate and store delay to Nets hash table */
            Nets[NetsKey].delay = calculate_net_delay( Pins[key].connNetName , k->instance_name);

          }

        }


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
    level++;
  }

  for ( auto i = Nets.begin(); i != Nets.end(); i++ ) 
      cout << "Net: " << i->first << " delay: " << (i->second).delay << endl;

  return 1;

}

int bfs_on_graph_bwd() {




}
