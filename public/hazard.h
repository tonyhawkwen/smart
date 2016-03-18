#ifndef PUBLIC_HAZARD_H
#define PUBLIC_HAZARD_H

#include <list>
#include <thread>
#include <atomic>

template<typename T>
struct HazardPointer;

template<typename T>
using HP = HazardPointer<T>;

template<typename T>
struct HazardPointer {
public:
    std::atomic<T*> hp;
    std::atomic<HP<T>*> next;
    std::atomic<bool> active;
};

template<typename T>
class Hazard {
public:
    using deleter = void(*)(T*);

    Hazard() : _hazards(nullptr), _deleter(nullptr) {
    }

    Hazard(deleter del) : _hazards(nullptr), _deleter(del) {
    }

    ~Hazard() {}

    void set_hazard(T* hp) {
        if (nullptr == _local_hazard) {
            hazard_thread_acquire();
        }

        _local_hazard->hp.store(hp, std::memory_order_release);
    }

    void unset_hazard() {
        set_hazard(nullptr);
    }

    bool is_set(T* hp) {
        HP<T>* hazard = _hazards.load(std::memory_order_acquire);
        while (hazard) {
            if (hazard->active.load(std::memory_order_acquire)
                    && hazard->hp.load(std::memory_order_acquire) == hp) {
                return true;
            }

            hazard = hazard->next.load(std::memory_order_acquire);
        }
        return false;
    }

    void hazard_thread_acquire() {
        HP<T>* hazard = _hazards.load(std::memory_order_acquire);
        while (hazard) {
            if (hazard->active.load(std::memory_order_acquire)) {
                hazard = hazard->next.load(std::memory_order_acquire);
                continue;
            }

            bool expect = false;
            if (hazard->active.compare_exchange_strong(expect, true,
                    std::memory_order_acq_rel)) {
                hazard = hazard->next.load(std::memory_order_acquire);
                continue;
            }

            _local_hazard = hazard;
            return;
        }

        HP<T>* pre;
        hazard = new HP<T>;
        hazard->hp.store(nullptr, std::memory_order_release);
        hazard->active.store(true, std::memory_order_release);
        do {
            pre = _hazards.load(std::memory_order_acquire);
            hazard->next.store(pre, std::memory_order_release);
        } while (!_hazards.compare_exchange_weak(pre, hazard,
                 std::memory_order_acq_rel));

        _local_hazard = hazard;
    }

    void hazard_thread_release() {
        _local_hazard->hp.store(nullptr, std::memory_order_release);
        _local_hazard->active.store(false, std::memory_order_release);
    }

    void reclaim(T* hp) {
        if (hp != nullptr) {
            _expired_list.push_back(hp);
        }
        try_reclaim();
    }

    void try_reclaim() {
        typename std::list<T*>::iterator itr;
        for (itr = _expired_list.begin(); itr != _expired_list.end();) {
            if (is_set(*itr)) {
                ++itr;
            } else {
                if (_deleter) {
                    _deleter(*itr);
                } else {
                    delete *itr;
                }

                itr = _expired_list.erase(itr);
            }
        }
    }

    void reclaim_all() {
        while (!_expired_list.empty()) {
            try_reclaim();
        }
    }

private:
    static thread_local HP<T>* _local_hazard;
    std::atomic<HP<T>*> _hazards;
    std::list<T*> _expired_list;
    deleter _deleter;
};

template<typename T>
thread_local HP<T>* Hazard<T>::_local_hazard = nullptr;

#endif /*PUBLIC_HAZARD_H*/

