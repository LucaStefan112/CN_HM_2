***DNS Simulator***

This repository contains a DNS (Domain Name System) simulator implemented in C. The simulator consists of various components, including one resolver server, one root server, three top-level domain (TLD) servers, and three authoritative servers. Additionally, there is a client program that can send DNS requests to the resolver.

**Components**

Resolver Server: The resolver server acts as an intermediary between the client and the various DNS servers. It receives DNS queries from the client and resolves them by forwarding the requests to the appropriate servers.

Root Server: The root server is the starting point of the DNS resolution process. It provides information about the top-level domain servers responsible for specific domain extensions.

Top-Level Domain (TLD) Servers: There are three TLD servers in the simulator, each responsible for a different top-level domain. These servers maintain information about authoritative servers for their respective domains.

Authoritative Servers: There are three authoritative servers, one for each top-level domain. These servers store the actual DNS records for the domains they are authoritative for and respond to queries for those domains.

Client: The client program interacts with the resolver server by sending DNS queries. It can request the resolution of domain names and receive the corresponding IP addresses.
