#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>
#include <thread>
#include <chrono>
#include <nlohmann/json.hpp>
#include <atomic>

using boost::asio::ip::tcp;
using json = nlohmann::json;

std::atomic<bool> running(true); // 用於控制線程運行狀態
// bool logged_in = false;
json account_json = {
    {"type", ""},
    {"username", ""},
    {"password", ""}};

json message_json = {
    {"type", ""},
    {"username", ""},
    {"message", ""},
    {"ip", ""},
    {"port", ""}};

bool try_connect(tcp::socket &socket, const std::string &server_ip, const int port, boost::asio::io_context &io_context, int timeout_seconds = 5)
{
    bool connected = false;
    boost::asio::steady_timer timer(io_context);
    timer.expires_after(std::chrono::seconds(timeout_seconds));

    timer.async_wait([&](const boost::system::error_code &ec)
                     {
        if (!ec) {
            socket.cancel();
        } });

    std::cout << "正在連接到伺服器 IP: " << server_ip << ", 埠: " << port << std::endl;

    socket.async_connect(tcp::endpoint(boost::asio::ip::address::from_string(server_ip), port), [&](const boost::system::error_code &ec)
                         {
        if (!ec) {
            connected = true;
            timer.cancel();
        } else {
            std::cerr << "連接伺服器時出錯: " << ec.message() << std::endl;
        } });

    io_context.run();
    io_context.restart();
    return connected;
}

void print_help()
{
    std::cout << "可用的命令:\n"
              << "  !exit          退出客戶端\n";
}

std::string receive_response(tcp::socket &socket)
{
    boost::asio::streambuf response_buf;
    boost::system::error_code ec;
    boost::asio::read_until(socket, response_buf, "\n", ec);
    if (ec)
    {
        if (running)
        {
            std::cerr << "接收伺服器回應時出錯: " << ec.message() << std::endl;
        }
        return "";
    }
    std::istream response_stream(&response_buf);
    std::string response;
    std::getline(response_stream, response);
    return response;
}

bool handle_registration(tcp::socket &socket, json &account_json)
{
    std::string username, password;
    std::cout << "請輸入使用者名稱：";
    std::getline(std::cin >> std::ws, username);
    std::cout << "請輸入密碼：";
    std::getline(std::cin >> std::ws, password);

    account_json = {
        {"type", "register"},
        {"username", username},
        {"password", password}};

    boost::system::error_code ec;
    boost::asio::write(socket, boost::asio::buffer(account_json.dump() + "\n"), ec);
    if (ec)
    {
        std::cerr << "發送註冊請求時出錯: " << ec.message() << std::endl;
        return false;
    }
    std::string response = receive_response(socket);
    std::cout << response << std::endl;
    return response != "使用者已存在，請重新註冊";
}

bool handle_login(tcp::socket &socket, json &account_json)
{
    if (account_json["type"] != "register")
    {
        std::string username, password;
        std::cout << "請輸入使用者名稱：";
        std::getline(std::cin >> std::ws, username);
        std::cout << "請輸入密碼：";
        std::getline(std::cin >> std::ws, password);
        account_json = {
            {"type", "login"},
            {"username", username},
            {"password", password}};
    }
    else
    {
        account_json["type"] = "login";
    }

    boost::system::error_code ec;
    boost::asio::write(socket, boost::asio::buffer(account_json.dump() + "\n"), ec);
    if (ec)
    {
        std::cerr << "發送登入請求時出錯: " << ec.message() << std::endl;
        return false;
    }

    std::string response = receive_response(socket);
    if (response == "登入成功")
    {
        std::cout << "登入成功，歡迎 " << account_json["username"] << "!" << std::endl;
        return true;
    }
    else
    {
        std::cout << "登入失敗: " << response << std::endl;
        return false;
    }
}

void receive_messages(tcp::socket &socket, json &account_json)
{
    try
    {
        while (running)
        {
            std::string response = receive_response(socket);
            if (!response.empty())
            {
                std::cout << "\r" << std::string(2, ' ') << "\r"; // 清除提示符
                std::cout << response << std::endl;
                std::cout << "<" << account_json["username"] << "> " << std::flush; // 再次顯示提示符
            }
        }
    }
    catch (std::exception &e)
    {
        if (running)
        {
            std::cerr << "接收訊息時發生錯誤: " << e.what() << std::endl;
        }
    }
}

// void receive_messages(tcp::socket &socket)
// {
//     try
//     {
//         for (;;)
//         {
//             std::string response = receive_response(socket);
//             if (!response.empty())
//             {
//                 std::cout << response << std::endl;
//             }
//         }
//     }
//     catch (std::exception &e)
//     {
//         std::cerr << "接收訊息時發生錯誤: " << e.what() << std::endl;
//     }
// }

void send_messages(tcp::socket &socket, json &account_json, json &message_json, std::string server_ip)
{
    //std::cout << account_json["ip"] << account_json["port"] << std::endl;
    std::string username = account_json["username"];
    try
    {
        while (running)
        {
            std::cout << "<" << account_json["username"] << "> ";
            std::string message;
            std::getline(std::cin, message);

            if (message.empty())
                continue; // 忽略空輸入
            std::cout << "\r\033[K";
            if (message[0] == '!')
            {
                if (message == "!help")
                {
                    print_help();
                }
                else if (message == "!exit")
                {
                    running = false;
                    return;
                }
                else
                {
                    std::cout << "輸入 !help 查看可用命令\n";
                }
            }
            else
            {

                std::cout << username << ": " << message << "\n";
                message_json = {
                    {"type", "message"},
                    {"username", username},
                    {"message", message},
                    {"ip", server_ip},
                    {"port", "8080"}};
                // if (message_json["message"] == NULL || message_json["type"] == NULL || message_json["username" == NULL]) continue;
                boost::system::error_code ec;
                boost::asio::write(socket, boost::asio::buffer(message_json.dump() + "\n"), ec);

                if (ec)
                {
                    std::cerr << "發送訊息時出錯: " << ec.message() << std::endl;
                }
            }
        }
    }
    catch (std::exception &e)
    {
        std::cerr << "發送訊息時發生錯誤: " << e.what() << std::endl;
    }
}
void server_init(tcp::socket &socket, boost::asio::io_context &io_context, bool logged_in)
{
    if (try_connect(socket, "172.17.0.1", 8080, io_context))
    {
        std::cout << "\033[32m" << "已連線至SQL伺服器" << std::endl;
    }
    else
    {
        std::cerr << "連線逾時" << std::endl;
    }
    while (!logged_in)
    {
        std::cout << "login or signup\n";
        std::string input;
        std::cin >> input;
        if (input == "login")
        {
            logged_in = handle_login(socket, account_json);
        }
        else if (input == "signup")
        {
            handle_registration(socket, account_json);
            logged_in = handle_login(socket, account_json);
        }
        else
        {
            std::cout << "輸入錯誤，請重新輸入\n";
        }
    }
}
int main()
{
    const std::string GREEN = "\033[32m";
    const std::string RED = "\033[31m";
    const std::string RESET = "\033[0m";
    bool logged_in = false;
    const int PORT = 8080;
    while (true)
    {
        try
        {
            boost::asio::io_context io_context_auth;
            tcp::socket socket_auth(io_context_auth);
            server_init(socket_auth, io_context_auth, logged_in);
            boost::asio::io_context io_context;
            tcp::socket socket(io_context);
            std::string server_ip;
            std::cout << "請輸入伺服器 IP 位址：";
            std::cin >> server_ip;
            std::cin.ignore(); // 忽略換行符
            if (server_ip == "exit")
                return 0;
            if (try_connect(socket, server_ip, PORT, io_context))
            {
                std::cout << GREEN << "已連線至伺服器 " << server_ip << " 的埠 " << PORT << RESET << std::endl;
                running = true;
                std::thread receive_thread(receive_messages, std::ref(socket), std::ref(account_json));
                std::thread send_thread(send_messages, std::ref(socket), std::ref(account_json), std::ref(message_json),std::ref(server_ip));
                send_thread.join();
                running = false; // 結束接收訊息線程
                socket.close();  // 關閉 socket 以觸發異常結束接收線程
                receive_thread.join();
            }
            else
            {
                std::cerr << "連線逾時。請輸入有效的伺服器 IP 位址" << std::endl;
                continue;
            }
        }
        catch (std::exception &e)
        {
            std::cerr << "例外情況: " << e.what() << std::endl;
        }
        std::cout << "test";
    }

    return 0;
}
