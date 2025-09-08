/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 */

#include "rmnet_mem_nl.h"
#include "rmnet_mem_priv.h"

#define MAX_POOL 500
#define DEF_PAGEO 3

#define RMNET_MEM_NL_SUCCESS 400
#define RMNET_MEM_NL_FAIL 401

extern struct work_struct pool_adjust_work;
extern struct workqueue_struct *mem_wq;

int rmnet_mem_nl_cmd_update_mode(struct sk_buff *skb, struct genl_info *info)
{
	u8 mode = 0;
	struct sk_buff *resp;
	struct rmnet_memzone_req mem_info;
	struct nlattr *na;

	if (info->attrs[RMNET_MEM_ATTR_MODE]) {
		na = info->attrs[RMNET_MEM_ATTR_MODE];
			if (nla_memcpy(&mem_info, na, sizeof(mem_info)) > 0) {
				rm_err("%s(): modeinfo %u\n", __func__, mem_info.zone);
			}
		rm_err("%s(): mode %u\n", __func__, mode);

		resp = nlmsg_new(NLMSG_DEFAULT_SIZE, GFP_KERNEL);
		if (!resp)
			return -ENOMEM;

		rmnet_mem_genl_send_int_to_userspace_no_info(RMNET_MEM_NL_SUCCESS, info);
	} else {
		rmnet_mem_genl_send_int_to_userspace_no_info(RMNET_MEM_NL_FAIL, info);
	}
	return 0;
}

int rmnet_mem_nl_cmd_update_pool_size(struct sk_buff *skb, struct genl_info *info)
{
	struct sk_buff *resp;
	struct rmnet_pool_update_req mem_info;
	struct nlattr *na;
	int i;
	u8 update_flag = 0;

	if (info->attrs[RMNET_MEM_ATTR_POOL_SIZE]) {
		 na = info->attrs[RMNET_MEM_ATTR_POOL_SIZE];
		if (nla_memcpy(&mem_info, na, sizeof(mem_info)) > 0) {
			pr_err("%s(): modeinfo %u\n", __func__, mem_info.valid_mask);
		}

		rm_err("%s(): pool_size %u\n", __func__, mem_info.poolsize[0]);

		for (i = 0; i < POOL_LEN; i++) {
			if (mem_info.valid_mask & 1 << i &&
				mem_info.poolsize[i] != static_pool_size[i]) {
				target_static_pool_size[i] = mem_info.poolsize[i];
				update_flag = 1;
		    }
		}

		if (update_flag && mem_wq)
			queue_work(mem_wq, &pool_adjust_work);

		resp = nlmsg_new(NLMSG_DEFAULT_SIZE, GFP_KERNEL);

		if (!resp)
			return -ENOMEM;

		rmnet_mem_genl_send_int_to_userspace_no_info(RMNET_MEM_NL_SUCCESS, info);
	} else {
		rmnet_mem_genl_send_int_to_userspace_no_info(RMNET_MEM_NL_FAIL, info);
	}

	return 0;
}
