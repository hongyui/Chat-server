# Chat server

 using C++

## Git TUT
<https://www.youtube.com/watch?v=Luu7V9Nx1EM>

## Cmake TUT
<https://sfumecjf.github.io/cmake-examples-Chinese/01-basic/1.2%20%20hello-headers.html>

## 構建 Docker 映像：
在專案的根目錄下執行以下命令來構建 Docker 映像：

```bash!
    docker-compose build
```

## 啟動開發環境
執行以下命令來啟動開發環境：

```bash!
    docker-compose up -d dev
```

## 進入開發環境
進入開發環境容器：

```bash!
    docker-compose exec dev bash
```

## 手動安裝 vcpkg
進入開發環境容器後，執行以下命令來克隆和安裝 vcpkg：

```bash!
    git clone https://github.com/Microsoft/vcpkg
```

## 安裝套件依賴

```bash!
    cd vcpkg
    ./bootstrap-vcpkg.sh
    ./vcpkg install boost-random websocketpp asio boost-type-traits libmariadb nlohmann-json
```

## 啟動編譯腳本

```bash!
    ./build.sh
```
## 啟動伺服器腳本

```bash!
    ./server.sh
```

## 啟動用戶端腳本

```bash!
    ./client.sh
```
## 進入 database 環境

```bash!
    docker-compose exec database bash
```

## MySQL 操作

1. 登入資料庫

```bash!
    mysql -u root -p database_name
```


2. 查詢所有資料庫

```bash!
    SHOW DATABASES;
```

3. 使用資料庫

```bash!
    USE database_name;
```

4. 查詢資料庫中的所有資料表

```bash!
    SHOW TABLES;
```
5. 查詢 `users` 資料表所有的內容

```bash!
    SELECT * FROM users;
```

6. 刪除 `users` 資料表

```bash!
    DROP TABLE users;
```

## 關閉開發環境

```bash!
    docker-compose down dev
```

## 啟動服務

```bash!
    #client端
    docker-compose run client
    
    #server端
    docker-compose run server
```

## 關閉所有服務

```bash!
    docker-compose down
```