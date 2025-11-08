#include "tests.hpp"

namespace test {
    Result booleanToResult(bool value) { return value ? Result::SUCCESS : Result::FAILURE; }

    std::string Tests::resultToStr(Result result) const {
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

    std::string Tests::resultToStrColored(Result result) const {
#ifdef BASH_COLORS
        switch (result) {
        case Result::SUCCESS:
            return TEST_RESULT_GREEN + "SUCCESS" + TEST_RESULT_END;
        case Result::FAILURE:
            return TEST_RESULT_RED + "FAILURE" + TEST_RESULT_END;
        case Result::ERROR:
            return TEST_RESULT_RED + "ERROR" + TEST_RESULT_END;
        case Result::BAD_RETURN:
            return TEST_RESULT_RED + "BAD_RETURN" + TEST_RESULT_END;
        default:
            return TEST_RESULT_RED + "UNKNOWN" + TEST_RESULT_END;
        }
#else
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
#endif
    }

    void Tests::displayBlocks() const {
        TestBlock *block = currentBlock;
        int tabs = 0;
        while (block != rootBlock) {
            displayTabs(tabs);
            std::cout << "in block '" << block->name << "'\n";
            block = block->parentBlock;
        }
    }

    void Tests::displayTestWithChrono(const Test &test, int testsNbSize) const {
        std::string result = resultToStrColored(test.result);
        std::string testNumberString = std::to_string(test.number);
        std::cout << "Test n°" << test.number << std::string(testsNbSize - testNumberString.size(), ' ') << ": " << result;
        std::cout
            << std::string(NB_SPACES_BEFORE_CHRONO - resultToStr(test.result).size(), ' ')
            << " "
            << std::fixed
            << std::setprecision(CHRONO_FLOAT_SIZE)
            << test.time
            << "s";
        std::cout << " (" << test.name << ") " << "\n";
    }

    void Tests::displayGlobalStats() const {
        std::cout << "Global stats:\n";
        std::cout << stats.nbTests << " tests in " << std::fixed << std::setprecision(CHRONO_FLOAT_SIZE) << totalTime << "s\n";
        std::cout << "Successes: " << stats.nbSuccesses << "\n";
        std::cout << "Failures: " << stats.nbFailures << "\n";
        std::cout << "Errors: " << stats.nbErrors << "\n";
        std::cout << "Bad returns: " << stats.nbBadReturns << "\n";
    }

    void Tests::displayBlocksSummary(const TestBlock *blockToDisplay, int tabs) const {
        std::cout << blockToDisplay->name << ": " << std::fixed << std::setprecision(CHRONO_FLOAT_SIZE) << blockToDisplay->time << "s\n";

        int testsNbSize = std::to_string(blockToDisplay->tests.size()).size(); // for a nice formatting
        for (const Test &testToDisplay : blockToDisplay->tests) {
            displayTabsAndPipe(tabs);
            displayTestWithChrono(testToDisplay, testsNbSize);
        }
        for (const TestBlock &innerBlock : blockToDisplay->innerBlocks) {
            displayTabsAndPipe(tabs);
            displayBlocksSummary(&innerBlock, tabs + 1);
        }

        displayTabsAndPipe(tabs - 1);
        std::cout << "(" << blockToDisplay->name << ")\n";
    }

    void Tests::displayTabs(int tabs) const {
        for (int tab = 0; tab < tabs; tab++)
            std::cout << '\t';
    }

    void Tests::displayTabsAndPipe(int tabs) const {
        for (int tab = 0; tab < tabs; tab++)
            std::cout << '\t';
        if (tabs >= 0) std::cout << "| ";
    }

    void Tests::startTest(Test &test) { startedSingleTestTimer = std::chrono::steady_clock::now(); }

    void Tests::endTest(Test &test) {
        test.time = std::chrono::duration<double>(std::chrono::steady_clock::now() - startedSingleTestTimer).count();
        stats.nbTests++;
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

    Result Tests::parentCode(Test &test) {
        int tmpChildStatus;
        int childStatus;
        char buffer[PIPE_BUFFER_SIZE];
        Result result = Result::NB_RESULT_TYPES;

        pid_t error = waitpid(test.pid, &tmpChildStatus, 0);
        endTest(test);

        if (error == -1) {
            perror((std::string("Error while waiting child '") + std::to_string(static_cast<int>(test.pid)) + "'").c_str());
            exit(errno);
        }
        if (WIFEXITED(tmpChildStatus)) {
            childStatus = WEXITSTATUS(tmpChildStatus);
            if (childStatus < 0 || childStatus >= static_cast<int>(Result::BAD_RETURN)) {
                std::cerr << "Child '" << test.pid << "'(" << test.name << ") exited with code '" << childStatus << "'\n";
                result = Result::BAD_RETURN;
            }
        }
        else if (WIFSIGNALED(tmpChildStatus)) {
            int signal = WTERMSIG(tmpChildStatus);
            std::cerr << "Child '" << test.pid << "'(" << test.name << ") terminated by signal '" << strsignal(signal) << "'\n";
            result = Result::BAD_RETURN;
        }
        else std::cerr << "Test returned an invalid result.\n";
        if (WCOREDUMP((tmpChildStatus))) {
            std::cerr << "Child '" << test.pid << "'(" << test.name << ") produced a core dump\n";
            result = Result::BAD_RETURN;
        }
        if (result != Result::BAD_RETURN) result = static_cast<Result>(childStatus);
        if (result != Result::SUCCESS) {
            displayBlocks();
            std::cout << "Test n°" << test.number << " (" << test.name << "): " << resultToStrColored(test.result) << "\n";
            std::cout << "LOGS:\n";
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
        return result;
    }

    Tests::~Tests() { delete rootBlock; }

    void Tests::addTest(std::function<Result()> function, const std::string &testName) {
        currentBlock->tests.push_back(Test{function, testName, 0, 0, currentBlock->tests.size(), Result::FAILURE, .0});
    }

    void Tests::beginTestBlock(const std::string &name) {
        currentBlock->innerBlocks.push_back(TestBlock(name, currentBlock));
        currentBlock = &currentBlock->innerBlocks.back();
    }

    void Tests::endTestBlock() {
        if (currentBlock == rootBlock) throw TestError("There is no block to close.");
        currentBlock = currentBlock->parentBlock;
    }

    void Tests::runTestBlock(std::list<Test>::iterator tests, std::list<Test>::iterator end) {
        if (tests == end) return;
        int _pipe[2];

        if (pipe(_pipe) == -1) {
            perror("Can't create pipes");
            exit(errno);
        }
        startTest(*tests);
        pid_t childPid = fork();
        switch (childPid) {
        case -1:
            perror("Can't fork test\n");
            exit(errno);
        case 0:
            close(_pipe[0]);
            dup2(_pipe[1], STDOUT_FILENO);
            dup2(_pipe[1], STDERR_FILENO);
            exit(static_cast<int>(tests->function()));
        default:
            close(_pipe[1]);
            tests->pid = childPid;
            tests->pipe = _pipe[0];
            // runTestBlock(std::next(tests), end);
            // tests->result = parentCode(*tests);
            break;
        }
    }

    void Tests::run(TestBlock *block) {
        runTestBlock(block->tests.begin(), block->tests.end());
        for (TestBlock &innerBlock : block->innerBlocks) {
            std::chrono::steady_clock::time_point startedTimer = std::chrono::steady_clock::now();
            run(&innerBlock);
            innerBlock.time = std::chrono::duration<double>(std::chrono::steady_clock::now() - startedTimer).count();
        }
    }

    void Tests::runTests() {
        startedGlobalTestsTimer = std::chrono::steady_clock::now();
        run(rootBlock);
        totalTime = std::chrono::duration<double>(std::chrono::steady_clock::now() - startedGlobalTestsTimer).count();
    }

    void Tests::displaySummary() {
        std::cout << "Summary:\n";
        for (const TestBlock &innerBlock : rootBlock->innerBlocks) {
            displayBlocksSummary(&innerBlock, 0);
        }
        displayGlobalStats();
    }

    bool Tests::allTestsPassed() { return stats.nbSuccesses == stats.nbTests; }

} // namespace test
