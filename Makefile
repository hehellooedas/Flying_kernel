BUILD_DIR = ./build
ENTRY_POINT = 0xc0001500
BOCHS = ./tools/bochs
QEMU = qemu-system-i386
AS = nasm
AR = tar
CC = gcc
LD = ld
RM = rm 

LIB = -I ./kernel -I ./lib -I ./lib/kernel -I ./lib/user -I ./device \
	  -I ./thread -I ./fs  -I ./device -I ./shell -I ./userprog

RMFLAGS = -rf

ASFLAGS = -f elf

CFLASGS = -m32 -march=i586 -finline-functions -fno-pic  -Wall $(LIB) -c \
		  -fno-builtin -fdiagnostics-color=always -fno-stack-protector \
		  -W -Wstrict-prototypes -Wmissing-prototypes -nostdinc -ffreestanding

LDFLASG = -m elf_i386  -Ttext 0xc0001500 -e main \
		  -o $(BUILD_DIR)/kernel.bin -Map $(BUILD_DIR)/kernel.map

OBJS = $(BUILD_DIR)/main.o     $(BUILD_DIR)/init.o         $(BUILD_DIR)/interrupt.o \
	   $(BUILD_DIR)/timer.o    $(BUILD_DIR)/kernel.o       $(BUILD_DIR)/print.o \
	   $(BUILD_DIR)/switch.o   $(BUILD_DIR)/debug.o        $(BUILD_DIR)/string.o \
	   $(BUILD_DIR)/memory.o   $(BUILD_DIR)/bitmap.o       $(BUILD_DIR)/thread.o \
	   $(BUILD_DIR)/list.o     $(BUILD_DIR)/sync.o         $(BUILD_DIR)/console.o \
	   $(BUILD_DIR)/keyboard.o $(BUILD_DIR)/ioqueue.o      $(BUILD_DIR)/tss.o \
	   $(BUILD_DIR)/process.o  $(BUILD_DIR)/syscall-init.o $(BUILD_DIR)/syscall.o \
	   $(BUILD_DIR)/stdio.o    $(BUILD_DIR)/time.o         $(BUILD_DIR)/stdio-kernel.o \
	   $(BUILD_DIR)/ide.o 	   $(BUILD_DIR)/math.o
		   
		



# c代码编译
$(BUILD_DIR)/main.o:kernel/main.c lib/kernel/print.h kernel/interrupt.h \
			kernel/init.h kernel/global.h thread/thread.h device/console.h
	$(CC) $(CFLASGS) $< -o $@

$(BUILD_DIR)/init.o:kernel/init.c kernel/init.h lib/kernel/print.h \
			kernel/interrupt.h device/timer.h kernel/memory.h thread/thread.h userprog/tss.h
	$(CC) $(CFLASGS) $< -o $@

$(BUILD_DIR)/interrupt.o:kernel/interrupt.c kernel/interrupt.h \
			lib/kernel/io.h lib/stdint.h kernel/global.h lib/kernel/print.h
	$(CC) $(CFLASGS) $< -o $@

$(BUILD_DIR)/timer.o:device/timer.c device/timer.h lib/stdint.h \
			lib/kernel/io.h lib/kernel/print.h kernel/init.h
	$(CC) $(CFLASGS) $< -o $@

$(BUILD_DIR)/debug.o:kernel/debug.c kernel/debug.h \
			lib/kernel/print.h  kernel/interrupt.h
	$(CC) $(CFLASGS) $< -o $@

$(BUILD_DIR)/string.o:lib/string.c lib/string.h lib/stdint.h kernel/debug.h \
			kernel/global.h
	$(CC) $(CFLASGS) $< -o $@

$(BUILD_DIR)/bitmap.o:lib/kernel/bitmap.c lib/kernel/bitmap.h lib/stdint.h lib/string.h \
			lib/kernel/print.h kernel/interrupt.h kernel/debug.h 
	$(CC) $(CFLASGS) $< -o $@

$(BUILD_DIR)/memory.o:kernel/memory.c kernel/memory.h lib/stdint.h lib/kernel/print.h \
			lib/string.h kernel/global.h kernel/debug.h lib/kernel/bitmap.h lib/string.c
	$(CC) $(CFLASGS) $< -o $@

$(BUILD_DIR)/thread.o:thread/thread.c thread/thread.h lib/stdint.h lib/string.h \
		kernel/global.h kernel/memory.h kernel/debug.h
	$(CC) $(CFLASGS) $< -o $@

$(BUILD_DIR)/list.o:lib/kernel/list.c lib/kernel/list.h kernel/interrupt.h
	$(CC) $(CFLASGS) $< -o $@

$(BUILD_DIR)/sync.o:thread/sync.c lib/stdint.h thread/sync.h thread/thread.h \
		kernel/interrupt.h kernel/debug.h lib/kernel/list.h
	$(CC) $(CFLASGS) $< -o $@

$(BUILD_DIR)/console.o:device/console.c device/console.h lib/kernel/print.h \
		lib/stdint.h thread/sync.h thread/thread.h
	$(CC) $(CFLASGS) $< -o $@

$(BUILD_DIR)/keyboard.o:device/keyboard.c device/keyboard.h lib/kernel/print.h \
		lib/stdint.h thread/sync.h
	$(CC) $(CFLASGS) $< -o $@

$(BUILD_DIR)/ioqueue.o:device/ioqueue.c device/ioqueue.h kernel/interrupt.h \
		kernel/debug.h kernel/global.h lib/stdint.h thread/thread.h thread/sync.h 
	$(CC) $(CFLASGS) $< -o $@

$(BUILD_DIR)/tss.o:userprog/tss.c userprog/tss.h kernel/global.h thread/thread.h \
		lib/kernel/print.h lib/string.h
	$(CC) $(CFLASGS) $< -o $@

$(BUILD_DIR)/process.o:userprog/process.c userprog/process.h thread/thread.h \
		kernel/global.h
	$(CC) $(CFLASGS) $< -o $@

$(BUILD_DIR)/syscall-init.o:userprog/syscall-init.c userprog/syscall-init.h \
		kernel/global.h lib/kernel/print.h device/console.h thread/thread.h
	$(CC) $(CFLASGS) $< -o $@

$(BUILD_DIR)/syscall.o:lib/user/syscall.c lib/user/syscall.h kernel/global.h
	$(CC) $(CFLASGS) $< -o $@

$(BUILD_DIR)/stdio.o:lib/stdio.c lib/stdio.h lib/kernel/print.h lib/user/syscall.h \
		kernel/global.h
	$(CC) $(CFLASGS) $< -o $@

$(BUILD_DIR)/time.o:lib/user/time.c lib/user/time.h lib/kernel/io.h 
	$(CC) $(CFLASGS) $< -o $@

$(BUILD_DIR)/stdio-kernel.o:lib/kernel/stdio-kernel.c lib/kernel/stdio-kernel.h \
		lib/stdio.h 
	$(CC) $(CFLASGS) $< -o $@

$(BUILD_DIR)/ide.o:device/ide.c device/ide.h lib/stdint.h thread/thread.h \
		thread/sync.h kernel/interrupt.h kernel/debug.h lib/kernel/io.h
	$(CC) $(CFLASGS) $< -o $@

$(BUILD_DIR)/math.o:lib/math.c lib/math.h
	$(CC) $(CFLASGS) $< -o $@




# 汇编代码编译
$(BUILD_DIR)/kernel.o:kernel/kernel.S
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/print.o:lib/kernel/print.S
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/switch.o:thread/switch.S
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/mbr.bin:boot/mbr.S boot/include/boot.inc
	$(AS) -I boot/include boot/mbr.S -o $@

$(BUILD_DIR)/loader.bin:boot/loader.S boot/include/boot.inc
	$(AS) -I boot/include boot/loader.S -o $@



# 链接
$(BUILD_DIR)/kernel.bin:$(OBJS)
	$(LD) $(LDFLASG) $^ -o $@


.DEFAULT_GOAL := default


# 定义伪目标
.PHONY: mk_dir boot hd clean all tar bochs default


tar:
	$(AR) -Jc . -f ../toyos.tar.xz

mk_dir:
	if [[ ! -d $(BUILD_DIR) ]];then mkdir $(BUILD_DIR);fi

boot:$(BUILD_DIR)/mbr.bin $(BUILD_DIR)/loader.bin

hd:
	echo "build目录中的文件:" && ls $(BUILD_DIR)
	dd if=$(BUILD_DIR)/mbr.bin of=./tools/hd60M.img bs=512 count=1 conv=notrunc
	dd if=$(BUILD_DIR)/loader.bin of=./tools/hd60M.img bs=512 count=4 seek=2 conv=notrunc
	#strip --remove-section=.note.gnu.property $(BUILD_DIR)/kernel.bin
	dd if=$(BUILD_DIR)/kernel.bin of=./tools/hd60M.img \
	bs=512 count=200 seek=9 conv=notrunc #kernel.bin放进第9个磁道

	dd if=$(BUILD_DIR)/mbr.bin of=./tools/Q_hd60M.img bs=512 count=1 conv=notrunc
	dd if=$(BUILD_DIR)/loader.bin of=./tools/Q_hd60M.img bs=512 count=4 seek=2 conv=notrunc
	#strip --remove-section=.note.gnu.property $(BUILD_DIR)/kernel.bin
	dd if=$(BUILD_DIR)/kernel.bin of=./tools/Q_hd60M.img bs=512 count=200 seek=9 conv=notrunc


clean:
	cd $(BUILD_DIR) && $(RM) $(RMFLAGS) ./*

debug:
	$(BOCHS) -f ./tools/bochsrc.disk


build: $(BUILD_DIR)/kernel.bin


all: mk_dir clean boot build hd


qemu: mk_dir clean boot build hd
	$(QEMU) -cpu pentium -drive format=raw,file=./tools/Q_hd60M.img -m 512 -D ./tools/Qemu.log


bochs: mk_dir clean boot build hd debug


default:bochs  #默认使用bochs模拟器来调试
