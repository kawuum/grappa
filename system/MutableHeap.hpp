
#ifndef __MUTABLE_HEAP_HPP__
#define __MUTABLE_HEAP_HPP__

#include <cassert>
#include <iostream>
#include <limits>

#include <vector>
#include <tr1/unordered_map>

#include "common.hpp"

template< typename Priority, typename Key >
class MutableHeap 
{
private:
  DISALLOW_COPY_AND_ASSIGN( MutableHeap );
  
  struct HeapElement {
    Priority priority;
    Key key;
  };

  typedef std::vector< HeapElement > Heap;
  Heap heap;

  typedef int HeapIndex;
  typedef std::tr1::unordered_map< Key, HeapIndex > IndexMap;
  IndexMap index_map;

  typedef typename std::vector< HeapElement >::iterator HeapIterator;

  bool is_heap_helper( HeapIterator first, int n ) {
    HeapIndex parent = 0;
    for( HeapIndex child = 1; child < n; ++child ) {
      if( first[ parent ].priority < first[ child ].priority ) {
	return false;
      }
      if( 0 == child & 1 ) {
	++parent;
      }
    }
    return true;
  }

  bool is_heap( HeapIterator first, HeapIterator last ) {
    return is_heap_helper( first, std::distance( first, last ) );
  }

  void push_heap_helper( HeapIterator first, HeapIndex hole_index, HeapIndex top_index, HeapElement value ) {
    HeapIndex parent = ( hole_index - 1 ) / 2;
    while( hole_index > top_index && (first + parent)->priority <= value.priority ) {
      *(first + hole_index) = *(first + parent);
      index_map[ (first + hole_index)->key ] = hole_index;
      hole_index = parent;
      parent = (hole_index - 1) / 2;
    }
    *(first + hole_index) = value;
    index_map[ (first + hole_index)->key ] = hole_index;
  }

  void push_heap( HeapIterator first, HeapIterator last ) {
    push_heap_helper( first, (last - first) - 1, 0, *(last - 1) );
  }

  void adjust_heap( HeapIterator first, HeapIndex hole_index, int len, HeapElement value ) {
    const HeapIndex top_index = hole_index;
    HeapIndex second_child = 2 * hole_index + 2;
    while( second_child < len ) {
      if( (first + second_child)->priority <= (first + (second_child - 1))->priority ) {
	second_child--;
      }
      *(first + hole_index) = *(first + second_child);
      index_map[ (first + hole_index)->key ] = hole_index;
      hole_index = second_child;
      second_child = 2 * (second_child + 1);
    }
    if( second_child == len ) {
      *(first + hole_index) = *(first + (second_child - 1));
      index_map[ (first + hole_index)->key ] = hole_index;
      hole_index = second_child - 1;
    }
    push_heap_helper( first, hole_index, top_index, value );
  }

  void pop_heap_helper( HeapIterator first, HeapIterator last, HeapIterator result, HeapElement value ) {
    *result = *first;
    index_map[ result->key ] = result - first;
    adjust_heap( first, 0, last - first, value );
  }

  void pop_heap( HeapIterator first, HeapIterator last ) {
    pop_heap_helper( first, last - 1, last - 1, *(last - 1) );
  }

  void make_heap( HeapIterator first, HeapIterator last ) {
    if( last - first < 2 ) {
      return;
    }

    const int len = last - first;
    HeapIndex parent = (len - 2) / 2;
    while( true ) {
      adjust_heap( first, parent, len, *(first + parent) );
      if( parent == 0 ) {
	return;
      }
      parent--;
    }
  }

public:
  MutableHeap( )
    : heap()
    , index_map()
  { }

  bool empty() {
    return heap.begin() == heap.end();
  }

  void insert( Key key, Priority priority ) {
    HeapElement e = { priority, key };
    heap.push_back( e );
    index_map[ key ] = (heap.end() - heap.begin()) - 1;
    push_heap( heap.begin(), heap.end() );
  }

  Priority top() {
    return (heap.begin())->priority;
  }

  Priority top_priority( ) {
    if( heap.begin() == heap.end() ) {
      return 0;
    } else {
      return (heap.begin())->priority;
    }
  }

  Priority top_key( ) {
    if( heap.begin() == heap.end() ) {
      return 0;
    } else {
      return (heap.begin())->key;
    }
  }

  void remove() {
    HeapElement e = *(heap.end() - 1);
    pop_heap( heap.begin(), heap.end() );
    heap.erase( heap.end() - 1 );
    index_map.erase( e.key );
  }

  void increase( Key key, Priority priority ) {
    HeapIndex location = index_map[ key ];
    heap[ location ].priority = priority;
    push_heap( heap.begin(), heap.begin() + index_map[ key ] + 1);
  }


  void update( Key key, Priority priority ) {
    HeapIndex location = index_map[ key ];
    heap[ location ].priority = std::numeric_limits< Priority >::max();
    push_heap( heap.begin(), heap.begin() + index_map[ key ] + 1);
    pop_heap( heap.begin(), heap.end() );
    assert( (heap.end() - 1)->key == key );
    (heap.end() - 1)->priority = priority;
    push_heap( heap.begin(), heap.end() );
  }

  void remove_key( Key key ) {
    HeapIndex location = index_map[ key ];
    heap[ location ].priority = std::numeric_limits< Priority >::max();
    push_heap( heap.begin(), heap.begin() + index_map[ key ] + 1);
    pop_heap( heap.begin(), heap.end() );
    assert( (heap.end() - 1)->key == key );
    HeapElement e = *(heap.end() - 1);
    pop_heap( heap.begin(), heap.end() );
    heap.erase( heap.end() - 1 );
    index_map.erase( e.key );
  }


  void update_or_insert( Key key, Priority priority ) {
    if( index_map.find( key ) == index_map.end() ) {
      insert( key, priority );
    } else if( priority >= heap[ index_map[ key ] ].priority ) {
      increase( key, priority );
    } else {
      heap[ index_map[ key ] ].priority = std::numeric_limits< Priority >::max();
      push_heap( heap.begin(), heap.begin() + index_map[ key ] + 1);
      pop_heap( heap.begin(), heap.end() );
      assert( (heap.end() - 1)->key == key );
      (heap.end() - 1)->priority = priority;
      push_heap( heap.begin(), heap.end() );
    } 
  }

  bool check() {
    return is_heap( heap.begin(), heap.end() );
  }

  void dump() {
    //assert( heap.size() == index.size() );
    std::cout << "heap {" << std::endl;
    for( HeapIndex i = 0; i < heap.size(); ++i ){
      std::cout << "  index " << i 
		<< ": priority " << heap[i].priority
		<< " key " << heap[i].key
		<< std::endl;
    }
    std::cout << "}" << std::endl;
    std::cout << "index {" << std::endl;
    for( typename std::tr1::unordered_map< Key, HeapIndex >::iterator i = index_map.begin(); i != index_map.end(); ++i ) {
      std::cout << "  key " << i->first
		<< ": index " << i->second
		<< " priority " << heap[ i->second ].priority
		<< " key " << heap[ i->second ].key
		<< std::endl;
    }
    std::cout << "}" << std::endl;
  }
};
#endif