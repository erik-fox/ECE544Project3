# 
# Usage: To re-create this platform project launch xsct with below options.
# xsct D:\PortlandState\ECE_544\Labs\project_3\Vitis2\FreeRTOS_P3_Update\platform.tcl
# 
# OR launch xsct and run below command.
# source D:\PortlandState\ECE_544\Labs\project_3\Vitis2\FreeRTOS_P3_Update\platform.tcl
# 
# To create the platform in a different location, modify the -out option of "platform create" command.
# -out option specifies the output directory of the platform project.

platform create -name {FreeRTOS_P3_Update}\
-hw {D:\PortlandState\ECE_544\Labs\project_3\nexysA7fpga.xsa}\
-proc {microblaze_0} -os {freertos10_xilinx} -fsbl-target {psu_cortexa53_0} -out {D:/PortlandState/ECE_544/Labs/project_3/Vitis2}

platform write
platform generate -domains 
platform active {FreeRTOS_P3_Update}
platform generate
platform clean
catch {platform remove FreeRTOS_Project3_Platform}
platform config -updatehw {D:/PortlandState/ECE_544/Labs/project_3/nexysA7fpga.xsa}
platform generate
platform clean
platform generate
platform clean
platform generate
platform generate -domains 
platform clean
platform generate
platform active {FreeRTOS_P3_Update}
platform config -updatehw {D:/PortlandState/ECE_544/Labs/project_3/nexysA7fpga.xsa}
platform clean
platform generate
platform clean
platform clean
platform generate
platform config -updatehw {D:/PortlandState/ECE_544/Labs/project_3/nexysA7fpga.xsa}
platform generate -domains 
platform clean
platform clean
platform generate
platform active {FreeRTOS_P3_Update}
platform generate -domains freertos10_xilinx_domain 
