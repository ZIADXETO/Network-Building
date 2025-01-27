Overview

This project involves the development of a multi-threaded HTTP server designed to handle concurrent client requests efficiently. The server can parse HTTP messages, manage headers and bodies, and process HTTP GET requests. The project emphasizes thread-safe operations, signal handling, and robust performance under multiple client connections.
What We Did

In this project, we focused on implementing and testing a multi-threaded HTTP server capable of handling multiple client requests simultaneously. The key functionalities we developed include:

    Parsing HTTP messages, including headers and body.
    Handling HTTP GET requests.
    Implementing thread-safe operations with mutex locks for accessing shared resources.
    Managing signal handling to prevent interference in the multi-threaded environment.

What We Did Not Do

    Complete implementation of advanced features such as POST requests handling.
    Full error handling and logging mechanisms for the server.
    Comprehensive performance optimization and extensive testing under high load conditions.

Remarks

    During the project, we made some changes in the conception to accommodate multi-threading. 
    This included ensuring that the handle_connection function could safely operate in a concurrent environment by blocking certain signals and adding mutex locks around critical sections.
    The decision to prioritize GET requests was based on time constraints and the primary focus on ensuring stable multi-threaded operation.

Additional Information

    The project was tested using multiple client connections initiated through browser tabs and curl commands to verify the robustness of the multi-threaded implementation.
    Future improvements could involve adding more HTTP methods, enhancing error handling, and optimizing performance for production use.