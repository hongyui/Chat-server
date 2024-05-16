#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include <iostream>
#include <string>
#include <functional> // 添加這行
#include <boost/asio.hpp> // 添加這行

typedef websocketpp::server<websocketpp::config::asio> server;

void on_message(server* s, websocketpp::connection_hdl hdl, server::message_ptr msg) {
    std::cout << "Received message: " << msg->get_payload() << std::endl;

    try {
        s->send(hdl, msg->get_payload(), msg->get_opcode());
    } catch (const websocketpp::lib::error_code& e) {
        std::cout << "Echo failed because: " << e.message() << std::endl;
    }
}

std::string get_local_ip() {
    boost::asio::io_service io_service;
    boost::asio::ip::udp::resolver resolver(io_service);
    boost::asio::ip::udp::resolver::query query(boost::asio::ip::host_name(), "");
    boost::asio::ip::udp::resolver::iterator iter = resolver.resolve(query);
    boost::asio::ip::udp::resolver::iterator end; // End marker

    while (iter != end) {
        boost::asio::ip::address addr = (iter++)->endpoint().address();
        if (addr.is_v4() && addr.to_string() != "127.0.0.1") {
            return addr.to_string();
        }
    }

    return "127.0.0.1";
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }

    int port = std::stoi(argv[1]);
    server echo_server;

    // 初始化 Asio
    echo_server.init_asio();

    // 設置消息處理程序
    echo_server.set_message_handler(websocketpp::lib::bind(&on_message, &echo_server, std::placeholders::_1, std::placeholders::_2));

    // 設置服務器端口
    echo_server.listen(port);

    // 打印本地IP地址
    std::string local_ip = get_local_ip();
    std::cout << "Server is running on: ws://" << local_ip << ":" << port << std::endl;

    // 啟動接受連接
    echo_server.start_accept();

    // 啟動 Asio io_service 事件循環
    echo_server.run();

    return 0;
}
