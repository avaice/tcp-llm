/**
 * TCP/IP Client for Lineo uLinux (circa 2002)
 * 
 * Uses only standard POSIX-compliant C language features without external libraries
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>

/* Constants */
#define DEFAULT_PORT 3000
#define DEFAULT_HOST "127.0.0.1"
#define BUFFER_SIZE 4096
#define MAX_INPUT_SIZE 1024
#define MAX_XML_SIZE 8192

/* Global variables */
int sockfd = -1;  /* Socket file descriptor */
int running = 1;  /* Program execution flag */

/* Function prototypes */
void cleanup(void);
void signal_handler(int sig);
void show_help(void);
int connect_to_server(const char *host, int port);
int send_message(int sock, const char *message);
char *receive_message(int sock);
void process_response(const char *response);
char *extract_xml_content(const char *xml, const char *tag);
void trim_string(char *str);

/**
 * Main function
 */
int main(int argc, char *argv[]) {
    char *host = DEFAULT_HOST;
    int port = DEFAULT_PORT;
    char input[MAX_INPUT_SIZE];
    char *response = NULL;
    size_t len;
    
    /* Process command line arguments */
    if (argc > 1) {
        host = argv[1];
    }
    if (argc > 2) {
        port = atoi(argv[2]);
        if (port <= 0 || port > 65535) {
            port = DEFAULT_PORT;
        }
    }
    
    /* Set up signal handlers */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* Connect to server */
    sockfd = connect_to_server(host, port);
    if (sockfd < 0) {
        fprintf(stderr, "Failed to connect to server\n");
        return 1;
    }
    
    printf("Connected to server (%s:%d)\n", host, port);
    printf("Enter a message (type '/help' for commands, 'exit' to quit):\n");
    
    /* Main loop */
    while (running) {
        printf("> ");
        fflush(stdout);
        
        /* Read input */
        if (fgets(input, MAX_INPUT_SIZE, stdin) == NULL) {
            break;
        }
        
        /* Remove newline character */
        len = strlen(input);
        if (len > 0 && input[len-1] == '\n') {
            input[len-1] = '\0';
        }
        
        /* Process exit command */
        if (strcmp(input, "exit") == 0) {
            printf("Terminating connection...\n");
            break;
        }
        
        /* Process help command */
        if (strcmp(input, "/help") == 0) {
            show_help();
            continue;
        }
        
        /* Send message to server */
        if (send_message(sockfd, input) < 0) {
            fprintf(stderr, "Failed to send message\n");
            break;
        }
        
        /* Receive response from server */
        response = receive_message(sockfd);
        if (response == NULL) {
            fprintf(stderr, "Failed to receive response\n");
            break;
        }
        
        /* Process response */
        process_response(response);
        
        /* Free memory */
        free(response);
        response = NULL;
    }
    
    /* Cleanup */
    cleanup();
    
    return 0;
}

/**
 * Cleanup function
 */
void cleanup(void) {
    if (sockfd >= 0) {
        close(sockfd);
        sockfd = -1;
    }
}

/**
 * Signal handler
 */
void signal_handler(int sig) {
    /* Suppress unused parameter warning */
    (void)sig;
    running = 0;
    printf("\nReceived termination signal. Exiting client...\n");
    cleanup();
    exit(0);
}

/**
 * Display help message
 */
void show_help(void) {
    printf("\n=== Available Commands ===\n");
    printf("/help   - Display this help message\n");
    printf("/clear  - Clear conversation history\n");
    printf("/models - Show available models and current model\n");
    printf("/model model_name - Change the model being used\n");
    printf("exit    - Exit the client\n");
    printf("========================\n\n");
}

/**
 * Connect to server
 */
int connect_to_server(const char *host, int port) {
    struct sockaddr_in server_addr;
    int sock;
    
    /* Create socket */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }
    
    /* Set up server address */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    /* Convert hostname to IP address */
    server_addr.sin_addr.s_addr = inet_addr(host);
    if (server_addr.sin_addr.s_addr == (in_addr_t)-1) {
        struct hostent *he;
        he = gethostbyname(host);
        if (he == NULL) {
            fprintf(stderr, "Failed to resolve hostname: %s\n", host);
            close(sock);
            return -1;
        }
        memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);
    }
    
    /* Connect to server */
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sock);
        return -1;
    }
    
    return sock;
}

/**
 * Send message
 */
int send_message(int sock, const char *message) {
    char buffer[MAX_INPUT_SIZE + 2];  /* Message + newline + NULL terminator */
    int len;
    
    /* Add newline to message */
    sprintf(buffer, "%s\n", message);
    len = strlen(buffer);
    
    /* Send message */
    if (write(sock, buffer, len) != len) {
        perror("write");
        return -1;
    }
    
    return 0;
}

/**
 * Receive message
 */
char *receive_message(int sock) {
    char buffer[BUFFER_SIZE];
    char *response = NULL;
    char *new_response;
    size_t response_size = 0;
    size_t total_size = 0;
    ssize_t bytes_read;
    int flags;
    fd_set readfds;
    struct timeval tv;
    
    /* Read response */
    while ((bytes_read = read(sock, buffer, BUFFER_SIZE - 1)) > 0) {
        buffer[bytes_read] = '\0';
        
        /* Expand response buffer */
        new_response = realloc(response, total_size + bytes_read + 1);
        if (new_response == NULL) {
            perror("realloc");
            free(response);
            return NULL;
        }
        response = new_response;
        
        /* Add new data */
        memcpy(response + total_size, buffer, bytes_read + 1);
        total_size += bytes_read;
        
        /* Check for end of response (ends with newline) */
        if (buffer[bytes_read - 1] == '\n') {
            break;
        }
        
        /* Timeout handling (switch to non-blocking mode) */
        if (response_size == 0) {
            flags = fcntl(sock, F_GETFL, 0);
            fcntl(sock, F_SETFL, flags | O_NONBLOCK);
            response_size = total_size;
        } else {
            /* End if no data arrives for a certain time */
            FD_ZERO(&readfds);
            FD_SET(sock, &readfds);
            
            tv.tv_sec = 1;
            tv.tv_usec = 0;
            
            if (select(sock + 1, &readfds, NULL, NULL, &tv) <= 0) {
                break;
            }
        }
    }
    
    /* Return socket to blocking mode */
    if (response_size > 0) {
        int flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags & ~O_NONBLOCK);
    }
    
    /* Error handling */
    if (bytes_read < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        perror("read");
        free(response);
        return NULL;
    }
    
    return response;
}

/**
 * Process response
 */
void process_response(const char *response) {
    char *message;
    char *current_model;
    char *available_models_start;
    char *available_models_end;
    int len;
    char models[BUFFER_SIZE];
    char *model;
    char *content;
    
    /* Try to parse XML response */
    if (response && response[0] == '<') {
        /* Process command response */
        if (strstr(response, "<response>") && strstr(response, "<type>command</type>")) {
            printf("\n=== Command Execution Result ===\n");
            
            /* Display based on command type */
            if (strstr(response, "<command>clear</command>")) {
                message = extract_xml_content(response, "message");
                if (message) {
                    printf("%s\n", message);
                    free(message);
                }
            } else if (strstr(response, "<command>models</command>")) {
                current_model = extract_xml_content(response, "current_model");
                message = extract_xml_content(response, "message");
                
                if (current_model) {
                    printf("Current model: %s\n", current_model);
                    free(current_model);
                }
                
                /* Display available models */
                available_models_start = strstr(response, "<available_models>");
                available_models_end = strstr(response, "</available_models>");
                
                if (available_models_start && available_models_end) {
                    available_models_start += strlen("<available_models>");
                    len = available_models_end - available_models_start;
                    
                    if (len > 0 && len < BUFFER_SIZE) {
                        /* Parse individual model tags */
                        char *model_start = models;
                        char *model_end;
                        char model_name[256];
                        
                        strncpy(models, available_models_start, len);
                        models[len] = '\0';
                        
                        printf("Available models:\n");
                        
                        
                        
                        while ((model_start = strstr(model_start, "<model>")) != NULL) {
                            model_start += strlen("<model>");
                            model_end = strstr(model_start, "</model>");
                            
                            if (model_end) {
                                len = model_end - model_start;
                                if (len > 0 && len < sizeof(model_name)) {
                                    strncpy(model_name, model_start, len);
                                    model_name[len] = '\0';
                                    trim_string(model_name);
                                    printf("  - %s\n", model_name);
                                }
                                model_start = model_end + strlen("</model>");
                            } else {
                                break;
                            }
                        }
                    }
                }
                
                if (message) {
                    printf("%s\n", message);
                    free(message);
                }
            } else if (strstr(response, "<command>model_change</command>")) {
                char *message = extract_xml_content(response, "message");
                if (message) {
                    printf("%s\n", message);
                    free(message);
                }
            } else {
                /* Other command responses */
                printf("%s\n", response);
            }
        }
        /* Process AI response */
        else if (strstr(response, "<model>") && strstr(response, "<content>")) {
            model = extract_xml_content(response, "model");
            content = extract_xml_content(response, "content");
            
            if (model && content) {
                printf("\n=== AI Response ===\n");
                printf("[Model: %s]\n", model);
                printf("%s\n", content);
                
                free(model);
                free(content);
            } else {
                /* Display as-is if parsing fails */
                printf("\n=== Server Response ===\n");
                printf("%s\n", response);
            }
        } else {
            /* Other XML responses */
            printf("\n=== Server Response ===\n");
            printf("%s\n", response);
        }
    } else {
        /* Plain text */
        printf("\n=== Server Response ===\n");
        printf("%s\n", response);
    }
    
    printf("\nEnter your next message (type '/help' for commands, 'exit' to quit):\n");
}

/**
 * Extract content from XML tag
 */
char *extract_xml_content(const char *xml, const char *tag) {
    char start_tag[64];
    char end_tag[64];
    char *start_ptr, *end_ptr;
    char *content = NULL;
    int len;
    
    /* Create tags */
    sprintf(start_tag, "<%s>", tag);
    sprintf(end_tag, "</%s>", tag);
    
    /* Find positions of start and end tags */
    start_ptr = strstr(xml, start_tag);
    if (start_ptr == NULL) {
        return NULL;
    }
    
    start_ptr += strlen(start_tag);
    end_ptr = strstr(start_ptr, end_tag);
    
    if (end_ptr == NULL) {
        return NULL;
    }
    
    /* Calculate content length */
    len = end_ptr - start_ptr;
    
    /* Allocate memory */
    content = (char *)malloc(len + 1);
    if (content == NULL) {
        return NULL;
    }
    
    /* Copy content */
    strncpy(content, start_ptr, len);
    content[len] = '\0';
    
    /* Trim string */
    trim_string(content);
    
    return content;
}

/**
 * Trim whitespace from beginning and end of string
 */
void trim_string(char *str) {
    char *start = str;
    char *end;
    
    /* Do nothing if string is empty */
    if (str == NULL || *str == '\0') {
        return;
    }
    
    /* Skip leading whitespace */
    while (*start && (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r')) {
        start++;
    }
    
    /* String is all whitespace */
    if (*start == '\0') {
        *str = '\0';
        return;
    }
    
    /* Find end of trailing whitespace */
    end = start + strlen(start) - 1;
    while (end > start && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        end--;
    }
    
    /* Set null terminator */
    *(end + 1) = '\0';
    
    /* Copy result back to original string (if needed) */
    if (start != str) {
        memmove(str, start, (end - start) + 2);
    }
}
