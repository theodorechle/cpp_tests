# cpp_tests

VERSION: 0.8.4

Tests are run in parallel in the same block.
The library can use multiple threads to run the tests in parallel.

# debug
For debugging, you can create a Tests instance with maxThreads = 0 and noProcesses = true to run tests in the main thread only and without threads, which allows for easier debugging. noProcesses should only be set to true for debugging, since if a test crashes, the entire program wil crash, instead of only the test process.
