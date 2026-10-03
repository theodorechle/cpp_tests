#ifndef TESTS_HPP
#define TESTS_HPP

#include "thread_safe_queue.hpp"
#include <chrono>
#include <functional>
#include <list>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
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
            int readPipe;
            Result result = Result::BAD_RETURN;
        };

        struct TestBlock {
            std::string name;
            TestBlock *parentBlock = nullptr;
            std::list<Test> tests = std::list<Test>();
            std::list<TestBlock> innerBlocks = std::list<TestBlock>();
            bool success = true;
        };

        /* if result != Result::NB_RESULT_TYPES, it removes all checks for tmpChildStatus in the afterTest method
         * it's useful for the "no process" testing function, since it doesn't have an actual "wait status", because it haven't called wait
         * */
        struct TestResult {
            Test *test;
            int childStatus;
            std::chrono::steady_clock::time_point endTime;
            Result result = Result::NB_RESULT_TYPES;
        };

        TestBlock _rootBlock = TestBlock{"", nullptr};
        TestBlock *_currentBlock = &_rootBlock;

        std::chrono::steady_clock::time_point _startedGlobalTestsTimer;
        double _totalTime = .0;

        bool _lastTestWasSuccessful = true;

        ThreadSafeQueue<Test *> _queue = {};

        const uint _maxThreads;

        const bool _debug;

        std::mutex _mutex;

        std::list<std::thread> threads = {};

        ThreadSafeQueue<TestResult> results = ThreadSafeQueue<TestResult>();

        void displayBlocks() const;
        void displayTestWithChrono(const Test &test, int testsNbSize) const;
        void displayGlobalStats();

        void displayBlocksSummary(const TestBlock &blockToDisplay, int tabs = 0);

        void displayTabsAndPipe(int tabs) const;

        void displayNbTestsRunned(bool erasePreviousLine, size_t nbTestsRunned, size_t nbTests);

        void updateStats(Test &test);

        void setTestResultFromReturnStatus(int returnStatus, Test &test, Result result = Result::NB_RESULT_TYPES);

        void displayLogs(Test &test);

        void afterTest(TestResult &result);

        void setBlocksStatus(TestBlock &block);

    public:
        /*
         * maxThreads is the max number of threads which will run tests in parallel. If maxThreads is 0, the number of threads is
         * determined automatically using std::thread::hardware_concurrency. Note that since each thread can only run one process at a time, it also
         * limits the number of parallel processes.
         *
         * If debug is true, tests will run directly on the main process.
         * It should only be used for debugging, since the use of processes allows to be resilient from test crashes.
         */
        Tests(uint maxThreads = 0, bool debug = false);

        void addTest(std::function<Result()> function, const std::string &testName = "");

        void beginTestBlock(const std::string &name);

        void endTestBlock();

    private:
        void parentCode(int _pipe[2], pid_t childPid, Test *test);
        void childCode(int _pipe[2], Test *test);

        void runTestsInThread();
        void runTestsInMainProcess();

        void spawnThreads();
        void joinThreads();
        void processTestsResults();

    public:
        void runTests();

        void displaySummary();

        bool allTestsPassed();
    };

} // namespace test

#endif // TESTS_HPP
