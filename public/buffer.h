#ifndef PUBLIC_BUFFER_H
#define PUBLIC_BUFFER_H

#include <iostream>
#include <assert.h>
#include <sys/uio.h>
#include <string.h>
#include "logging.h"

namespace smart {

class Buffer {
public:
    //conflict with cut(), make sure that buffer is not modified when using iterator
    class const_iterator {
    public:
        explicit const_iterator() : _pointer(nullptr), _index(0), _mask(0){};
        explicit const_iterator(const unsigned char* pointer,
                                size_t index,
                                size_t mask) :
                _pointer(pointer),
                _index(index),
                _mask(mask){
        }

        const_iterator(const const_iterator& rhs) :
                _pointer(rhs._pointer),
                _index(rhs._index),
                _mask(rhs._mask){
        }

        const_iterator& operator= (const const_iterator& rhs)
        {
            if (this == &rhs)
            {
                return *this;
            }

            _pointer = rhs._pointer;
            _index = rhs._index;
            _mask = rhs._mask;

            return *this;
        }

        const_iterator(const_iterator&& rhs) :
                _pointer(rhs._pointer),
                _index(rhs._index),
                _mask(rhs._mask)
        {
            rhs._pointer = nullptr;
            rhs._index = 0;
            rhs._mask = 0;
        }

        const_iterator& operator= (const_iterator&& rhs)
        {
            if (this == &rhs)
            {
                return *this;
            }

            _pointer = rhs._pointer;
            _index = rhs._index;
            _mask = rhs._mask;
            rhs._pointer = nullptr;
            rhs._index = 0;
            rhs._mask = 0;

            return *this;
        }

        ~const_iterator(){
        }

        const_iterator& operator++()
        {
            _index = (_index + 1) & _mask;
            return *this;
        }

        const const_iterator operator++(int)
        {
            const_iterator old(*this);
            _index = (_index + 1) & _mask;

            return old;
        }

        const_iterator& operator--() = delete;
        const const_iterator& operator--(int) = delete;

        const_iterator operator+(const size_t offset)
        {
            return const_iterator(_pointer, (_index + offset) & _mask, _mask);
        }

        const_iterator operator+=(const size_t offset)
        {
            _index = (_index + offset) & _mask;
            return *this;
        }

        bool operator==(const const_iterator& rhs)
        {
            return _index == rhs._index && _pointer == rhs._pointer && _mask == rhs._mask;
        }

        bool operator!=(const const_iterator& rhs)
        {
            return _index != rhs._index || _pointer != rhs._pointer || _mask != rhs._mask;
        }

        unsigned char operator*()
        {
            return _pointer[_index];
        }

    private:
        const unsigned char* _pointer;
        size_t _index;
        size_t _mask;
    };

    static constexpr auto DEFAULT_INITIAL_SIZE = 10240;

    explicit Buffer(size_t size) :
            _read_index(0),
            _write_index(0),
            _capacity(size),
            _mask(size - 1),
            _ring_buffer(static_cast<unsigned char*>(std::malloc(sizeof(unsigned char) * size))),
            _read_buffer(nullptr)
    {
        assert(!(_capacity & _mask));
    }

    explicit Buffer() :
            _read_index(0),
            _write_index(0),
            _capacity(DEFAULT_INITIAL_SIZE),
            _mask(DEFAULT_INITIAL_SIZE - 1),
            _ring_buffer(static_cast<unsigned char*>(std::malloc(sizeof(unsigned char) * _capacity))),
            _read_buffer(nullptr)
    {
    }

    Buffer(const Buffer& rhs):
            _read_index(rhs._read_index),
            _write_index(rhs._write_index),
            _capacity(rhs._capacity),
            _mask(rhs._mask),
            _ring_buffer(static_cast<unsigned char*>(std::malloc(sizeof(unsigned char) * _capacity))),
            _read_buffer(nullptr)
    {
        memmove(_ring_buffer, rhs._ring_buffer, _capacity);
    }

    Buffer& operator= (const Buffer& rhs)
    {
        if (this == &rhs)
        {
            return *this;
        }
        _read_index = rhs._read_index;
        _write_index = rhs._write_index;
        _capacity = rhs._capacity;
        _mask = rhs._mask;
        if (_ring_buffer)
        {
            std::free(_ring_buffer);
        }
        _ring_buffer = static_cast<unsigned char*>(std::malloc(sizeof(unsigned char) * _capacity));
        memmove(_ring_buffer, rhs._ring_buffer, _capacity);
        if (_read_buffer)
        {
            std::free(_read_buffer);
        }

        return *this;
    }

    Buffer(Buffer&& rhs):
            _read_index(rhs._read_index),
            _write_index(rhs._write_index),
            _capacity(rhs._capacity),
            _mask(rhs._mask),
            _ring_buffer(rhs._ring_buffer),
            _read_buffer(rhs._read_buffer)
    {
        rhs._read_index = 0;
        rhs._write_index = 0;
        rhs._capacity = 0;
        rhs._mask = 0;
        rhs._ring_buffer = nullptr;
        rhs._read_buffer = nullptr;
    }

    Buffer& operator= (Buffer&& rhs)
    {
        if (this == &rhs)
        {
            return *this;
        }
        _read_index = rhs._read_index;
        _write_index = rhs._write_index;
        _capacity = rhs._capacity;
        _mask = rhs._mask;
        if (_ring_buffer)
        {
            std::free(_ring_buffer);
        }
        _ring_buffer = rhs._ring_buffer;
        if (_read_buffer)
        {
            std::free(_read_buffer);
        }
        _read_buffer = rhs._read_buffer;

        rhs._read_index = 0;
        rhs._write_index = 0;
        rhs._capacity = 0;
        rhs._mask = 0;
        rhs._ring_buffer = nullptr;
        rhs._read_buffer = nullptr;

        return *this;
    }

    ~Buffer()
    {
        if (_ring_buffer)
        {
            std::free(_ring_buffer);
            _ring_buffer = nullptr;
        }

        if (_read_buffer)
        {
            std::free(_read_buffer);
            _read_buffer = nullptr;
        }
    }

    void swap(Buffer& rhs)
    {
        if (&rhs == this)
        {
            return;
        }

        std::swap(_ring_buffer, rhs._ring_buffer);
        std::swap(_read_index, rhs._read_index);
        std::swap(_write_index, rhs._write_index);
        std::swap(_capacity, rhs._capacity);
        std::swap(_mask, rhs._mask);
    }

    size_t readable_bytes() const
    {
        return (_capacity + _write_index - _read_index) & _mask;
    }

    size_t size() const
    {
        return readable_bytes();
    }

    size_t capacity() const
    {
        return _mask;
    }

    void clear()
    {
        _read_index = _write_index = 0;
    }

    const_iterator begin() const
    {
        return const_iterator(_ring_buffer, _read_index, _mask);
    }

    const_iterator end() const
    {
        return const_iterator(_ring_buffer, _write_index, _mask);
    }

    std::pair<const unsigned char*, size_t> str() const
    {
        std::pair<const unsigned char*, size_t> ret;
        if (_read_index <= _write_index)
        {
            ret.first = _ring_buffer + _read_index;
            ret.second = _write_index - _read_index;
        }
        /*else if (_read_index == _write_index)//merge with upper case
        {
            ret.first = _ring_buffer;
            ret.second = 0;
        }
        else if (_read_index == _capacity) //impossible case
        {
            ret.first = _ring_buffer;
            ret.second = _write_index;
        }*/
        else
        {
            /// +-------------------+------------------+------------------+
            /// |   readable bytes  |  writable bytes  |  readable bytes  |
            /// |     (CONTENT)     |                  |   (CONTENT)      |
            /// +-------------------+------------------+------------------+
            /// |                   |                  |                  |
            /// 0      <=      writerIndex   <=   readerIndex    <=     size
            if (!_read_buffer)
            {
                _read_buffer = static_cast<unsigned char*>(std::malloc(sizeof(unsigned char) * _capacity));
            }

            memmove(_read_buffer, _ring_buffer + _read_index, _capacity - _read_index);
            memmove(_read_buffer + _capacity - _read_index, _ring_buffer, _write_index);
            ret.first = _read_buffer;
            ret.second = readable_bytes();
        }

        return ret;
    }

    size_t writable_bytes() const
    {
        return (_mask + _read_index - _write_index) & _mask;
    }

    size_t append(const std::string& other)
    {
        return append((const unsigned char *)other.c_str(), other.length());
    }

    size_t append(const Buffer& other)
    {
        //TO DO:not efficient when in _read_buffer case
        std::pair<const unsigned char*, size_t> ref = other.str();
        return append(ref.first, ref.second);
    }

    size_t append(const unsigned char* data, size_t size)
    {
        size_t nw = writable_bytes();
        if (nw > size)
        {
            nw = size;
        }

        if (nw == 0)
        {
            return nw;
        }

        if (_read_index < _write_index)
        {
            /// +-------------------+------------------+------------------+
            /// |   writable bytes  |  readable bytes  |  writable bytes  |
            /// |                   |     (CONTENT)    |                  |
            /// +-------------------+------------------+------------------+
            /// |                   |                  |                  |
            /// 0      <=      _read_index   <=   _write_index    <=     size
            if (_capacity - _write_index >= nw)
            {
                memmove(_ring_buffer + _write_index/*writeBegin1()*/, data, nw);
            }
            else
            {
                size_t writen = _capacity - _write_index;
                memmove(_ring_buffer + _write_index/*writeBegin1()*/, data, writen);
                memmove(_ring_buffer, data + writen, nw - writen);
            }
        }
        else
        {
            /// +-------------------+------------------+------------------+
            /// |   readable bytes  |  writable bytes  |  readable bytes  |
            /// |     (CONTENT)     |                  |   (CONTENT)      |
            /// +-------------------+------------------+------------------+
            /// |                   |                  |                  |
            /// 0      <=      _write_index   <=   _read_index     <=     size
            memmove(_ring_buffer + _write_index, data, nw);
        }

        _write_index = (_write_index + nw) & _mask;
        return nw;
    }

    size_t cut(size_t size)
    {
        size_t nr = readable_bytes();
        if (size < readable_bytes())
        {
            nr = size;
        }
        _read_index = (_read_index + nr) & _mask;

        return nr;
    }

    ssize_t append_from_fd(int fd, int* error)
    {
        iovec vec[2];
        int nvec = 0;
        size_t nw = writable_bytes();
        if (nw == 0)
        {
            return -2;
        }

        if (_read_index < _write_index)
        {
            /// +-------------------+------------------+------------------+
            /// |   writable bytes  |  readable bytes  |  writable bytes  |
            /// |                   |     (CONTENT)    |                  |
            /// +-------------------+------------------+------------------+
            /// |                   |                  |                  |
            /// 0      <=      _read_index   <=   _write_index    <=     size
            if (_capacity - _write_index >= nw)
            {
                vec[0].iov_base = _ring_buffer + _write_index;
                vec[0].iov_len = nw;
                nvec = 1;
            }
            else
            {
                size_t writen = _capacity - _write_index;
                vec[0].iov_base = _ring_buffer + _write_index;
                vec[0].iov_len = writen;
                vec[1].iov_base = _ring_buffer;
                vec[1].iov_len = nw - writen;
                nvec = 2;
            }
        }
        else
        {
            /// +-------------------+------------------+------------------+
            /// |   readable bytes  |  writable bytes  |  readable bytes  |
            /// |     (CONTENT)     |                  |   (CONTENT)      |
            /// +-------------------+------------------+------------------+
            /// |                   |                  |                  |
            /// 0      <=      _write_index   <=   _read_index     <=     size
            vec[0].iov_base = _ring_buffer + _write_index;
            vec[0].iov_len = nw;
            nvec = 1;
        }

        auto ret = readv(fd, vec, nvec);
        if (ret > 0)
        {
            _write_index = (_write_index + ret) & _mask;
        } else {
            *error = errno;
        }

        return ret;
    }

    ssize_t cut_into_fd(int fd, size_t size)
    {
        iovec vec[2];
        int nvec = 0;
        size_t nr = readable_bytes();
        if (nr > size)
        {
            nr = size;
        }

        if (_read_index < _write_index)
        {
            /// +-------------------+------------------+------------------+
            /// |   writable bytes  |  readable bytes  |  writable bytes  |
            /// |                   |     (CONTENT)    |                  |
            /// +-------------------+------------------+------------------+
            /// |                   |                  |                  |
            /// 0      <=      _read_index   <=   _write_index    <=     size
            vec[0].iov_base = _ring_buffer + _read_index;
            vec[0].iov_len = nr;
            nvec = 1;
        }
        else
        {
            /// +-------------------+------------------+------------------+
            /// |   readable bytes  |  writable bytes  |  readable bytes  |
            /// |     (CONTENT)     |                  |   (CONTENT)      |
            /// +-------------------+------------------+------------------+
            /// |                   |                  |                  |
            /// 0      <=      _write_index   <=   _read_index     <=     size
            if (_capacity - _read_index > nr)
            {
                vec[0].iov_base = _ring_buffer + _read_index;
                vec[0].iov_len = nr;
                nvec = 1;
            }
            else
            {
                auto read = _capacity - _read_index;
                vec[0].iov_base = _ring_buffer + _read_index;
                vec[0].iov_len = read;
                vec[1].iov_base = _ring_buffer;
                vec[1].iov_len = nr - read;
                nvec = 2;
            }
        }

        auto ret = writev(fd, vec, nvec);
        if (ret > 0)
        {
            _read_index = (_read_index + nr) & _mask;
        }

        return ret;
    }

    friend inline std::ostream& operator<< (std::ostream& os, const Buffer& buf)
    {
        for (auto itr = buf.begin(); itr != buf.end(); ++itr)
        {
            os << *itr;
        }
        return os;
    }

    //for gtest
    size_t read_index() const{return  _read_index;}
    size_t write_index() const{return _write_index;}

private:
    size_t _read_index;
    size_t _write_index;
    size_t _capacity;
    size_t _mask;
    unsigned char* _ring_buffer;
    mutable unsigned char* _read_buffer;
};

}

#endif /*PUBLIC_BUFFER_H*/

