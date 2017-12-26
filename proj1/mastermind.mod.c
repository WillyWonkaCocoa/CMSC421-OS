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
	{ 0x9d4ba808, __VMLINUX_SYMBOL_STR(param_ops_bool) },
	{ 0x999e8297, __VMLINUX_SYMBOL_STR(vfree) },
	{ 0xe8deebe3, __VMLINUX_SYMBOL_STR(misc_deregister) },
	{ 0xc8964a02, __VMLINUX_SYMBOL_STR(misc_register) },
	{ 0xd6ee688f, __VMLINUX_SYMBOL_STR(vmalloc) },
	{ 0x79aa04a2, __VMLINUX_SYMBOL_STR(get_random_bytes) },
	{ 0x449ad0a7, __VMLINUX_SYMBOL_STR(memcmp) },
	{ 0xb44ad4b3, __VMLINUX_SYMBOL_STR(_copy_to_user) },
	{ 0x61651be, __VMLINUX_SYMBOL_STR(strcat) },
	{ 0x4ca9669f, __VMLINUX_SYMBOL_STR(scnprintf) },
	{ 0xdb7305a1, __VMLINUX_SYMBOL_STR(__stack_chk_fail) },
	{ 0x362ef408, __VMLINUX_SYMBOL_STR(_copy_from_user) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0x6dbdde60, __VMLINUX_SYMBOL_STR(remap_pfn_range) },
	{ 0x3744cf36, __VMLINUX_SYMBOL_STR(vmalloc_to_pfn) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "C4E0A5AFBCEEDC6AA7B95F4");
