#!/usr/bin/env bash
#gdb -ex 'set arch i386:x86-64:intel' -ex 'target remote localhost:1234' -ex 'break kmain_bsp' -ex c .vmake/amd64.debug/beelzebub/beelzebub.bin

#gdb \
#    -ex "add-auto-load-safe-path $(pwd)" \
#    -ex "file .vmake/amd64.debug/beelzebub/beelzebub.bin" \
#    -ex 'set arch i386:x86-64:intel' \
#    -ex 'target remote localhost:1234' \
#    -ex 'break kmain_bsp' \
#    -ex 'continue' \
#    -ex 'disconnect' \
#    -ex 'set arch i386:x86-64' \
#    -ex 'target remote localhost:1234'

gdb \
    -ex "add-auto-load-safe-path $(pwd)" \
    -ex "file .vmake/amd64.debug/beelzebub/beelzebub.bin" \
    -ex 'set arch i386:x86-64:intel' \
    -ex 'target remote localhost:1234' \
    -ex 'break kmain_bsp' \

