#include "tests.hpp"

namespace test {
    void Tests::beginTestBlock(const std::string &name) {
        if (!testsStarted) throw TestError("Can't open a block while tests are not started.");
        if (testRunning) throw TestError("Can't open a block while a test is running.");
        currentBlock->innerBlocks.push_back(TestBlock(name, currentBlock));
        currentBlock = &currentBlock->innerBlocks.back();
    }

    void Tests::endTestBlock() {
        if (!testsStarted) throw TestError("Can't end a block while tests are not started.");
        if (testRunning) throw TestError("Can't end a block while a test is running.");
        if (currentBlock == rootBlock) throw TestError("There is no block to close.");
        currentBlock->time = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - currentBlock->startedTimer).count();
        currentBlock = currentBlock->parentBlock;
    }

    void Tests::startTest(const std::string &name) {
        if (currentBlock == rootBlock) throw TestError("Can't start a test if no block were created.");
        if (!testsStarted) throw TestError("Can't start a test while tests are not started.");
        if (testRunning) throw TestError("Can't start a test while an other one is running.");
        testRunning = true;
        currentBlock->results.push_back(Test{name, currentBlock->results.size(), Result::FAILURE, .0});
        startedSingleTestTimer = std::chrono::high_resolution_clock::now();
    }

    void Tests::endTest(Result result) {
        if (!testsStarted) throw TestError("Can't end a test while tests are not started.");
        if (!testRunning) throw TestError("No test to end.");
        Test &test = currentBlock->results.back();
        test.time = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - startedSingleTestTimer).count();
        test.result = result;
        testRunning = false;
    }

    void Tests::parentCode(int _pipe, pid_t childPid, const std::string &testName) {
        int childStatus;
        char buffer[PIPE_BUFFER_SIZE];
        Result result;

        startTest(testName);
        if (waitpid(childPid, &childStatus, 0) == -1) {
            perror((std::string("Error while waiting child '") + std::to_string((int)childPid) + "'").c_str());
            exit(errno);
        }

        if (childStatus < 0 || childStatus > NB_RESULT_TYPES) throw TestError("Test returned an invalid result.");
        result = static_cast<Result>(childStatus);
        if (result != Result::SUCCESS) {
            displayBlocks();
            displayTest(currentBlock->results.back());
            std::cout << "Logs:\n";
            bool reading = true;
            while (reading) {
                ssize_t readSize = read(_pipe, buffer, PIPE_BUFFER_SIZE);
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
        endTest(result);
    }

    Tests::~Tests() { delete rootBlock; }

    void Tests::start() {
        if (testsStarted) throw TestError("Tests already started.");
        testsStarted = true;
        startedGlobalTestsTimer = std::chrono::high_resolution_clock::now();
    }

    void Tests::stop() {
        if (!testsStarted) throw TestError("Tests already stopped.");
        testsStarted = false;
        totalTime = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - startedGlobalTestsTimer).count();
    }

    void Tests::runTest(Result (*f)(void), const std::string &testName) {
        int _pipe[2];

        if (pipe(_pipe) == -1) {
            perror("Can't create pipes");
            exit(errno);
        }
        pid_t childPid = fork();
        switch (childPid) {
        case -1:
            perror("Can't fork test\n");
            exit(errno);
        case 0:
            close(_pipe[0]);
            dup2(_pipe[1], STDOUT_FILENO);
            dup2(_pipe[1], STDERR_FILENO);
            exit((int)f());
        default:
            close(_pipe[1]);
            parentCode(_pipe[0], childPid, testName);
            break;
        }
    }

    std::string Tests::resultToStr(Result result) const {
        switch (result) {
        case Result::SUCCESS:
            return "SUCCESS";
        case Result::FAILURE:
            return "FAILURE";
        case Result::ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
        }
    }

    std::string Tests::resultToStrColored(Result result) const {
        switch (result) {
        case Result::SUCCESS:
            return TEST_RESULT_GREEN + "SUCCESS" + TEST_RESULT_END;
        case Result::FAILURE:
            return TEST_RESULT_RED + "FAILURE" + TEST_RESULT_END;
        case Result::ERROR:
            return TEST_RESULT_RED + "ERROR" + TEST_RESULT_END;
        default:
            return TEST_RESULT_RED + "UNKNOWN" + TEST_RESULT_END;
        }
    }

    void Tests::displayBlocks() const {
        TestBlock *block = currentBlock;
        int tabs = 1;
        if (block != rootBlock) {
            std::cout << "'" << block->name << "'\n";
            block = block->parentBlock;
        }

        while (block != rootBlock) {
            displayTabs(tabs);
            std::cout << "in '" << block->name << "'\n";
            block = block->parentBlock;
        }
    }

    void Tests::displayTest(const Test &test) const {
        std::cout << "Test n°" << test.number << "(" << test.name << "): " << resultToStrColored(test.result) << "\n";
    }

    void Tests::displayTestWithChrono(const Test &test, int testsNbSize) const {
        std::string result = resultToStrColored(test.result);

        std::cout << "Test n°" << test.number << ": " << result;
        std::cout << std::string(NB_SPACES_BEFORE_CHRONO - testsNbSize + result.size() - result.size(), ' ') << " " << std::fixed
                  << std::setprecision(CHRONO_FLOAT_SIZE) << test.time << "s";
        std::cout << " (" << test.name << ") " << "\n";
    }

    void Tests::displayGlobalStats() const {
        Stats stats = Stats{0, 0, 0, 0};
        getStats(&stats, rootBlock);
        std::cout << "Global stats:\n";
        std::cout << stats.nbTests << " tests in " << std::fixed << std::setprecision(CHRONO_FLOAT_SIZE) << totalTime << "s\n";
        std::cout << "Successes: " << stats.nbSuccesses << "\n";
        std::cout << "Failures: " << stats.nbFailures << "\n";
        std::cout << "Errors: " << stats.nbErrors << "\n";
    }

    void Tests::displayBlocksSummary(const TestBlock *blockToDisplay, int tabs) const {
        size_t index = 0;

        displayTabs(tabs - 1);

        std::cout << blockToDisplay->name << ": " << std::fixed << std::setprecision(CHRONO_FLOAT_SIZE) << blockToDisplay->time << "s\n";

        int testsNbSize = std::to_string(blockToDisplay->results.size()).size(); // for a nice formatting
        for (const Test &testToDisplay : blockToDisplay->results) {
            displayTabs(tabs);
            std::cout << "| ";
            displayTestWithChrono(testToDisplay, testsNbSize);
        }
        for (const TestBlock &innerBlock : blockToDisplay->innerBlocks) {
            displayTabs(tabs);
            std::cout << "| ";
            displayBlocksSummary(&innerBlock, tabs + 1);
        }

        displayTabs(tabs - 1);
        std::cout << "(" << blockToDisplay->name << ")\n";
        index++;
    }

    void Tests::displaySummary() {
        std::cout << "Summary:\n";
        for (const TestBlock &innerBlock : rootBlock->innerBlocks) {
            displayBlocksSummary(&innerBlock, 1);
        }
        displayGlobalStats();
    }

    void Tests::displayTabs(int tabs) const {
        for (int tab = 0; tab < tabs; tab++)
            std::cout << '\t';
    }

    void Tests::getStats(Stats *stats, TestBlock *testBlock) const {
        if (stats == nullptr) return;
        stats->nbTests += testBlock->results.size();
        for (Test &test : testBlock->results) {
            switch (test.result) {
            case Result::SUCCESS:
                stats->nbSuccesses++;
                break;
            case Result::FAILURE:
                stats->nbFailures++;
                break;
            case Result::ERROR:
                stats->nbErrors++;
                break;
            default:
                break;
            }
        }
        for (TestBlock &childBlock : testBlock->innerBlocks) {
            getStats(stats, &childBlock);
        }
    }

} // namespace test
