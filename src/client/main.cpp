#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <thread>
#include <chrono>
#include <nlohmann/json.hpp>

using boost::asio::ip::tcp;
using json = nlohmann::json;

bool try_connect(tcp::socket& socket, const std::string& server_ip, const int port, boost::asio::io_context& io_context, int timeout_seconds = 5) {
    bool connected = false;
    boost::asio::steady_timer timer(io_context);
    timer.expires_after(std::chrono::seconds(timeout_seconds));

    timer.async_wait([&](const boost::system::error_code& ec) {
        if (!ec) {
            socket.cancel();
        }
    });

    std::cout << "正在連接到伺服器 IP: " << server_ip << ", 埠: " << port << std::endl;

    socket.async_connect(tcp::endpoint(boost::asio::ip::address::from_string(server_ip), port), [&](const boost::system::error_code& ec) {
        if (!ec) {
            connected = true;
            timer.cancel();
        } else {
            std::cerr << "連接伺服器時出錯: " << ec.message() << std::endl;
        }
    });

    io_context.run();
    io_context.restart();
    return connected;
}

void print_help() {
    std::cout << "可用的命令:\n"
              << "  --message       發送訊息到伺服器\n"
              << "  --exit          退出客戶端\n";
}

std::string receive_response(tcp::socket& socket) {
    boost::asio::streambuf response_buf;
    boost::system::error_code ec;
    boost::asio::read_until(socket, response_buf, "\n", ec);
    if (ec) {
        std::cerr << "接收伺服器回應時出錯: " << ec.message() << std::endl;
        return "";
    }
    std::istream response_stream(&response_buf);
    std::string response;
    std::getline(response_stream, response);
    return response;
}

bool handle_registration(tcp::socket& socket) {
    std::string username, password;
    std::cout << "請輸入使用者名稱：";
    std::getline(std::cin >> std::ws, username);
    std::cout << "請輸入密碼：";
    std::getline(std::cin >> std::ws, password);

    json account_json = {
        {"type", "register"},
        {"username", username},
        {"password", password}
    };

    boost::system::error_code ec;
    boost::asio::write(socket, boost::asio::buffer(account_json.dump() + "\n"), ec);
    if (ec) {
        std::cerr << "發送註冊請求時出錯: " << ec.message() << std::endl;
        return false;
    }

    std::string response = receive_response(socket);
    std::cout << response << std::endl;
    return response != "使用者已存在，請重新註冊";
}

bool handle_login(tcp::socket& socket, json& account_json) {
    std::string username, password;
    std::cout << "請輸入使用者名稱：";
    std::getline(std::cin >> std::ws, username); 
    std::cout << "請輸入密碼：";
    std::getline(std::cin >> std::ws, password);

    account_json = {
        {"type", "login"},
        {"username", username},
        {"password", password}
    };

    boost::system::error_code ec;
    boost::asio::write(socket, boost::asio::buffer(account_json.dump() + "\n"), ec);
    if (ec) {
        std::cerr << "發送登入請求時出錯: " << ec.message() << std::endl;
        return false;
    }

    std::string response = receive_response(socket);
    if (response == "登入成功") {
        std::cout << "登入成功，歡迎 " << username << "!" << std::endl;
        account_json["username"] = username;
        return true;
    } else {
        std::cout << "登入失敗: " << response << std::endl;
        return false;
    }
}

void receive_messages(tcp::socket& socket) {
    try {
        for (;;) {
            std::string response = receive_response(socket);
            if (!response.empty()) {
                std::cout << response << std::endl;
            }
        }
    } catch (std::exception& e) {
        std::cerr << "接收訊息時發生錯誤: " << e.what() << std::endl;
    }
}

int main() {
    const std::string GREEN = "\033[32m";
    const std::string RED = "\033[31m";
    const std::string RESET = "\033[0m";
    
    const int PORT = 8080;
    json account_json;
    
    try {
        boost::asio::io_context io_context;
        tcp::socket socket(io_context);

        std::string server_ip;
        std::cout << "請輸入伺服器 IP 位址：";
        std::cin >> server_ip;
        if (try_connect(socket, server_ip, PORT, io_context)) {
            std::cout << GREEN << "已連線至伺服器 " << server_ip << " 的埠 " << PORT << RESET <<std::endl;
            bool logged_in = false;

            while (!logged_in) {
                std::cout << "輸入1註冊，輸入2登入\n";
                int choice;
                std::cin >> choice;

                if (choice == 1) {
                    logged_in = handle_registration(socket);
                } else if (choice == 2) {
                    logged_in = handle_login(socket, account_json);
                } else {
                    std::cout << "輸入錯誤，請重新輸入\n";
                }
            }

            std::cout << "請輸入 --help 查看可用命令\n";

            std::thread(receive_messages, std::ref(socket)).detach();

            std::string command;
            while (true) {
                std::cout << "> ";
                std::cin >> command;

                if (command == "--help") {
                    print_help();
                } else if (command == "--message") {
                    std::cin.ignore();
                    std::string message;
                    std::cout << "輸入訊息：";
                    std::getline(std::cin, message);
                    std::cout << account_json["username"] << ": " << message << "\n";

                    json message_json = {
                        {"type", "message"},
                        {"username", account_json["username"]},
                        {"message", message}
                    };

                    boost::system::error_code ec;
                    boost::asio::write(socket, boost::asio::buffer(message_json.dump() + "\n"), ec);
                    if (ec) {
                        std::cerr << "發送訊息時出錯: " << ec.message() << std::endl;
                    }
                } else if (command == "--exit") {
                    break;
                } else {
                    std::cout << "無效的命令。請輸入 --help 查看可用命令\n";
                }
            }
        } else {
            std::cerr << "連線逾時。請輸入有效的伺服器 IP 位址" << std::endl;
        }
    } catch (std::exception& e) {
        std::cerr << "例外情況: " << e.what() << std::endl;
    }

    return 0;
}
