/* mbed Microcontroller Library
 * Copyright (c) 2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mbed.h"
#include "json.hpp"
#include <array>
#include <cstring>

// Blinking rate in milliseconds
#define BLINKING_RATE 5000ms

nsapi_size_or_error_t send_request(Socket *socket, const char *request) {
  if (socket == nullptr || request == nullptr) {
    printf("Invalid function arguments\n");
    return NSAPI_ERROR_PARAMETER;
  }

  // The request might not be fully sent in one go,
  // so keep track of how much we have sent
  nsapi_size_t bytes_to_send = strlen(request);
  nsapi_size_or_error_t bytes_sent = 0;

  printf("Sending message: \n%s", request);

  // Loop as long as there are more data to send
  while (bytes_to_send) {
    // Try to send the remaining data, send()
    // returns how many bytes were actually sent
    bytes_sent = socket->send(request + bytes_sent, bytes_to_send);

    if (bytes_sent < 0) {
      // Negative return values from send() are errors
      return bytes_sent;
    } else {
      printf("Sent %d bytes\n", bytes_sent);
    }

    bytes_to_send -= bytes_sent;
  }

  printf("Complete message sent\n");
  return 0;
}

nsapi_size_or_error_t read_response(Socket *socket, char *buffer,
                                    int buffer_length) {
  if (socket == nullptr || buffer == nullptr || buffer_length < 1) {
    printf("Invalid function arguments\n");
    return NSAPI_ERROR_PARAMETER;
  }
  int remaining_bytes = buffer_length;
  int received_bytes = 0;
  char chunk[101] = {0};
    nsapi_size_or_error_t result = 1;
  // Loop as long as there are more data to read,
  // we might not read all in one call to recv()
  while (result > 0) {
     result = socket->recv(chunk, 100);

   
    // If the result is 0 there are no more bytes to read
    if (result == 0) {

      break;
    }

    // Negative return values from recv() are errors
    if (result < 0) {
      return result;
    }

    printf("%s\n...\n", chunk);

    received_bytes += result;
    remaining_bytes -= result;
    strcat(buffer, chunk);
  }

  // Print out first line in response
  printf("\nReceived %d bytes:\n%.*s\n", received_bytes,
         strstr(buffer, "\n") - buffer, buffer);

  return received_bytes;
}

void parse_json_data(char *input_data) {

  nlohmann::json j_object = nlohmann::json::parse(input_data, nullptr, false);

  if (j_object.is_discarded()) {
    printf("The input is invalid JSON\n");
    return;
  }

  printf("The input is valid JSON\n");

  if (j_object["first name"].is_string()) {

    std::string firstName = j_object["first name"];
    printf("First name: %s\n", firstName.c_str());
  }
  if (j_object["last name"].is_string()) {
    std::string lastName = j_object["last name"];
    printf("Last name: %s\n", lastName.c_str());
  }
  if (j_object["age"].is_number()){
    int age = j_object["age"];
    printf("Age: %d\n", j_object["age"].get<int>());
  }
}


int main() {
  // Initialise the digital pin LED1 as an output
  DigitalOut led(LED1);

  // Get pointer to default network interface
  NetworkInterface *network = NetworkInterface::get_default_instance();

  if (!network) {
    printf("Failed to get default network interface\n");
    while (1)
      ;
  }

  nsapi_size_or_error_t result;

  do {
    printf("Connecting to the network...\n");
    result = network->connect();

    if (result != 0) {
      printf("Failed to connect to network: %d\n", result);
    }
  } while (result != 0);

  SocketAddress address;
  network->get_ip_address(&address);

  printf("Connected to WLAN and got IP address %s\n", address.get_ip_address());

  while (true) {
    led = !led;
    ThisThread::sleep_for(BLINKING_RATE);

    TCPSocket *socket = new TCPSocket;

    if (socket == nullptr) {
      printf("Failed to allocate socket instance\n");
      continue;
    }
    socket->open(network);

    const char *host = "www.mocky.io";
    result = network->gethostbyname(host, &address);

    if (result != 0) {
      printf("Failed to get IP address of host %s: %d\n", host, result);
      socket->close(); // Will also delete socket;
      continue;
    }

    printf("IP address of server %s is %s\n", host, address.get_ip_address());

    // Set server TCP port number
    address.set_port(80);

    // Connect to server at the given address
    result = socket->connect(address);

    // Check result
    if (result != 0) {
      printf("Failed to connect to server at %s: %d\n", host, result);
      socket->close(); // Will also delete socket;
      continue;
    }

    printf("Successfully connected to server %s\n", host);

    // Create HTTP request
    const char request[] = "GET /v2/5e37e64b3100004c00d37d03 HTTP/1.1\r\n"
                           "Host: www.mocky.io\r\n"
                           "Connection: close\r\n"
                           "\r\n";

    // Send request
    result = send_request(socket, request);

    // Check result
    if (result != 0) {
      printf("Failed to send request: %d\n", result);
      socket->close(); // Will also delete socket;
      continue;
    }

    // Buffer for HTTP responses

    static constexpr size_t HTTP_RESPONSE_BUF_SIZE = 1000;

    static char response[HTTP_RESPONSE_BUF_SIZE];

    result = read_response(socket, response, HTTP_RESPONSE_BUF_SIZE);

    if (result < 0) {
      printf("Failed to read response: %d\n", result);
      socket->close();
    }

    socket->close();
    response[result] = '\0';
    printf("\nThe HTTP GET response:\n%s\n", response);

    char *json_source_start = strchr(response, '{');

    if (json_source_start == nullptr) {
      printf("No starting { found, invalid JSON\n");
    }

    uint32_t json_source_size = strlen(response);

    while (json_source_size > 0) {
      if (response[json_source_size] == '}') {
        break;
      } else {
        response[json_source_size] = '\0';
      }
      json_source_size--;
    }

    if ((json_source_size == 0) ||
        (response + json_source_size < json_source_start)) {
      printf("No ending } found, invalid JSON\n");
    }

    parse_json_data(json_source_start);

}

    // Use static keyword to move response[] from stack to bss

    // or allacate from heap using keyword new, but remember
    // to delete[] response; when buffer no longer needed
    // char *response = new char[HTTP_RESPONSE_BUF_SIZE];

    // Read response


    // Check result
    

    // Take a look at the response, but before doing
    // that make sure we have a null terminated string
    

    // Will also delete socket;
    // delete[] response;
  }
