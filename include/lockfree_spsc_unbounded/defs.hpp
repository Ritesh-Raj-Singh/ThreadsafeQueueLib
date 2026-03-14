#ifndef LOCKFREE_SPSC_UNBOUNDED_DEFS
#define LOCKFREE_SPSC_UNBOUNDED_DEFS

#include "utils.hpp"
#include <atomic>
#include <memory>
#include <type_traits>

namespace tsfqueue::__impl {
template <typename T> class lockfree_spsc_unbounded {
  // Works exactly same as the blocking_mpmc_unbounded queue (see this once)
  // with tail pointer pointing to stub node and your head pointer updates as
  // per the pushes. See the Lockless_Node in utils to understand the working.

  // Note that the next pointers are atomic there. Why ?? [Reason this]
  //--------------------My understanding:------------------
  //if head and tail point to same stub node ,sp in trying to modify next ptr,sc is trying to read
  // next ptr(to check if queue is empty)->race condition

  // Also the head and tail members are cache-aligned. Why ?? [Reason this] (ask
  // me for details)
  //-------------------My understanding:-------------------
  //cpu update on common cache line,if sp makes some changes,in invalidates other cores(sc) cache,so this has to keep on updating
  //which is time consuming
  //cache aligning make the pointers sit on different cache lines,so now we can spam modifications(very latent)

  // [Copy of blocking_mpmc_unbounded]
  // For the implementation, we start with a stub node and both head and tail
  // are initialized to it. 
  //When we push, we make a new stub node, move the data
  // into the current tail and then change the tail to the new stub. 
  //We have two methods : wait_and_pop() which waits on the queue and returns element &
  // try_pop() which returns an element if queue is not empty otherwise returns
  // some neutral element OR a false boolean whichever is applicable. Pop works
  // by returning the data stored in head node and replacing head to its next
  // node. We handle the empty queue gracefully as per the pop type.

private:
  using node = tsfqueue::__utils::Lockless_Node<T>;

  // Add the private members :
  // 1. node* head;
  // 2. node* tail;

  //------------------Note:--------------------
  //we do not need atomic head/tail due to spsc
  //doing cache align so that head and tail are in different cache lines
  //we do not use shared pointers due to reference counter bottlneck-we are keeping the pointers light weight

  alignas(tsfq::__impl::cache_line_size)node * head;
  alignas(tsfq::__impl::cache_line_size)node *tail;

  // Description of priavte members :
  // 1. node* head -> Pointer to the head node
  // 2. node* tail -> Pointer to tail node
  // 3. Cache align 1-2

public:
  // Public member functions :

  // Add relevant constructors and destructors -> Add these here only
  //-----------Constructor-------------
  //using memory_order_relaxed because default ordering is memory_order_seq_cst
  lockfree_spsc_unbounded(){
    node* stub= new node();
    stub->next.store(nullptr,std::memory_order_relaxed);

    head=stub;
    tail=stub;
  }
  //-----------Destructor---------------
  ~lockfree_spsc_unbounded(){
    while(head!=nullptr){

      node* current=head;
      head=head->next.load(std::memory_order_relaxed);
      delete current;

    }
  }


  // 1. void push(value) : Pushes the value inside the queue, copies the value
  void push(T);

  // 2. void wait_and_pop(value ref) : Blocking wait on queue, returns value in
  // the reference passed as parameter
  void wait_and_pop(T&);

  // 3. bool try_pop(value ref) : Returns true and
  // gives the value in reference passed, false otherwise
  bool try_pop(T&);

  // 4. bool empty() : Returns
  // whether the queue is empty or not at that instant
  bool empty();

  // 5. bool peek(value ref) : Returns the front/top element of queue in ref (false if empty queue)
  bool peek(T&);

  // 6. Add static asserts

  // 7. Add emplace_back using perfect forwarding and variadic templates (you
  // can use this in push then)
  template<typename... Args>
  void emplace_back(Args&&... args);

  // 8. Add size() function
  int size();

  // 9. Any more suggestions ??

  // 10. Why no shared_ptr ?? [Reason this]
};
} // namespace tsfqueue::__impl

#endif