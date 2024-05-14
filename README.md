# Chat server

 using C++

## Git TUT
<https://www.youtube.com/watch?v=Luu7V9Nx1EM>

## Cmake TUT
<https://sfumecjf.github.io/cmake-examples-Chinese/01-basic/1.2%20%20hello-headers.html>

## 啟動開發環境

```bash!
    docker-compose up -d dev
```

## 進入開發環境

```bash!
    docker-compose exec dev bash
```

## 啟動編譯腳本

```bash!
    ./autobuild.sh
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