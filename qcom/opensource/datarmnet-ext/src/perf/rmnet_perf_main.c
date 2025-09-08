/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2021-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * RMNET PERF framework
 *
 */

#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/in.h>
#include <linux/udp.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <net/ip.h>
#include <net/ipv6.h>
#include "rmnet_module.h"
#include <net/ipv6.h>
#include <net/ip.h>

#include "rmnet_perf_tcp.h"
#include "rmnet_perf_udp.h"

MODULE_LICENSE("GPL v2");

/* Insert newest first, last 4 bytes of the change id */
static char *verinfo[] = {
	"8ab0a8ee",
	"f22bace0",
	"cc98f08a",
	"ce79321c",
	"5dcdd4c0",
	"4c9b5337",
	"a3babd40",
	"7f078f96",
	"e56cb55d",
	"e218f451",
	"39cbd7d5",
	"7415921c",
	"4de49db5",
	"b1c44b4c"
};

module_param_array(verinfo, charp, NULL, 0444);
MODULE_PARM_DESC(verinfo, "Version of the driver");

bool enable_tcp = true;
module_param_named(rmnet_perf_knob0, enable_tcp, bool, 0644);

static bool enable_udp = true;
module_param_named(rmnet_perf_knob1, enable_udp, bool, 0644);

extern void (*rmnet_perf_egress_hook1)(struct sk_buff *skb);

#define RMNET_INGRESS_QUIC_PORT 443

static inline bool rmnet_perf_is_quic_packet(struct udphdr *uh)
{
	return be16_to_cpu(uh->source) == RMNET_INGRESS_QUIC_PORT ||
	       be16_to_cpu(uh->dest) == RMNET_INGRESS_QUIC_PORT;
}

static bool rmnet_perf_is_quic_initial_packet(struct sk_buff *skb, int ip_len)
{
	u8 *first_byte, __first_byte;
	struct udphdr *uh, __uh;

	uh = skb_header_pointer(skb, ip_len, sizeof(*uh), &__uh);

	if (!uh || !rmnet_perf_is_quic_packet(uh))
		return false;

	/* Length sanity check. Could check for the full QUIC header length if
	 * need be, but since all we really care about is the first byte, just
	 * make sure there is one.
	 */
	if (be16_to_cpu(uh->len) < sizeof(struct udphdr) + 1)
		return false;

	/* I am a very paranoid accessor of data at this point... */
	first_byte = skb_header_pointer(skb, ip_len + sizeof(struct udphdr),
					1, &__first_byte);
	if (!first_byte)
		return false;

	return ((*first_byte) & 0xC0) == 0xC0;
}

static int rmnet_perf_ingress_handle_quic(struct sk_buff *skb, int ip_len)
{
	if (rmnet_perf_is_quic_initial_packet(skb, ip_len)) {
		skb->hash = 0;
		skb->sw_hash = 1;
		return 0;
	}

	return -EINVAL;
}

int rmnet_perf_ingress_handle(struct sk_buff *skb)
{
	if (skb->protocol == htons(ETH_P_IP)) {
		struct iphdr *iph, __iph;

		iph = skb_header_pointer(skb, 0, sizeof(*iph), &__iph);
		if (!iph || ip_is_fragment(iph))
			return -EINVAL;

		if (iph->protocol == IPPROTO_UDP) {
			if (enable_udp)
				rmnet_perf_ingress_handle_udp(skb);

			return rmnet_perf_ingress_handle_quic(skb,
							      iph->ihl * 4);
		}

		if (iph->protocol == IPPROTO_TCP) {
			if (enable_tcp)
				rmnet_perf_ingress_handle_tcp(skb);

			/* Don't skip SHS processing for TCP */
			return -EINVAL;
		}
	} else if (skb->protocol == htons(ETH_P_IPV6)) {
		struct ipv6hdr *ip6h, __ip6h;
		int ip_len;
		__be16 frag_off;
		u8 proto;

		ip6h = skb_header_pointer(skb, 0, sizeof(*ip6h), &__ip6h);
		if (!ip6h)
			return -EINVAL;

		proto = ip6h->nexthdr;
		ip_len = ipv6_skip_exthdr(skb, sizeof(*ip6h), &proto,
					  &frag_off);
		if (ip_len < 0 || frag_off)
			return -EINVAL;

		if (proto == IPPROTO_UDP) {
			if (enable_udp)
				rmnet_perf_ingress_handle_udp(skb);

			return rmnet_perf_ingress_handle_quic(skb, ip_len);
		}

		if (proto == IPPROTO_TCP) {
			if (enable_tcp)
				rmnet_perf_ingress_handle_tcp(skb);

			return -EINVAL;
		}
	}

	return -EINVAL;
}

void rmnet_perf_ingress_rx_handler(struct sk_buff *skb)
{
	if (skb->protocol == htons(ETH_P_IP)) {
		struct iphdr *iph, __iph;

		iph = skb_header_pointer(skb, 0, sizeof(*iph), &__iph);
		if (!iph || ip_is_fragment(iph))
			return;

		if (iph->protocol == IPPROTO_TCP) {
			if (enable_tcp)
				rmnet_perf_ingress_rx_handler_tcp(skb);
		}
	} else if (skb->protocol == htons(ETH_P_IPV6)) {
		struct ipv6hdr *ip6h, __ip6h;
		int ip_len;
		__be16 frag_off;
		u8 proto;

		ip6h = skb_header_pointer(skb, 0, sizeof(*ip6h), &__ip6h);
		if (!ip6h)
			return;

		proto = ip6h->nexthdr;
		ip_len = ipv6_skip_exthdr(skb, sizeof(*ip6h), &proto,
					  &frag_off);
		if (ip_len < 0 || frag_off)
			return;

		if (proto == IPPROTO_TCP) {
			if (enable_tcp)
				rmnet_perf_ingress_rx_handler_tcp(skb);
		}
	}
}

static void rmnet_perf_egress_handle_quic(struct sk_buff *skb, int ip_len)
{
	if (rmnet_perf_is_quic_initial_packet(skb, ip_len))
		skb->priority = 0xDA001A;
}

void rmnet_perf_egress_handle(struct sk_buff *skb)
{
	if (skb->protocol == htons(ETH_P_IP)) {
		struct iphdr *iph, __iph;

		iph = skb_header_pointer(skb, 0, sizeof(*iph), &__iph);
		/* Potentially problematic, but the problem is secondary
		 * fragments have no transport header.
		 */
		if (!iph || ip_is_fragment(iph))
			return;

		if (iph->protocol == IPPROTO_UDP) {
			if (enable_udp)
				rmnet_perf_egress_handle_udp(skb);

			rmnet_perf_egress_handle_quic(skb, iph->ihl * 4);
			return;
		}

		if (iph->protocol == IPPROTO_TCP) {
			if (enable_tcp)
				rmnet_perf_egress_handle_tcp(skb);

			return;
		}
	} else if (skb->protocol == htons(ETH_P_IPV6)) {
		struct ipv6hdr *ip6h, __ip6h;
		int ip_len;
		__be16 frag_off;
		u8 proto;

		ip6h = skb_header_pointer(skb, 0, sizeof(*ip6h), &__ip6h);
		if (!ip6h)
			return;

		proto = ip6h->nexthdr;
		ip_len = ipv6_skip_exthdr(skb, sizeof(*ip6h), &proto,
					  &frag_off);
		if (ip_len < 0 || frag_off)
			return;

		if (proto == IPPROTO_UDP) {
			if (enable_udp)
				rmnet_perf_egress_handle_udp(skb);

			rmnet_perf_egress_handle_quic(skb, ip_len);
			return;
		}

		if (proto == IPPROTO_TCP) {
			if (enable_tcp)
				rmnet_perf_egress_handle_tcp(skb);

			return;
		}
	}
}

static const struct rmnet_module_hook_register_info
rmnet_perf_module_hooks[] = {
	{
		.hooknum = RMNET_MODULE_HOOK_PERF_INGRESS,
		.func = rmnet_perf_ingress_handle,
	},
	{
		.hooknum = RMNET_MODULE_HOOK_PERF_EGRESS,
		.func = rmnet_perf_egress_handle,
	},
	{
		.hooknum = RMNET_MODULE_HOOK_PERF_SET_THRESH,
		.func = rmnet_perf_tcp_update_quickack_thresh,
	},
	{
		.hooknum = RMNET_MODULE_HOOK_PERF_INGRESS_RX_HANDLER,
		.func = rmnet_perf_ingress_rx_handler,
	},
};

void rmnet_perf_set_hooks(void)
{
	rcu_assign_pointer(rmnet_perf_egress_hook1,
			   rmnet_perf_egress_handle);
	rmnet_module_hook_register(rmnet_perf_module_hooks,
				   ARRAY_SIZE(rmnet_perf_module_hooks));
}

void rmnet_perf_unset_hooks(void)
{
	rcu_assign_pointer(rmnet_perf_egress_hook1, NULL);
	rmnet_module_hook_unregister(rmnet_perf_module_hooks,
				     ARRAY_SIZE(rmnet_perf_module_hooks));
}

static int __init rmnet_perf_init(void)
{
	int rc;

	pr_info("%s(): Loading\n", __func__);
	rc = rmnet_perf_tcp_init();
	if (rc)
		return rc;

	rc = rmnet_perf_udp_init();
	if (rc) {
		rmnet_perf_tcp_exit();
		return rc;
	}

	rmnet_perf_set_hooks();
	return 0;
}

static void __exit rmnet_perf_exit(void)
{
	rmnet_perf_unset_hooks();
	rmnet_perf_udp_exit();
	rmnet_perf_tcp_exit();
	pr_info("%s(): exiting\n", __func__);
}

module_init(rmnet_perf_init);
module_exit(rmnet_perf_exit);
