#include <iostream>
#include <boost/asio.hpp>
#include <mysql.h>
#include <nlohmann/json.hpp>
#include <set>
#include <mutex>
#include <thread>
#include <unistd.h>
#include <fstream>

// 使用 boost::asio::ip::tcp 命名空間
using boost::asio::ip::tcp;
// 使用 nlohmann::json 命名空間
using json = nlohmann::json;

// 定義伺服器的埠號
const int PORT = 8080;
// 定義一個 std::set 來存儲所有客戶端的 socket
std::set<tcp::socket *> clients;
// 定義一個 std::mutex 來保護 clients 集合
std::mutex clients_mutex;

// 獲取伺服器的 IP 位址
std::string get_ip_address() {
    // 創建一個 io_context 物件
    boost::asio::io_context io_context;
    // 創建一個 tcp::resolver 物件用來解析主機名
    tcp::resolver resolver(io_context);
    // 使用 boost::asio::ip::host_name() 函數獲取主機名
    auto endpoints = resolver.resolve(boost::asio::ip::host_name(), "");
    // 使用for - range 迴圈遍歷所有的 IP 位址
    for (const auto &endpoint : endpoints) {
        // 如果 IP 位址是 IPv4 位址，則回傳該位址
        if (endpoint.endpoint().address().is_v4()) {
            // 使用 to_string() 函數將 IP 位址轉換為字符串
            return endpoint.endpoint().address().to_string();
        }
    }
    return "0.0.0.0";
}

// 將訊息廣播給所有已連接的客戶端 需傳入參數有 訊息和發送訊息的客戶端
void broadcast_message(const std::string &message, tcp::socket *sender_socket) {
    // 使用 std::lock_guard 來鎖定 clients_mutex
    std::lock_guard<std::mutex> lock(clients_mutex);
    // 使用 for - range 迴圈遍歷所有的客戶端
    for (auto &client : clients) {
        // 如果客戶端不是發送訊息的客戶端，則將訊息發送給該客戶端
        if (client != sender_socket) {
            // 定義一個 boost::system::error_code 物件
            boost::system::error_code ec;
            // 使用 boost::asio::write() 函數將訊息發送給客戶端
            boost::asio::write(*client, boost::asio::buffer(message + "\n"), ec);
            // 如果發送訊息時出錯，則輸出錯誤信息
            if (ec) {
                std::cerr << "發送訊息給客戶端時出錯: " << ec.message() << std::endl;
            }
        }
    }
}

// 將訊息廣播給當前客戶端 需傳入參數有 訊息和發送訊息的客戶端
void send_message(const json history, tcp::socket *sender_socket, std::string type) {
    std::string message_str;
    boost::system::error_code ec;
    if (history.empty() && type == "history") {
        message_str = "NO_MESSAGE\n";
        boost::asio::write(*sender_socket, boost::asio::buffer(message_str + "\n"), ec);
        sleep(0.01);
        boost::asio::write(*sender_socket, boost::asio::buffer("END\n"), ec);
    }
    for (auto &message : history) {
        // 判別回傳訊息的類型
        if (type == "history") {
            message_str = message["timestamp"].get<std::string>() + " " + message["message"].get<std::string>();
        } else if (type == "fetch") {
            message_str = message["username"].get<std::string>() + ": " + message["message"].get<std::string>();
        }
        // 使用 boost::asio::write() 函數將訊息發送給客戶端
        boost::asio::write(*sender_socket, boost::asio::buffer(message_str + "\n"), ec);
        // 如果發送訊息時出錯，則輸出錯誤信息
        if (ec) {
            std::cerr << "發送訊息給客戶端時出錯: " << ec.message() << std::endl;
        }
        sleep(0.01);
    }
    if (type == "history" || type == "fetch") {
        boost::asio::write(*sender_socket, boost::asio::buffer("END\n"), ec);
        sleep(0.001);
    }
    if (ec) {
        std::cerr << "發送訊息給客戶端時出錯: " << ec.message() << std::endl;
    }
}

// 執行 SQL 查詢 需傳入參數有 MYSQL 連接和 SQL 查詢語句
bool execute_query(MYSQL *conn, const std::string &query) {
    // 如果查詢執行失敗，則輸出錯誤信息並回傳 false
    if (mysql_query(conn, query.c_str())) {
        std::cerr << "Query execution error: " << mysql_error(conn) << std::endl;
        return false;
    }
    // 否則回傳 true
    return true;
}

// 執行 SELECT 查詢 需傳入參數有 MYSQL 連接和 SQL 查詢語句
MYSQL_RES *perform_select_query(MYSQL *conn, const std::string &query) {
    // 如果查詢執行失敗，則回傳 nullptr
    if (!execute_query(conn, query)) {
        return nullptr;
    }
    // 使用 mysql_store_result() 函數獲取查詢結果
    MYSQL_RES *res = mysql_store_result(conn);
    // 如果查詢結果為空，則輸出錯誤信息並回傳 nullptr
    if (res == NULL) {
        std::cerr << "mysql_store_result() failed: " << mysql_error(conn) << std::endl;
        return nullptr;
    }
    // 回傳查詢結果
    return res;
}

// 執行 SELECT 查詢 需傳入參數有 MYSQL 連接和 SQL 查詢語句
json history_message(MYSQL *conn, const std::string &username, const std::string &ip) {
    // SQL 查詢語句
    std::string search_query =
        "SELECT timestamp,message FROM messages WHERE username = '" + username +
        "' AND ip = '" + ip + "'";
    // 執行 SQL 查詢語句
    MYSQL_RES *res = perform_select_query(conn, search_query);
    // 如果查詢結果為空，則回傳 false
    if (res == NULL) {
        return "查無訊息";
    }
    // 使用 mysql_num_rows() 函數獲取查詢結果的行數
    int num_rows = mysql_num_rows(res);
    // 定義一個 json 物件來儲存查詢結果
    json result;
    // 使用 mysql_fetch_row() 函數遍歷查詢結果
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        // 將查詢結果添加到 json 物件中
        result.push_back({{"timestamp", row[0]}, {"message", row[1]}});
    }
    // 釋放查詢結果
    mysql_free_result(res);
    // 回傳查詢結果
    return result;
}

// 插入訊息 需傳入參數有 MYSQL 連接、使用者名稱和訊息
bool insert_message(MYSQL *conn, const std::string &username, const std::string &message, const std::string &ip_address) {
    // SQL 插入語句
    std::string insert_query =
        "INSERT INTO messages (username, message, ip) VALUES ('" + username +
        "', '" + message + "', '" + ip_address + "')";
    // 執行 SQL 插入語句
    return execute_query(conn, insert_query);
}

// 執行 SELECT 查詢 需傳入參數有 MYSQL 連接和 SQL 查詢語句
json fetch_messages(MYSQL *conn, const std::string &ip_address, const std::string &port) {
    std::string fetch_query =
        "SELECT username,message FROM messages WHERE ip = '" + ip_address +
        ":" + port + "'";
    MYSQL_RES *res = perform_select_query(conn, fetch_query);
    // 如果查無訊息，回傳"查無訊息"
    if (res == NULL) {
        return "查無訊息";
    }

    json init_message_logs;  // 創建JSON對象以存儲消息
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {  // 跌代每一行查詢結果
        json init_message_log;  // 創建JSON對象以存儲單條消息
        init_message_log["username"] = row[0];  // 設置用戶名
        init_message_log["message"] = row[1];   // 設置消息內容
        init_message_logs.push_back(init_message_log);  // 將消息添加到消息列表中
    }
    // 釋放查詢結果資源
    mysql_free_result(res);

    return init_message_logs;  // 返回消息 JSON 數組
}

// 處理客戶端 需傳入參數有客戶端的 socket 和 MYSQL 連接
void handle_client(tcp::socket socket, MYSQL *conn) {
    // 使用 std::lock_guard 來鎖定 clients_mutex
    // 為了確保對客戶端集合的操作是線程安全，多個執行緒可能同時訪問或修改這個集合，鎖定
    // clients_mutex
    // 使得在操作集合時只有一個執行緒能夠進行，防止資料競爭和不一致的狀態
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.insert(&socket);
    }

    try {
        while (true) {
            // 使用 boost::asio::streambuf 類別來讀取客戶端發送的訊息
            boost::asio::streambuf buf;
            // 使用 boost::asio::read_until()
            // 函數讀取客戶端發送的訊息直到遇到換行符
            boost::asio::read_until(socket, buf, "\n");
            // 使用 std::istream 類別來讀取 boost::asio::streambuf 中的數據
            std::istream input_stream(&buf);
            // 定義一個字符串變數來存儲讀取的數據
            std::string json_str;
            // 使用 std::getline() 函數讀取數據
            std::getline(input_stream, json_str);
            // 使用 nlohmann::json::parse() 函數解析 JSON 字符串
            json message_json = json::parse(json_str);
            // 獲取 JSON 物件中的 type 屬性
            std::string type = message_json["type"];

            if (type == "message") {
                // 獲取 JSON 物件中的 username 和 message 屬性
                std::string username = message_json["username"];
                std::string message = message_json["message"];
                std::string ip_address = message_json["ip"];
                std::string port = message_json["port"];
                // 使用者名稱和訊息組合成完整的訊息
                std::string full_message = username + ": " + message;
                std::string full_ip = ip_address + ":" + port;
                // std::cout << "Received message: " << full_message <<
                // std::endl;

                if (!insert_message(conn, username, message, full_ip)) {
                    std::cerr << "Error inserting message into database\n";
                }
                // 廣播訊息給所有已連接的客戶端
                broadcast_message(full_message, &socket);
            } else if (type == "history") {
                // 獲取 JSON 物件中的 username 和 message 屬性
                std::string username = message_json["username"];
                std::string ip_address = message_json["ip"];
                std::string port = message_json["port"];

                // 組合伺服器完整IP
                std::string full_ip = ip_address + ":" + port;
                //  廣播訊息給當前客戶端
                send_message(history_message(conn, username, full_ip), &socket, type);
            } else if (type == "fetch") {
                // 獲取json中的IP和PORT的屬性
                std::string ip_history = message_json["ip"];
                std::string port = message_json["port"];
                // 廣播訊息給當前客戶端
                send_message(fetch_messages(conn, ip_history, port), &socket, type);
            }
        }
    } catch (std::exception &e) {
        std::cerr << "Exception in thread: " << e.what() << "\n";
    }

    // 清除客戶端集合中的客戶端
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.erase(&socket);
    }
    // 關閉客戶端的 socket
    socket.close();
}

// 使用 map 來存儲環境變數 key 和 value 使用 map 是因為 map
// 會自動按照鍵的順序進行排序也允許動態添加和刪除元素，不需要預先定義大小。
void load_env(std::map<std::string, std::string> &env) {
    // 打開 .env 文件
    std::ifstream file(".env");
    // 宣告一個字串變數來存儲每一行的數據
    std::string line;
    // 使用 while 迴圈遍歷文件中的每一行
    while (std::getline(file, line)) {
        // 使用size_t類型的pos變數來存儲等號的位置 size_t
        // 是一個無符號整數類型，通常用來表示大小或者長度如果沒有找到等號，則返回特殊常數
        // std::string::npos，表示該字符未找到。
        size_t pos = line.find('=');
        // 如果找到等號
        if (pos != std::string::npos) {
            // 使用 substr() 函數來截取字符串 substr()
            // 函數的第一個參數是截取的起始位置，第二個參數是截取的長度如果以DATABASE_HOST=database來看，就是從0開始截取到等號的位置
            std::string key = line.substr(0, pos);
            // pos + 1 表示從等號的下一個位置開始截取
            std::string value = line.substr(pos + 1);
            // 將 key 和 value 添加到 map 中
            env[key] = value;
        }
    }
}
int main() {
    const std::string GREEN = "\033[32m";
    const std::string RED = "\033[31m";
    const std::string RESET = "\033[0m";

    // 定義一個 map 來存儲環境變數
    std::map<std::string, std::string> env;
    // 載入環境變數
    load_env(env);
    MYSQL *conn;
    const char *server = env["DATABASE_HOST"].c_str();
    const char *user = env["DATABASE_USER"].c_str();
    const char *password = env["DATABASE_PASSWORD"].c_str();
    const char *database = env["DATABASE_NAME"].c_str();

    conn = mysql_init(NULL);
    if (!mysql_real_connect(conn, server, user, password, database, 0, NULL, 0)) {
        std::cerr << "Error: " << mysql_error(conn) << std::endl;
        return 1;
    } else {
        std::cout << GREEN << "Connected to the MySQL server successfully!" << RESET << std::endl;
    }

    if (mysql_query(conn, "SHOW TABLES LIKE 'users';")) {
        std::cerr << "Error: " << mysql_error(conn) << std::endl;
        return 1;
    } else {
        MYSQL_RES *res = mysql_store_result(conn);
        if (res == NULL) {
            std::cerr << RED << "Error: " << mysql_error(conn) << RESET << std::endl;
            return 1;
        }
        if (mysql_num_rows(res) == 0) {
            std::string create_account_query =
                "CREATE TABLE IF NOT EXISTS users (id INT PRIMARY KEY "
                "AUTO_INCREMENT, username VARCHAR(255), password "
                "VARCHAR(255));";
            if (mysql_query(conn, create_account_query.c_str())) {
                std::cerr << RED << "Error: " << mysql_error(conn) << RESET << std::endl;
                mysql_free_result(res);
                return 1;
            } else {
                std::cout << GREEN << "Users table created successfully!" << RESET << std::endl;
            }
        } else {
            std::cout << GREEN << "Users table already exists!" << RESET << std::endl;
        }
        mysql_free_result(res);
    }

    if (mysql_query(conn, "SHOW TABLES LIKE 'messages';")) {
        std::cerr << "Error: " << mysql_error(conn) << std::endl;
        return 1;
    } else {
        MYSQL_RES *res = mysql_store_result(conn);
        if (res == NULL) {
            std::cerr << RED << "Error: " << mysql_error(conn) << RESET << std::endl;
            return 1;
        }
        if (mysql_num_rows(res) == 0) {
            std::string create_message_query =
                "CREATE TABLE IF NOT EXISTS messages (id INT PRIMARY KEY "
                "AUTO_INCREMENT, username VARCHAR(255), message TEXT, ip "
                "VARCHAR(45), timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP);";
            if (mysql_query(conn, create_message_query.c_str())) {
                std::cerr << RED << "Error: " << mysql_error(conn) << RESET << std::endl;
                mysql_free_result(res);
                return 1;
            } else {
                std::cout << GREEN << "Messages table created successfully!" << RESET << std::endl;
            }
        } else {
            std::cout << GREEN << "Messages table already exists!" << RESET << std::endl;
        }
        mysql_free_result(res);
    }

    try {
        boost::asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), PORT));
        std::string ip_address = get_ip_address();
        std::cout << GREEN << "Server is listening on IP " << ip_address << " and port " << PORT << RESET << std::endl;

        for (;;) {
            tcp::socket socket(io_context);
            acceptor.accept(socket);
            std::thread(handle_client, std::move(socket), conn).detach();
        }
        
    } catch (std::exception &e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    mysql_close(conn);
    return 0;
}
