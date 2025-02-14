#include "tests.hpp"

namespace test {

    bool Tests::openFile() {
        logFile.open(logFileName);
        if (!logFile.is_open()) {
            std::cerr << "Can't create file '" << logFileName << "' used for logging\n";
            return false;
        }
        return true;
    }

    bool Tests::closeFile() {
        if (logFile.is_open()) {
            logFile.close();
        }
        return true;
    }

    bool Tests::startTestSession() {
        testsRunning = true;
        if (!showLogMessages || alwaysShowLogMessages) return true;

        struct stat buffer;
        if (stat(logFileName.c_str(), &buffer) != -1) {
            std::cerr << "File '" << logFileName << "' who should have been used for logging already exists\n";
            return false;
        }

        std::cout << "Started test session \"" << testsName << "\"\n";

        startAllTestsTime = std::chrono::high_resolution_clock::now();

        return true;
    }

    bool Tests::endTestSession() {
        resetErrorOutput();
        resetStandardOutput();
        testsRunning = false;
        endAllTestsTime = std::chrono::high_resolution_clock::now();
        std::cout << "Ended test session \"" << testsName << "\"\n";
        displaySummary();

        // if nothing was written, the file wasn't created
        if (std::filesystem::exists(logFileName) && remove(logFileName.c_str())) {
            std::cerr << "Can't delete file '" << logFileName << "'\n";
            return false;
        }

        return true;
    }

    void Tests::redirectStandardOutput() { oldOutBuffer = std::cout.rdbuf(logFile.rdbuf()); }
    void Tests::redirectErrorOutput() { oldErrBuffer = std::cerr.rdbuf(logFile.rdbuf()); }

    void Tests::resetStandardOutput() { std::cerr.rdbuf(oldErrBuffer); }

    void Tests::resetErrorOutput() { std::cout.rdbuf(oldOutBuffer); }

    void Tests::startTest(const std::string &testName) {
        openFile();
        if (!alwaysShowLogMessages) {
            redirectStandardOutput();
            redirectErrorOutput();
        }
        std::cout << "Test n°" << getTestNumber();
        results.push_back(std::tuple(testName, Result::NOT_GIVEN, 0));
        if (!std::get<0>(results.back()).empty()) {
            std::cout << " (" << std::get<0>(results.back()) << ")";
        }
        std::cout << ":\n";
        startSingleTestTime = std::chrono::high_resolution_clock::now();
    }

    void Tests::endTest(Result result) {
        std::chrono::_V2::system_clock::time_point endTime = std::chrono::high_resolution_clock::now();

        std::get<1>(results.back()) = result;
        std::get<2>(results.back()) = std::chrono::duration<float>(endTime - startSingleTestTime).count();

        std::stringstream buffer;
        if (!alwaysShowLogMessages) {
            resetStandardOutput();
            resetErrorOutput();
        }
        if (logFile.is_open() && std::get<1>(results.back()) != Result::OK) {
            std::ifstream file(logFileName);
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::cerr << buffer.str() << "\n";
        }
        closeFile();
        openFile(); // reopen it to erase the existing content
    }

    bool Tests::runTests() {
        bool testsValid = true;
        if (!startTestSession()) return false;
        try {
            tests();
        }
        catch (const std::exception &exception) {
            endTest(Result::ERROR);
            std::cerr << "Tests stopped after an exception was raised: " << exception.what() << "\n";
            testsValid = false;
        }
        catch (...) {
            endTest(Result::ERROR);
            std::cerr << "Tests stopped after an exception was raised who is not a subclass of std::exception\n";
            testsValid = false;
        }

        if (!endTestSession()) return false;
        return testsValid;
    }

    std::string Tests::getFileContent(std::string fileName) {
        std::ifstream file(fileName);
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    std::string Tests::resultToStr(Result result) const {
        switch (result) {
        case Result::OK:
            return "OK";
        case Result::KO:
            return "KO";
        case Result::ERROR:
            return "ERROR";
        case Result::NOT_GIVEN:
            return "NOT GIVEN";
        default:
            return "UNKNOWN";
        }
    }

    void Tests::setLogFile(const std::string &newFileName) {
        if (!testsRunning) logFileName = newFileName;
    }

    void Tests::showLogs(bool show) {
        if (!testsRunning) showLogMessages = show;
    }

    bool Tests::showLogs() const { return showLogMessages; }

    void Tests::alwaysShowLogs(bool alwaysShow) {
        if (!testsRunning) alwaysShowLogMessages = alwaysShow;
    }

    bool Tests::alwaysShowLogs() const { return alwaysShowLogMessages; }

    void Tests::displaySummary() const {
        std::cout << "Summary:\n";
        size_t index = 0;
        std::string resultStr;
        std::string chrono;
        for (std::list<std::tuple<std::string, Result, float>>::const_iterator it = results.cbegin(); it != results.cend(); it++) {
            resultStr = resultToStr(std::get<1>(*it));
            std::cout << "\tTest n°" << index << ": " << resultStr;
            chrono = std::to_string(std::get<2>(*it));
            std::cout << std::string(NB_SPACES_BEFORE_CHRONO - resultStr.size(), ' ') << " " << chrono << "s";
            if (!std::get<0>(*it).empty()) {
                std::cout << " " << std::string(NB_SPACES_BEFORE_TEST_DESCRIPTION - chrono.size(), ' ') << "(" << std::get<0>(*it) << ")";
            }
            std::cout << "\n";
            index++;
        }
        std::cout << results.size() << " tests in " << std::to_string(std::chrono::duration<float>(endAllTestsTime - startAllTestsTime).count())
                  << "s\n";
        std::cout << "Errors in this run: " << getNbErrors() << "\n";
        std::cout << "End of summary\n";
    }

    int Tests::getNbErrors() const {
        int nbErrors = 0;
        for (std::tuple<std::string, Result, float> result : results) {
            if (std::get<1>(result) != Result::OK) nbErrors++;
        }
        return nbErrors;
    }

} // namespace test
