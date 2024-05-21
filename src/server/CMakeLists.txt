# 為server端設定來源文件
aux_source_directory(. SERVER_SRC)

# 新增server端可執行文件
add_executable(server ${SERVER_SRC})

# 連結套件庫到server端
target_link_libraries(server PRIVATE websocketpp::websocketpp asio asio::asio unofficial::libmariadb ${Boost_LIBRARIES})