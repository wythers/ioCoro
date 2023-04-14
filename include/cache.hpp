#pragma once

#include <atomic>
#include <cstring>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

using std::atomic;
using std::make_shared;
using std::make_unique;
using std::memset;
using std::move;
using std::pair;
using std::shared_ptr;
using std::thread;
using std::unique_ptr;
using std::vector;

namespace zeus {

// from https://github.com/wythers/zeus


/**
 * The zeus::Pool provides a thread-safe object pool, and Get and Put
 * lock-free. It uses a thread-local Cache to store the objects, Volume as a
 * scheduling unit, and Circle to manage the objects, as well as gracefully
 * construction/destruction is implemented.
 *
 *  @b Policies:
 *
 *    ===============================================
 *    #                      Cache                  #
 *    #                    ---------                #
 *    #  --------          |  L1   |                #
 *    #  |thread| <------> |  L2   |                #
 *    #  --------          |  L3   |                #
 *    #                    |Victims|                #
 *    #                    ---------                #
 *    ===============================================
 *
 *  @b Requirement:
 *  - Pool must be unique in the one project
 *  - T
 *  - clusterSize must be a power of 2, default is 8
 *
 *  @authors
 *  - Stanv Chou
 *  - Wyther Yang
 * */
template <typename T>
class Pool {
  class Cache;
  class Volumn;
  class Circle;

  inline static constexpr auto RX = std::memory_order_relaxed;
  inline static constexpr auto RE = std::memory_order_release;
  inline static constexpr auto AC = std::memory_order_acquire;
  inline static constexpr auto AR = std::memory_order_acq_rel;

  inline static constexpr uint32_t MaxCapOfVolumn = 1 << 16;
  // idle is an address at kernel space as an idle tag
  inline static Volumn* const idle = (Volumn*)(0xffff'dead'0000'0000);

  template <typename VC>
  struct alignas(64) Atomic : atomic<VC*> {};

  class Circle {
   public:
    // push must only be called by a single producer
    auto push(T* val) noexcept -> bool {
      int len = slots.size();
      auto [head, tail] = headAndtail.load(RX);

      // the circle is full
      if (tail + len == head) return false;

      atomic<T*>& slot = slots[head & (len - 1)];

      // has a consumer racing with us for the last slot, so give up the round
      if (slot.load(RX)) return false;

      // we won the round, and keep going
      slot.store(val, RE);

      auto transformer = reinterpret_cast<atomic<uint64_t>*>(&headAndtail);
      transformer->fetch_add(1, RX);

      return true;
    }

    auto pop() noexcept -> pair<T*, bool> {
      int idx{}, len = slots.size();
      for (;;) {
        auto cur = headAndtail.load(RX);
        auto [head, tail] = cur;

        // the circle is empty
        if (head == tail) return {nullptr, false};

        decltype(cur) tmp = {cur.head, cur.tail + 1};
        if (headAndtail.compare_exchange_weak(cur, tmp, RX, RX)) {
          idx = cur.tail & (len - 1);
          break;
        }
      }

      auto& slot = slots[idx];
      // get resource right now
      T* ret = slot.exchange(nullptr, AC);

      return {ret, true};
    }

    auto popFromHead() noexcept -> pair<T*, bool> {
      int idx{}, len = slots.size();
      for (;;) {
        auto cur = headAndtail.load(RX);
        auto [head, tail] = cur;

        if (head == tail) return {nullptr, false};

        decltype(cur) tmp = {cur.head - 1, cur.tail};
        if (headAndtail.compare_exchange_weak(cur, tmp, RX, RX)) {
          idx = (cur.head - 1) & (len - 1);
          break;
        }
      }

      auto& slot = slots[idx];

      // no racing with, RX instead of RE
      T* ret = slot.exchange(nullptr, RX);

      return {ret, true};
    }

    Circle(uint cap = 8) : slots(cap) {
#ifdef ZEUS_LIBRARY_DEBUG
      NumOfCirs.fetch_add(1, RX);
#endif
    }

#ifdef ZEUS_LIBRARY_DEBUG
    ~Circle() { NumOfCirs.fetch_sub(1, RX); }
#endif

#ifdef ZEUS_LIBRARY_DEBUG
    inline static atomic<uint64_t> NumOfCirs{};
#endif

   protected:
    // 64bits is a luck number, almost meet all hard arch
    struct packed {
      uint32_t head{};
      uint32_t tail{};
    };
    atomic<packed> headAndtail{};

    // Volumn guarantees the slots visible to other threads, so its not atomic
    vector<atomic<T*>> slots{};
  };

  class Volumn : public Circle {
    friend class Cache;

   public:
    Volumn(uint cap = 8) : Circle(cap) {
#ifdef ZEUS_LIBRARY_DEBUG
      NumOfVolumns.fetch_add(1, RX);
#endif
    }

    ~Volumn() {
      for (;;) {
        auto [p, ok] = this->popFromHead();
        if (ok)
          operator delete(p);
        else
          break;
      }
#ifdef ZEUS_LIBRARY_DEBUG
      NumOfVolumns.fetch_sub(1, RX);
#endif
    }

    auto size() const noexcept -> int { return this->slots.size(); }

   public:
#ifdef ZEUS_LIBRARY_DEBUG
    inline static atomic<uint64_t> NumOfVolumns{};
#endif

   private:
    Volumn* victim{};
    atomic<Volumn*> next{};
  };

  class Cache {
   private:
    friend class Pool;
    using L1 = T*;
    using L2 = Volumn*;
    using L3 = atomic<Volumn*>;
    using Victim = atomic<Volumn*>;

    using ID_t = atomic<int32_t>;

   public:
    Cache() = default;
    Cache(Cache const&) = delete;
    Cache const& operator=(Cache const&) = delete;

    ~Cache() {
      // because the last cache's deconstruction follows the deconstruction of the pool.
      // so must make the last cache does not access the pool that have been deconstructed.
      if (!last) {
        auto id = myself.load(RX);
        refs[id].store(nullptr, RX);

        while (!askForBuddy(buddies, this)) {
          std::this_thread::yield();
          fromVictims(false);
        }

        hazards[id].store(nullptr, RX);
        buddies[id].store(nullptr, RX);

        ids[id].store(thread::id{}, RX);
      }

      fromVictims(false);
      if (slab) operator delete(slab);

      auto* head = shared.exchange(nullptr, AC);
      while (head) {
        unique_ptr<Volumn> _{head};
        auto* next = head->next.load(AC);
        head = next;
      }
    }

    auto putToL1OrL2(T* single) noexcept -> void {
      if (!slab) {
        slab = single;
        return;
      }

      // lazy initialization
      if (!local) {
        local = new Volumn{};
        shared.store(local, RE);
      }

      auto ok = local->push(single);
      if (ok) return;

      // try get new volumn and GC
      auto* newVol = fromVictims(true);
      if (!newVol) {
        uint32_t cap = (local->size()) * 2;
        cap = cap >= MaxCapOfVolumn ? 8 : cap;
        newVol = new Volumn{cap};
      }

      local->next.store(newVol, RE);
      local = newVol;

      local->push(single);
    }

    auto getFromL1OrL2() noexcept -> T* {
      if (slab) {
        T* ret = slab;
        slab = nullptr;
        return ret;
      }

      if (!local) return nullptr;

      auto [ret, _] = local->popFromHead();

      return ret;
    }

    auto getFromL3(atomic<Volumn*>& hazard) noexcept -> T* {
      // try GC first
      fromVictims(false);

      Volumn* top = shared.load(RX);

      // issue:1172
      if (!top) return nullptr;

      for (;;) {
        Volumn* swapper{};
        do {
          swapper = top;
          hazard.store(swapper, RX);
          top = shared.load(AC);
        } while (swapper != top);

        // issue:1172, null ptr never appear here
        // if (!top)
        // return nullptr;

        auto [ret, ok] = top->pop();
        if (ok) {
          hazard.store(nullptr, RX);
          return ret;
        }

        auto* next = top->next.load(AC);

        if (!next) {
          hazard.store(nullptr, RX);
          return nullptr;
        }

        // got a zombie Volumn, give up the round
        if (next == idle) {
          top = shared.load(RX);
          continue;
        }

        if (shared.compare_exchange_strong(top, next, RE, RX)) {
          hazard.store(nullptr, RX);
          if (askForBuddy(hazards, top)) {
            // GC the single volumn
            unique_ptr<Volumn> _{top};
          } else {
            top->next.store(idle, RX);
            Volumn* newV{};
            do {
              top->victim = newV;
            } while (!victims.compare_exchange_weak(newV, top, RE, RX));
          }
          top = next;
        }
      }
    }

    auto fromVictims(bool get) noexcept -> Volumn* {
      Volumn *ret{}, *vlist = victims.exchange(nullptr, AC);
      Volumn *head{}, *tail{};

      while (vlist) {
        auto* next = vlist->victim;
        vlist->victim = nullptr;
        auto ok = askForBuddy(hazards, vlist);

        if (!ok) {
          if (!head) {
            head = vlist;
            tail = head;
          } else {
            vlist->victim = head;
            head = vlist;
          }
        } else {
          if (get && !ret) {
            vlist->next.store(nullptr, RX);
            vlist->victim = nullptr;
            ret = vlist;
          } else
            unique_ptr<Volumn> _{vlist};
        }

        vlist = next;
      }

      if (head) {
        Volumn* tmp{};
        do {
          tail->victim = tmp;
        } while (!victims.compare_exchange_weak(tmp, head, RE, RX));
      }

      return ret;
    }

    auto askForBuddy(auto arr, auto garbage) const noexcept -> bool {
      for (uint i = 0; i < clusterSize; i++) {
        auto* tmp = arr[i].load(RX);
        if (tmp == garbage) return false;
      }

      return true;
    }

    operator bool() const noexcept { return hazards || buddies || refs || ids; }

    // must ensure that the number of threads is less than or equal to clusterSize,
    // otherwise it's looping forever
    auto init() noexcept -> void {
      auto cur = std::this_thread::get_id();
      for (uint i = 0;; i &= (clusterSize-1)) {
        thread::id nil{};
        if (ids[i].compare_exchange_strong(nil, cur, RX, RX)) {
          myself.store(i, RX);
          refs[i].store(this, RX);
          break;
        }
        i++;
      }
    }

    template<typename R>
    struct ref {
      vector<R>* p{};

      operator bool() const {
        return p;
      }

      R& operator[](uint i) {
        return (*p)[i];
      }

      void operator=(vector<R>& arr) {
        p = &arr;
      }
    };

   private:
    L1 slab{};

    L2 local{};

    L3 shared{};

    Victim victims{};

    ID_t myself{};

    bool last{};

    // context arr
    ref<Atomic<Volumn>> hazards{};
    ref<Atomic<Cache>> buddies{};
    ref<atomic<Cache*>> refs{};
    ref<atomic<thread::id>> ids{};
  };

 public:
  Pool(uint num = std::thread::hardware_concurrency() * 2)
  : hazards(num)
  , buddies(num)
  , refs(num)
  , ids(num)
  {
    static_assert(is_always_lock_free(),
                  "the platform does not support lock-free atomic operation");
    clusterSize = num;
  }
  ~Pool() { cache.last = true; }

  Pool(Pool const&) = delete;
  Pool& operator=(Pool const&) = delete;

  /**
   * @return true, if implementation is lock-free.
   *
   * @warning On most platforms, the whole implementation is lock-free
   **/
  static constexpr auto is_always_lock_free() -> bool {
    return Cache::L3::is_always_lock_free;
  }
  // Put adds the single to the pool, it first call destruction of single.
  // similar to:
  //      delete single;
  auto Put(T* single) noexcept -> void {
    if (!single) return;

    if (!cache) {
      cache.hazards = hazards;
      cache.buddies = buddies;
      cache.refs = refs;
      cache.ids = ids;
      cache.init();
    }

    // access cache to save the resource
    cache.putToL1OrL2(single);
  }

  // Get selects an arbitrary item from the Pool, removes it from the Pool,
  // and returns it to the caller after calling construction.
  // similar to:
  //      return new T{arg1, arg2, ...};
  template <typename... Args>
  [[nodiscard]] auto Get(Args&&... args) noexcept -> T* {
    if (!cache) {
      cache.hazards = hazards;
      cache.buddies = buddies;
      cache.refs = refs;
      cache.ids = ids;
      cache.init();
    }

    T* src = cache.getFromL1OrL2();
    auto myself = cache.myself.load(RX);

    for (uint i = 0; !src && i < clusterSize; i++) {
      auto idx = (myself + i) & (clusterSize-1);
      auto* ref = refs[idx].load(RX);
      Cache* swapper{};
      do {
        swapper = ref;
        buddies[myself].store(swapper, RX);
        ref = refs[idx].load(RX);
      } while (swapper != ref);

      if (!ref) continue;

      src = ref->getFromL3(hazards[myself]);
      buddies[myself].store(nullptr, RX);
    }

#ifdef ZEUS_LIBRARY_DEBUG
    return src ? new (src) T{forward<Args>(args)...} : nullptr;
#else
    return src ? src
               : new T{forward<Args>(args)...};
#endif
  }

  // GetSize returns cluster size, which is the maximum number of threads that
  // the current pool can serve.
  inline auto GetSize() const noexcept -> size_t {
    return clusterSize;
  }

#ifdef ZEUS_LIBRARY_DEBUG
  inline static constexpr auto ZeusUnitMonitor()
      -> pair<atomic<uint64_t>&, atomic<uint64_t>&> {
    return {Volumn::NumOfVolumns, Circle::NumOfCirs};
  }
#endif

  // issue:972
  // inline static Buddy_t buddies[clusterSize]{};
 private:
  vector<Atomic<Volumn>> hazards{};
  vector<Atomic<Cache>> buddies{};
  vector<atomic<Cache*>> refs{};
  vector<atomic<thread::id>> ids{};

inline static uint clusterSize{};

//  Atomic<Volumn> hazards[clusterSize]{};
//  Atomic<Cache> buddies[clusterSize]{};
//  atomic<Cache*> refs[clusterSize]{};
//  atomic<thread::id> ids[clusterSize]{};

  inline static thread_local Cache cache{};
};

}  // namespace zeus
