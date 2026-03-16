    #ifndef LOCKFREE_SPSC_UNBOUNDED_IMPL
    #define LOCKFREE_SPSC_UNBOUNDED_IMPL

    #include "defs.hpp"

    //----------------------------------------
    //compiler was not able to find defintions using this alias
    // template <typename T>
    // using queue = tsfqueue::__impl::lockfree_spsc_unbounded<T>;
    //----------------------------------------


    template <typename T> void tsfqueue::__impl::lockfree_spsc_unbounded<T>::push(T value) {
        //std::move casts it as rvalue-no cost of copying
        emplace_back(std::move(value));
    }

    template <typename T> bool tsfqueue::__impl::lockfree_spsc_unbounded<T>::try_pop(T &value) {    
        node* new_head = head->next.load(std::memory_order_acquire);

        if(new_head == nullptr){
            return false;
        }

        node* old_head = head;
        value = std::move(head->data);
        head = new_head;
        
        delete old_head;

        capacity.fetch_sub(1,std::memory_order_relaxed);

        return true;
    }   

    template <typename T> void tsfqueue::__impl::lockfree_spsc_unbounded<T>::wait_and_pop(T &value) {
        
        while(head->next.load(std::memory_order_acquire) == nullptr){
            std::this_thread::yield();//low latency-high cpu usage
        }

        node* new_head = head->next.load(std::memory_order_relaxed);
        node* old_head = head;
        value = std::move(head->data);
        
        head = new_head;

        delete old_head;

        capacity.fetch_sub(1,std::memory_order_relaxed);
        
    }

    template <typename T> bool tsfqueue::__impl::lockfree_spsc_unbounded<T>::peek(T &value) {
        if(empty()){
            return false;
        }
        value = head -> data;
        return true;
    }

    template <typename T> bool tsfqueue::__impl::lockfree_spsc_unbounded<T>::empty(void) {
        return (head->next.load(std::memory_order_acquire) == nullptr);
    }

    template<typename T>
    template<typename... Args>
    void tsfqueue::__impl::lockfree_spsc_unbounded<T>::emplace_back(Args&&... args){
            node* stub = new node();
            stub->next.store(nullptr,std::memory_order_relaxed);

            tail->data = T(std::forward<Args>(args)...);
            capacity.fetch_add(1,std::memory_order_relaxed);
            tail->next.store(stub,std::memory_order_release);

            tail = stub;
    }

    template<typename T>
    size_t tsfqueue::__impl::lockfree_spsc_unbounded<T>::size(){
        return capacity.load(std::memory_order_relaxed);
    }

    #endif

    // 1. Add static asserts

    // 2. Add emplace_back using perfect forwarding and variadic templates (you
    // can use this in push then)

    // 3. Add size() function

    // 4. Any more suggestions ??