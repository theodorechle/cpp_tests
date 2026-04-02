#ifndef TESTS_HPP
#define TESTS_HPP

#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <list>
#include <mutex>
#include <sstream>
#include <string.h>
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
        } stats = {0, 0, 0, 0, 0};

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
            double time = .0;
            bool success = true;
        };

        TestBlock _rootBlock = TestBlock{"", nullptr, false};
        TestBlock *_currentBlock = &_rootBlock;

        std::chrono::steady_clock::time_point _startedGlobalTestsTimer;
        double _totalTime = .0;

        bool lastTestWasSuccessful = true;

        void displayBlocks() const;
        void displayTestWithChrono(const Test &test, int testsNbSize) const;
        void displayGlobalStats() const;

        void displayBlocksSummary(const TestBlock &blockToDisplay, int tabs = 0) const;

        void displayTabsAndPipe(int tabs) const;

        void displayNbTestsRunned(bool erasePreviousLine, size_t nbTestsRunned, size_t nbTests);

        void updateStats(Test &test);

        void afterTest(Test &test, int tmpChildStatus, std::chrono::steady_clock::time_point endTime);

        void runTestBlock(TestBlock &block);
        void runTestBlockParallel(TestBlock &block);
        void run(TestBlock &block);

    public:
        void addTest(std::function<Result()> function, const std::string &testName = "");

        void beginTestBlock(const std::string &name, bool runTestsInParallel = true);

        void endTestBlock();

        void runTests();

        void displaySummary();

        bool allTestsPassed();
    };

} // namespace test

#endif // TESTS_HPP
