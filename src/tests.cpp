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

    void Tests::displayBlocks() const {
        TestBlock *block = currentBlock;
        while (block != &rootBlock) {
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

    void Tests::displayGlobalStats() const {
        std::cout << "Global stats:\n";
        std::cout << stats.nbTests << " tests in " << std::fixed << std::setprecision(CHRONO_FLOAT_SIZE) << totalTime << "s\n";
        std::cout << "Successes: " << stats.nbSuccesses << "\n";
        std::cout << "Failures: " << stats.nbFailures << "\n";
        std::cout << "Errors: " << stats.nbErrors << "\n";
        std::cout << "Bad returns: " << stats.nbBadReturns << "\n";
    }

    void Tests::displayBlocksSummary(const TestBlock &block, int tabs) const {
        std::cout << "group '" << block.name << "': ";

        std::string resultString = resultToStr(block.success ? Result::SUCCESS : Result::FAILURE);

        if (block.success) std::cout << TEST_RESULT_COLOR_SUCCESS;
        else std::cout << TEST_RESULT_COLOR_FAILURE;
        std::cout << resultString << TEST_RESULT_COLOR_END;

        std::cout
            << std::string(NB_SPACES_BEFORE_CHRONO - resultString.size(), ' ')
            << std::fixed
            << std::setprecision(CHRONO_FLOAT_SIZE)
            << block.time
            << "s\n";

        int testsNbSize = std::to_string(block.tests.size()).size(); // for a nice formatting
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

    void Tests::updateStats(Test &test) {
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

    void Tests::afterTest(Test &test, int tmpChildStatus, std::chrono::steady_clock::time_point endTime) {
        int childStatus;
        char buffer[PIPE_BUFFER_SIZE];
        test.time = std::chrono::duration<double>(endTime - test.startTime).count();

        if (WIFEXITED(tmpChildStatus)) {
            childStatus = WEXITSTATUS(tmpChildStatus);
            if (childStatus >= 0 && childStatus < static_cast<int>(Result::NB_RESULT_TYPES)) {
                test.result = static_cast<Result>(childStatus);
                updateStats(test);
                return;
            }
            else std::cerr << "Child '" << test.pid << "'(" << test.name << ") exited with code '" << childStatus << "'\n";
        }
        else if (WIFSIGNALED(tmpChildStatus)) {
            int signal = WTERMSIG(tmpChildStatus);
            std::cerr << "Child '" << test.pid << "'(" << test.name << ") terminated by signal '" << strsignal(signal) << "'\n";
        }
        else std::cerr << "Test returned an invalid result.\n";
        if (WCOREDUMP((tmpChildStatus))) {
            std::cerr << "Child '" << test.pid << "'(" << test.name << ") produced a core dump\n";
        }
        updateStats(test);
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

    void Tests::addTest(std::function<Result()> function, const std::string &testName) {
        currentBlock->tests.push_back(Test{function, testName, currentBlock->tests.size()});
    }

    void Tests::beginTestBlock(const std::string &name, bool runTestsInParallel) {
        currentBlock->innerBlocks.push_back(TestBlock{name, currentBlock, runTestsInParallel});
        currentBlock = &currentBlock->innerBlocks.back();
    }

    void Tests::endTestBlock() {
        if (currentBlock == &rootBlock) throw TestError("There is no block to close.");
        currentBlock = currentBlock->parentBlock;
    }

    void Tests::runTestBlock(TestBlock &block) {
        for (Test &test : block.tests) {
            int _pipe[2];

            if (pipe(_pipe) == -1) {
                perror("Can't create pipes");
                exit(errno);
            }
            test.startTime = std::chrono::steady_clock::now();
            pid_t childPid = fork();
            switch (childPid) {
            case -1:
                perror("Can't fork test\n");
                exit(errno);
            case 0:
                close(_pipe[0]);
                dup2(_pipe[1], STDOUT_FILENO);
                dup2(_pipe[1], STDERR_FILENO);
                exit(static_cast<int>(test.function()));
            default:
                close(_pipe[1]);
                test.pid = childPid;
                test.pipe = _pipe[0];
                int tmpChildStatus;
                pid_t pid = wait(&tmpChildStatus);
                std::chrono::steady_clock::time_point endTime = std::chrono::steady_clock::now();
                if (pid == -1) {
                    perror("Error while waiting childs ");
                    exit(errno);
                }

                afterTest(test, tmpChildStatus, endTime);
                break;
            }
        }
    }

    void Tests::runTestBlockParallel(TestBlock &block) {
        for (Test &test : block.tests) {
            int _pipe[2];

            if (pipe(_pipe) == -1) {
                perror("Can't create pipes");
                exit(errno);
            }
            test.startTime = std::chrono::steady_clock::now();
            pid_t childPid = fork();
            switch (childPid) {
            case -1:
                perror("Can't fork test\n");
                exit(errno);
            case 0:
                close(_pipe[0]);
                dup2(_pipe[1], STDOUT_FILENO);
                dup2(_pipe[1], STDERR_FILENO);
                exit(static_cast<int>(test.function()));
            default:
                close(_pipe[1]);
                test.pid = childPid;
                test.pipe = _pipe[0];
                break;
            }
        }

        for (size_t i = 0; i < block.tests.size(); i++) {
            int tmpChildStatus;
            pid_t pid = wait(&tmpChildStatus);
            std::chrono::steady_clock::time_point endTime = std::chrono::steady_clock::now();
            if (pid == -1) {
                perror("Error while waiting childs ");
                exit(errno);
            }

            for (Test &test : block.tests) {
                if (test.pid != pid) continue;
                afterTest(test, tmpChildStatus, endTime);
                break;
            }
        }
    }

    void Tests::run(TestBlock &block) {
        size_t nbFailures = stats.nbFailures;
        if (block.parallel) runTestBlockParallel(block);
        else runTestBlock(block);
        if (stats.nbFailures > nbFailures) block.success = false;

        for (TestBlock &innerBlock : block.innerBlocks) {
            std::chrono::steady_clock::time_point startedTimer = std::chrono::steady_clock::now();
            run(innerBlock);
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
        for (const TestBlock &innerBlock : rootBlock.innerBlocks) {
            displayBlocksSummary(innerBlock, 0);
        }
        displayGlobalStats();
    }

    bool Tests::allTestsPassed() { return stats.nbSuccesses == stats.nbTests; }

} // namespace test
