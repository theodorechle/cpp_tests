#include "tests.hpp"
#include <chrono>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sys/wait.h>
#include <thread>

namespace test {
    Result booleanToResult(bool value) { return value ? Result::SUCCESS : Result::FAILURE; }

    std::string resultToStr(Result result) {
        switch (result) {
        case Result::SUCCESS:
            return "SUCCESS";
        case Result::FAILURE:
            return "FAILURE";
        case Result::ERROR:
            return "ERROR";
        case Result::BAD_RETURN:
            return "BAD_RETURN";
        default:
            return "UNKNOWN";
        }
    }

    void Tests::displayBlocks() const {
        TestBlock *block = _currentBlock;
        while (block != &_rootBlock) {
            std::cout << "in block '" << block->name << "'\n";
            block = block->parentBlock;
        }
    }

    void Tests::displayTestWithChrono(const Test &test, int testsNbSize) const {
        std::string result = resultToStr(test.result);
        std::string testNumberString = std::to_string(test.number);
        std::cout << "Test n°" << test.number << std::string(testsNbSize - testNumberString.size(), ' ') << ": ";
        if (test.result == Result::SUCCESS) std::cout << TEST_RESULT_COLOR_SUCCESS;
        else std::cout << TEST_RESULT_COLOR_FAILURE;
        std::cout
            << result
            << TEST_RESULT_COLOR_END
            << std::string(NB_SPACES_BEFORE_CHRONO - result.size(), ' ')
            << std::fixed
            << std::setprecision(CHRONO_FLOAT_SIZE)
            << test.time
            << "s";
        std::cout << " (" << test.name << ") " << "\n";
    }

    void Tests::displayGlobalStats() {
        std::cout << "Global stats:\n";

        std::cout << "Tests infos:\n";
        std::cout << "\t" << stats.nbTestsRunned << " tests in " << std::fixed << std::setprecision(CHRONO_FLOAT_SIZE) << _totalTime << "s\n";
        std::cout << "\tthreads: ";
        if (_maxThreads == 0) std::cout << "main thread only\n";
        else std::cout << _maxThreads << "\n";
        std::cout << "\tprocess separated tests: " << (_noProcesses ? "no" : "yes") << "\n";

        std::cout << "Tests results:\n";
        std::cout << TEST_RESULT_COLOR_SUCCESS << "\tSuccesses: " << stats.nbSuccesses << TEST_RESULT_COLOR_END << "\n";
        std::cout << TEST_RESULT_COLOR_FAILURE << "\tFailures: " << stats.nbFailures << TEST_RESULT_COLOR_END << "\n";
        std::cout << TEST_RESULT_COLOR_FAILURE << "\tErrors: " << stats.nbErrors << TEST_RESULT_COLOR_END << "\n";
        std::cout << TEST_RESULT_COLOR_FAILURE << "\tBad returns: " << stats.nbBadReturns << TEST_RESULT_COLOR_END << "\n";
    }

    void Tests::displayBlocksSummary(const TestBlock &block, int tabs) {

        std::cout << "group '" << block.name << "': ";

        std::string resultString = resultToStr(block.success ? Result::SUCCESS : Result::FAILURE);

        if (block.success) std::cout << TEST_RESULT_COLOR_SUCCESS;
        else std::cout << TEST_RESULT_COLOR_FAILURE;
        std::cout << resultString << TEST_RESULT_COLOR_END;

        std::cout << std::string(NB_SPACES_BEFORE_CHRONO - resultString.size(), ' ') << "\n";

        int testsNbSize = std::to_string(stats.nbTests).size(); // for a nice formatting
        for (const Test &testToDisplay : block.tests) {
            displayTabsAndPipe(tabs);
            displayTestWithChrono(testToDisplay, testsNbSize);
        }
        for (const TestBlock &innerBlock : block.innerBlocks) {
            displayTabsAndPipe(tabs);
            displayBlocksSummary(innerBlock, tabs + 1);
        }
    }

    void Tests::displayTabsAndPipe(int tabs) const {
        for (int tab = 0; tab < tabs; tab++)
            std::cout << '\t';
        if (tabs >= 0) std::cout << "| ";
    }

    void Tests::displayNbTestsRunned(bool erasePreviousLine, size_t nbTestsRunned, size_t nbTests) {
        if (erasePreviousLine) std::cout << "\033[A";
        std::cout << LOADING_CHARS[nbTestsRunned % NB_LOADING_CHARS] << " tests runned: " << nbTestsRunned << "/" << nbTests;
        if (stats.nbTests > 0) {
            std::cout << " (" << nbTestsRunned * 100 / nbTests << "%)";
        }
        std::cout << std::endl;
    }

    void Tests::updateStats(Test &test) {
        stats.nbTestsRunned++;
        switch (test.result) {
        case Result::SUCCESS:
            stats.nbSuccesses++;
            break;
        case Result::FAILURE:
            stats.nbFailures++;
            break;
        case Result::ERROR:
            stats.nbErrors++;
            break;
        case Result::BAD_RETURN:
            stats.nbBadReturns++;
            break;
        default:
            break;
        }
    }

    void Tests::afterTest(Test &test, int tmpChildStatus, std::chrono::steady_clock::time_point endTime, Result result) {
        std::lock_guard<std::mutex> lock(_mutex);
        std::cerr << "status: " << tmpChildStatus << ", result: " << static_cast<int>(result) << "\n";
        int childStatus;
        char buffer[PIPE_BUFFER_SIZE];
        test.time = std::chrono::duration<double>(endTime - test.startTime).count();

        displayNbTestsRunned(_lastTestWasSuccessful, stats.nbTestsRunned + 1, stats.nbTests);

        if (result != Result::NB_RESULT_TYPES) test.result = result;
        else {
            if (WIFEXITED(tmpChildStatus)) {
                childStatus = WEXITSTATUS(tmpChildStatus);
                if (childStatus >= 0 && childStatus < static_cast<int>(Result::NB_RESULT_TYPES)) {
                    test.result = static_cast<Result>(childStatus);
                }
                else {
                    std::cerr << "Child '" << test.pid << "'(" << test.name << ") exited with code '" << childStatus << "'\n";
                }
            }
            else if (WIFSIGNALED(tmpChildStatus)) {
                int signal = WTERMSIG(tmpChildStatus);
                std::cerr << "Child '" << test.pid << "'(" << test.name << ") terminated by signal '" << strsignal(signal) << "'\n";
            }
            else {
                std::cerr << "Test returned an invalid result.\n";
            }
            if (WCOREDUMP((tmpChildStatus))) {
                std::cerr << "Child '" << test.pid << "'(" << test.name << ") produced a core dump\n";
            }
        }

        updateStats(test);

        _lastTestWasSuccessful = test.result == Result::SUCCESS;

        if (!_lastTestWasSuccessful) {
            displayBlocks();
            std::cout << "Test n°" << test.number << " (" << test.name << "): ";
            if (test.result == Result::SUCCESS) std::cout << TEST_RESULT_COLOR_SUCCESS;
            else std::cout << TEST_RESULT_COLOR_FAILURE;
            std::cout << resultToStr(test.result) << TEST_RESULT_COLOR_END << "\n" << "LOGS:\n";
            bool reading = true;
            while (reading) {
                ssize_t readSize = read(test.pipe, buffer, PIPE_BUFFER_SIZE);
                if (readSize == 0) reading = false;
                else if (readSize == -1) {
                    perror("Can't read from child's pipe");
                    exit(errno);
                }
                else {
                    buffer[readSize] = '\0';
                    std::cout << buffer;
                }
            }
            std::cout << "\n";
        }
    }

    Tests::Tests(int maxThreads, bool noProcesses)
        : _maxThreads{maxThreads == -1 ? std::thread::hardware_concurrency() : static_cast<unsigned int>(maxThreads)}, _noProcesses{noProcesses} {
#ifdef DEBUG
        std::clog << "max threads: " << _maxThreads << "\n";
#endif
    }

    void Tests::addTest(std::function<Result()> function, const std::string &testName) {
        stats.nbTests++;
        _currentBlock->tests.push_back(Test{function, testName, stats.nbTests});
        _queue.push(&_currentBlock->tests.back());
    }

    void Tests::beginTestBlock(const std::string &name) {
        _currentBlock->innerBlocks.push_back(TestBlock{name, _currentBlock});
        _currentBlock = &_currentBlock->innerBlocks.back();
    }

    void Tests::endTestBlock() {
        if (_currentBlock == &_rootBlock) throw TestError("There is no block to close.");
        _currentBlock = _currentBlock->parentBlock;
    }

    void Tests::parentCode(int _pipe[2], pid_t childPid, Test *test) {
        close(_pipe[1]);
        test->pid = childPid;
        test->pipe = _pipe[0];
        int tmpChildStatus;
        pid_t pid = waitpid(childPid, &tmpChildStatus, 0);
        std::chrono::steady_clock::time_point endTime = std::chrono::steady_clock::now();
        if (pid == -1) {
            perror("Error while waiting children");
            exit(errno);
        }

        afterTest(*test, tmpChildStatus, endTime);
        close(_pipe[0]);
    }

    void Tests::childCode(int _pipe[2], Test *test) {
        close(_pipe[0]);
        dup2(_pipe[1], STDOUT_FILENO);
        dup2(_pipe[1], STDERR_FILENO);
        int result = static_cast<int>(test->function());
        std::cerr << "result: " << result << "\n";
        close(_pipe[1]);
        exit(result);
    }

    void Tests::runTestsInThread() {
        Test *test;
        while (_queue.tryPop(&test)) {
            int _pipe[2];

            if (pipe(_pipe) == -1) {
                perror("Can't create pipes");
                exit(errno);
            }
            test->startTime = std::chrono::steady_clock::now();
            pid_t childPid = fork();
            switch (childPid) {
            case -1:
                perror("Can't fork test");
                exit(errno);
            case 0:
                childCode(_pipe, test);
            default:
                parentCode(_pipe, childPid, test);
                break;
            }
        }
    }

    void Tests::runTestsInThreadNoProcesses() {
        test::Tests::Test *test;
        while (_queue.tryPop(&test)) {
            int _pipe[2];
            if (pipe(_pipe) == -1) {
                perror("Can't create pipes");
                exit(errno);
            }
            int savedStdout = dup(STDOUT_FILENO);
            int savedStderr = dup(STDERR_FILENO);

            dup2(_pipe[1], STDOUT_FILENO);
            dup2(_pipe[1], STDERR_FILENO);
            test->pid = -1;
            test->pipe = _pipe[0];

            test->startTime = std::chrono::steady_clock::now();
            Result result = test->function();
            dup2(savedStdout, STDOUT_FILENO);
            dup2(savedStderr, STDERR_FILENO);
            close(_pipe[1]);
            afterTest(*test, 0, std::chrono::steady_clock::now(), result);
            close(_pipe[0]);
        }
    }

    void Tests::setBlockStatus(TestBlock &block) {
        bool success = true;
        for (TestBlock &childBlock : block.innerBlocks) {
            setBlockStatus(childBlock);
            success &= childBlock.success;
        }
        for (Test &test : block.tests) {
            success &= test.result == Result::SUCCESS;
        }
        block.success = success;
    }

    void Tests::runTests() {
        _startedGlobalTestsTimer = std::chrono::steady_clock::now();
        displayNbTestsRunned(false, stats.nbTestsRunned, stats.nbTests);

        std::list<std::thread> threads = {};
        for (unsigned int i = 0; i < _maxThreads; i++) {
#ifdef DEBUG
            std::unique_lock<std::mutex> lock(_mutex);
            std::clog << "Starting thread " << i << "\n";
            lock.unlock();
#endif
            threads.push_back(std::thread([this]() { _noProcesses ? runTestsInThreadNoProcesses() : runTestsInThread(); }));
        }

        // main thread also participates, allow easy debugging by setting threads to 0 with noProcesses set on true, only main thread will be running
        _noProcesses ? runTestsInThreadNoProcesses() : runTestsInThread();

        for (std::thread &thread : threads) {
#ifdef DEBUG
            std::unique_lock<std::mutex> lock(_mutex);
            std::clog << "Joining thread\n";
            lock.unlock();
#endif
            thread.join();
        }

        _totalTime = std::chrono::duration<double>(std::chrono::steady_clock::now() - _startedGlobalTestsTimer).count();

        setBlockStatus(_rootBlock);
    }

    void Tests::displaySummary() {
        std::cout << "Summary:\n";
        for (const TestBlock &innerBlock : _rootBlock.innerBlocks) {
            displayBlocksSummary(innerBlock, 0);
        }
        displayGlobalStats();
    }

    bool Tests::allTestsPassed() { return stats.nbSuccesses == stats.nbTests; }

} // namespace test
