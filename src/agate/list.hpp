//
// Created by Maxwell on 2023-11-30.
//

#ifndef AGATE_INTERNAL_LIST_HPP
#define AGATE_INTERNAL_LIST_HPP

namespace agt {



  struct list_node {
    list_node* next;
    list_node* prev;
  };


  struct flist_node {
    flist_node* next;
  };



  class raw_flist {
    flist_node*  m_head;
    flist_node** m_tail;
  public:

    raw_flist() : m_head(nullptr), m_tail(&m_head) { }


    void push_back(flist_node* node) noexcept {
      *m_tail = node;
      m_tail = &node->next;
    }

    void push_front(flist_node* node) noexcept {
      if (m_head == nullptr)
        m_tail = &node->next;
      node->next = m_head;
      m_head = node;
    }

    void pop_front() noexcept {
      m_head = m_head->next;
      if (m_head == nullptr)
        m_tail = &m_head;
    }

    [[nodiscard]] flist_node* front() const noexcept {
      return m_head;
    }
  };

  template <typename T>
  concept flist_capable = std::derived_from<T, object> && requires(T* obj, T*& lval)
  {
    obj->next = obj;
    lval = obj->next;
    atomicCompareExchange(obj->next, lval, obj);
  };


  template <typename T>
  class flist {
  public:
    using pointer = T*;

    static_assert(flist_capable<T>);


    flist() = default;


    [[nodiscard]] uint32_t size() const noexcept {
      return m_size;
    }

    [[nodiscard]] bool empty() const noexcept {
      return m_size == 0;
    }

    void push_back(pointer obj) noexcept {
      *m_tail = obj;
      m_tail  = &obj->next;
      ++m_size;
    }

    // to is inclusive.
    void push_back(pointer from, pointer to, uint32_t count) noexcept {
      *m_tail = from;
      m_tail = &to->next;
      m_size += count;
    }

    void push_front(pointer obj) noexcept {
      if (m_head == nullptr)
        m_tail = &obj->next;
      obj->next = m_head;
      m_head = obj;
      ++m_size;
    }

    // to is inclusive.
    void push_front(pointer from, pointer to, uint32_t count) noexcept {
      if (m_head == nullptr)
        m_tail = &to->next;
      to->next = m_head;
      m_head = from;
      m_size += count;
    }

    void pop_front() noexcept {
      m_head = m_head->next;
      if (m_head == nullptr)
        m_tail = &m_head;
      --m_size;
    }

    // to is inclusive.
    void pop_front(pointer to, uint32_t count) noexcept {
      if (count == m_size)
        m_tail = &m_head;
      m_head = to->next;
      m_size -= count;
    }




    // inserts element after pos
    void insert(pointer pos, pointer element) noexcept {
      assert( pos != nullptr );
      assert( element != nullptr );
      if (m_tail == &pos->next)
        m_tail = &element->next; // this is effectively pushing to back in this case, so we adjust the tail as necessary.
      element->next = pos->next;
      pos->next = element;
      ++m_size;
    }





    [[nodiscard]] pointer front() const noexcept {
      return m_head;
    }

    [[nodiscard]] pointer back() const noexcept {
      assert( !empty() );
      return *m_tail;
    }

  private:
    pointer   m_head = nullptr;
    pointer*  m_tail = &m_head;
    uint32_t  m_size = 0;
  };


}

#endif //AGATE_INTERNAL_LIST_HPP
