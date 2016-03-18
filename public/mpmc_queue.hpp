#ifndef PUBLIC_MPMC_QUEUE_H
#define PUBLIC_MPMC_QUEUE_H

#include <atomic>
#include <cassert>

template <typename T>
class MPMCQueue {
public:
    /**
     * @brief default construtor
     */
    MPMCQueue() : _size(DEFAULT_RING_BUFFER_SIZE),
        _ring_buffer(static_cast<T*>(std::malloc(sizeof(T) * DEFAULT_RING_BUFFER_SIZE))),
        _read_index(0),
        _write_index(0),
        _read_commit_index(0),
        _write_commit_index(0) {
    }

    /**
     * @brief construtor with specific size
     */
    explicit MPMCQueue(std::size_t size) :
        _size(size + 1),
        _ring_buffer(static_cast<T*>(std::malloc(sizeof(T) * _size))),
        _read_index(0),
        _write_index(0),
        _read_commit_index(0),
        _write_commit_index(0) {
        assert(_size > 1);
    }

    /**
     * @brief copy constructor, which is deleted
     */
    MPMCQueue(const MPMCQueue<T>& rhs) = delete;

    /**
     * @brief move constructor, which is not thread-safe
     */
    MPMCQueue(MPMCQueue<T> && rhs) :
        _size(rhs._size),
        _ring_buffer(rhs._ring_buffer),
        _read_index(rhs._read_index.load(std::memory_order_relaxed)),
        _write_index(rhs._write_index.load(std::memory_order_relaxed)),
        _read_commit_index(rhs._read_commit_index.load(std::memory_order_relaxed)),
        _write_commit_index(rhs._write_commit_index.load(std::memory_order_relaxed)) {
        rhs._size = 0;
        rhs._ring_buffer = nullptr;
        rhs._read_index.store(0, std::memory_order_relaxed);
        rhs._write_index.store(0, std::memory_order_relaxed);
        rhs._read_commit_index.store(0, std::memory_order_relaxed);
        rhs._write_commit_index.store(0, std::memory_order_relaxed);
    }

    /**
     * @brief assign operation, which is deleted
     */
    MPMCQueue<T>& operator=(const MPMCQueue<T>& rhs) = delete;

    /**
     * @brief move assign operation, which is not thread-safe
     */
    MPMCQueue<T>& operator=(MPMCQueue<T> && rhs) {
        if (this == &rhs) {
            return *this;
        }

        ~MPMCQueue();

        _size = rhs._size;
        _ring_buffer = rhs._ring_buffer;
        _read_index = rhs._read_index.load(std::memory_order_relaxed);
        _write_index = rhs._write_index.load(std::memory_order_relaxed);
        _read_commit_index = rhs._read_commit_index.load(std::memory_order_relaxed);
        _write_commit_index = rhs._write_commit_index.load(std::memory_order_relaxed);

        rhs._size = 0;
        rhs._ring_buffer = nullptr;
        rhs._read_index.store(0, std::memory_order_relaxed);
        rhs._write_index.store(0, std::memory_order_relaxed);
        rhs._read_commit_index.store(0, std::memory_order_relaxed);
        rhs._write_commit_index.store(0, std::memory_order_relaxed);

        return *this;
    }

    /**
     * @brief destrutor, which is not thread safe
     */
    ~MPMCQueue() {
        if (!std::is_trivially_destructible<T>::value) {
            auto read = _read_index.load(std::memory_order_acquire);
            auto end = _write_index.load(std::memory_order_acquire);
            while (read != end) {
                _ring_buffer[read].~T();
                if (++read == _size) {
                    read = 0;
                }
            }
        }

        std::free(_ring_buffer);
    }

    /**
     * @brief pop a data from queue, for multiple cosumer
     * @param[out] data: data with type T, move from the poped data if pop success
                         @data's memory should be allocated before calling this method 
     * @return true: pop success
               false: queue is empty
     */
    bool pop(T* data) {
        if (nullptr == data) {
            return false;
        }

        auto cur_read = _read_index.load(std::memory_order_relaxed);
        auto next_read = cur_read;

        do {
            if (cur_read ==
                    _write_commit_index.load(std::memory_order_acquire)) {
                //queue is empty
                return false;
            }

            next_read = cur_read + 1;
            if (next_read == _size) {
                next_read = 0;
            }
        } while (!_read_index.compare_exchange_weak(
                 cur_read, next_read, std::memory_order_relaxed));

        *data = std::move(_ring_buffer[cur_read]);
        _ring_buffer[cur_read].~T();

        auto cur_commit = cur_read;
        auto next_commit = cur_read;

        do {
            cur_commit = cur_read;
            next_commit = cur_read + 1;
            if (next_commit == _size) {
                next_commit = 0;
            }
        } while (!_read_commit_index.compare_exchange_weak(
                 cur_commit, next_commit, std::memory_order_acq_rel));

        return true;
    }

    /**
     * @brief pop a data from queue, for single cosumer
     * @param[out] data: data with type T, move from the poped data if pop success
                         @data's memory should be allocated before calling this method 
     * @return true: pop success
               false: queue is empty
     */
    bool sc_pop(T* data) {
        if (nullptr == data) {
            return false;
        }

        auto cur_read = _read_index.load(std::memory_order_relaxed);

        if (cur_read ==
                _write_commit_index.load(std::memory_order_acquire)) {
            //queue is empty
            return false;
        }

        auto next_read = cur_read + 1;
        if (next_read == _size) {
            next_read = 0;
        }

        _read_index.store(next_read, std::memory_order_relaxed);

        *data = std::move(_ring_buffer[cur_read]);
        _ring_buffer[cur_read].~T();

        _read_commit_index.store(next_read, std::memory_order_release);
        return true;
    }

    /**
     * @brief push the data to queue, for multiple producer
     * @param[in] args: arguments which are same with onesthat constructors of type T to be pushed
     * @return true: push success
               false: queue is full
     */
    template<class ...Args>
    bool push(Args && ... recordArgs) {
        auto cur_write = _write_index.load(std::memory_order_relaxed);
        auto next_write = cur_write;

        // try to get next write index, compare current write index,
        // if it is same with before then update m_WriteIndex to next index.
        do {
            next_write = cur_write + 1;
            if (next_write == _size) {
                next_write = 0;
            }

            if (next_write == _read_commit_index.load(std::memory_order_acquire)) {
                // queue is full
                return false;
            }
        } while (!_write_index.compare_exchange_weak(
                 cur_write, next_write, std::memory_order_relaxed));

        // update current write index's data
        new(&_ring_buffer[cur_write]) T(std::forward<Args>(recordArgs)...);

        auto cur_commit = cur_write;
        auto next_commit = cur_write;
        do {
            cur_commit = cur_write;
            next_commit = cur_write + 1;
            if (next_commit == _size) {
                next_commit = 0;
            }
        } while (!_write_commit_index.compare_exchange_weak(
                 cur_commit, next_commit, std::memory_order_acq_rel));

        return true;
    }

    /**
     * @brief push the data to queue, for single producer
     * @param[in] args: arguments which are same with onesthat constructors of type T to be pushed
     * @return true: push success
               false: queue is full
     */
    template<class ...Args>
    bool sp_push(Args && ... recordArgs) {
        auto cur_write = _write_index.load(std::memory_order_relaxed);
        auto next_write = cur_write + 1;
        if (next_write == _size) {
            next_write = 0;
        }

        if (next_write == _read_commit_index.load(std::memory_order_acquire)) {
            // queue is full
            return false;
        }

        _write_index.store(next_write, std::memory_order_relaxed);

        // update current write index's data
        new(&_ring_buffer[cur_write]) T(std::forward<Args>(recordArgs)...);

        _write_commit_index.store(next_write, std::memory_order_release);

        return true;
    }

    /**
     * @brief check if queue is empty
     * @return true: queue is empty
               false: queue is not empty
     */
    bool empty() const {
        auto cur_read = _read_index.load(std::memory_order_relaxed);
        if (cur_read == _write_commit_index.load(std::memory_order_acquire)) {
            // queue is empty
            return true;
        }

        return false;
    }

    /**
     * @brief check if queue is full
     * @return true: queue is full
               false: queue is not full
     */
    bool full() const {
        auto next_write = _write_index.load(std::memory_order_relaxed) + 1;
        if (next_write == _size) {
            next_write = 0;
        }

        if (next_write == _read_commit_index.load(std::memory_order_acquire)) {
            return true;
        }

        return false;
    }

private:
    static constexpr auto CACHELINE_BYTES = 64;
    // default ring buffer size 1024, plus one split size
    static constexpr auto DEFAULT_RING_BUFFER_SIZE = 1025;
    static constexpr auto PADDING_SIZE =
        CACHELINE_BYTES - sizeof(std::atomic<std::size_t>);
    // total size of ring buffer
    std::size_t _size;
    // ring buffer
    T* _ring_buffer;

    // current available read index, which always changed
    alignas(CACHELINE_BYTES) std::atomic<std::size_t> _read_index;
    // current available write index, which always changed
    alignas(CACHELINE_BYTES) std::atomic<std::size_t> _write_index;
    // current committed read index, which always changed
    alignas(CACHELINE_BYTES) std::atomic<std::size_t> _read_commit_index;
    // current committed write index, which always changed
    alignas(CACHELINE_BYTES) std::atomic<std::size_t> _write_commit_index;

    // fill out the last cache line at the end of struct
    char _padding[PADDING_SIZE];
};

#endif //PUBLIC_MPMC_QUEUE_H

