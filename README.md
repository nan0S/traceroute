# traceroute
Standard traceroute tool to print route packets trace, written in C. It displays IP adresses of routers on the path to the destination IP address.
Traceroute sends *ICMP echo requests*. 

##### Usage
```./traceroute DEST_IP_ADDRES``` 

##### Example usage
```./traceroute 156.17.254.113```

Example output:
```
1. 156.17.4.254 40ms
2. 156.17.252.34 ???
3. *
4. *
5. 156.17.254.113 156.17.254.114 50ms
6. 6. 156.17.254.113 65ms
```

##### Output clarification
ICMP packets are sent with ascending value of TTL (Time To Live). If we do not get any response, we print '*'. Otherwise every IP address that responded is displayed. If we get as many responses as we sent requests in within timespan (default 1 second), there will be additonally displayed mean answer time. Otherwise '???' will be displayed.