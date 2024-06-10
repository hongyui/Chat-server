#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <thread>
#include <chrono>
#include <nlohmann/json.hpp>
#include <atomic>
#include <cctype>
#include <algorithm>
#include <unistd.h>
#define PAUSE                                        \
    std::cout << "Press Enter key to continue...\n"; \
    std::cin.get();

using boost::asio::ip::tcp;
using json = nlohmann::json;
bool paused = false;
std::atomic<bool> running(true);
bool in = false;
void clearScreen()
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void send_fetch(tcp::socket &socket, const std::string &username, const std::string &server_ip);
void init_message(tcp::socket &socket, const std::string &username, const std::string &server_ip);
void send_history(tcp::socket &socket, const std::string &username, const std::string &server_ip);
void history_message(tcp::socket &socket, const std::string &username, const std::string &server_ip);
std::string auth_ip = "172.17.0.1";
// bool logged_in = false;
json account_json = {
    {"type", ""},
    {"username", ""},
    {"password", ""}
};

json message_json = {
    {"type", ""},
    {"username", ""},
    {"message", ""},
    {"ip", ""},
    {"port", ""}
};

bool try_connect(tcp::socket &socket, const std::string &server_ip, const int port, boost::asio::io_context &io_context, int timeout_seconds = 5) {
    bool connected = false;
    boost::asio::steady_timer timer(io_context);
    timer.expires_after(std::chrono::seconds(timeout_seconds));
    timer.async_wait([&](const boost::system::error_code &ec) {
        if (!ec) {
            socket.cancel();
        }
    });
    if (server_ip != auth_ip){
        std::cout << "正在連接到伺服器 IP: " << server_ip << ", 埠: " << port << std::endl;
    }
    socket.async_connect(tcp::endpoint(boost::asio::ip::address::from_string(server_ip), port), [&](const boost::system::error_code &ec) {
        if (!ec) {
            connected = true;
            timer.cancel();
        } else {
            if(server_ip != auth_ip) {
                std::cerr << "\033[31m" << "連接伺服器時出錯: " << ec.message() << "\033[0m" << std::endl;
            }
        }
    });
    io_context.run();
    io_context.restart();
    return connected;
}

void print_help() {
    std::cout << "可用的命令:\n" << "  !history       查看歷史訊息\n";
}

std::string receive_response(tcp::socket &socket) {
    boost::asio::streambuf response_buf;
    boost::system::error_code ec;
    boost::asio::read_until(socket, response_buf, "\n", ec);
    if (ec) {
        if (running) {
            std::cerr << "\033[31m" << "接收伺服器回應時出錯: " << ec.message() << "\033[0m" << std::endl;
        }
        return "";
    }
    std::istream response_stream(&response_buf);
    std::string response;
    std::getline(response_stream, response);
    return response;
}

bool handle_registration(tcp::socket &socket, json &account_json) {
    std::string username, password;
    std::cout << "\033[32m" << "請輸入使用者名稱：" << "\033[0m";
    std::getline(std::cin >> std::ws, username);
    std::cout << "\033[32m" << "請輸入密碼：" << "\033[0m";
    std::getline(std::cin >> std::ws, password);
    account_json = {
        {"type", "register"},
        {"username", username},
        {"password", password}
    };
    boost::system::error_code ec;
    boost::asio::write(socket, boost::asio::buffer(account_json.dump() + "\n"), ec);
    if (ec) {
        std::cerr << "\033[31m" << "發送註冊請求時出錯: " << ec.message() << "\033[0m" << std::endl;
        return false;
    }
    std::string response = receive_response(socket);
    std::cout << "\033[31m" << response << "\033[0m" << std::endl;
    if (response == "使用者已存在，請重新註冊") {
        account_json["type"] = "login";
        return false;
    } else {
        return true;
    }
}

bool handle_login(tcp::socket &socket, json &account_json) {
    if (account_json["type"] != "register") {
        std::string username, password;
        std::cout << "\033[32m" << "請輸入使用者名稱：" << "\033[0m";
        std::getline(std::cin >> std::ws, username);
        std::cout << "\033[32m" << "請輸入密碼：" << "\033[0m";
        std::getline(std::cin >> std::ws, password);
        account_json = {
            {"type", "login"},
            {"username", username},
            {"password", password}
        };
    } else {
        account_json["type"] = "login";
    }
    boost::system::error_code ec;
    boost::asio::write(socket, boost::asio::buffer(account_json.dump() + "\n"), ec);
    if (ec) {
        std::cerr << "\033[31m" << "發送登入請求時出錯: " << ec.message() << "\033[0m" << std::endl;
        return false;
    }
    std::string response = receive_response(socket);
    if (response == "登入成功") {
        std::cout << "\033[33m" << "登入成功，歡迎 " << account_json["username"] << "!" << "\033[0m" << std::endl;
        return true;
    } else {
        std::cout << "\033[31m" << "登入失敗: " << response << "\033[0m" << std::endl;
        return false;
    }
}

void receive_messages(tcp::socket &socket, json &account_json, std::string server_ip) {
    try {
        while (running) {
            if (paused) {
                continue;
            } else {
                std::string response = receive_response(socket);
                if (!response.empty()) {
                    if (!paused) {
                        std::cout << "\r\033[2K"; // 清除提示符
                        std::cout << "\033[0m";
                        std::cout << "\033[33m" << response << "\033[0m" << std::endl;
                        std::cout << "\033[7m" << "<" << account_json["username"] << "> " << "\033[0m" << std::flush; // 再次顯示提示符
                    }
                }
            }
        }
    }
    catch (std::exception &e) {
        if (running) {
            std::cerr << "\033[31m" << "接收訊息時發生錯誤: " << e.what() << "\033[0m" << std::endl;
        }
    }
}

bool isSpase(const std::string &str) {
    return std::all_of(str.begin(), str.end(), [](char ch) {
        return std::isspace(static_cast<unsigned char>(ch));
        }
    );
}

void message_command(tcp::socket &socket, std::string server_ip, std::string command) {
    if (command == "!exit") {
        in = true;
        running = false;
        return;
    } else if (command == "!history") {
        std::cout << "\033[0m" << "請輸入要查詢的帳號：";
        std::string target;
        std::getline(std::cin >> std::ws, target);
        paused = true;
        std::cout << "\033[0m";
        history_message(socket, target, server_ip);
    } else if (command == "!help") {
        print_help();
    } else if (command == "!online") {
        message_json = {
            {"type", "online"},
            {"username", account_json["username"]},
            {"ip", server_ip},
            {"port", "8080"}
        };
        boost::system::error_code ec;
        boost::asio::write(socket, boost::asio::buffer(message_json.dump() + "\n"), ec);
        if (ec) {
            std::cerr << "\033[31m" << "發送訊息時出錯: " << ec.message() << "\033[0m" << std::endl;
        }
    } else {
        std::cout << "輸入 !help 查看可用命令\n";
    }
}

void send_messages(tcp::socket &socket, json &account_json, json &message_json, std::string server_ip, bool &restart) {
    std::string username = account_json["username"];
    try {
        while (running) {
            if (paused) {
                continue;
            } else {
                std::cout << "\033[7m" << "<" << account_json["username"] << "> " << std::flush;
                std::string message;
                std::getline(std::cin, message);
                std::cin.clear();
                std::cout << "\033[0m";
                if (message.empty() || isSpase(message)) {
                    std::cout << "\033[A\033[2K"; // 上移一行並清除行內容
                    continue;                     // 忽略空輸入
                }
                std::cout << "\033[A\033[2K"; // 上移一行並清除行內容
                // std::cout << "\r\033[K";
                if (message[0] == '!') {
                    message_command(socket, server_ip, message);
                } else {
                    std::cout << "\033[33m" << username << ": " << message << "\033[0m" << "\n";
                    message_json = {
                        {"type", "message"},
                        {"username", username},
                        {"message", message},
                        {"ip", server_ip},
                        {"port", "8080"}
                    };
                    // if (message_json["message"] == NULL || message_json["type"] == NULL || message_json["username" == NULL]) continue;
                    boost::system::error_code ec;
                    boost::asio::write(socket, boost::asio::buffer(message_json.dump() + "\n"), ec);
                    if (ec) {
                        std::cerr << "\033[31m" << "發送訊息時出錯: " << ec.message() << "\033[0m" << std::endl;
                    }
                }
            }
        }
        return;
    } catch (std::exception &e) {
        std::cerr << "\033[31m" << "發送訊息時發生錯誤: " << e.what() << "\033[0m" << std::endl;
    }
}

bool server_init(tcp::socket &socket, boost::asio::io_context &io_context, bool logged_in) {
    if (!try_connect(socket, auth_ip, 8080, io_context)) {
        std::cerr << "\033[31m" << "無法連線至驗證伺服器，請確認連線狀態" << "\033[0m" << std::endl;
        return false;
    }
    while (!logged_in) {
        std::cout << "\033[32m" << "請選擇登入(login)或是註冊(signup)\n輸入:" << "\033[0m";
        std::string input;
        std::cin >> input;
        if (input == "login") {
            logged_in = handle_login(socket, account_json);
        } else if (input == "signup") {
            if (handle_registration(socket, account_json)) {
                logged_in = handle_login(socket, account_json);
            } else {
                continue;
            }
        } else {
            std::cout << "\033[31m" << "輸入錯誤，請重新輸入\n" << "\033[0m";
        }
    }
    return true;
}

void send_fetch(tcp::socket &socket, const std::string &username, const std::string &server_ip) {
    json fetch_request = {
        {"type", "fetch"},
        {"username", username},
        {"ip", server_ip},
        {"port", "8080"}
    };

    boost::system::error_code ec;
    boost::asio::write(socket, boost::asio::buffer(fetch_request.dump() + "\n"), ec);
    if (ec) {
        std::cerr << "\033[31m" << "發送訊息時出錯: " << ec.message() << "\033[0m" << std::endl;
    }
}

void init_message(tcp::socket &socket, const std::string &username, const std::string &server_ip) {
    std::cout << "\033[32m" << "已連線至伺服器 " << server_ip << "\033[0m" << std::endl;
    std::string response = "START";
    send_fetch(socket, username, server_ip);

    while (response != "END") {
        response = receive_response(socket);
        if (response != "END") {
            std::cout << "\033[33m" << response << "\033[0m" << std::endl;
        }
    }
}

void send_history(tcp::socket &socket, const std::string &username, const std::string &server_ip) {
    json fetch_request = {
        {"type", "history"},
        {"username", username},
        {"ip", server_ip},
        {"port", "8080"}
    };

    boost::system::error_code ec;
    boost::asio::write(socket, boost::asio::buffer(fetch_request.dump() + "\n"), ec);
    if (ec) {
        std::cerr << "\033[31m" << "發送訊息時出錯: " << ec.message() << "\033[0m" << std::endl;
    }
}

void history_message(tcp::socket &socket, const std::string &username, const std::string &server_ip) {
    system("clear");
    std::string response = "START";
    send_history(socket, username, server_ip);

    std::cout << "使用者'" << username << "'的歷史訊息：\n" << "\033[0m";
    while (response != "END") {
        if (response == "NO_MESSAGE") {
            std::cout << "查無歷史訊息\n";
            continue;
        }
        response = receive_response(socket);
        if (response != "END") {
            std::cout << "\033[41m" << response << "\033[0m" << std::endl;
        }
    }
    PAUSE;
    system("clear");
    init_message(socket, account_json["username"], server_ip);
    paused = false;
    std::cout << "\033[0m";
}

int main() {
    std::cout << "\033[2J\033[1;1H";
    std::cout << "\033[0m";
    const std::string GREEN = "\033[32m";
    const std::string RED = "\033[31m";
    const std::string RESET = "\033[0m";
    bool logged_in = false;
    const int PORT = 8080;
    int count = 1;
    std::cout << "\n"
              << " __          __    _                                 _____  _             _     _____                               \n"
              << " \\ \\        / /   | |                               / ____|| |           | |   / ____|                              \n"
              << "  \\ \\  /\\  / /___ | |  ___  ___   _ __ ___    ___  | |     | |__    __ _ | |_ | (___    ___  _ __ __   __ ___  _ __ \n"
              << "   \\ \\/  \\/ // _ \\| | / __|/ _ \\ | '_ ` _ \\  / _ \\ | |     | '_ \\  / _` || __| \\___ \\  / _ \\| '__|\\ \\ / // _ \\| '__|\n"
              << "    \\  /\\  /|  __/| || (__| (_) || | | | | ||  __/ | |____ | | | || (_| || |_  ____) ||  __/| |    \\ V /|  __/| |   \n"
              << "     \\/  \\/  \\___||_| \\___|\\___/ |_| |_| |_| \\___|  \\_____||_| |_| \\__,_| \\__||_____/  \\___||_|     \\_/  \\___||_|   \n"
              << "                                                                                                                    \n"
              << std::endl;
    while (true) {
        bool restart = false;
        try {
            boost::asio::io_context io_context_auth;
            tcp::socket socket_auth(io_context_auth);
            if (server_init(socket_auth, io_context_auth, logged_in)) {
                boost::asio::io_context io_context;
                tcp::socket socket(io_context);
                std::string server_ip;
                std::cout << GREEN << "請輸入伺服器 IP 位址：" << RESET;
                std::cin >> server_ip;
                std::cin.ignore(); // 忽略換行符
                std::cin.clear();  // 清除錯誤標誌

                if (server_ip == "exit"){
                    return 0;
                }
                if (try_connect(socket, server_ip, PORT, io_context)) {
                    clearScreen();
                    running = true;
                    init_message(socket, account_json["username"], server_ip);
                    std::thread receive_thread(receive_messages, std::ref(socket), std::ref(account_json), std::ref(server_ip));
                    std::thread send_thread(send_messages, std::ref(socket), std::ref(account_json), std::ref(message_json), std::ref(server_ip), std::ref(restart));
                    receive_thread.join();
                    send_thread.join();
                    running = false;
                    clearScreen();
                    if (restart) {
                        continue;
                    }
                } else {
                    std::cerr << "\033[31m" << "連線逾時。請輸入有效的伺服器 IP 位址" << "\033[0m" << std::endl;
                    continue;
                }
            }
        } catch (std::exception &e) {
            std::cerr << "例外情況: " << e.what() << std::endl;
        }
        if (count == 3) {
            std::cerr << "嘗試次數過多，請稍後再試" << std::endl;
            break;
        } else {
            count++;
        }
    }

    return 0;
}
