Server Usage:
$ make udp_select_server
$ ./udp_select_server <port no>

Client Usage:
$ make udp_client
$ ./udp_client <server_ip> <server_portno>

The client automatically generates a list(1 to 100 numbers) in the range 0-1000 and sends them to server.
The server returns the sum of sent numbers.