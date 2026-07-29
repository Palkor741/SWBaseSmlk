/* TCP BBRplus Congestion Control
 * BBRplus algorithm for Linux Kernel
 */

#include <linux/module.h>
#include <net/tcp.h>
#include <linux/inet_diag.h>
#include <linux/random.h>
#include <net/sock.h>

#define BBRPLUS_SCALE 8
#define BBRPLUS_UNIT (1 << BBRPLUS_SCALE)

struct bbrplus {
	u32	min_rtt_us;
	u32	min_rtt_stamp;
	u32	probe_rtt_done_stamp;
	struct minmax bw;
	u32	rtt_cnt;
	u32	next_rtt_delivered;
	u64	cycle_mstamp;
	u32	mode:3,
		prev_ca_state:3,
		packet_conservation:1,
		restore_extra_acked:1,
		round_start:1,
		tso_segs_goal:7,
		idle_restart:1,
		probe_rtt_round_done:1,
		unused:13;
	u32	pacing_gain:10,
		cwnd_gain:10,
		full_bw_cnt:3,
		cycle_idx:3,
		has_seen_rtt:1,
		unused_2:5;
	u32	prior_cwnd;
	u32	full_bw;
};

enum bbrplus_mode {
	BBRPLUS_STARTUP,
	BBRPLUS_DRAIN,
	BBRPLUS_PROBE_BW,
	BBRPLUS_PROBE_RTT,
};

static void bbrplus_init(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);
	struct bbrplus *bbr = inet_csk_ca(sk);

	bbr->min_rtt_us = tcp_min_rtt(tp);
	bbr->min_rtt_stamp = tcp_time_stamp;
	bbr->probe_rtt_done_stamp = 0;
	bbr->probe_rtt_round_done = 0;
	bbr->rtt_cnt = 0;
	bbr->next_rtt_delivered = 0;
	bbr->mode = BBRPLUS_STARTUP;
	bbr->pacing_gain = BBRPLUS_UNIT * 289 / 100;
	bbr->cwnd_gain = BBRPLUS_UNIT * 2;
	bbr->full_bw = 0;
	bbr->full_bw_cnt = 0;
	bbr->cycle_idx = 0;

	cmpxchg(&sk->sk_pacing_status, SK_PACING_NONE, SK_PACING_NEEDED);
}

static u32 bbrplus_sndbuf_expand(struct sock *sk)
{
	return 3;
}

static struct tcp_congestion_ops tcp_bbrplus_cong_ops __read_mostly = {
	.flags		= TCP_CONG_NON_RESTRICTED,
	.name		= "bbrplus",
	.owner		= THIS_MODULE,
	.init		= bbrplus_init,
	.sndbuf_expand	= bbrplus_sndbuf_expand,
};

static int __init bbrplus_register(void)
{
	BUILD_BUG_ON(sizeof(struct bbrplus) > ICSK_CA_PRIV_SIZE);
	return tcp_register_congestion_control(&tcp_bbrplus_cong_ops);
}

static void __exit bbrplus_unregister(void)
{
	tcp_unregister_congestion_control(&tcp_bbrplus_cong_ops);
}

module_init(bbrplus_register);
module_exit(bbrplus_unregister);

MODULE_AUTHOR("Palkor741 <nurrahmanjr741@gmail.com>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("TCP BBRplus Congestion Control");
