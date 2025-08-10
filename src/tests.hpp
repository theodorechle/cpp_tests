#ifndef TESTS_HPP
#define TESTS_HPP

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <sstream>
#include <string.h>
#include <sys/stat.h>
#include <wait.h>

#define DEBUG

namespace test {
    constexpr size_t PIPE_BUFFER_SIZE = 255;

    // bash colors (not standard to all shells)
    const std::string TEST_RESULT_RED = "\e[31m";
    const std::string TEST_RESULT_GREEN = "\e[32m";
    const std::string TEST_RESULT_END = "\e[0m";

    enum class Result { SUCCESS, FAILURE, ERROR, BAD_RETURN, NB_RESULT_TYPES };

    Result booleanToResult(bool value);

    class TestError : public std::exception {
        std::string message;

    public:
        TestError(const std::string &message) : message{message} {};
        const char *what() const noexcept override { return message.c_str(); }
    };

    class Tests {
        struct testStats {
            size_t nbTests;
            size_t nbSuccesses;
            size_t nbFailures;
            size_t nbErrors;
            size_t nbBadReturns;
        } stats = {0, 0, 0, 0, 0};

        const int NB_SPACES_BEFORE_CHRONO = 11;
        const int CHRONO_FLOAT_SIZE = 8;

        typedef struct testResult {
            std::string name;
            size_t number;
            Result result;
            double time;
        } Test;

        typedef struct testBlock {
            std::string name;
            std::list<Test> results = std::list<Test>();
            std::list<testBlock> innerBlocks = std::list<testBlock>();
            double time = .0;
            testBlock *parentBlock;
            std::chrono::steady_clock::time_point startedTimer;

            testBlock(const std::string &name, testBlock *parentBlock = nullptr)
                : name{name}, parentBlock{parentBlock}, startedTimer{std::chrono::steady_clock::now()} {}
        } TestBlock;

        TestBlock *rootBlock = new TestBlock("");
        TestBlock *currentBlock = rootBlock;

        bool testsStarted = false;
        bool testRunning = false;
        std::chrono::steady_clock::time_point startedSingleTestTimer;

        std::chrono::steady_clock::time_point startedGlobalTestsTimer;
        double totalTime = .0;

        std::string resultToStr(Result result) const;

        std::string resultToStrColored(Result result) const;

        void displayBlocks() const;
        void displayTest(const Test &test) const;
        void displayTestWithChrono(const Test &test, int testsNbSize) const;
        void displayGlobalStats() const;

        void displayBlocksSummary(const TestBlock *blockToDisplay, int tabs = 0) const;
        void displayTabs(int tabs) const;

        void displayTabsAndPipe(int tabs) const;

        /**
         * Starts a test.
         * Must be called before each test.
         *
         */
        void startTest(const std::string &name = "");

        /**
         * Ends a test.
         * Must be called after each test.
         */
        void endTest(Result result);

        void parentCode(int _pipe, pid_t childPid, const std::string &testName);

    public:
        ~Tests();
        void start();
        void stop();

        /**
         * If a test is running, raises a TestError.
         */
        void beginTestBlock(const std::string &name);

        /**
         * If a test is running or if there is no block to close, raises a TestError.
         */
        void endTestBlock();

        void runTest(Result (*f)(void), const std::string &testName = "");
        void displaySummary();

        bool allTestsPassed();
    };

} // namespace test

#endif // TESTS_HPP