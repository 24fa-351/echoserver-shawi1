#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFFER_SIZE 1024

// Function to handle each client connection
void *handle_client(void *client_socket)
{
    int client_fd = *(int *)client_socket;
    free(client_socket); // Free the dynamically allocated socket pointer
    ssize_t bytes_received;
    char buffer[BUFFER_SIZE];
    while ((bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0)) > 0)
    {
        buffer[bytes_received] = '\0';  // Null-terminate the received data
        printf("Received: %s", buffer); // Print the received line

        // Echo the entire received buffer back to the client
        send(client_fd, buffer, bytes_received, 0);
    }

    if (bytes_received == 0)
    {
        printf("Client disconnected.\n");
    }
    else if (bytes_received < 0)
    {
        perror("Receiving failed");
    }

    // Close the client connection
    close(client_fd);
    return NULL;
}

void start_echo_server(int port)
{
    int server_fd;
    struct sockaddr_in server_addr;

    // Create a TCP socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up the server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind the socket to the specified port
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Binding failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 10) == -1)
    { // Increased backlog to 10 for handling multiple clients
        perror("Listening failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", port);

    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        // Accept an incoming connection
        int *client_fd = malloc(sizeof(int)); // Allocate memory for client socket
        if ((*client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len)) == -1)
        {
            perror("Accepting connection failed");
            free(client_fd);
            continue;
        }

        printf("Connection accepted from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Create a new thread to handle the client
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, client_fd) != 0)
        {
            perror("Thread creation failed");
            close(*client_fd);
            free(client_fd);
            continue;
        }

        // Detach the thread to automatically free resources when done
        pthread_detach(thread_id);
    }

    // Close the server socket
    close(server_fd);
}

int main(int argc, char *argv[])
{
    int port = 0;

    // Check for the -p argument and parse the port
    if (argc != 3 || strcmp(argv[1], "-p") != 0)
    {
        fprintf(stderr, "Usage: %s -p <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    port = atoi(argv[2]);
    if (port <= 0 || port > 65535)
    {
        fprintf(stderr, "Invalid port number\n");
        return EXIT_FAILURE;
    }

    // Start the echo server on the specified port
    start_echo_server(port);

    return 0;
}