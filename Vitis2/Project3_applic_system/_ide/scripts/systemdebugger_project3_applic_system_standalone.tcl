# Usage with Vitis IDE:
# In Vitis IDE create a Single Application Debug launch configuration,
# change the debug type to 'Attach to running target' and provide this 
# tcl script in 'Execute Script' option.
# Path of this script: D:\PortlandState\ECE_544\Labs\project_3\Vitis2\Project3_applic_system\_ide\scripts\systemdebugger_project3_applic_system_standalone.tcl
# 
# 
# Usage with xsct:
# To debug using xsct, launch xsct and run below command
# source D:\PortlandState\ECE_544\Labs\project_3\Vitis2\Project3_applic_system\_ide\scripts\systemdebugger_project3_applic_system_standalone.tcl
# 
connect -url tcp:127.0.0.1:3121
targets -set -filter {jtag_cable_name =~ "Digilent Nexys A7 -100T 210292B17F4AA" && level==0 && jtag_device_ctx=="jsn-Nexys A7 -100T-210292B17F4AA-13631093-0"}
fpga -file D:/PortlandState/ECE_544/Labs/project_3/Vitis2/Project3_applic/_ide/bitstream/nexysA7fpga.bit
targets -set -nocase -filter {name =~ "*microblaze*#0" && bscan=="USER2" }
loadhw -hw D:/PortlandState/ECE_544/Labs/project_3/Vitis2/Project3_platform/export/Project3_platform/hw/nexysA7fpga.xsa -regs
configparams mdm-detect-bscan-mask 2
targets -set -nocase -filter {name =~ "*microblaze*#0" && bscan=="USER2" }
rst -system
after 3000
targets -set -nocase -filter {name =~ "*microblaze*#0" && bscan=="USER2" }
dow D:/PortlandState/ECE_544/Labs/project_3/Vitis2/Project3_applic/Debug/Project3_applic.elf
targets -set -nocase -filter {name =~ "*microblaze*#0" && bscan=="USER2" }
con
