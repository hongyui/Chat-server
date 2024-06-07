#include <iostream>
#include <boost/asio.hpp>
#include <mysql.h>
#include <nlohmann/json.hpp>
#include <set>
#include <mutex>
#include <thread>

// 使用 boost::asio::ip::tcp 命名空間
using boost::asio::ip::tcp;
// 使用 nlohmann::json 命名空間
using json = nlohmann::json;

// 定義伺服器的埠號
const int PORT = 8081;
// 定義一個 std::set 來存儲所有客戶端的 socket
std::set<tcp::socket*> clients;
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
    for (const auto& endpoint : endpoints) {
        // 如果 IP 位址是 IPv4 位址，則回傳該位址
        if (endpoint.endpoint().address().is_v4()) {
            // 使用 to_string() 函數將 IP 位址轉換為字符串
            return endpoint.endpoint().address().to_string();
        }
    }
    return "0.0.0.0";
}


// 執行 SQL 查詢 需傳入參數有 MYSQL 連接和 SQL 查詢語句
bool execute_query(MYSQL* conn, const std::string& query) {
    // 如果查詢執行失敗，則輸出錯誤信息並回傳 false
    if (mysql_query(conn, query.c_str())) {
        std::cerr << "Query execution error: " << mysql_error(conn) << std::endl;
        return false;
    }
    // 否則回傳 true
    return true;
}

// 執行 SELECT 查詢 需傳入參數有 MYSQL 連接和 SQL 查詢語句
MYSQL_RES* perform_select_query(MYSQL* conn, const std::string& query) {
    // 如果查詢執行失敗，則回傳 nullptr
    if (!execute_query(conn, query)) {
        return nullptr;
    }
    // 使用 mysql_store_result() 函數獲取查詢結果
    MYSQL_RES* res = mysql_store_result(conn);
    // 如果查詢結果為空，則輸出錯誤信息並回傳 nullptr
    if (res == NULL) {
        std::cerr << "mysql_store_result() failed: " << mysql_error(conn) << std::endl;
        return nullptr;
    }
    // 回傳查詢結果
    return res;
}
// 檢查使用者名稱是否存在 需傳入參數有 MYSQL 連接和使用者名稱
bool username_exists(MYSQL* conn, const std::string& username) {
    // SQL 查詢語句
    std::string check_query = "SELECT COUNT(*) FROM users WHERE username = '" + username + "';";
    // 執行 SQL 查詢
    MYSQL_RES* res = perform_select_query(conn, check_query);
    // 如果查詢結果為空，則回傳 false
    if (res == nullptr) {
        return false;
    }
    // 使用 mysql_fetch_row() 函數獲取查詢結果的行
    MYSQL_ROW row = mysql_fetch_row(res);
    // 將查詢結果的第一列轉換為整數
    int count = std::stoi(row[0]);
    // 釋放查詢結果
    mysql_free_result(res);
    // 如果查詢結果的第一列大於 0，則回傳 true，否則回傳 false
    return count > 0;
}
// 獲取存儲的密碼 需傳入參數有 MYSQL 連接和使用者名稱
std::string get_stored_password(MYSQL* conn, const std::string& username) {
    // SQL 查詢語句
    std::string query = "SELECT password FROM users WHERE username = '" + username + "'";
    // 執行 SQL 查詢
    MYSQL_RES* res = perform_select_query(conn, query);
    // 如果查詢結果為空，則回傳空字符串
    if (res == nullptr) {
        return "";
    }
    // 使用 mysql_fetch_row() 函數獲取查詢結果的行
    MYSQL_ROW row = mysql_fetch_row(res);
    // 將查詢結果的第一列轉換為字串
    std::string stored_password = row ? row[0] : "";
    // 釋放查詢結果
    mysql_free_result(res);
    // 回傳存儲的密碼
    return stored_password;
}

// 插入使用者 需傳入參數有 MYSQL 連接、使用者名稱和密碼
bool insert_user(MYSQL* conn, const std::string& username, const std::string& password) {
    // SQL 插入語句
    std::string query = "INSERT INTO users (username, password) VALUES ('" + username + "', '" + password + "');";
    // 執行 SQL 插入語句
    return execute_query(conn, query);
}
// 插入訊息 需傳入參數有 MYSQL 連接、使用者名稱和訊息
bool insert_message(MYSQL* conn, const std::string& username, const std::string& message) {
    // SQL 插入語句
    std::string insert_query = "INSERT INTO messages (username, message) VALUES ('" + username + "', '" + message + "');";
    // 執行 SQL 插入語句
    return execute_query(conn, insert_query);
}
// 處理客戶端 需傳入參數有客戶端的 socket 和 MYSQL 連接
void handle_client(tcp::socket socket, MYSQL* conn) {
    // 使用 std::lock_guard 來鎖定 clients_mutex
    // 為了確保對客戶端集合的操作是線程安全，多個執行緒可能同時訪問或修改這個集合，鎖定 clients_mutex 使得在操作集合時只有一個執行緒能夠進行，防止資料競爭和不一致的狀態
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.insert(&socket);
    }

    try {
        while (true) {
            // 使用 boost::asio::streambuf 類別來讀取客戶端發送的訊息
            boost::asio::streambuf buf;
            // 使用 boost::asio::read_until() 函數讀取客戶端發送的訊息直到遇到換行符
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

            // 如果type為register，則處理註冊請求
            if (type == "register") {
                // 獲取 JSON 物件中的 username 和 password 屬性
                std::string username = message_json["username"];
                std::string password = message_json["password"];
                // 如果使用者名稱已存在，則發送錯誤信息給客戶端
                if (username_exists(conn, username)) {
                    std::string response = "使用者已存在，請重新註冊\n";
                    boost::asio::write(socket, boost::asio::buffer(response));
                } else {
                    // 否則插入使用者到資料庫中
                    if (insert_user(conn, username, password)) {
                        std::string response = "使用者 " + username + " 註冊成功!\n";
                        boost::asio::write(socket, boost::asio::buffer(response));
                    } else {
                        std::cerr << "Error inserting user into database\n";
                    }
                }
            // 如果type為login，則處理登入請求
            } else if (type == "login") {
                // 獲取 JSON 物件中的 username 和 password 屬性
                std::string username = message_json["username"];
                std::string password = message_json["password"];
                // 從資料庫中獲取存儲的密碼
                std::string stored_password = get_stored_password(conn, username);
                // 如果無法獲取存儲的密碼，則輸出錯誤信息並返回
                if (stored_password.empty()) {
                    std::cerr << "Error retrieving stored password\n";
                    return;
                }
                // 如果存儲的密碼等於輸入的密碼，則發送登入成功信息給客戶端
                if (stored_password == password) {
                    std::string response = "登入成功\n";
                    boost::asio::write(socket, boost::asio::buffer(response));
                } else {
                    std::string response = "帳號或密碼錯誤!\n";
                    boost::asio::write(socket, boost::asio::buffer(response));
                }
            // 如果type為message，則處理訊息
            }else if (type == "message"){
                std::cout << "message received" << std::endl;
            }
        }
    } catch (std::exception& e) {
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


int main() {
    const std::string GREEN = "\033[32m";
    const std::string RED = "\033[31m";
    const std::string RESET = "\033[0m";

    MYSQL *conn;
    const char *server = "database";
    const char *user = "root";
    const char *password = "rootpassword";
    const char *database = "database_name";

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
            std::string create_account_query = "CREATE TABLE IF NOT EXISTS users (id INT PRIMARY KEY AUTO_INCREMENT, username VARCHAR(255), password VARCHAR(255));";
            if (mysql_query(conn, create_account_query.c_str())) {
                std::cerr << RED << "Error: " << mysql_error(conn) << RESET << std::endl;
                mysql_free_result(res);
                return 1;
            } else {
                std::cout << GREEN << "Users table created successfully!"<< RESET << std::endl;
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
            std::string create_message_query = "CREATE TABLE IF NOT EXISTS messages (id INT PRIMARY KEY AUTO_INCREMENT, username VARCHAR(255), message TEXT, ip VARCHAR(45), timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP);";
            if (mysql_query(conn, create_message_query.c_str())) {
                std::cerr << RED << "Error: " << mysql_error(conn) << RESET << std::endl;
                mysql_free_result(res);
                return 1;
            } else {
                std::cout << GREEN << "Messages table created successfully!"<< RESET << std::endl;
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
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    mysql_close(conn);
    return 0;
}