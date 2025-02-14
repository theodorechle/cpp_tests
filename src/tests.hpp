#ifndef TESTS
#define TESTS

#include <bits/stdc++.h>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <list>
#include <sstream>
#include <string>
#include <sys/stat.h>

namespace test {

    enum class Result {
        OK,
        KO,
        ERROR,
        NOT_GIVEN,
    };

    /**
     * To use this class, you must create a subclass of it and use this subclass.
     * The tests must be written by overriding the 'tests' method;
     * To run the tests, call runTests.
     * Every new test method must call startTest at the beginning of the method and endTest at the end.
     *
     * Notice that if a non-catchable error such as a segfault or a stack overflow is raised, the program will crash.
     *
     * A temporary file used by the class may not be erased.
     * If the program crash, this file could not be erased.
     * This file is used to register all log messages of a test and is read after the test execution to display it if the test fails.
     * This default behavior can be changed.
     *
     * If it's specified to always show log messages, all messages will be displayed when they are written.
     * If it's not, they will be shown only when a test ends.
     * If it's specified to not show log messages, they will not be registered and shown even if it's specified to always show log messages.
     *
     * "check" functions should returns a Result and "test" functions should starts and ends a test.
     */
    class Tests {
        // TODO: add blocks of tests
        // TODO: run tests in other process to ensure not crashing
        // TODO: find a way to run a test in a simpler way (something like python decorators (maybe a macro))
        // TODO: tests summaries are not aligned when more than 10 tests
        // TODO: ask for a tmp file
        std::string testsName;
        const int NB_SPACES_BEFORE_CHRONO = 9;
        const int NB_SPACES_BEFORE_TEST_DESCRIPTION = 8;

        std::list<std::tuple<std::string, Result, float>> results;
        std::string logFileName = "/tmp/logFile";
        std::ofstream logFile = std::ofstream();
        bool testsRunning = false;
        bool showLogMessages = true;
        bool alwaysShowLogMessages = false;
        std::chrono::_V2::system_clock::time_point startAllTestsTime;
        std::chrono::_V2::system_clock::time_point endAllTestsTime;
        std::chrono::_V2::system_clock::time_point startSingleTestTime;
        std::streambuf *oldErrBuffer = nullptr;
        std::streambuf *oldOutBuffer = nullptr;

        bool openFile();
        bool closeFile();

        /**
         * Starts a test session
         */
        bool startTestSession();

        /**
         * Ends a test session
         */
        bool endTestSession();

        size_t getTestNumber() const { return results.size(); }

        std::string resultToStr(Result result) const;
        void displaySummary() const;

        void redirectStandardOutput();
        void redirectErrorOutput();
        void resetStandardOutput();
        void resetErrorOutput();

    protected:
        /**
         * Starts a test.
         * Must be called before each test.
         *
         */
        void startTest(const std::string &testName = "");

        /**
         * Ends a test.
         * Must be called after each test.
         */
        void endTest(Result result = Result::NOT_GIVEN);
        static std::string getFileContent(std::string fileName);
        virtual void tests() = 0;

    public:
        Tests(const std::string &testsName) : testsName{testsName} {}

        virtual ~Tests() {};

        bool runTests();

        /**
         * By default the file name is "logFile" but you can change it by calling this method with a new file name.
         * Note that this file will only be used if tests are not running and if the file does not already exists.
         */
        void setLogFile(const std::string &newFileName);

        /**
         * If false, will never show log messages even if specified to always show log messages.
         */
        void showLogs(bool show);
        bool showLogs() const;

        /**
         * If true, every log message will be written, all messages will be displayed when they are written.
         * If false, they will be shown only when a test ends.
         */
        void alwaysShowLogs(bool alwaysShow);
        bool alwaysShowLogs() const;

        int getNbErrors() const;
    };

} // namespace test

#endif // TESTS