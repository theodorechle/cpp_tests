#ifndef TESTS_HPP
#define TESTS_HPP

#include "thread_safe_queue.hpp"
#include <chrono>
#include <functional>
#include <list>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <wait.h>

namespace test {
    constexpr size_t PIPE_BUFFER_SIZE = 255;

    constexpr int NB_LOADING_CHARS = 8;
    const std::string LOADING_CHARS[NB_LOADING_CHARS] = {"⣷", "⣯", "⣟", "⡿", "⢿", "⣻", "⣽", "⣾"};

    enum class Result {
        SUCCESS,
        FAILURE,
        ERROR,
        BAD_RETURN,
        NB_RESULT_TYPES // used to know the size of the enum
    };

    std::string resultToStr(Result result);

    Result booleanToResult(bool value);

    class TestError : public std::exception {
        std::string message;

    public:
        TestError(const std::string &message) : message{message} {};
        const char *what() const noexcept override { return message.c_str(); }
    };

    class Tests {
        struct {
            size_t nbTests;
            size_t nbTestsRunned;
            size_t nbSuccesses;
            size_t nbFailures;
            size_t nbErrors;
            size_t nbBadReturns;
        } stats = {0, 0, 0, 0, 0, 0};

        const int NB_SPACES_BEFORE_CHRONO = 11;
        const int CHRONO_FLOAT_SIZE = 8;

        const std::string TEST_RESULT_COLOR_FAILURE = "\e[31m";
        const std::string TEST_RESULT_COLOR_SUCCESS = "\e[32m";
        const std::string TEST_RESULT_COLOR_END = "\e[0m";

        struct Test {
            std::function<Result()> function;
            std::string name;
            size_t number;
            std::chrono::steady_clock::time_point startTime;
            double time;
            pid_t pid;
            int pipe;
            Result result = Result::BAD_RETURN;
        };

        struct TestBlock {
            std::string name;
            TestBlock *parentBlock = nullptr;
            bool parallel = true;
            std::list<Test> tests = std::list<Test>();
            std::list<TestBlock> innerBlocks = std::list<TestBlock>();
            bool success = true;
        };

        TestBlock _rootBlock = TestBlock{"", nullptr, false};
        TestBlock *_currentBlock = &_rootBlock;

        std::chrono::steady_clock::time_point _startedGlobalTestsTimer;
        double _totalTime = .0;

        bool _lastTestWasSuccessful = true;

        ThreadSafeQueue<Test *> _queue = {};

        const unsigned int _maxThreads;

        const bool _noProcesses;

        std::mutex _mutex;

        void displayBlocks() const;
        void displayTestWithChrono(const Test &test, int testsNbSize) const;
        void displayGlobalStats();

        void displayBlocksSummary(const TestBlock &blockToDisplay, int tabs = 0);

        void displayTabsAndPipe(int tabs) const;

        void displayNbTestsRunned(bool erasePreviousLine, size_t nbTestsRunned, size_t nbTests);

        void updateStats(Test &test);

        void afterTest(Test &test, int tmpChildStatus, std::chrono::steady_clock::time_point endTime);

        void run(TestBlock &block);

        void runTestsInThread();

        void runTestsInThreadNoProcesses();

    public:
        /*
         * maxThreads is the max number of threads which will run tests in parallel, minus 1 since the main thread will also run tests. It allows setting maxThreads to 0 and having tests running in main thread only for easier debugging.
         * If maxThreads is -1, the number of threads is determined automatically using std::thread::hardware_concurrency.
         * Note that since each thread can only run one process at a time, it also limits the number of parallel processes.
         *
         * If noProcesses is true, tests will run directly on the main process.
         * It should only be used for debugging, since the use of processes allows to be resilient from test crashes.
         */
        Tests(int maxThreads = -1, bool noProcesses = false);

        void addTest(std::function<Result()> function, const std::string &testName = "");

        void beginTestBlock(const std::string &name, bool runTestsInParallel = true);

        void endTestBlock();

        void runTests();

        void displaySummary();

        bool allTestsPassed();
    };

} // namespace test

#endif // TESTS_HPP
