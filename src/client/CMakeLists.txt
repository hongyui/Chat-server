# 為client端設定來源文件
aux_source_directory(. CLIENT_SRC)
# 新增client端可執行文件
add_executable(client ${CLIENT_SRC})

# 連結套件庫到client端
target_link_libraries(client PRIVATE websocketpp::websocketpp asio asio::asio ${Boost_LIBRARIES})