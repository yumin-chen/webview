#include "../subprocess.hpp"
#include <iostream>
#include <cassert>
#include <vector>
#include <unistd.h>

using namespace alloy;

void test_basic_spawn() {
    std::vector<std::string> cmd = {"echo", "hello"};
    spawn_options options;
    options.stdout_mode = "pipe";

    subprocess proc(cmd, options);
    assert(proc.pid() > 0);

    char buf[128];
    ssize_t n = read(proc.get_stdout_fd(), buf, sizeof(buf));
    assert(n > 0);
    std::string output(buf, n);
    assert(output == "hello\n");

    int exit_code = proc.wait();
    assert(exit_code == 0);
    std::cout << "test_basic_spawn passed" << std::endl;
}

void test_spawn_sync() {
    std::vector<std::string> cmd = {"echo", "sync"};
    spawn_options options;
    options.stdout_mode = "pipe";

    subprocess proc(cmd, options);
    auto res = proc.wait_sync();
    assert(res.exit_code == 0);
    assert(res.stdout_data == "sync\n");
    std::cout << "test_spawn_sync passed" << std::endl;
}

int main() {
    test_basic_spawn();
    test_spawn_sync();
    std::cout << "All C++ tests passed!" << std::endl;
    return 0;
}
