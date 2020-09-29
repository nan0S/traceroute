/* Hubert Obrzut, 309306 */

#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>

struct icmp_response
{
	uint8_t type;
	uint16_t id;
	uint16_t seq;
	char addr[20];
};

uint16_t compute_icmp_checksum(const void* buff, int length)
{
	uint32_t sum;
	const uint16_t* ptr = buff;
	assert(length % 2 == 0);
	for (sum = 0; length > 0; length -= 2)
		sum += *ptr++;
	sum = (sum >> 16) + (sum & 0xffff);
	return (uint16_t)(~(sum + (sum >> 16)));
}

void send_echo_request(int sockfd, struct sockaddr_in* dest, size_t sockaddr_len, int sequence)
{
	struct icmphdr header;	
	header.type = ICMP_ECHO;
	header.code = 0;
	header.un.echo.id = getpid() & 0xffff;
	header.un.echo.sequence = sequence;

	header.checksum = 0;
	header.checksum = compute_icmp_checksum((void*)&header, sizeof(header));

	sendto(sockfd, &header, sizeof(header), 0, (struct sockaddr*)dest, sockaddr_len);
}

int recv_icmp_packet(int sockfd, struct icmp_response* resp, struct timeval* tv)
{
	fd_set descriptors;
	FD_ZERO(&descriptors);
	FD_SET(sockfd, &descriptors);

	int ready = select(sockfd + 1, &descriptors, NULL, NULL, tv);
	if (ready > 0)
	{
		struct sockaddr_in source;
		socklen_t source_len = sizeof(source);
		uint8_t buffer[IP_MAXPACKET];

		ssize_t packet_len = recvfrom(sockfd, buffer, IP_MAXPACKET, 0, (struct sockaddr*)&source, &source_len);
		if (packet_len <= 0)
			return 0;

		char source_ip_str[20];
		inet_ntop(AF_INET, &(source.sin_addr), source_ip_str, sizeof(source_ip_str));
		strcpy(resp->addr, source_ip_str);

		struct ip* ip_header = (struct ip*)buffer;
		struct icmphdr* icmp_header = (struct icmphdr*)(buffer + 4 * ip_header->ip_hl);

		resp->type = icmp_header->type;

		if (resp->type == ICMP_TIME_EXCEEDED)
			icmp_header = (void*)icmp_header + 8 + 4 * ip_header->ip_hl;

		resp->id = icmp_header->un.echo.id;
		resp->seq = icmp_header->un.echo.sequence;
	}

	return ready > 0;
}

int new_addr(char* addr, char* addrs[3], int diff_addr)
{
	for (int i = 0; i < diff_addr; ++i)
		if (!strcmp(addr, addrs[i]))
			return 0;
	return 1;
}

void print(struct icmp_response* resp)
{
	printf("ICMP RESPONSE:\n");
	printf("type: %d\n", resp->type);
	printf("id: %d\n", resp->id);
	printf("seq: %d\n", resp->seq);
	printf("addr: %s\n\n", resp->addr);
}

int main(int argc, char* argv[])
{	
	if (argc < 2)
	{
		fprintf(stderr, "Destination's ip address not specified!\n");
		return EXIT_FAILURE;
	}

	int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sockfd < 0)
	{
		fprintf(stderr, "socket error: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	struct sockaddr_in dest;
	bzero(&dest, sizeof(dest));
	dest.sin_family = AF_INET;
	inet_pton(AF_INET, argv[1], &dest.sin_addr);

	int ttl = 1, done = 0;
	uint16_t ppid = getpid() & 0xffff;
	const int MAX_PACKETS = 3;

	while (1)
	{
		setsockopt(sockfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(int));
		for (int i = 0; i < MAX_PACKETS; ++i)
			send_echo_request(sockfd, &dest, sizeof(dest), ttl);

		printf("%d. ", ttl);

		struct timeval rem = {0, 1e6};
		int recv_cnt = 0;
		int mics_sum = 0;

		int diff_addr = 0;
		char* addrs[MAX_PACKETS];

		while (rem.tv_usec > 0)
		{
			struct icmp_response resp;
			if (recv_icmp_packet(sockfd, &resp, &rem))
			{
				if (resp.id != ppid || resp.seq != ttl || (resp.type != ICMP_TIME_EXCEEDED && resp.type != ICMP_ECHOREPLY))
					continue;

				++recv_cnt;
				mics_sum += 1e6 - rem.tv_usec;
				if (resp.type == ICMP_ECHOREPLY)
					done = 1;

				if (new_addr(resp.addr, addrs, diff_addr))
				{
					printf("%s ", resp.addr);
					addrs[diff_addr] = malloc(20);
					strcpy(addrs[diff_addr++], resp.addr);
				}

				if (recv_cnt == MAX_PACKETS)
					break;
			}	
		}

		if (recv_cnt == 0)
			printf("*\n");
		else if (recv_cnt == MAX_PACKETS)
			printf("%dms\n", mics_sum / (MAX_PACKETS * 1000));
		else
			printf("???\n");

		if (done || ++ttl == 31)
			break;
	}

	return 0;
}
