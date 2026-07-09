cmd_/home/wdz/test_drv/Module.symvers := sed 's/\.ko$$/\.o/' /home/wdz/test_drv/modules.order | scripts/mod/modpost     -o /home/wdz/test_drv/Module.symvers -e    -T -
