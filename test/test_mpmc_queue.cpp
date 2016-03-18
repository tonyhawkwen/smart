#include "mpmc_queue.hpp"
#include <iostream>
#include <thread>
#include <gtest/gtest.h>

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

class TestMpMcQueueSuite : public ::testing::Test {
protected:
    TestMpMcQueueSuite() {
    }
    virtual void SetUp() {
    }
    virtual void TearDown() {
    }
};

/**
 * @brief 测试单线程下调用sp_push往队列插入数据的正确性
 */
TEST_F(TestMpMcQueueSuite, test_MPMCQueue_sp_push) {
    ic::MPMCQueue<int> mq(3);
    EXPECT_TRUE(mq.empty());
    EXPECT_FALSE(mq.full());
    EXPECT_TRUE(mq.sp_push(1));
    EXPECT_FALSE(mq.empty());
    EXPECT_TRUE(mq.sp_push(2));
    EXPECT_TRUE(mq.sp_push(3));
    EXPECT_TRUE(mq.full());
    EXPECT_FALSE(mq.sp_push(4));
    int pop_val = 0;
    EXPECT_TRUE(mq.sc_pop(&pop_val));
    EXPECT_EQ(pop_val, 1);
    EXPECT_TRUE(mq.sp_push(4));
    EXPECT_TRUE(mq.sc_pop(&pop_val));
    EXPECT_EQ(pop_val, 2);
    EXPECT_TRUE(mq.sc_pop(&pop_val));
    EXPECT_EQ(pop_val, 3);
    EXPECT_TRUE(mq.sc_pop(&pop_val));
    EXPECT_EQ(pop_val, 4);
    EXPECT_TRUE(mq.empty());
}

/**
 * @brief 测试单线程下调用sc_pop从队列提取数据的正确性
 */
TEST_F(TestMpMcQueueSuite, test_MPMCQueue_sc_pop) {
    ic::MPMCQueue<int> mq(3);
    EXPECT_TRUE(mq.empty());
    EXPECT_FALSE(mq.full());
    EXPECT_TRUE(mq.sp_push(1));
    EXPECT_TRUE(mq.sp_push(2));
    EXPECT_TRUE(mq.sp_push(3));
    EXPECT_TRUE(mq.full());
    int pop_val = 0;
    EXPECT_TRUE(mq.sc_pop(&pop_val));
    EXPECT_EQ(pop_val, 1);
    EXPECT_TRUE(mq.sp_push(4));
    EXPECT_TRUE(mq.sc_pop(&pop_val));
    EXPECT_EQ(pop_val, 2);
    EXPECT_TRUE(mq.sc_pop(&pop_val));
    EXPECT_EQ(pop_val, 3);
    EXPECT_TRUE(mq.sc_pop(&pop_val));
    EXPECT_EQ(pop_val, 4);
    EXPECT_TRUE(mq.empty());
}

/**
 * @brief 测试单线程下调用push往队列插入数据的正确性
 */
TEST_F(TestMpMcQueueSuite, test_MPMCQueue_push) {
    ic::MPMCQueue<int> mq(3);
    EXPECT_TRUE(mq.empty());
    EXPECT_FALSE(mq.full());
    EXPECT_TRUE(mq.push(1));
    EXPECT_FALSE(mq.empty());
    EXPECT_TRUE(mq.push(2));
    EXPECT_TRUE(mq.push(3));
    EXPECT_TRUE(mq.full());
    EXPECT_FALSE(mq.push(4));
    int pop_val = 0;
    EXPECT_TRUE(mq.pop(&pop_val));
    EXPECT_EQ(pop_val, 1);
    EXPECT_TRUE(mq.push(4));
    EXPECT_TRUE(mq.pop(&pop_val));
    EXPECT_EQ(pop_val, 2);
    EXPECT_TRUE(mq.pop(&pop_val));
    EXPECT_EQ(pop_val, 3);
    EXPECT_TRUE(mq.pop(&pop_val));
    EXPECT_EQ(pop_val, 4);
    EXPECT_TRUE(mq.empty());
}

/**
 * @brief 测试单线程下调用pop从队列提取数据的正确性
 */
TEST_F(TestMpMcQueueSuite, test_MPMCQueue_pop) {
    ic::MPMCQueue<int> mq(3);
    EXPECT_TRUE(mq.empty());
    EXPECT_FALSE(mq.full());
    EXPECT_TRUE(mq.push(1));
    EXPECT_TRUE(mq.push(2));
    EXPECT_TRUE(mq.push(3));
    EXPECT_TRUE(mq.full());
    int pop_val = 0;
    EXPECT_TRUE(mq.pop(&pop_val));
    EXPECT_EQ(pop_val, 1);
    EXPECT_TRUE(mq.push(4));
    EXPECT_TRUE(mq.pop(&pop_val));
    EXPECT_EQ(pop_val, 2);
    EXPECT_TRUE(mq.pop(&pop_val));
    EXPECT_EQ(pop_val, 3);
    EXPECT_TRUE(mq.pop(&pop_val));
    EXPECT_EQ(pop_val, 4);
    EXPECT_TRUE(mq.empty());
}

/**
 * @brief 测试默认构造函数队列大小的正确性
 */
TEST_F(TestMpMcQueueSuite, test_MPMCQueue_default_construtor) {
    ic::MPMCQueue<int> mq;
    EXPECT_TRUE(mq.empty());
    EXPECT_FALSE(mq.full());
    for (auto i = 0; i < 1024; ++i) {
        EXPECT_TRUE(mq.push(i));
    }
    EXPECT_TRUE(mq.full()); 
    EXPECT_FALSE(mq.empty());
    EXPECT_FALSE(mq.push(1024));
}

/**
 * @brief 测试单生产单消费下调用pop/push从队列提取/插入数据的正确性
 */
TEST_F(TestMpMcQueueSuite, test_MPMCQueue_spsc_threads_0) {
    ic::MPMCQueue<int> mq;

    std::thread t0([&mq]() {
        for (auto i = 0; i < 1024; ++i) {
            int pop_val = 0;
            while (!mq.pop(&pop_val)) {
                ;
            }
            EXPECT_EQ(pop_val, i);
        }
    });

    std::thread t1([&mq]() {
        for (auto i = 0; i < 1024; ++i) {
            EXPECT_TRUE(mq.push(i));
        }
    });

    t0.join();
    t1.join();

    EXPECT_TRUE(mq.empty());
}

/**
 * @brief 测试单生产单消费下调用sc_pop/sp_push从队列提取/插入数据的正确性
 */
TEST_F(TestMpMcQueueSuite, test_MPMCQueue_spsc_threads_1) {
    ic::MPMCQueue<int> mq;

    std::thread t0([&mq]() {
        for (auto i = 0; i < 1024; ++i) {
            int pop_val = 0;
            while (!mq.sc_pop(&pop_val)) {
                ;
            }
            EXPECT_EQ(pop_val, i);
        }
    });

    std::thread t1([&mq]() {
        for (auto i = 0; i < 1024; ++i) {
            EXPECT_TRUE(mq.sp_push(i));
        }
    });

    t0.join();
    t1.join();

    EXPECT_TRUE(mq.empty());
}

/**
 * @brief 测试多生产单消费下调用sc_pop/push从队列提取/插入数据的正确性
 */
TEST_F(TestMpMcQueueSuite, test_MPMCQueue_mpsc_threads) {
    ic::MPMCQueue<int> mq;

    std::set<int> returns;
    std::thread t0([&mq, &returns]() {
        for (auto i = 0; i < 1024; ++i) {
            int pop_val = 0;
            while (!mq.sc_pop(&pop_val)) {
                ;
            }
            returns.insert(pop_val);
        }
    });

    std::thread t1([&mq]() {
        for (auto i = 0; i < 512; ++i) {
            EXPECT_TRUE(mq.push(i));
        }
    });

    std::thread t2([&mq]() {
        for (auto i = 512; i < 1024; ++i) {
            EXPECT_TRUE(mq.push(i));
        }
    });

    t0.join();
    t1.join();
    t2.join();

    EXPECT_TRUE(mq.empty());
    auto i = 0;
    for (auto val : returns) {
        EXPECT_EQ(val, i++);
    }
}

/**
 * @brief 测试单生产多消费下调用pop/sp_push从队列提取/插入数据的正确性
 */
TEST_F(TestMpMcQueueSuite, test_MPMCQueue_spmc_threads) {
    ic::MPMCQueue<int> mq;

    bool returns[1024] = {false, };

    std::thread t0([&mq, &returns]() {
        for (auto i = 0; i < 512; ++i) {
            int pop_val = 0;
            while (!mq.pop(&pop_val)) {
                ;
            }
            returns[pop_val] = true;
        }
    });

    std::thread t1([&mq, &returns]() {
        for (auto i = 0; i < 512; ++i) {
            int pop_val = 0;
            while (!mq.pop(&pop_val)) {
                ;
            }
            returns[pop_val] = true;
        }
    });

    std::thread t2([&mq]() {
        for (auto i = 0; i < 1024; ++i) {
            EXPECT_TRUE(mq.sp_push(i));
        }
    });

    t0.join();
    t1.join();
    t2.join();

    EXPECT_TRUE(mq.empty());
    for (auto val : returns) {
        EXPECT_TRUE(val);
    }
}

/**
 * @brief 测试多生产多消费下调用pop/push从队列提取/插入数据的正确性
 */
TEST_F(TestMpMcQueueSuite, test_MPMCQueue_mpmc_threads) {
    ic::MPMCQueue<int> mq;

    bool returns[1024] = {false, };

    std::thread t0([&mq, &returns]() {
        for (auto i = 0; i < 512; ++i) {
            int pop_val = 0;
            while (!mq.pop(&pop_val)) {
                ;
            }
            returns[pop_val] = true;
        }
    });

    std::thread t1([&mq, &returns]() {
        for (auto i = 0; i < 512; ++i) {
            int pop_val = 0;
            while (!mq.pop(&pop_val)) {
                ;
            }
            returns[pop_val] = true;
        }
    });

    std::thread t2([&mq]() {
        for (auto i = 0; i < 512; ++i) {
            EXPECT_TRUE(mq.push(i));
        }
    });

    std::thread t3([&mq]() {
        for (auto i = 512; i < 1024; ++i) {
            EXPECT_TRUE(mq.push(i));
        }
    });

    t0.join();
    t1.join();
    t2.join();
    t3.join();

    EXPECT_TRUE(mq.empty());
    for (auto val : returns) {
        EXPECT_TRUE(val);
    }
}

