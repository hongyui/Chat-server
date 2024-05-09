# Chat Server

使用 C++ 語言

## 基本目錄操作

### 1. 創建目錄

要在命令提示符（Windows）或終端（Mac 和 Linux）中創建一個新目錄，你可以使用 `mkdir` 命令。例如，在 Windows 中，要創建一個名為 `my_project` 的新目錄，你可以執行以下命令：

```bash
mkdir my_project
```

### 2. 切換目錄

要在命令提示符或終端中切換目錄，你可以使用 `cd` 命令。例如，如果你要進入剛剛創建的 `my_project` 目錄，你可以執行以下命令：

```bash
cd my_project
```

要返回上一層目錄，可以使用 `cd ..` 命令：

```bash
cd ..
```

### 3. 列出目錄中的文件和子目錄

要列出目錄中的所有文件和子目錄，你可以使用 `dir` 命令（Windows）或 `ls` 命令（Mac 和 Linux）。例如，在 Windows 中，要列出 `my_project` 目錄中的所有文件和子目錄，你可以執行以下命令：

```bash
dir
```

在 Mac 和 Linux 中，你可以使用以下命令：

```bash
ls
```

### 4. 刪除目錄

要刪除目錄，可以使用 `rmdir` 命令（Windows）或 `rm -r` 命令（Mac 和 Linux）。例如，在 Windows 中，要刪除 `my_project` 目錄及其所有內容，可以執行以下命令：

```bash
rmdir my_project
```

在 Mac 和 Linux 中，你可以使用以下命令：

```bash
rm -r my_project
```

## Git 

- 安裝 Git (兩個方法，選擇一個)
    1. 方法一：[點此下載 Windows 版本](https://git-scm.com/download/win)
    2. 方法二：打開 PowerShell 並以管理員身份執行以下命令：

    ```bash
    winget install --id Git.Git -e --source winget
    ```

- Git 設定
    1. 開啟 CLI 設定簽署，user name 可以隨便打，但 email 要與 GitHub 的 email 相同：

    ```bash
    git config --global user.name <User>
    ```

    ```bash
    git config --global user.email <Email>
    ```

- 使用 Git Clone 將專案下載至本地：

    ```bash
    git clone https://github.com/hongyui/Chat-server.git
    ```

## 專案說明

### 功能說明：

終端機多人聊天系統，用戶可以通過終端機連接到聊天室頻道，與其他用戶進行即時通訊。以下是系統的主要功能：

- 用戶連接：用戶通過終端機連接到聊天室頻道連接時，系統會要求用戶輸入其名稱以便識別。
- 即時通訊：用戶可以在聊天室頻道內與其他用戶進行即時通訊。他們可以發送文字消息或語音訊息，並即時收到其他用戶的消息。
- 查看歷史記錄：用戶可以查看聊天室頻道的過去聊天記錄，這些記錄需要被保存。
- 退出頻道：用戶可以隨時退出聊天室頻道，終止與其他用戶的連接。

### 技術要求：

- 使用 C++ 語言進行開發。
- 使用 Socket 建立服務器和客戶端之間的通信。
- 實現文件讀寫操作以保存聊天記錄（使用 MySQL 或 MariaDB）。

### 使用 CMake 管理

由於團隊成員的開發環境包括 Mac 和 Windows 兩個系統，為了確保建置時的一致性並減少編譯時的問題，我們使用 CMake 生成標準的建置檔，以便進行協作。

## 安裝 vcpkg

將目錄切換到 script 下，然後執行以下命令：
- Windows 安裝方式：

    ```bash
    vcpkg_install.bat
    ```

- Mac 安裝方式：

    ```bash
    sh vcpkg_install.sh
    ```

## 編譯專案

將目錄切換到此專案，然後執行以下命令：

- Windows 安裝方式：

    ```bash
    build.bat
    ```

- Mac 安裝方式：

    ```bash
    sh build.sh
    ```