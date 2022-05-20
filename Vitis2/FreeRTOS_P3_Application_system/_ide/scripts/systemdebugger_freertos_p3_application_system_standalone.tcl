# Usage with Vitis IDE:
# In Vitis IDE create a Single Application Debug launch configuration,
# change the debug type to 'Attach to running target' and provide this 
# tcl script in 'Execute Script' option.
# Path of this script: D:\PortlandState\ECE_544\Labs\project_3\Vitis2\FreeRTOS_P3_Application_system\_ide\scripts\systemdebugger_freertos_p3_application_system_standalone.tcl
# 
# 
# Usage with xsct:
# To debug using xsct, launch xsct and run below command
# source D:\PortlandState\ECE_544\Labs\project_3\Vitis2\FreeRTOS_P3_Application_system\_ide\scripts\systemdebugger_freertos_p3_application_system_standalone.tcl
# 
connect -url tcp:127.0.0.1:3121
targets -set -filter {jtag_cable_name =~ "Digilent Nexys4DDR 210292742995A" && level==0 && jtag_device_ctx=="jsn-Nexys4DDR-210292742995A-13631093-0"}
fpga -file D:/PortlandState/ECE_544/Labs/project_3/Vitis2/FreeRTOS_P3_Application/_ide/bitstream/nexysA7fpga.bit
targets -set -nocase -filter {name =~ "*microblaze*#0" && bscan=="USER2" }
loadhw -hw D:/PortlandState/ECE_544/Labs/project_3/Vitis2/FreeRTOS_P3_Update/export/FreeRTOS_P3_Update/hw/nexysA7fpga.xsa -regs
configparams mdm-detect-bscan-mask 2
targets -set -nocase -filter {name =~ "*microblaze*#0" && bscan=="USER2" }
rst -system
after 3000
targets -set -nocase -filter {name =~ "*microblaze*#0" && bscan=="USER2" }
dow D:/PortlandState/ECE_544/Labs/project_3/Vitis2/FreeRTOS_P3_Application/Debug/FreeRTOS_P3_Application.elf
bpadd -addr &main
