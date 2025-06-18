#include "kernel.h"

int sys_gethid(unsigned int *ptr_hid)
{
	//printf("--> sys_gethid, arg0 = 0x%x\n", ptr_hid);
	if (ptr_hid == NULL)
	{
		printf("ptr_hid == NULL\n");
		return -1;	}
	else
	{
		//printf("ptr_hid != NULL\n");
		*ptr_hid = r_tp(); // Use tp register which contains hartid in S-mode
		return 0;
	}
}

void do_syscall(struct context *ctx)
{
	uint32_t syscall_num = ctx->a7;
	//printf("syscall_num: %d\n", syscall_num);
	switch (syscall_num)
	{
	case 1:
		ctx->a0 = sys_gethid((unsigned int *)(ctx->a0));
		break;
	case 2:
		task_exit();
		break;
	default:
		printf("Unknown syscall no: %d\n", syscall_num);
		ctx->a0 = -1;
	}

	return;
}
