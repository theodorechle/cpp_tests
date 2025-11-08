#ifndef TESTS_HPP
#define TESTS_HPP

#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
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

#ifdef BASH_COLORS
    // bash colors (not standard to all shells)
    const std::string TEST_RESULT_RED = "\e[31m";
    const std::string TEST_RESULT_GREEN = "\e[32m";
    const std::string TEST_RESULT_END = "\e[0m";
#endif

    enum class Result {
        SUCCESS,
        FAILURE,
        ERROR,
        BAD_RETURN,
        NB_RESULT_TYPES // used to know the size of the enum
    };

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

        typedef struct test {
            std::function<Result()> function;
            std::string name;
            pid_t pid;
            int pipe;
            size_t number;
            Result result;
            double time;
        } Test;

        typedef struct testBlock {
            std::string name;
            std::list<Test> tests = std::list<Test>();
            std::list<testBlock> innerBlocks = std::list<testBlock>();
            double time = .0;
            testBlock *parentBlock;

            testBlock(const std::string &name, testBlock *parentBlock = nullptr) : name{name}, parentBlock{parentBlock} {}
        } TestBlock;

        TestBlock *rootBlock = new TestBlock("");
        TestBlock *currentBlock = rootBlock;

        std::chrono::steady_clock::time_point startedSingleTestTimer;
        std::chrono::steady_clock::time_point startedGlobalTestsTimer;
        double totalTime = .0;

        std::string resultToStr(Result result) const;

        std::string resultToStrColored(Result result) const;

        void displayBlocks() const;
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
        void startTest(Test &test);

        /**
         * Ends a test.
         * Must be called after each test.
         */
        void endTest(Test &test);

        Result parentCode(Test &test);

        void runTestBlock(std::list<Test>::iterator tests, std::list<Test>::iterator end);
        void run(TestBlock *block);

    public:
        ~Tests();
        void addTest(std::function<Result()> function, const std::string &testName = "");

        /**
         * If a test is running, raises a TestError.
         */
        void beginTestBlock(const std::string &name);

        /**
         * If a test is running or if there is no block to close, raises a TestError.
         */
        void endTestBlock();

        void runTests();

        void displaySummary();

        bool allTestsPassed();
    };

} // namespace test

#endif // TESTS_HPP