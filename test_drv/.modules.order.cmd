cmd_/home/wdz/test_drv/modules.order := {   echo /home/wdz/test_drv/bmp280_drv.ko; :; } | awk '!x[$$0]++' - > /home/wdz/test_drv/modules.order
