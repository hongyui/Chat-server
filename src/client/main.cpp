#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
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

int main() {
    std::string server_ip;
    const int PORT = 8080;
    json account_json;

    try {
        boost::asio::io_context io_context;
        tcp::socket socket(io_context);

        std::cout << "請輸入伺服器 IP 位址：";
        std::cin >> server_ip;

        if (try_connect(socket, server_ip, PORT, io_context)) {
            while (true) {
                std::cout << "輸入1註冊，輸入2登入\n";
                int choice;
                std::cin >> choice;
                std::string username;
                std::string password;

                if (choice == 1) {
                    std::cout << "請輸入使用者名稱：";
                    std::cin >> username;
                    std::cout << "請輸入密碼：";
                    std::cin >> password;

                    account_json = {
                        {"username", username},
                        {"password", password}
                    };

                    // 傳送註冊資料到伺服器，並加上換行符號
                    boost::asio::write(socket, boost::asio::buffer(account_json.dump() + "\n"));
                    std::cout << "已註冊使用者 " << username << std::endl;
                    break; // 註冊成功後退出循環
                } else if (choice == 2) {
                    std::cout << "請輸入使用者名稱：";
                    std::cin >> username;
                    std::cout << "請輸入密碼：";
                    std::cin >> password;

                    account_json = {
                        {"username", username},
                        {"password", password}
                    };

                    // 傳送登入資料到伺服器，並加上換行符號
                    boost::asio::write(socket, boost::asio::buffer(account_json.dump() + "\n"));
                    std::cout << "已登入使用者 " << username << std::endl;
                    break; // 登入成功後退出循環
                } else {
                    std::cout << "輸入錯誤，請重新輸入\n";
                }
            }

            std::cout << "已連線至伺服器 " << server_ip << " 的埠 " << PORT << std::endl;
            std::cout << "請輸入 --help 查看可用命令\n";

            std::string command;
            while (true) {
                std::cout << "> ";
                std::cin >> command;

                if (command == "--help") {
                    print_help();
                } else if (command == "--message") {
                    // 清除之前輸入的換行符號
                    std::cin.ignore();
                    while (true) {
                        std::string message;
                        std::cout << "請輸入訊息：";
                        std::getline(std::cin, message);
                        if (message == "--exit") {
                            break;
                        }
                        message = account_json["username"].get<std::string>() + ": " + message;
                        boost::asio::write(socket, boost::asio::buffer(message + "\n"));
                        std::cout << "已發送訊息：" << message << std::endl;
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
