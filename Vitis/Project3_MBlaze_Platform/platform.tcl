# 
# Usage: To re-create this platform project launch xsct with below options.
# xsct D:\PortlandState\ECE_544\Labs\project_3\Vitis\Project3_MBlaze_Platform\platform.tcl
# 
# OR launch xsct and run below command.
# source D:\PortlandState\ECE_544\Labs\project_3\Vitis\Project3_MBlaze_Platform\platform.tcl
# 
# To create the platform in a different location, modify the -out option of "platform create" command.
# -out option specifies the output directory of the platform project.

platform create -name {Project3_MBlaze_Platform}\
-hw {D:\PortlandState\ECE_544\Labs\project_2\nexysA7fpga.xsa}\
-proc {microblaze_0} -os {standalone} -fsbl-target {psu_cortexa53_0} -out {D:/PortlandState/ECE_544/Labs/project_3/Vitis}

platform write
platform generate -domains 
platform active {Project3_MBlaze_Platform}
platform generate
platform generate
