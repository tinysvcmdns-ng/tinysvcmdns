/*
 * tinysvcmdns - a tiny MDNS implementation for publishing services
 * Copyright (C) 2011 Darell Tan
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <ifaddrs.h>

#include <stdio.h>
#include "mdns.h"
#include "mdnsd.h"

static in_addr_t search_host(void)
{
	in_addr_t addr = 0;
	struct ifaddrs *ifa_list;
	struct ifaddrs *ifa_main;

	if (getifaddrs(&ifa_list) < 0) {
		DEBUG_PRINTF("getifaddrs() failed");
		return 0;
	}

	for (ifa_main = ifa_list; ifa_main != NULL; ifa_main = ifa_main->ifa_next)
	{
		if ((ifa_main->ifa_flags & IFF_LOOPBACK) || !(ifa_main->ifa_flags & IFF_MULTICAST))
			continue;
		if (ifa_main->ifa_addr && ifa_main->ifa_addr->sa_family == AF_INET)
		{
			addr = ((struct sockaddr_in *)ifa_main->ifa_addr)->sin_addr.s_addr;
			break;
		}
	}
	return addr;
}

int main(int argc, char *argv[]) {
	// create host entries
	char *hostname = "some-random-host.local";

	struct mdnsd *svr = mdnsd_start(NULL);
	if (svr == NULL) {
		printf("mdnsd_start() error\n");
		return 1;
	}

	printf("mdnsd_start OK. press ENTER to add hostname & service\n");
	getchar();

	struct in_addr v4addr;
	v4addr.s_addr = search_host();
	mdnsd_set_hostname(svr, hostname, &v4addr);

	// Add all alternative IP addresses for this host
	struct rr_entry *a2_e = NULL;
	v4addr.s_addr = inet_addr("192.168.0.10");
	a2_e = rr_create_a(create_nlabel(hostname), &v4addr);
	mdnsd_add_rr(svr, a2_e);

	struct rr_entry *aaaa_e = NULL;

	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET6;
	hints.ai_flags = AI_NUMERICHOST;
	struct addrinfo* results;
	getaddrinfo(
		"fe80::e2f8:47ff:fe20:28e0",
		NULL,
		&hints,
		&results);
	struct sockaddr_in6* addr = (struct sockaddr_in6*)results->ai_addr;

	aaaa_e = rr_create_aaaa(create_nlabel(hostname), &addr->sin6_addr);
	freeaddrinfo(results);

	mdnsd_add_rr(svr, aaaa_e);

	const char *txt[] = {
		"name=toto", 
		NULL
	};
	struct mdns_service *svc = mdnsd_register_svc(svr, "mytest", 
									"_http._tcp.local", 8080, NULL, txt);

	printf("added service and hostname. press ENTER to search hostname\n");
	getchar();

	struct in_addr in;
	in.s_addr = mdnsd_search_hostname(svr, "mysmartwifi.local");
	printf("hostname %s found. press ENTER to remove service\n", inet_ntoa(in));
	getchar();

	mdns_service_remove(svr, svc);

	mdns_service_destroy(svc);
	printf("removed service. press ENTER to exit\n");
	getchar();

	in.s_addr = mdnsd_search_hostname(svr, "mysmartwifi.local");
	printf("hostname %s found. press ENTER to exit\n", inet_ntoa(in));
	getchar();

	mdnsd_stop(svr);

	return 0;
}

