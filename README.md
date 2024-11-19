# C++ Webserver
## Introduction

This project is a lightweight, high-performance Webserver built in C++ using **Boost** and **MySQL**. It provides functionalities such as file download and a message board for user communication. The server efficiently manages resources with a thread pool and database connection pool, ensuring scalability and robustness.


## Features

### Core Features

1. **File Download**:
   - Hosts static files (e.g., images, PDFs).
   - Users can download files from the `/static/data` directory via the web interface.

2. **Message Board**:
   - Users can send messages to each other.
   - Dynamic content is handled through HTTP requests.
   - Users can read the latest messages every 5 seconds.

3. **User Management**:
   - Users can sign up and log in using a simple web interface.
   - Personal information (e.g., username, email, and avatar) can be updated.
   - Avatars can be uploaded or selected, stored in the `/static/avatar` directory.
   - All user data is securely stored in the MySQL database.

4. **Logging**:
   - Provides detailed logging for server events and errors for better debugging and monitoring.

5. **Efficient Resource Management**:
   - **Thread Pool**: Manages worker threads to handle multiple client connections concurrently.
   - **SQL Connection Pool**: Reuses MySQL connections to reduce overhead and improve performance.



## Prerequisites

### Libraries and Tools
1. **Boost**:
   - Boost version 1.81 or above.
   - Required components: `filesystem`, `system`, and `json`.

2. **MySQL Connector C++**:
   - Version 9.1 or higher for database connectivity.

3. **CMake**:
   - Version 3.16 or above to build the project.

4. **Compiler**:
   - A modern C++ compiler supporting C++17 or above.


## Building the Project

### 1. Install Dependencies
- Ensure **Boost** and **MySQL Connector C++** are installed on your system.

### 2. Clone the Repository and Build the Project
- Update `main.cpp` with the correct IP/port and MySQL credentials.
```sh
git clone https://github.com/blue-soda/Webserver.git
cd Webserver
mkdir build
cd build
cmake ..
make
```
## Run the WebServer
- Ensure a MySQL server is running and accessible.
- Set up the required database and tables.
- Place all static files in the `static/data` directory for download.
```
./bin/webserver
```
- Open your browser and navigate to `http://localhost:8080` (or the configured IP/port).
  

  This project is an exercise in designing a web server using modern C++ tools and libraries.
