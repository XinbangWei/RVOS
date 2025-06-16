#ifndef SYSCALL_H
#define SYSCALL_H

/* 定义系统调用号 */
#define SYS_write 64
#define SYS_exit  93

int sys_write(int fd, const char *buf, int count);
void sys_exit();
extern int gethid(unsigned int *hid);


#endif // SYSCALL_H