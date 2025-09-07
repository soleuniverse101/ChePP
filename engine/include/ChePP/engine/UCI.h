//
// Created by paul on 9/6/25.
//

#ifndef CHEPP_UCI_H
#define CHEPP_UCI_H

#include "ChePP/engine/position.h"
#include "ChePP/engine/search.h"


class UCIEngine {

    Position pos;
    std::vector<Move> moves;

    SearchThreadHandler handler{};

    struct Options
    {
        int num_threads = 1;
        int tt_size = 64;
    };

    Options options;

public:
    static void uci() {
        std::cout << "id name ChePP\n";
        std::cout << "id author Paul\n";
        std::cout << "uciok\n" << std::endl;
    }

    static void isready() {
        std::cout << "readyok" << std::endl ;
    }

    void ucinewgame() {
        g_tt.reset();
    }

    void position(const std::string& cmd) {
        std::istringstream iss(cmd);
        std::string token;
        iss >> token;

        moves.clear();
        moves.reserve(MAX_PLY);

        std::string type;
        iss >> type;
        if (type == "startpos") {
            pos.from_fen(start_fen);
            Position movegen = pos;
            if (iss >> token && token == "moves") {
                while (iss >> token) {
                    auto m = move_from_uci(token, movegen);
                    if (m)
                    {
                        moves.push_back(m.value());
                        movegen.do_move(m.value());
                    }
                }
            }
        } else if (type == "fen") {
            std::string fen, part;
            for (int i = 0; i < 6 && iss >> part; ++i) {
                if (i) fen += " ";
                fen += part;
            }
            if (pos.from_fen(fen))
            {
                Position movegen = pos;
                std::string is_moves;
                if (iss >> token && token == "moves") {
                    while (iss >> token) {
                        auto m = move_from_uci(token, movegen);
                        if (m)
                        {
                            moves.push_back(m.value());
                            movegen.do_move(m.value());
                        }
                    }
                }
            }
        }
        std::cout << pos << std::endl;
    }

    void go(int depth = 99, int time_ms = std::numeric_limits<int>::max()) {
        handler.set(options.num_threads, {depth, time_ms}, pos, moves);
        handler.start();
    }

    void go(const std::string& cmd)
    {
        std::istringstream iss(cmd);;
        std::string command;
        iss >> command;
        std::string token;
        iss >> token;

        if (token == "depth")
        {
            iss >> token;
            go(std::stoi(token));
        }
        else
        {
            go();
        }
    }




    void stop()
    {
        handler.stop_all();
        handler.join();
    }

    int loop() {
        std::string line;
        while (std::getline(std::cin, line)) {
            if (line == "uci") {
                uci();
            } else if (line == "isready") {
                isready();
            } else if (line == "ucinewgame") {
                ucinewgame();
            } else if (line.rfind("position", 0) == 0) {
                position(line);
            } else if (line.rfind("go", 0) == 0) {
                go(line);
            } else if (line == "stop") {
                stop();
            } else if (line == "quit")
            {
                stop();
                break;
            }
        }
        return 0;
    }
};



#endif // CHEPP_UCI_H
