Implementing ping function with TCP. Examine the reachability and calculate Round-trip time.## Client

### Command

`./client [-n number] [-t timeout] host_1:port_1 host_2:port_2`

- `-n`: pack number send to server. If 0, then send message until closing program. (default: 0)
- `-t`: maximum millisecond client need to wait. (default: 1000)
- `host`: host name or IP

### Output

1. If server is reachable and RTT is smaller or equal than timeout	```
	recv from [server_ip], RTT = [delay] msec
	```2. If server is not reachable or RTT is bigger than timeout	
	```	timeout when connect to [server_ip]
	```

## Server

### Command

`./server listen_port`, listen_port the port server socket listen to	

### Output

```
recv from [client_ip:client_port]
```
