#include "../shell_lexer.hpp"
#include "../shell_parser.hpp"
#include <iostream>
#include <cassert>

using namespace alloy;

void test_lexer() {
    auto tokens = shell_lexer::tokenize("echo \"hello world\" | wc -l > out.txt");
    assert(tokens.size() == 7);
    assert(tokens[0].value == "echo");
    assert(tokens[1].value == "hello world");
    assert(tokens[2].type == shell_token_type::pipe);
    assert(tokens[3].value == "wc");
    assert(tokens[4].value == "-l");
    assert(tokens[5].type == shell_token_type::redir_out);
    assert(tokens[6].value == "out.txt");
    std::cout << "test_lexer passed" << std::endl;
}

void test_parser() {
    auto tokens = shell_lexer::tokenize("echo hi | cat");
    auto pipeline = shell_parser::parse(tokens);
    assert(pipeline.commands.size() == 2);
    assert(pipeline.commands[0].args[0] == "echo");
    assert(pipeline.commands[1].args[0] == "cat");
    std::cout << "test_parser passed" << std::endl;
}

int main() {
    test_lexer();
    test_parser();
    std::cout << "All C++ shell tests passed!" << std::endl;
    return 0;
}
