
#ifndef GRAPHNODE_H
#define GRAPHNODE_H

#include <vector>
#include "llvm/ADT/GraphTraits.h"



template<typename T>
class GraphNode {
public:
  T Value;
  std::vector<GraphNode<T>*> Edges;
};


namespace llvm {
template <typename T>
struct GraphTraits<GraphNode<T> > {
  typedef GraphNode<T>* NodeRef;
  typedef typename std::vector<GraphNode<T>* >::iterator ChildIteratorType;


  static NodeRef getEntryNode(const GraphNode<T> &G){
    return const_cast<GraphNode<T>*>(&G);
  }

  static ChildIteratorType child_begin(NodeRef Node){
    return Node->Edges.begin();
  }

  static ChildIteratorType child_end(NodeRef Node){
    return Node->Edges.end();
  }

};
} // End namespace llvm


#endif
