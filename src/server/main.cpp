#include <iostream>
#include <boost/asio.hpp>
#include <mysql.h>
#include <nlohmann/json.hpp>

// 使用 boost::asio::ip::tcp 命名空间
using boost::asio::ip::tcp;

// 使用 nlohmann::json 命名空间
using json = nlohmann::json;

const int PORT = 8080;

// 定義一個函數get_ip_address()，用於獲取本機IPv4地址
std::string get_ip_address() {
    // 創建一個asio的io_context物件，用於處理網絡操作
    boost::asio::io_context io_context;
    // 創建一個resolver物件，用於解析本機主機名
    tcp::resolver resolver(io_context);
    // 解析本機主機名到IP地址列表
    auto endpoints = resolver.resolve(boost::asio::ip::host_name(), "");
    // 使用range-based for loop遍歷IP地址列表，找到第一個IPv4地址，並回傳，range-based for loop可以用來遍歷物件或向量
    for (const auto& endpoint : endpoints) {
        // 判斷IP地址是否是IPv4地址
        if (endpoint.endpoint().address().is_v4()) {
            // 回傳IP地址的字符串形式
            return endpoint.endpoint().address().to_string();
        }
    }
    // 如果沒有找到IPv4地址就回傳
    return "0.0.0.0";
}

int main() {
    // 建立MySQL連接
    MYSQL *conn;
    // 定義MySQL的連接參數資料庫名稱、用戶名、密碼、伺服器地址
    const char *server = "database";
    const char *user = "root";
    const char *password = "rootpassword";
    const char *database = "database_name";
    // 初始化MySQL連接
    conn = mysql_init(NULL);
    // 如果連接失敗，輸出錯誤信息
    if (!mysql_real_connect(conn, server, user, password, database, 0, NULL, 0)) {
        std::cerr << "Error: " << mysql_error(conn) << std::endl;
        return 1;
    } else {
        std::cout << "Connected to the MySQL server successfully!" << std::endl;
    }

    // 如果沒有資料表users，則創建一個
    if (mysql_query(conn, "SHOW TABLES LIKE 'users';")) {
        std::cerr << "Error: " << mysql_error(conn) << std::endl;
        return 1;
    } else {
        MYSQL_RES *res = mysql_store_result(conn);
        if (res == NULL) {
            std::cerr << "Error: " << mysql_error(conn) << std::endl;
            return 1;
        }
        if (mysql_num_rows(res) == 0) {
            std::string create_account_query = "CREATE TABLE IF NOT EXISTS users (id INT PRIMARY KEY AUTO_INCREMENT, username VARCHAR(255), password VARCHAR(255));";
            if (mysql_query(conn, create_account_query.c_str())) {
                std::cerr << "Error: " << mysql_error(conn) << std::endl;
                mysql_free_result(res);
                return 1;
            } else {
                std::cout << "Table created successfully!" << std::endl;
            }
        } else {
            std::cout << "Table already exists!" << std::endl;
        }
        mysql_free_result(res);
    }

    try {
        // 創建一個asio的io_context物件，用於處理網絡操作
        boost::asio::io_context io_context;
        // 創建一個acceptor物件，用於監聽本機的PORT端口定義在上面的const int PORT = 8080;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), PORT));
        // 獲取本機的IPv4地址
        std::string ip_address = get_ip_address();
        // 輸出本機的IPv4地址和PORT
        std::cout << "Server is listening on IP " << ip_address << " and port " << PORT << std::endl;

        for (;;) {
            // 創建一個socket物件，接受客戶端的連接
            tcp::socket socket(io_context);
            // 等待客戶端的連接
            acceptor.accept(socket);

            // 讀取客戶端發送的數據
            boost::asio::streambuf buf; // 創建一個streambuf物件，用於保存數據
            boost::asio::read_until(socket, buf, "\n"); // 讀取socket中的數據，直到遇到換行符\n
            std::istream input_stream(&buf); // 創建一個istream物件，用於讀取streambuf中的數據
            std::string json_str; // 定義一個字符串變量，用於保存讀取的數據
            std::getline(input_stream, json_str); // 讀取streambuf中的數據到字符串變量中

            //解析 json_str 並出入資料庫
            json account_info = json::parse(json_str); // 解析json字符串
            std::string username = account_info["username"]; // 獲取json中的username
            std::string password = account_info["password"]; // 獲取json中的password
            std::string query = "INSERT INTO users (username, password) VALUES ('" + username + "', '" + password + "');"; // 將username和password插入到users表中

            if (mysql_query(conn, query.c_str())) {
                std::cerr << "Error: " << mysql_error(conn) << std::endl;
            } else {
                std::cout << "User " << username << " 註冊 successfully!" << std::endl;
                // 註冊成功
                std::string response = "User " + username + " 註冊 successfully!\n";
                boost::asio::write(socket, boost::asio::buffer(response));
            }
        }
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    // 關閉MySQL連接
    mysql_close(conn);

    return 0;
}
