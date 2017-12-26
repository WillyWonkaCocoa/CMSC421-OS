#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x27d38ece, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x37a0cba, __VMLINUX_SYMBOL_STR(kfree) },
	{ 0x11a7cb7a, __VMLINUX_SYMBOL_STR(wake_up_process) },
	{ 0x8f261dd5, __VMLINUX_SYMBOL_STR(kthread_create_on_node) },
	{ 0x3b341b5b, __VMLINUX_SYMBOL_STR(kmem_cache_alloc) },
	{ 0xdc06f3cb, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0x4e6d24c3, __VMLINUX_SYMBOL_STR(get_random_u32) },
	{ 0x54496b4, __VMLINUX_SYMBOL_STR(schedule_timeout_interruptible) },
	{ 0xb2ff7ddd, __VMLINUX_SYMBOL_STR(wait_for_completion_interruptible) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0x96ce5809, __VMLINUX_SYMBOL_STR(complete) },
	{ 0x78bb2b29, __VMLINUX_SYMBOL_STR(mutex_unlock) },
	{ 0x402f9f53, __VMLINUX_SYMBOL_STR(mutex_lock) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "57D78948F309C325FB1B627");
