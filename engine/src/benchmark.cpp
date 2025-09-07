#include "ChePP/engine/movegen.h"
#include "ChePP/engine/position.h"

#include <array>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
#include <vector>

struct EngineProcess {
    std::string path;
    pid_t pid;
    std::unique_ptr<FILE, decltype(&fclose)> in;
    std::unique_ptr<FILE, decltype(&fclose)> out;

    EngineProcess(std::string p, pid_t child,
                  FILE* i, FILE* o)
        : path(std::move(p)), pid(child),
          in(i, fclose), out(o, fclose) {}

    ~EngineProcess() {
        if (pid > 0) {
            int status = 0;
            kill(pid, SIGTERM);
            waitpid(pid, &status, 0);
        }
    }
};

EngineProcess start_engine(const std::string& path) {
    int in_pipe[2];
    int out_pipe[2];
    if (pipe(in_pipe) == -1 || pipe(out_pipe) == -1) {
        throw std::runtime_error("pipe() failed");
    }

    pid_t pid = fork();
    if (pid == -1) {
        throw std::runtime_error("fork() failed");
    }

    if (pid == 0) {
        dup2(in_pipe[0], STDIN_FILENO);
        dup2(out_pipe[1], STDOUT_FILENO);

        close(in_pipe[1]);
        close(out_pipe[0]);

        execl(path.c_str(), path.c_str(), (char*)nullptr);
        perror("execl failed");
        _exit(1);
    }

    close(in_pipe[0]);
    close(out_pipe[1]);

    FILE* child_stdin  = fdopen(in_pipe[1], "w");
    FILE* child_stdout = fdopen(out_pipe[0], "r");
    if (!child_stdin || !child_stdout) {
        throw std::runtime_error("fdopen() failed");
    }

    return EngineProcess(path, pid, child_stdin, child_stdout);
}

void send_cmd(EngineProcess& engine, const std::string& cmd) {
    fprintf(engine.in.get(), "%s\n", cmd.c_str());
    fflush(engine.in.get());
}

std::string read_until(EngineProcess& engine, const std::string& keyword) {
    char buffer[512];
    std::string line;
    while (fgets(buffer, sizeof(buffer), engine.out.get())) {
        line = buffer;
        if (line.find(keyword) != std::string::npos) {
            return line.substr(line.find(keyword));
        }
    }
    throw std::runtime_error("engine pipe closed");
}

void benchmark(const std::string& eng1_path,
               const std::string& eng2_path,
               int n_games = 10) {
    int draws = 0;
    EnumArray<Color, int> scores{};

    for (int game = 0; game < n_games; ++game) {
        std::cout << "Starting game " << game + 1 << "\n";

        auto white = start_engine(eng1_path);
        auto black = start_engine(eng2_path);

        send_cmd(white, "uci");
        read_until(white, "uciok");
        send_cmd(black, "uci");
        read_until(black, "uciok");

        send_cmd(white, "isready");
        read_until(white, "readyok");
        send_cmd(black, "isready");
        read_until(black, "readyok");

        send_cmd(white, "ucinewgame");
        send_cmd(black, "ucinewgame");

        send_cmd(white, "position startpos");
        send_cmd(black, "position startpos");

        std::cout << "setup complete " << std::endl;

        std::vector<Position> positions{};
        positions.reserve(MAX_PLY);
        positions.emplace_back();
        positions.back().from_fen(start_fen);

        std::string last_move;
        std::string all_moves;

        while (true) {
            if (Position::is_repetition(positions)) {
                draws++;
                break;
            }
            if (gen_legal(positions.back()).empty()) {
                if (positions.back().checkers(positions.back().side_to_move())) {
                    scores[~positions.back().side_to_move()] += 1;
                }
                break;
            }

            auto& engine = positions.back().side_to_move() == WHITE ? white : black;
            send_cmd(engine, "go depth 12");
            std::string bestmove_line = read_until(engine, "bestmove");
            send_cmd(engine, "stop");

            std::istringstream iss(bestmove_line);
            std::string token;
            iss >> token;
            iss >> last_move;

            all_moves += last_move + " ";
            std::cout << bestmove_line << " -> " << last_move << std::endl;

            const auto move = move_from_uci(last_move, positions.back());
            if (!move) throw std::invalid_argument("invalid move");
            std::cout << move.value() << std::endl;
            positions.emplace_back(positions.back());
            positions.back().do_move(*move);

            send_cmd(white, "position startpos moves " + all_moves);
            send_cmd(black, "position startpos moves " + all_moves);

            std::cout << Position::to_pgn(positions,
                { "benchmark", "ThinkPad", {2025, 9, 7}, game,
                                                "white", "black", NO_RESULT}) << std::endl;        }

    }

    std::cout << "Results after " << n_games << " games:\n";
    std::cout << eng1_path << " wins: " << scores.at(WHITE) << "\n";
    std::cout << eng2_path << " wins: " << scores.at(BLACK) << "\n";
    std::cout << "Draws: " << draws << "\n";
}

int main() {
    benchmark("./versions/ChePP_3474e90", "./versions/ChePP_3474e90", 10);
}