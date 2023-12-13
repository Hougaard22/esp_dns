A simple DNS server implementation that leverages sockets to relay DNS requests from the client.

To prevent overwhelming the ESP32, rate-limiting measures have been put in place.

## DNS
- The DNS server task forwards incoming DNS queries to an external DNS server specified by DNS_SERVER_IP and DNS_SERVER_PORT.

- A new socket (external_dns_sock) is created to communicate with the external DNS server, and the received DNS query is sent to it.

- The DNS response from the external DNS server is received and forwarded back to the original client.
Note: Make sure to replace "your_wifi_ssid" and "your_wifi_password" with your actual WiFi credentials, and adjust DNS_SERVER_IP and DNS_SERVER_PORT to match the IP address and port of your chosen external DNS server.

Keep in mind that this example assumes a simple DNS query/response without additional complexities. In a real-world scenario, you might need to handle DNS labels, different record types, and other DNS protocol details.


## Rate limiting
- The rate-limiting mechanism is based on a token bucket algorithm. The tokens variable keeps track of the available tokens in the bucket.

- The last_token_time variable keeps track of the last time tokens were replenished.

- The token_interval is the time interval between token replenishments.
Each time a DNS query is received, it checks if there are available tokens. If tokens are available, the query is processed and tokens are decremented. If tokens are not available, the query is dropped, and a rate-limiting warning is logged.

- Tokens are replenished at a fixed rate defined by MAX_REQUESTS_PER_SECOND and TOKEN_BUCKET_SIZE.
Feel free to adjust the rate-limit