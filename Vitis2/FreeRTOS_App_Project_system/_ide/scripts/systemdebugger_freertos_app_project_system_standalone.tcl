# Usage with Vitis IDE:
# In Vitis IDE create a Single Application Debug launch configuration,
# change the debug type to 'Attach to running target' and provide this 
# tcl script in 'Execute Script' option.
# Path of this script: D:\PortlandState\ECE_544\Labs\project_3\Vitis2\FreeRTOS_App_Project_system\_ide\scripts\systemdebugger_freertos_app_project_system_standalone.tcl
# 
# 
# Usage with xsct:
# To debug using xsct, launch xsct and run below command
# source D:\PortlandState\ECE_544\Labs\project_3\Vitis2\FreeRTOS_App_Project_system\_ide\scripts\systemdebugger_freertos_app_project_system_standalone.tcl
# 
connect -url tcp:127.0.0.1:3121
targets -set -filter {jtag_cable_name =~ "Digilent Nexys4DDR 210292A8B0CDA" && level==0 && jtag_device_ctx=="jsn-Nexys4DDR-210292A8B0CDA-13631093-0"}
fpga -file D:/PortlandState/ECE_544/Labs/project_3/Vitis2/FreeRTOS_App_Project/_ide/bitstream/nexysA7fpga.bit
targets -set -nocase -filter {name =~ "*microblaze*#0" && bscan=="USER2" }
loadhw -hw D:/PortlandState/ECE_544/Labs/project_3/Vitis2/FreeRTOS_Project3_Platform/export/FreeRTOS_Project3_Platform/hw/nexysA7fpga.xsa -regs
configparams mdm-detect-bscan-mask 2
targets -set -nocase -filter {name =~ "*microblaze*#0" && bscan=="USER2" }
rst -system
after 3000
targets -set -nocase -filter {name =~ "*microblaze*#0" && bscan=="USER2" }
dow D:/PortlandState/ECE_544/Labs/project_3/Vitis2/FreeRTOS_App_Project/Debug/FreeRTOS_App_Project.elf
bpadd -addr &main
